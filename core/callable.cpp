/*************************************************************************/
/*  callable.cpp                                                         */
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

#include "callable.h"

#include "core/script_language.h"
#include "core/ecs_registry.h"
#include "string_utils.h"
#include "message_queue.h"
#include "object.h"
#include "object_db.h"
#include "reference.h"

#include "entt/entity/entity.hpp"

#include "entt/entity/registry.hpp"
/*void Callable::call_deferred(const Variant **p_arguments, int p_argcount) const {
    MessageQueue::get_singleton()->push_callable(*this, p_arguments, p_argcount);
}*/

void Callable::call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, CallError &r_call_error) const {
    if (is_null()) {
        r_call_error.error = CallError::CALL_ERROR_INSTANCE_IS_NULL;
        r_call_error.argument = 0;
        r_call_error.expected = VariantType::NIL;
        r_return_value = Variant();
    } else if (is_custom()) {
        custom->call(p_arguments, p_argcount, r_return_value, r_call_error);
    } else {
        Object *obj = object_for_entity(object);
        r_return_value = obj->call(method, p_arguments, p_argcount, r_call_error);
    }
}

Object *Callable::get_object() const {
    if (is_null()) {
        return nullptr;
    } else if (is_custom()) {
        return object_for_entity(custom->get_object());
    } else {
        return object_for_entity(GameEntity(object));
    }
}

GameEntity Callable::get_object_id() const {
    if (is_null()) {
        return entt::null;
    }
    if (is_custom()) {
        return custom->get_object();
    } else {
        return GameEntity(object);
    }
}

StringName Callable::get_method() const {
    ERR_FAIL_COND_V_MSG(is_custom(), StringName(),
            String(String::CtorSprintf(),"Can't get method on CallableCustom \"%s\".", (operator String()).c_str()));
    return method;
}

CallableCustom *Callable::get_custom() const {
    ERR_FAIL_COND_V_MSG(!is_custom(), nullptr,
            String(String::CtorSprintf(),"Can't get custom on non-CallableCustom \"%s\".", (operator String()).c_str())
            );
    return custom;
}

uint32_t Callable::hash() const {
    if (is_custom()) {
        return custom->hash();
    }
    uint32_t hash = method.hash();
    return hash_djb2_one_64(entt::to_integral(object), hash);
}

bool Callable::operator==(const Callable &p_callable) const {
    bool custom_a = is_custom();
    bool custom_b = p_callable.is_custom();

    if (custom_a != custom_b) {
        return false;
    }
    if (!custom_a) {
        return object == p_callable.object && method == p_callable.method;
    }
    if (custom == p_callable.custom) {
        return true; //same pointer, dont even compare
    }

    CallableCustom::CompareEqualFunc eq_a = custom->get_compare_equal_func();
    CallableCustom::CompareEqualFunc eq_b = p_callable.custom->get_compare_equal_func();
    if (eq_a != eq_b) {
        return false;
    }
    return eq_a(custom, p_callable.custom);
}

bool Callable::operator!=(const Callable &p_callable) const {
    return !(*this == p_callable);
}

bool Callable::operator<(const Callable &p_callable) const {
    bool custom_a = is_custom();
    bool custom_b = p_callable.is_custom();

    if (custom_a == custom_b) {
        if (custom_a) {
            if (custom == p_callable.custom) {
                return false; //same pointer, dont even compare
            }

            CallableCustom::CompareLessFunc less_a = custom->get_compare_less_func();
            CallableCustom::CompareLessFunc less_b = p_callable.custom->get_compare_less_func();
            if (less_a == less_b) {
                return less_a(custom, p_callable.custom);
            } else {
                return less_a < less_b; //it's something..
            }

        } else {
            if (object == p_callable.object) {
                return method < p_callable.method;
            } else {
                return entt::to_integral(object) < entt::to_integral(p_callable.object);
            }
        }
    } else {
        return int(custom_a ? 1 : 0) < int(custom_b ? 1 : 0);
    }
}

void Callable::operator=(const Callable &p_callable) {
    if (is_custom()) {
        if (p_callable.is_custom()) {
            if (custom == p_callable.custom) {
                return;
            }
        }

        if (custom->ref_count.unref()) {
            memdelete(custom);
        }
    }

    if (p_callable.is_custom()) {
        method = StringName();
        if (p_callable.custom->ref_count.ref()) {
            custom = p_callable.custom;
            custom_method = p_callable.custom_method;
        } else {
            object = entt::null;
        }
    } else {
        method = p_callable.method;
        object = p_callable.object;
        custom_method =false;
    }

}

Callable::operator String() const {
    if (is_custom()) {
        return custom->get_as_text();
    }
    if (is_null()) {
        return "null::null";
    }

    Object *base = get_object();
    if (!base) {
        return "null::" + String(method);
    }

    String class_name = base->get_class();
    Ref<Script> script = refFromRefPtr<Script>(base->get_script());
    if (script && PathUtils::is_resource_file(script->get_path())) {
        class_name += "(" + PathUtils::get_file(script->get_path()) + ")";
    }
    return class_name + "::" + String(method);
}

Callable::Callable(const Object *p_object, const StringName &p_method) {
    if (p_method == StringName()) {
        object = entt::null;
        ERR_FAIL_MSG("Method argument to Callable constructor must be a non-empty string");
    }
    if (p_object == nullptr) {
        object = entt::null;
        ERR_FAIL_MSG("Object argument to Callable constructor must be non-null");
    }

    object = p_object->get_instance_id();
    assert(game_object_registry.registry.valid(object));
    method = p_method;
}

Callable::Callable(GameEntity p_object, const StringName &p_method) {
    if (p_method.empty()) {
        object = entt::null;
        ERR_FAIL_MSG("Method argument to Callable constructor must be a non-empty string");
    }

    object = p_object;
    assert(game_object_registry.valid(object));
    method = p_method;
}

Callable::Callable(CallableCustom *p_custom) {
    if (p_custom->referenced) {
        object = entt::null;
        ERR_FAIL_MSG("Callable custom is already referenced");
    }
    p_custom->referenced = true;
    object = entt::null; //ensure object is all zero, since pointer may be 32 bits
    custom = p_custom;
    custom_method = p_custom!=nullptr;
}

Callable::Callable(const Callable &p_callable) {
    if (p_callable.is_custom()) {
        if (p_callable.custom->ref_count.ref()) {
            custom = p_callable.custom;
            custom_method= p_callable.custom_method;
        }
    } else {
        method = p_callable.method;
        object = p_callable.object;
    }
}

Callable::~Callable() {
    if (is_custom()) {
        if (custom->ref_count.unref()) {
            memdelete(custom);
        }
    }
}

CallableCustom::CallableCustom() {
    ref_count.init();
}

//////////////////////////////////
Object *Signal::get_object() const {
    return object_for_entity(object);
}

GameEntity Signal::get_object_id() const {
    return object;
}

StringName Signal::get_name() const {
    return name;
}

bool Signal::operator!=(const Signal &p_signal) const {
    return object != p_signal.object || name != p_signal.name;
}

bool Signal::operator<(const Signal &p_signal) const {
    if (object == p_signal.object) {
        return name < p_signal.name;
    } else {
        return entt::to_integral(object) < entt::to_integral(p_signal.object);
    }
}

Signal::operator String() const {
    Object *base = get_object();
    if (!base) {
        return "null::[signal]" + String(name);
    }

    String class_name = base->get_class();
    Ref<Script> script = refFromRefPtr<Script>(base->get_script());
    if (script && PathUtils::is_resource_file(script->get_path())) {
        class_name += "(" + PathUtils::get_file(script->get_path()) + ")";
    }
    return class_name + "::[signal]" + String(name);
}

//void Signal::emit_signal(const Variant **p_arguments, int p_argcount) const {
//    Object *obj = object_for_entity(object);
//    if (!obj) {
//        return;// ERR_INVALID_DATA;
//    }

//    obj->emit_signal(name, p_arguments, p_argcount);
//}

Error Signal::connect(const Callable &p_callable, uint32_t p_flags) {
    Object *object = get_object();
    ERR_FAIL_COND_V(!object, ERR_UNCONFIGURED);

    return object->connect(name, p_callable,/* p_binds,*/ p_flags);
}

void Signal::disconnect(const Callable &p_callable) {
    Object *object = get_object();
    ERR_FAIL_COND(!object);
    object->disconnect(name, p_callable);
}

bool Signal::is_connected(const Callable &p_callable) const {
    Object *object = get_object();
    ERR_FAIL_COND_V(!object, false);

    return object->is_connected(name, p_callable);
}

Array Signal::get_connections() const {
    Object *object = get_object();
    if (!object) {
        return Array();
    }

    Vector<Object::Connection> connections;
    object->get_signal_connection_list(name, &connections);

    Array arr;
    arr.reserve(connections.size());
    for (Object::Connection &E : connections) {
        arr.emplace_back(E);
    }
    return arr;
}

Signal::Signal(const Object *p_object, const StringName &p_name) : name(p_name) {
    ERR_FAIL_COND_MSG(p_object == nullptr, "Object argument to Signal constructor must be non-null");

    object = p_object->get_instance_id();
}

