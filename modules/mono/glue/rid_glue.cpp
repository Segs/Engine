/*************************************************************************/
/*  rid_glue.cpp                                                         */
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

#include "rid_glue.h"

#include "core/resource.h"

RID *godot_icall_RID_Ctor(Object *p_from) {
    Resource *res_from = object_cast<Resource>(p_from);

    if (res_from)
        return memnew(RID(res_from->get_phys_rid()));

    return memnew(RID);
}

void godot_icall_RID_Dtor(RID *p_ptr) {
    ERR_FAIL_NULL(p_ptr);
    memdelete(p_ptr);
}

uint32_t godot_icall_RID_get_id(RID *p_ptr) {
    return p_ptr->get_id();
}
static_assert(entt::to_integral(GameEntity(entt::null)) == ~0, "GameEntity(entt::null) must be ~0");
static_assert(entt::to_integral(RenderingEntity(entt::null)) == ~0, "RenderingEntity(entt::null) must be ~0");

void godot_register_rid_icalls() {
    mono_add_internal_call("Godot.RID::godot_icall_RID_Ctor", (void *)godot_icall_RID_Ctor);
    mono_add_internal_call("Godot.RID::godot_icall_RID_Dtor", (void *)godot_icall_RID_Dtor);
    mono_add_internal_call("Godot.RID::godot_icall_RID_get_id", (void *)godot_icall_RID_get_id);
}
