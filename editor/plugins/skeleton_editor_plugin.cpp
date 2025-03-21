/*************************************************************************/
/*  skeleton_editor_plugin.cpp                                           */
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

#include "skeleton_editor_plugin.h"

#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "core/translation_helpers.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/3d/physics_joint_3d.h"
#include "scene/gui/menu_button.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/capsule_shape_3d.h"
#include "scene/resources/sphere_shape_3d.h"
#include "node_3d_editor_plugin.h"

IMPL_GDCLASS(SkeletonEditor)
IMPL_GDCLASS(SkeletonEditorPlugin)

void SkeletonEditor::_on_click_option(int p_option) {
    if (!skeleton) {
        return;
    }

    switch (p_option) {
        case MENU_OPTION_CREATE_PHYSICAL_SKELETON: {
            create_physical_skeleton();
        } break;
    }
}

void SkeletonEditor::create_physical_skeleton() {
    UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
    Node *owner = skeleton == get_tree()->get_edited_scene_root() ? skeleton : skeleton->get_owner();

    const int bc = skeleton->get_bone_count();

    if (!bc) {
        return;
    }

    Vector<BoneInfo> bones_infos;
    bones_infos.resize(bc);

    for (int bone_id = 0; bc > bone_id; ++bone_id) {

        const int parent = skeleton->get_bone_parent(bone_id);

        if (parent < 0) {

            bones_infos[bone_id].relative_rest = skeleton->get_bone_rest(bone_id);

        } else {

            const int parent_parent = skeleton->get_bone_parent(parent);

            bones_infos[bone_id].relative_rest = bones_infos[parent].relative_rest * skeleton->get_bone_rest(bone_id);

            /// create physical bone on parent
            if (!bones_infos[parent].physical_bone) {

                bones_infos[parent].physical_bone = create_physical_bone(parent, bone_id, bones_infos);

                ur->create_action(TTR("Create physical bones"), UndoRedo::MERGE_ALL);
                ur->add_do_method(skeleton, "add_child", Variant(bones_infos[parent].physical_bone));
                ur->add_do_reference(bones_infos[parent].physical_bone);
                ur->add_undo_method(skeleton, "remove_child", Variant(bones_infos[parent].physical_bone));
                ur->commit_action();

                bones_infos[parent].physical_bone->set_bone_name(skeleton->get_bone_name(parent));
                bones_infos[parent].physical_bone->set_owner(owner);
                bones_infos[parent].physical_bone->get_child(0)->set_owner(owner); // set shape owner

                /// Create joint between parent of parent
                if (-1 != parent_parent) {

                    bones_infos[parent].physical_bone->set_joint_type(PhysicalBone3D::JOINT_TYPE_PIN);
                }
            }
        }
    }
}

PhysicalBone3D *SkeletonEditor::create_physical_bone(int bone_id, int bone_child_id, const Vector<SkeletonEditor::BoneInfo> &bones_infos) {
    const Transform child_rest = skeleton->get_bone_rest(bone_child_id);

    real_t half_height(child_rest.origin.length() * 0.5);
    real_t radius(half_height * 0.2);

    CapsuleShape3D *bone_shape_capsule = memnew(CapsuleShape3D);
    bone_shape_capsule->set_height((half_height - radius) * 2);
    bone_shape_capsule->set_radius(radius);

    CollisionShape3D *bone_shape = memnew(CollisionShape3D);
    bone_shape->set_shape(Ref<Shape>(bone_shape_capsule,DoNotAddRef));

    Transform capsule_transform;
    bone_shape->set_transform(capsule_transform);
    Vector3 up = Vector3(0, 1, 0);
    if (up.cross(child_rest.origin).length() == 0) {
        up = Vector3(0, 0, 1);
    }


    Transform body_transform;
    body_transform.set_look_at(Vector3(0, 0, 0), child_rest.origin, up);
    body_transform.origin = body_transform.basis.xform(Vector3(0, 0, -half_height));

    Transform joint_transform;
    joint_transform.origin = Vector3(0, 0, half_height);

    PhysicalBone3D *physical_bone = memnew(PhysicalBone3D);
    physical_bone->add_child(bone_shape);
    physical_bone->set_name("Physical Bone " + skeleton->get_bone_name(bone_id));
    physical_bone->set_body_offset(body_transform);
    physical_bone->set_joint_offset(joint_transform);
    return physical_bone;
}

void SkeletonEditor::edit(Skeleton *p_node) {

    skeleton = p_node;
}

void SkeletonEditor::_notification(int p_what) {
    if (p_what == NOTIFICATION_ENTER_TREE) {
        get_tree()->connect("node_removed",callable_mp(this, &ClassName::_node_removed));
    }
}

void SkeletonEditor::_node_removed(Node *p_node) {

    if (p_node == skeleton) {
        skeleton = nullptr;
        options->hide();
    }
}

void SkeletonEditor::_bind_methods() {
    MethodBinder::bind_method("_on_click_option", &SkeletonEditor::_on_click_option);
}

SkeletonEditor::SkeletonEditor() {
    skeleton = nullptr;
    options = memnew(MenuButton);
    Node3DEditor::get_singleton()->add_control_to_menu_panel(options);

    options->set_text(TTR("Skeleton"));
    options->set_button_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("Skeleton3D", "EditorIcons"));

    options->get_popup()->add_item(TTR("Create physical skeleton"), MENU_OPTION_CREATE_PHYSICAL_SKELETON);

    options->get_popup()->connect("id_pressed",callable_mp(this, &ClassName::_on_click_option));
    options->hide();
}

SkeletonEditor::~SkeletonEditor() {}

void SkeletonEditorPlugin::edit(Object *p_object) {
    skeleton_editor->edit(object_cast<Skeleton>(p_object));
}

bool SkeletonEditorPlugin::handles(Object *p_object) const {
    return p_object->is_class("Skeleton");
}

void SkeletonEditorPlugin::make_visible(bool p_visible) {
    if (p_visible) {
        skeleton_editor->options->show();
    } else {

        skeleton_editor->options->hide();
        skeleton_editor->edit(nullptr);
    }
}

SkeletonEditorPlugin::SkeletonEditorPlugin(EditorNode *p_node) {
    editor = p_node;
    skeleton_editor = memnew(SkeletonEditor);
    editor->get_viewport()->add_child(skeleton_editor);
}

SkeletonEditorPlugin::~SkeletonEditorPlugin() {}
