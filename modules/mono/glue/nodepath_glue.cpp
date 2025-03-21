/*************************************************************************/
/*  nodepath_glue.cpp                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "nodepath_glue.h"

#include "core/ustring.h"

NodePath *godot_icall_NodePath_Ctor(MonoString *p_path) {
    return memnew(NodePath(GDMonoMarshal::mono_string_to_godot(p_path)));
}

void godot_icall_NodePath_Dtor(NodePath *p_ptr) {
    ERR_FAIL_NULL(p_ptr);
    memdelete(p_ptr);
}

MonoString *godot_icall_NodePath_operator_String(NodePath *p_np) {
    return GDMonoMarshal::mono_string_from_godot(p_np->operator String());
}

MonoBoolean godot_icall_NodePath_is_absolute(NodePath *p_ptr) {
    return (MonoBoolean)p_ptr->is_absolute();
}

uint32_t godot_icall_NodePath_get_name_count(NodePath *p_ptr) {
    return p_ptr->get_name_count();
}

MonoString *godot_icall_NodePath_get_name(NodePath *p_ptr, uint32_t p_idx) {
    return GDMonoMarshal::mono_string_from_godot(p_ptr->get_name(p_idx));
}

uint32_t godot_icall_NodePath_get_subname_count(NodePath *p_ptr) {
    return p_ptr->get_subname_count();
}

MonoString *godot_icall_NodePath_get_subname(NodePath *p_ptr, uint32_t p_idx) {
    return GDMonoMarshal::mono_string_from_godot(p_ptr->get_subname(p_idx));
}

MonoString *godot_icall_NodePath_get_concatenated_subnames(NodePath *p_ptr) {
    return GDMonoMarshal::mono_string_from_godot(p_ptr->get_concatenated_subnames());
}

NodePath *godot_icall_NodePath_get_as_property_path(NodePath *p_ptr) {
    return memnew(NodePath(p_ptr->get_as_property_path()));
}

MonoBoolean godot_icall_NodePath_is_empty(NodePath *p_ptr) {
    return (MonoBoolean)p_ptr->is_empty();
}

void godot_register_nodepath_icalls() {
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_Ctor", (void *)godot_icall_NodePath_Ctor);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_Dtor", (void *)godot_icall_NodePath_Dtor);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_operator_String", (void *)godot_icall_NodePath_operator_String);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_get_as_property_path", (void *)godot_icall_NodePath_get_as_property_path);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_get_concatenated_subnames", (void *)godot_icall_NodePath_get_concatenated_subnames);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_get_name", (void *)godot_icall_NodePath_get_name);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_get_name_count", (void *)godot_icall_NodePath_get_name_count);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_get_subname", (void *)godot_icall_NodePath_get_subname);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_get_subname_count", (void *)godot_icall_NodePath_get_subname_count);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_is_absolute", (void *)godot_icall_NodePath_is_absolute);
    mono_add_internal_call("Godot.NodePath::godot_icall_NodePath_is_empty", (void *)godot_icall_NodePath_is_empty);
}
