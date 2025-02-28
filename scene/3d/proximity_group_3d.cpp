/*************************************************************************/
/*  proximity_group_3d.cpp                                                  */
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

#include "proximity_group_3d.h"

#include "scene/main/scene_tree.h"
#include "core/math/math_funcs.h"
#include "core/method_bind.h"
#include "core/map.h"

IMPL_GDCLASS(ProximityGroup3D)
VARIANT_ENUM_CAST(ProximityGroup3D::DispatchMode);

void ProximityGroup3D::_clear_groups() {

    HashMap<StringName, uint32_t>::iterator E;

    const int size = 16;
    do {
        StringName remove_list[size];
        E = groups.begin();
        int num = 0;
        while (E!=groups.end() && num < size) {

            if (E->second != group_version) {
                remove_list[num++] = E->first;
            }

            ++E;
        }
        for (int i = 0; i < num; i++) {

            groups.erase(remove_list[i]);
        }
    } while (E != groups.end());
}

void ProximityGroup3D::_update_groups() {

    if (grid_radius == Vector3(0, 0, 0))
        return;

    ++group_version;

    Vector3 pos = get_global_transform().get_origin();
    Vector3 vcell = pos / cell_size;
    int cell[3] = { Math::fast_ftoi(vcell.x), Math::fast_ftoi(vcell.y), Math::fast_ftoi(vcell.z) };

    _add_groups(cell, group_name, 0);

    _clear_groups();
}

void ProximityGroup3D::_add_groups(int *p_cell, StringName p_base, int p_depth) {

    p_base = StringName(p_base + String("|"));
    if (grid_radius[p_depth] == 0) {

        if (p_depth == 2) {
            _new_group(p_base);
        } else {
            _add_groups(p_cell, p_base, p_depth + 1);
        }
    }

    int start = p_cell[p_depth] - grid_radius[p_depth];
    int end = p_cell[p_depth] + grid_radius[p_depth];

    for (int i = start; i <= end; i++) {

        StringName gname(p_base + itos(i));
        if (p_depth == 2) {
            _new_group(gname);
        } else {
            _add_groups(p_cell, gname, p_depth + 1);
        }
    }
}

void ProximityGroup3D::_new_group(const StringName& p_name) {

    const HashMap<StringName, uint32_t>::iterator E = groups.find(p_name);
    if (E==groups.end()) {
        add_to_group(p_name);
    }

    groups[p_name] = group_version;
}

void ProximityGroup3D::_notification(int p_what) {

    switch (p_what) {

        case NOTIFICATION_EXIT_TREE:
            ++group_version;
            _clear_groups();
            break;
        case NOTIFICATION_TRANSFORM_CHANGED:
            _update_groups();
            break;
    }
}

void ProximityGroup3D::broadcast(StringView p_method, const Variant& p_parameters) {

    HashMap<StringName, uint32_t>::iterator E;
    E = groups.begin();
    while (E!=groups.end()) {

        get_tree()->call_group_flags(SceneTree::GROUP_CALL_DEFAULT, E->first, "_proximity_group_broadcast", p_method, p_parameters);
        ++E;
    }
}

void ProximityGroup3D::_proximity_group_broadcast(const StringName& p_method, const Variant& p_parameters) {

    if (dispatch_mode == MODE_PROXY) {
        ERR_FAIL_COND(!is_inside_tree());
        get_parent()->call_va(p_method, p_parameters);
    } else {

        emit_signal("broadcast", p_method, p_parameters);
    }
}

void ProximityGroup3D::set_group_name(const StringName &p_group_name) {

    group_name = p_group_name;
}

StringName ProximityGroup3D::get_group_name() const {

    return group_name;
}

void ProximityGroup3D::set_dispatch_mode(DispatchMode p_mode) {

    dispatch_mode = p_mode;
}

ProximityGroup3D::DispatchMode ProximityGroup3D::get_dispatch_mode() const {

    return dispatch_mode;
}

void ProximityGroup3D::set_grid_radius(const Vector3 &p_radius) {

    grid_radius = p_radius;
}

Vector3 ProximityGroup3D::get_grid_radius() const {

    return grid_radius;
}

void ProximityGroup3D::_bind_methods() {

    SE_BIND_METHOD(ProximityGroup3D,set_group_name);
    SE_BIND_METHOD(ProximityGroup3D,get_group_name);
    SE_BIND_METHOD(ProximityGroup3D,set_dispatch_mode);
    SE_BIND_METHOD(ProximityGroup3D,get_dispatch_mode);
    SE_BIND_METHOD(ProximityGroup3D,set_grid_radius);
    SE_BIND_METHOD(ProximityGroup3D,get_grid_radius);
    SE_BIND_METHOD(ProximityGroup3D,broadcast);
    SE_BIND_METHOD(ProximityGroup3D,_proximity_group_broadcast);

    ADD_PROPERTY(PropertyInfo(VariantType::STRING, "group_name"), "set_group_name", "get_group_name");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "dispatch_mode", PropertyHint::Enum, "Proxy,Signal"), "set_dispatch_mode", "get_dispatch_mode");
    ADD_PROPERTY(PropertyInfo(VariantType::VECTOR3, "grid_radius"), "set_grid_radius", "get_grid_radius");

    ADD_SIGNAL(MethodInfo("broadcast", PropertyInfo(VariantType::STRING, "method"), PropertyInfo(VariantType::ARRAY, "parameters")));

    BIND_ENUM_CONSTANT(MODE_PROXY);
    BIND_ENUM_CONSTANT(MODE_SIGNAL);
}

ProximityGroup3D::ProximityGroup3D() {

    group_version = 0;
    dispatch_mode = MODE_PROXY;

    cell_size = 1.0;
    grid_radius = Vector3(1, 1, 1);
    set_notify_transform(true);
}
