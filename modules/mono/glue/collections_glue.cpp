/*************************************************************************/
/*  collections_glue.cpp                                                 */
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

#include "collections_glue.h"

#include "core/dictionary.h"

#include "../mono_gd/gd_mono_cache.h"
#include "../mono_gd/gd_mono_class.h"
#include "../mono_gd/gd_mono_utils.h"

#include <mono/metadata/exception.h>

Array *godot_icall_Array_Ctor() {
    return memnew(Array);
}

void godot_icall_Array_Dtor(Array *ptr) {
    memdelete(ptr);
}

MonoObject *godot_icall_Array_At(Array *ptr, int index) {
    if (index < 0 || index >= ptr->size()) {
        GDMonoUtils::set_pending_exception(mono_get_exception_index_out_of_range());
        return nullptr;
    }
    return GDMonoMarshal::variant_to_mono_object(ptr->operator[](index));
}

MonoObject *godot_icall_Array_At_Generic(Array *ptr, int index, uint32_t type_encoding, GDMonoClass *type_class) {
    if (index < 0 || index >= ptr->size()) {
        GDMonoUtils::set_pending_exception(mono_get_exception_index_out_of_range());
        return nullptr;
    }
    return GDMonoMarshal::variant_to_mono_object(ptr->operator[](index), ManagedType(type_encoding, type_class));
}

void godot_icall_Array_SetAt(Array *ptr, int index, MonoObject *value) {
    if (index < 0 || index >= ptr->size()) {
        GDMonoUtils::set_pending_exception(mono_get_exception_index_out_of_range());
        return;
    }
    ptr->operator[](index) = GDMonoMarshal::mono_object_to_variant(value);
}

int godot_icall_Array_Count(Array *ptr) {
    return ptr->size();
}

int godot_icall_Array_Add(Array *ptr, MonoObject *item) {
    ptr->append(GDMonoMarshal::mono_object_to_variant(item));
    return ptr->size();
}

void godot_icall_Array_Clear(Array *ptr) {
    ptr->clear();
}

MonoBoolean godot_icall_Array_Contains(Array *ptr, MonoObject *item) {
    return ptr->find(GDMonoMarshal::mono_object_to_variant(item)) != -1;
}

void godot_icall_Array_CopyTo(Array *ptr, MonoArray *array, int array_index) {
    unsigned int count = ptr->size();

    if (mono_array_length(array) < (array_index + count)) {
        MonoException *exc = mono_get_exception_argument("", "Destination array was not long enough. Check destIndex and length, and the array's lower bounds.");
        GDMonoUtils::set_pending_exception(exc);
        return;
    }

    for (unsigned int i = 0; i < count; i++) {
        MonoObject *boxed = GDMonoMarshal::variant_to_mono_object(ptr->operator[](i));
        mono_array_setref(array, array_index, boxed);
        array_index++;
    }
}

Array *godot_icall_Array_Ctor_MonoArray(MonoArray *mono_array) {
    Array *godot_array = memnew(Array);
    unsigned int count = mono_array_length(mono_array);
    godot_array->resize(count);
    for (unsigned int i = 0; i < count; i++) {
        MonoObject *item = mono_array_get(mono_array, MonoObject *, i);
        godot_icall_Array_SetAt(godot_array, i, item);
    }
    return godot_array;
}
Array *godot_icall_Array_Duplicate(Array *ptr, MonoBoolean deep) {
    return memnew(Array(ptr->duplicate(deep)));
}
Array *godot_icall_Array_Concatenate(Array *left, Array *right) {
    int count = left->size() + right->size();
    Array *new_array = memnew(Array(left->duplicate(false)));
    new_array->resize(count);
    for (unsigned int i = 0; i < (unsigned int)right->size(); i++) {
        new_array->operator[](i + left->size()) = right->operator[](i);
    }
    return new_array;
}
int godot_icall_Array_IndexOf(Array *ptr, MonoObject *item) {
    return ptr->find(GDMonoMarshal::mono_object_to_variant(item));
}

void godot_icall_Array_Insert(Array *ptr, int index, MonoObject *item) {
    if (index < 0 || index > ptr->size()) {
        GDMonoUtils::set_pending_exception(mono_get_exception_index_out_of_range());
        return;
    }
    ptr->insert(index, GDMonoMarshal::mono_object_to_variant(item));
}

MonoBoolean godot_icall_Array_Remove(Array *ptr, MonoObject *item) {
    int idx = ptr->find(GDMonoMarshal::mono_object_to_variant(item));
    if (idx >= 0) {
        ptr->remove(idx);
        return true;
    }
    return false;
}

void godot_icall_Array_RemoveAt(Array *ptr, int index) {
    if (index < 0 || index >= ptr->size()) {
        GDMonoUtils::set_pending_exception(mono_get_exception_index_out_of_range());
        return;
    }
    ptr->remove(index);
}

Error godot_icall_Array_Resize(Array *ptr, int new_size) {
    ptr->resize(new_size);
    return OK;
}

void godot_icall_Array_Generic_GetElementTypeInfo(MonoReflectionType *refltype, uint32_t *type_encoding, GDMonoClass **type_class) {
    MonoType *elem_type = mono_reflection_type_get_type(refltype);

    *type_encoding = mono_type_get_type(elem_type);
    MonoClass *type_class_raw = mono_class_from_mono_type(elem_type);
    *type_class = GDMono::get_singleton()->get_class(type_class_raw);
}

MonoString *godot_icall_Array_ToString(Array *ptr) {
    return GDMonoMarshal::mono_string_from_godot(Variant(*ptr).as<String>());
}

Dictionary *godot_icall_Dictionary_Ctor() {
    return memnew(Dictionary);
}

void godot_icall_Dictionary_Dtor(Dictionary *ptr) {
    memdelete(ptr);
}

MonoObject *godot_icall_Dictionary_GetValue(Dictionary *ptr, MonoString* key) {
    Variant *ret = ptr->getptr(StringName(GDMonoMarshal::mono_string_to_godot(key)));
    if (ret != nullptr) 
        return GDMonoMarshal::variant_to_mono_object(*ret);

    MonoObject *exc = mono_object_new(mono_domain_get(), CACHED_CLASS(KeyNotFoundException)->get_mono_ptr());
#ifdef DEBUG_ENABLED
    CRASH_COND(!exc);
#endif
    GDMonoUtils::runtime_object_init(exc, CACHED_CLASS(KeyNotFoundException));
    GDMonoUtils::set_pending_exception((MonoException *)exc);
    return nullptr;
}

MonoObject *godot_icall_Dictionary_GetValue_Generic(Dictionary *ptr, MonoString* key, uint32_t type_encoding, GDMonoClass *type_class) {
    Variant *ret = ptr->getptr(StringName(GDMonoMarshal::mono_string_to_godot(key)));
    if (ret == nullptr) {
        MonoObject *exc = mono_object_new(mono_domain_get(), CACHED_CLASS(KeyNotFoundException)->get_mono_ptr());
#ifdef DEBUG_ENABLED
        CRASH_COND(!exc);
#endif
        GDMonoUtils::runtime_object_init(exc, CACHED_CLASS(KeyNotFoundException));
        GDMonoUtils::set_pending_exception((MonoException *)exc);
        return nullptr;
    }
    return GDMonoMarshal::variant_to_mono_object(ret, ManagedType(type_encoding, type_class));
}

void godot_icall_Dictionary_SetValue(Dictionary *ptr, MonoString* key, MonoObject *value) {
    ptr->operator[](StringName(GDMonoMarshal::mono_string_to_godot(key))) = GDMonoMarshal::mono_object_to_variant(value);
}

Array *godot_icall_Dictionary_Keys(Dictionary *ptr) {
    return memnew(Array(ptr->keys()));
}

Array *godot_icall_Dictionary_Values(Dictionary *ptr) {
    return memnew(Array(ptr->values()));
}

int godot_icall_Dictionary_Count(Dictionary *ptr) {
    return ptr->size();
}

int32_t godot_icall_Dictionary_KeyValuePairs(Dictionary *ptr, Array **keys, Array **values) {
    *keys = godot_icall_Dictionary_Keys(ptr);
    *values = godot_icall_Dictionary_Values(ptr);
    return godot_icall_Dictionary_Count(ptr);
}

void godot_icall_Dictionary_KeyValuePairAt(Dictionary *ptr, int index, MonoString **key, MonoObject **value) {
    *key = GDMonoMarshal::mono_string_from_godot(ptr->get_key_at_index(index));
    *value = GDMonoMarshal::variant_to_mono_object(ptr->get_value_at_index(index));
}

void godot_icall_Dictionary_KeyValuePairAt_Generic(Dictionary *ptr, int index, MonoString **key, MonoObject **value, uint32_t value_type_encoding, GDMonoClass *value_type_class) {
    ManagedType type(value_type_encoding, value_type_class);
    *key = GDMonoMarshal::mono_string_from_godot(ptr->get_key_at_index(index));
    *value = GDMonoMarshal::variant_to_mono_object(ptr->get_value_at_index(index), type);
}
void godot_icall_Dictionary_Add(Dictionary *ptr, MonoString*key, MonoObject *value) {
    auto varKey = StringName(GDMonoMarshal::mono_string_to_godot(key));
    Variant *ret = ptr->getptr(varKey);
    if (ret != nullptr) {
        GDMonoUtils::set_pending_exception(mono_get_exception_argument("key", "An element with the same key already exists"));
        return;
    }
    ptr->operator[](varKey) = GDMonoMarshal::mono_object_to_variant(value);
}

void godot_icall_Dictionary_Clear(Dictionary *ptr) {
    ptr->clear();
}

MonoBoolean godot_icall_Dictionary_Contains(Dictionary *ptr, MonoString*key, MonoObject *value) {
    // no dupes
    Variant *ret = ptr->getptr(StringName(GDMonoMarshal::mono_string_to_godot(key)));
    return ret != nullptr && *ret == GDMonoMarshal::mono_object_to_variant(value);
}

MonoBoolean godot_icall_Dictionary_ContainsKey(Dictionary *ptr, MonoString*key) {
    return ptr->has(StringName(GDMonoMarshal::mono_string_to_godot(key)));
}

Dictionary *godot_icall_Dictionary_Duplicate(Dictionary *ptr, MonoBoolean deep) {
    return memnew(Dictionary(ptr->duplicate(deep)));
}

MonoBoolean godot_icall_Dictionary_RemoveKey(Dictionary *ptr, MonoString *key) {
    return ptr->erase(StringName(GDMonoMarshal::mono_string_to_godot(key)));
}

MonoBoolean godot_icall_Dictionary_Remove(Dictionary *ptr, MonoString*key, MonoObject *value) {
    auto varKey = StringName(GDMonoMarshal::mono_string_to_godot(key));

    // no dupes
    Variant *ret = ptr->getptr(varKey);
    if (ret != nullptr && *ret == GDMonoMarshal::mono_object_to_variant(value)) {
        ptr->erase(varKey);
        return true;
    }

    return false;
}

MonoBoolean godot_icall_Dictionary_TryGetValue(Dictionary *ptr, MonoString*key, MonoObject **value) {
    Variant *ret = ptr->getptr(StringName(GDMonoMarshal::mono_string_to_godot(key)));
    if (ret == nullptr) {
        *value = nullptr;
        return false;
    }
    *value = GDMonoMarshal::variant_to_mono_object(*ret);
    return true;
}

MonoBoolean godot_icall_Dictionary_TryGetValue_Generic(Dictionary *ptr, MonoString*key, MonoObject **value, uint32_t type_encoding, GDMonoClass *type_class) {
    Variant *ret = ptr->getptr(StringName(GDMonoMarshal::mono_string_to_godot(key)));
    if (ret == nullptr) {
        *value = nullptr;
        return false;
    }
    *value = GDMonoMarshal::variant_to_mono_object(ret, ManagedType(type_encoding, type_class));
    return true;
}

void godot_icall_Dictionary_Generic_GetValueTypeInfo(MonoReflectionType *refltype, uint32_t *type_encoding, GDMonoClass **type_class) {
    MonoType *value_type = mono_reflection_type_get_type(refltype);

    *type_encoding = mono_type_get_type(value_type);
    MonoClass *type_class_raw = mono_class_from_mono_type(value_type);
    *type_class = GDMono::get_singleton()->get_class(type_class_raw);
}

MonoString *godot_icall_Dictionary_ToString(Dictionary *ptr) {
    return GDMonoMarshal::mono_string_from_godot(Variant(*ptr).as<String>());
}

void godot_register_collections_icalls() {
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Ctor", (void *)godot_icall_Array_Ctor);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Ctor_MonoArray", (void *)godot_icall_Array_Ctor_MonoArray);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Dtor", (void *)godot_icall_Array_Dtor);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_At", (void *)godot_icall_Array_At);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_At_Generic", (void *)godot_icall_Array_At_Generic);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_SetAt", (void *)godot_icall_Array_SetAt);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Count", (void *)godot_icall_Array_Count);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Add", (void *)godot_icall_Array_Add);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Clear", (void *)godot_icall_Array_Clear);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Concatenate", (void *)godot_icall_Array_Concatenate);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Contains", (void *)godot_icall_Array_Contains);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_CopyTo", (void *)godot_icall_Array_CopyTo);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Duplicate", (void *)godot_icall_Array_Duplicate);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_IndexOf", (void *)godot_icall_Array_IndexOf);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Insert", (void *)godot_icall_Array_Insert);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Remove", (void *)godot_icall_Array_Remove);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_RemoveAt", (void *)godot_icall_Array_RemoveAt);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Resize", (void *)godot_icall_Array_Resize);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_Generic_GetElementTypeInfo", (void *)godot_icall_Array_Generic_GetElementTypeInfo);
    mono_add_internal_call("Godot.Collections.Array::godot_icall_Array_ToString", (void *)godot_icall_Array_ToString);

    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Ctor", (void *)godot_icall_Dictionary_Ctor);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Dtor", (void *)godot_icall_Dictionary_Dtor);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_GetValue", (void *)godot_icall_Dictionary_GetValue);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_GetValue_Generic", (void *)godot_icall_Dictionary_GetValue_Generic);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_SetValue", (void *)godot_icall_Dictionary_SetValue);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Keys", (void *)godot_icall_Dictionary_Keys);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Values", (void *)godot_icall_Dictionary_Values);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Count", (void *)godot_icall_Dictionary_Count);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_KeyValuePairs", (void *)godot_icall_Dictionary_KeyValuePairs);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_KeyValuePairAt", (void *)godot_icall_Dictionary_KeyValuePairAt);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_KeyValuePairAt_Generic", (void*)godot_icall_Dictionary_KeyValuePairAt_Generic);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Add", (void *)godot_icall_Dictionary_Add);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Clear", (void *)godot_icall_Dictionary_Clear);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Contains", (void *)godot_icall_Dictionary_Contains);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_ContainsKey", (void *)godot_icall_Dictionary_ContainsKey);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Duplicate", (void *)godot_icall_Dictionary_Duplicate);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_RemoveKey", (void *)godot_icall_Dictionary_RemoveKey);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Remove", (void *)godot_icall_Dictionary_Remove);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_TryGetValue", (void *)godot_icall_Dictionary_TryGetValue);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_TryGetValue_Generic", (void *)godot_icall_Dictionary_TryGetValue_Generic);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_Generic_GetValueTypeInfo", (void *)godot_icall_Dictionary_Generic_GetValueTypeInfo);
    mono_add_internal_call("Godot.Collections.Dictionary::godot_icall_Dictionary_ToString", (void *)godot_icall_Dictionary_ToString);
}
