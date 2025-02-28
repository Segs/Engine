/*************************************************************************/
/*  base_object_glue.cpp                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "base_object_glue.h"

#include "core/debugger/script_debugger.h"

#include "core/reference.h"
#include "core/string_name.h"
#include "core/object_db.h"

#include "../csharp_script.h"
#include "../mono_gd/gd_mono_cache.h"
#include "../mono_gd/gd_mono_class.h"
#include "../mono_gd/gd_mono_internals.h"
#include "../mono_gd/gd_mono_utils.h"
#include "../signal_awaiter_utils.h"
#include "arguments_vector.h"

Object *godot_icall_Object_Ctor(MonoObject *p_obj) {
    Object *instance = memnew(Object);
    GDMonoInternals::tie_managed_to_unmanaged(p_obj, instance);
    return instance;
}

void godot_icall_Object_Disposed(MonoObject *p_obj, Object *p_ptr) {
#ifdef DEBUG_ENABLED
    CRASH_COND(p_ptr == nullptr);
#endif

    if (p_ptr->get_script_instance()) {
        CSharpInstance *cs_instance = CAST_CSHARP_INSTANCE(p_ptr->get_script_instance());
        if (cs_instance) {
            if (!cs_instance->is_destructing_script_instance()) {
                cs_instance->mono_object_disposed(p_obj);
                p_ptr->set_script_instance(nullptr);
            }
            return;
        }
    }

    void *data = p_ptr->get_script_instance_binding(CSharpLanguage::get_singleton()->get_language_index());

    if (data) {
        Map<Object *, CSharpScriptBinding>::iterator iter;
        iter.mpNode = (Map<Object *, CSharpScriptBinding>::iterator::node_type *)data;
        CSharpScriptBinding &script_binding = iter->second;
        if (script_binding.inited) {
            MonoGCHandleData &gchandle = script_binding.gchandle;
            if (!gchandle.is_released()) {
                CSharpLanguage::release_script_gchandle(p_obj, gchandle);
            }
        }
    }
}

void godot_icall_Reference_Disposed(MonoObject *p_obj, Object *p_ptr, MonoBoolean p_is_finalizer) {
#ifdef DEBUG_ENABLED
    CRASH_COND(p_ptr == nullptr);
    // This is only called with Reference derived classes
    CRASH_COND(!object_cast<RefCounted>(p_ptr));
#endif

    RefCounted *ref = static_cast<RefCounted *>(p_ptr);

    if (ref->get_script_instance()) {
        CSharpInstance *cs_instance = CAST_CSHARP_INSTANCE(ref->get_script_instance());
        if (cs_instance) {
            if (!cs_instance->is_destructing_script_instance()) {
                bool delete_owner;
                bool remove_script_instance;

                cs_instance->mono_object_disposed_baseref(p_obj, p_is_finalizer, delete_owner, remove_script_instance);

                if (delete_owner) {
                    memdelete(ref);
                } else if (remove_script_instance) {
                    ref->set_script_instance(nullptr);
                }
            }
            return;
        }
    }

    // Unsafe refcount decrement. The managed instance also counts as a reference.
    // See: CSharpLanguage::alloc_instance_binding_data(Object *p_object)
    CSharpLanguage::get_singleton()->pre_unsafe_unreference(ref);
    if (ref->unreference()) {
        memdelete(ref);
    } else {
        void *data = ref->get_script_instance_binding(CSharpLanguage::get_singleton()->get_language_index());

        if (data) {
            Map<Object *, CSharpScriptBinding>::iterator iter;
            iter.mpNode = (Map<Object *, CSharpScriptBinding>::iterator::node_type *)data;
            CSharpScriptBinding &script_binding = iter->second;
            if (script_binding.inited) {
                MonoGCHandleData &gchandle = script_binding.gchandle;
                if (!gchandle.is_released()) {
                    CSharpLanguage::release_script_gchandle(p_obj, gchandle);
                }
            }
        }
    }
}
void godot_icall_Object_ConnectEventSignals(Object *p_ptr) {
    CSharpInstance *csharp_instance = CAST_CSHARP_INSTANCE(p_ptr->get_script_instance());
    if (csharp_instance) {
        csharp_instance->connect_event_signals();
    }
}
MethodBind *godot_icall_Object_ClassDB_get_method(StringName *p_type, MonoString *p_method) {
    StringName type = p_type ? *p_type : StringName();
    StringName method(GDMonoMarshal::mono_string_to_godot(p_method));
    return ClassDB::get_method(type, method);
}

MonoObject *godot_icall_Object_weakref(Object *p_obj) {
    if (!p_obj) {
        return nullptr;
    }

    Ref<WeakRef> wref;
    RefCounted  *ref = object_cast<RefCounted>(p_obj);

    if (ref) {
        REF r(ref);
        if (!r)
            return nullptr;
        wref= make_ref_counted<WeakRef>();
        wref->set_ref(r);
    } else {
        wref= make_ref_counted<WeakRef>();
        wref->set_obj(p_obj);
    }

    return GDMonoUtils::unmanaged_get_managed(wref.get());
}

Error godot_icall_SignalAwaiter_connect(Object *p_source, StringName *p_signal, Object *p_target, MonoObject *p_awaiter) {
    StringName signal = p_signal ? *p_signal : StringName();
    return gd_mono_connect_signal_awaiter(p_source, signal, p_target, p_awaiter);
}

MonoArray *godot_icall_DynamicGodotObject_SetMemberList(Object *p_ptr) {
    Vector<PropertyInfo> property_list;
    p_ptr->get_property_list(&property_list);

    MonoArray *result = mono_array_new(mono_domain_get(), CACHED_CLASS_RAW(String), property_list.size());

    int i = 0;
    for (const PropertyInfo &E : property_list) {
        MonoString *boxed = GDMonoMarshal::mono_string_from_godot(E.name);
        mono_array_setref(result, i, boxed);
        i++;
    }

    return result;
}

MonoBoolean godot_icall_DynamicGodotObject_InvokeMember(Object *p_ptr, MonoString *p_name, MonoArray *p_args, MonoObject **r_result) {
    String name = GDMonoMarshal::mono_string_to_godot(p_name);

    int argc = mono_array_length(p_args);

    ArgumentsVector<Variant> arg_store(argc);
    ArgumentsVector<const Variant *> args(argc);

    for (int i = 0; i < argc; i++) {
        MonoObject *elem = mono_array_get(p_args, MonoObject *, i);
        arg_store.emplace_back(GDMonoMarshal::mono_object_to_variant(elem));
    }
    for (int i = 0; i < argc; i++) {
        args.push_back(&arg_store[i]);
    }
    Callable::CallError error;
    Variant result = p_ptr->call(StringName(name), args.data(), argc, error);

    *r_result = GDMonoMarshal::variant_to_mono_object(result);

    return error.error == Callable::CallError::CALL_OK;
}

MonoBoolean godot_icall_DynamicGodotObject_GetMember(Object *p_ptr, MonoString *p_name, MonoObject **r_result) {
    String name = GDMonoMarshal::mono_string_to_godot(p_name);

    bool valid;
    Variant value = p_ptr->get(StringName(name), &valid);

    if (valid) {
        *r_result = GDMonoMarshal::variant_to_mono_object(value);
    }

    return valid;
}

MonoBoolean godot_icall_DynamicGodotObject_SetMember(Object *p_ptr, MonoString *p_name, MonoObject *p_value) {
    String name = GDMonoMarshal::mono_string_to_godot(p_name);
    Variant value = GDMonoMarshal::mono_object_to_variant(p_value);

    bool valid;
    p_ptr->set(StringName(name), value, &valid);

    return valid;
}

MonoString *godot_icall_Object_ToString(Object *p_ptr) {
#ifdef DEBUG_ENABLED
    // Cannot happen in C#; would get an ObjectDisposedException instead.
    CRASH_COND(p_ptr == nullptr);

#endif

    String result = String("[") + p_ptr->get_class() + ":" + itos(entt::to_integral(p_ptr->get_instance_id())) + "]";
    return GDMonoMarshal::mono_string_from_godot(result);
}

void godot_register_object_icalls() {
    mono_add_internal_call("Godot.Object::godot_icall_Object_Ctor", (void *)godot_icall_Object_Ctor);
    mono_add_internal_call("Godot.Object::godot_icall_Object_Disposed", (void *)godot_icall_Object_Disposed);
    mono_add_internal_call("Godot.Object::godot_icall_Reference_Disposed", (void *)godot_icall_Reference_Disposed);
    mono_add_internal_call("Godot.Object::godot_icall_Object_ConnectEventSignals", (void *)godot_icall_Object_ConnectEventSignals);
    mono_add_internal_call("Godot.Object::godot_icall_Object_ClassDB_get_method", (void *)godot_icall_Object_ClassDB_get_method);
    mono_add_internal_call("Godot.Object::godot_icall_Object_ToString", (void *)godot_icall_Object_ToString);
    mono_add_internal_call("Godot.Object::godot_icall_Object_weakref", (void *)godot_icall_Object_weakref);
    mono_add_internal_call("Godot.SignalAwaiter::godot_icall_SignalAwaiter_connect", (void *)godot_icall_SignalAwaiter_connect);
    mono_add_internal_call("Godot.DynamicGodotObject::godot_icall_DynamicGodotObject_SetMemberList", (void *)godot_icall_DynamicGodotObject_SetMemberList);
    mono_add_internal_call("Godot.DynamicGodotObject::godot_icall_DynamicGodotObject_InvokeMember", (void *)godot_icall_DynamicGodotObject_InvokeMember);
    mono_add_internal_call("Godot.DynamicGodotObject::godot_icall_DynamicGodotObject_GetMember", (void *)godot_icall_DynamicGodotObject_GetMember);
    mono_add_internal_call("Godot.DynamicGodotObject::godot_icall_DynamicGodotObject_SetMember", (void *)godot_icall_DynamicGodotObject_SetMember);
}
