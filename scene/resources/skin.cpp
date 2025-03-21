/*************************************************************************/
/*  skin.cpp                                                             */
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
#include "skin.h"

#include "core/list.h"

#include "core/method_bind_interface.h"
#include "core/method_bind.h"
#include "core/object_tooling.h"


IMPL_GDCLASS(Skin)

void Skin::set_bind_count(int p_size) {
    ERR_FAIL_COND(p_size < 0);
    binds.resize(p_size);
    binds_ptr = binds.data();
    bind_count = p_size;
    emit_changed();
}

void Skin::add_bind(int p_bone, const Transform &p_pose) {
    uint32_t index = bind_count;
    set_bind_count(bind_count + 1);
    set_bind_bone(index, p_bone);
    set_bind_pose(index, p_pose);
}

void Skin::add_named_bind(const StringName &p_name, const Transform &p_pose) {

    uint32_t index = bind_count;
    set_bind_count(bind_count + 1);
    set_bind_name(index, p_name);
    set_bind_pose(index, p_pose);
}

void Skin::set_bind_name(int p_index, const StringName &p_name) {
    ERR_FAIL_INDEX(p_index, bind_count);
    bool notify_change = (binds_ptr[p_index].name != StringName()) != (p_name != StringName());
    binds_ptr[p_index].name = p_name;
    emit_changed();
    if (notify_change) {
        Object_change_notify(this);
    }
}

void Skin::set_bind_bone(int p_index, int p_bone) {
    ERR_FAIL_INDEX(p_index, bind_count);
    binds_ptr[p_index].bone = p_bone;
    emit_changed();
}

void Skin::set_bind_pose(int p_index, const Transform &p_pose) {
    ERR_FAIL_INDEX(p_index, bind_count);
    binds_ptr[p_index].pose = p_pose;
    emit_changed();
}

void Skin::clear_binds() {
    binds.clear();
    binds_ptr = nullptr;
    bind_count = 0;
    emit_changed();
}

bool Skin::_set(const StringName &p_name, const Variant &p_value) {
    using namespace StringUtils;
    if (p_name == "bind_count") {
        set_bind_count(p_value.as<int>());
        return true;
    } else if (begins_with(p_name,"bind/")) {
        int index = to_int(get_slice(p_name,'/', 1));
        StringName what(get_slice(p_name,'/', 2));
        if (what == "bone") {
            set_bind_bone(index, p_value.as<int>());
            return true;
        } else if (what == "name") {
            set_bind_name(index, p_value.as<StringName>());
            return true;
        } else if (what == "pose") {
            set_bind_pose(index, p_value.as<Transform>());
            return true;
        }
    }
    return false;
}

bool Skin::_get(const StringName &p_name, Variant &r_ret) const {
    using namespace StringUtils;

    if (p_name == "bind_count") {
        r_ret = get_bind_count();
        return true;
    } else if (begins_with(p_name,"bind/")) {
        int index = to_int(get_slice(p_name,'/', 1));
        StringName what(get_slice(p_name,'/', 2));
        if (what == "bone") {
            r_ret = get_bind_bone(index);
            return true;
        } else if (what == "name") {
            r_ret = get_bind_name(index);
            return true;
        } else if (what == "pose") {
            r_ret = get_bind_pose(index);
            return true;
        }
    }
    return false;
}
void Skin::_get_property_list(Vector<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(VariantType::INT, "bind_count", PropertyHint::Range, "0,16384,1,or_greater"));
    for (int i = 0; i < get_bind_count(); i++) {
        p_list->emplace_back(VariantType::STRING_NAME, StringName("bind/" + itos(i) + "/name"));
        auto bone_flag= !get_bind_name(i).empty() ? PROPERTY_USAGE_NOEDITOR : PROPERTY_USAGE_DEFAULT;
        p_list->emplace_back(VariantType::INT, StringName("bind/" + itos(i) + "/bone"), PropertyHint::Range, "0,16384,1,or_greater",bone_flag);
        p_list->emplace_back(VariantType::TRANSFORM, StringName("bind/" + itos(i) + "/pose"));
    }
}

void Skin::_bind_methods() {

    SE_BIND_METHOD(Skin,set_bind_count);
    SE_BIND_METHOD(Skin,get_bind_count);

    SE_BIND_METHOD(Skin,add_bind);

    SE_BIND_METHOD(Skin,set_bind_pose);
    SE_BIND_METHOD(Skin,get_bind_pose);

    SE_BIND_METHOD(Skin,set_bind_name);
    SE_BIND_METHOD(Skin,get_bind_name);

    SE_BIND_METHOD(Skin,set_bind_bone);
    SE_BIND_METHOD(Skin,get_bind_bone);

    SE_BIND_METHOD(Skin,clear_binds);
}

Skin::Skin() {
    bind_count = 0;
    binds_ptr = nullptr;
}
