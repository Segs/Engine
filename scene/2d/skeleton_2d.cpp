/*************************************************************************/
/*  skeleton_2d.cpp                                                      */
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

#include "skeleton_2d.h"
#include "core/method_bind.h"
#include "core/translation_helpers.h"
#include "servers/rendering_server.h"
#include "EASTL/sort.h"

IMPL_GDCLASS(Bone2D)
IMPL_GDCLASS(Skeleton2D)

void Bone2D::_notification(int p_what) {

    if (p_what == NOTIFICATION_ENTER_TREE) {
        Node *parent = get_parent();
        parent_bone = object_cast<Bone2D>(parent);
        skeleton = nullptr;
        while (parent) {
            skeleton = object_cast<Skeleton2D>(parent);
            if (skeleton)
                break;
            if (!object_cast<Bone2D>(parent))
                break; //skeletons must be chained to Bone2Ds.

            parent = parent->get_parent();
        }

        if (skeleton) {
            Skeleton2D::Bone bone;
            bone.bone = this;
            skeleton->bones.push_back(bone);
            skeleton->_make_bone_setup_dirty();
        }
    }
    if (p_what == NOTIFICATION_LOCAL_TRANSFORM_CHANGED) {
        if (skeleton) {
            skeleton->_make_transform_dirty();
        }
    }
    if (p_what == NOTIFICATION_MOVED_IN_PARENT) {
        if (skeleton) {
            skeleton->_make_bone_setup_dirty();
        }
    }

    if (p_what == NOTIFICATION_EXIT_TREE) {
        if (skeleton) {
            for (int i = 0; i < skeleton->bones.size(); i++) {
                if (skeleton->bones[i].bone == this) {
                    skeleton->bones.erase_at(i);
                    break;
                }
            }
            skeleton->_make_bone_setup_dirty();
            skeleton = nullptr;
        }
        parent_bone = nullptr;
    }
}
void Bone2D::_bind_methods() {

    SE_BIND_METHOD(Bone2D,set_rest);
    SE_BIND_METHOD(Bone2D,get_rest);
    SE_BIND_METHOD(Bone2D,apply_rest);
    SE_BIND_METHOD(Bone2D,get_skeleton_rest);
    SE_BIND_METHOD(Bone2D,get_index_in_skeleton);

    SE_BIND_METHOD(Bone2D,set_default_length);
    SE_BIND_METHOD(Bone2D,get_default_length);

    ADD_PROPERTY(PropertyInfo(VariantType::TRANSFORM2D, "rest"), "set_rest", "get_rest");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "default_length", PropertyHint::Range, "1,1024,1"), "set_default_length", "get_default_length");
}

void Bone2D::set_rest(const Transform2D &p_rest) {
    rest = p_rest;
    if (skeleton)
        skeleton->_make_bone_setup_dirty();

    update_configuration_warning();
}

Transform2D Bone2D::get_rest() const {
    return rest;
}

Transform2D Bone2D::get_skeleton_rest() const {

    if (parent_bone) {
        return parent_bone->get_skeleton_rest() * rest;
    } else {
        return rest;
    }
}

void Bone2D::apply_rest() {
    set_transform(rest);
}

void Bone2D::set_default_length(float p_length) {

    default_length = p_length;
}

float Bone2D::get_default_length() const {
    return default_length;
}

int Bone2D::get_index_in_skeleton() const {
    ERR_FAIL_COND_V(!skeleton, -1);
    skeleton->_update_bone_setup();
    return skeleton_index;
}
String Bone2D::get_configuration_warning() const {

    String warning(Node2D::get_configuration_warning());
    if (!skeleton) {
        if (!warning.empty()) {
            warning += "\n\n";
        }
        if (parent_bone) {
            warning += TTR("This Bone2D chain should end at a Skeleton2D node.");
        } else {
            warning += TTR("A Bone2D only works with a Skeleton2D or another Bone2D as parent node.");
        }
    }

    if (rest == Transform2D(0, 0, 0, 0, 0, 0)) {
        if (!warning.empty()) {
            warning += "\n\n";
        }
        warning += TTR("This bone lacks a proper REST pose. Go to the Skeleton2D node and set one.");
    }

    return warning;
}

Bone2D::Bone2D() {
    skeleton = nullptr;
    parent_bone = nullptr;
    skeleton_index = -1;
    default_length = 16;
    set_notify_local_transform(true);
    //this is a clever hack so the bone knows no rest has been set yet, allowing to show an error.
    for (int i = 0; i < 3; i++) {
        rest[i] = Vector2(0, 0);
    }
}

//////////////////////////////////////

void Skeleton2D::_make_bone_setup_dirty() {

    if (bone_setup_dirty)
        return;
    bone_setup_dirty = true;
    if (is_inside_tree()) {
        call_deferred([this]() { _update_bone_setup(); });
    }
}

void Skeleton2D::_update_bone_setup() {

    if (!bone_setup_dirty)
        return;

    bone_setup_dirty = false;
    RenderingServer::get_singleton()->skeleton_allocate(skeleton, bones.size(), true);
    eastl::sort(bones.begin(), bones.end()); //sorty so they are always in the same order/index

    for (int i = 0; i < bones.size(); i++) {
        bones[i].rest_inverse = bones[i].bone->get_skeleton_rest().affine_inverse(); //bind pose
        bones[i].bone->skeleton_index = i;
        Bone2D *parent_bone = object_cast<Bone2D>(bones[i].bone->get_parent());
        if (parent_bone) {
            bones[i].parent_index = parent_bone->skeleton_index;
        } else {
            bones[i].parent_index = -1;
        }
    }

    transform_dirty = true;
    _update_transform();
    emit_signal("bone_setup_changed");
}

void Skeleton2D::_make_transform_dirty() {

    if (transform_dirty)
        return;
    transform_dirty = true;
    if (is_inside_tree()) {
        call_deferred([this]() { _update_transform(); });
    }
}

void Skeleton2D::_update_transform() {

    if (bone_setup_dirty) {
        _update_bone_setup();
        return; //above will update transform anyway
    }
    if (!transform_dirty)
        return;

    transform_dirty = false;

    for (int i = 0; i < bones.size(); i++) {

        ERR_CONTINUE(bones[i].parent_index >= i);
        if (bones[i].parent_index >= 0) {
            bones[i].accum_transform = bones[bones[i].parent_index].accum_transform * bones[i].bone->get_transform();
        } else {
            bones[i].accum_transform = bones[i].bone->get_transform();
        }
    }

    for (int i = 0; i < bones.size(); i++) {

        Transform2D final_xform = bones[i].accum_transform * bones[i].rest_inverse;
        RenderingServer::get_singleton()->skeleton_bone_set_transform_2d(skeleton, i, final_xform);
    }
}

int Skeleton2D::get_bone_count() const {

    ERR_FAIL_COND_V(!is_inside_tree(), 0);

    if (bone_setup_dirty) {
        const_cast<Skeleton2D *>(this)->_update_bone_setup();
    }

    return bones.size();
}

Bone2D *Skeleton2D::get_bone(int p_idx) {

    ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
    ERR_FAIL_INDEX_V(p_idx, bones.size(), nullptr);

    return bones[p_idx].bone;
}

void Skeleton2D::_notification(int p_what) {

    if (p_what == NOTIFICATION_READY) {

        if (bone_setup_dirty)
            _update_bone_setup();
        if (transform_dirty)
            _update_transform();

        request_ready();
    }

    if (p_what == NOTIFICATION_TRANSFORM_CHANGED) {
        RenderingServer::get_singleton()->skeleton_set_base_transform_2d(skeleton, get_global_transform());
    }
}

RenderingEntity Skeleton2D::get_skeleton() const {
    return skeleton;
}
void Skeleton2D::_bind_methods() {

    SE_BIND_METHOD(Skeleton2D,_update_bone_setup);
    SE_BIND_METHOD(Skeleton2D,_update_transform);

    SE_BIND_METHOD(Skeleton2D,get_bone_count);
    SE_BIND_METHOD(Skeleton2D,get_bone);

    SE_BIND_METHOD(Skeleton2D,get_skeleton);

    ADD_SIGNAL(MethodInfo("bone_setup_changed"));
}

Skeleton2D::Skeleton2D() {
    bone_setup_dirty = true;
    transform_dirty = true;

    skeleton = RenderingServer::get_singleton()->skeleton_create();
    set_notify_transform(true);
}

Skeleton2D::~Skeleton2D() {

    RenderingServer::get_singleton()->free_rid(skeleton);
}
