/*************************************************************************/
/*  root_motion_editor_plugin.cpp                                        */
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

#include "root_motion_editor_plugin.h"

#include "core/callable_method_pointer.h"
#include "editor/editor_node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "core/method_bind.h"
#include "core/translation_helpers.h"

IMPL_GDCLASS(EditorPropertyRootMotion)
IMPL_GDCLASS(EditorInspectorRootMotionPlugin)

void EditorPropertyRootMotion::_confirmed() {

    TreeItem *ti = filters->get_selected();
    if (!ti)
        return;

    NodePath path = ti->get_metadata(0).as<NodePath>();
    emit_changed(get_edited_property(), path);
    update_property();
    filter_dialog->hide(); //may come from activated
}

void EditorPropertyRootMotion::_node_assign() {

    NodePath current = get_edited_object()->getT<NodePath>(get_edited_property());

    AnimationTree *atree = object_cast<AnimationTree>(get_edited_object());
    if (!atree->has_node(atree->get_animation_player())) {
        EditorNode::get_singleton()->show_warning(TTR("AnimationTree has no path set to an AnimationPlayer"));
        return;
    }
    AnimationPlayer *player = object_cast<AnimationPlayer>(atree->get_node(atree->get_animation_player()));
    if (!player) {
        EditorNode::get_singleton()->show_warning(TTR("Path to AnimationPlayer is invalid"));
        return;
    }

    Node *base = player->get_node(player->get_root());

    if (!base) {
        EditorNode::get_singleton()->show_warning(TTR("Animation player has no valid root node path, so unable to retrieve track names."));
        return;
    }

    Set<String> paths;
    {
        Vector<StringName> animations(player->get_animation_list());

        for (const StringName & E : animations) {

            Ref<Animation> anim = player->get_animation(E);
            for (int i = 0; i < anim->get_track_count(); i++) {
                paths.insert(anim->track_get_path(i));
            }
        }
    }

    filters->clear();
    TreeItem *root = filters->create_item();

    Map<String, TreeItem *> parenthood;

    for (const String &E : paths) {

        NodePath path(E);
        TreeItem *ti = nullptr;
        String accum;
        for (int i = 0; i < path.get_name_count(); i++) {
            StringName name = path.get_name(i);
            if (!accum.empty()) {
                accum += '/';
            }
            accum += name;
            NodePath accum_np(accum);
            if (!parenthood.contains(accum)) {
                if (ti) {
                    ti = filters->create_item(ti);
                } else {
                    ti = filters->create_item(root);
                }
                parenthood[accum] = ti;
                ti->set_text(0, name);
                ti->set_selectable(0, false);
                ti->set_editable(0, false);

                if (base->has_node(accum_np)) {
                    Node *node = base->get_node(accum_np);
                    ti->set_icon(0, EditorNode::get_singleton()->get_object_icon(node, "Node"));
                }

            } else {
                ti = parenthood[accum];
            }
        }

        Node *node = nullptr;
        if (base->has_node(NodePath(accum))) {
            node = base->get_node(NodePath(accum));
        }
        if (!node)
            continue; //no node, can't edit

        if (path.get_subname_count()) {

            String concat(path.get_concatenated_subnames());

            Skeleton *skeleton = object_cast<Skeleton>(node);
            if (skeleton && skeleton->find_bone(concat) != -1) {
                //path in skeleton
                const String &bone = concat;
                int idx = skeleton->find_bone(bone);
                Vector<String> bone_path;
                while (idx != -1) {
                    bone_path.push_front(skeleton->get_bone_name(idx));
                    idx = skeleton->get_bone_parent(idx);
                }

                accum += ':';
                for (const String & F : bone_path) {
                    if ((void *)F.data() != (void *)bone_path.front().data()) { // compare pointers to check if first
                        accum += '/';
                    }

                    accum += F;
                    if (!parenthood.contains(accum)) {
                        ti = filters->create_item(ti);
                        parenthood[accum] = ti;
                        ti->set_text_utf8(0, F);
                        ti->set_selectable(0, true);
                        ti->set_editable(0, false);
                        ti->set_icon(0, get_theme_icon("BoneAttachment3D", "EditorIcons"));
                        ti->set_metadata(0, accum);
                    } else {
                        ti = parenthood[accum];
                    }
                }

                ti->set_selectable(0, true);
                ti->set_text_utf8(0, concat);
                ti->set_icon(0, get_theme_icon("BoneAttachment3D", "EditorIcons"));
                ti->set_metadata(0, path);
                if (path == current) {
                    ti->select(0);
                }

            } else {
                //just a property
                ti = filters->create_item(ti);
                ti->set_text_utf8(0, concat);
                ti->set_selectable(0, true);
                ti->set_metadata(0, path);
                if (path == current) {
                    ti->select(0);
                }
            }
        } else {
            if (ti) {
                //just a node, likely call or animation track
                ti->set_selectable(0, true);
                ti->set_metadata(0, path);
                if (path == current) {
                    ti->select(0);
                }
            }
        }
    }

    filters->ensure_cursor_is_visible();
    filter_dialog->popup_centered_ratio();
}

void EditorPropertyRootMotion::_node_clear() {

    emit_changed(get_edited_property(), NodePath());
    update_property();
}

void EditorPropertyRootMotion::update_property() {

    NodePath p = get_edited_object()->getT<NodePath>(get_edited_property());

    assign->set_tooltip((String)p);
    if (p == NodePath()) {
        assign->set_button_icon(Ref<Texture>());
        assign->set_text(TTR("Assign..."));
        assign->set_flat(false);
        return;
    }
    assign->set_flat(true);

    Node *base_node = nullptr;
    if (base_hint != NodePath()) {
        if (get_tree()->get_root()->has_node(base_hint)) {
            base_node = get_tree()->get_root()->get_node(base_hint);
        }
    } else {
        base_node = object_cast<Node>(get_edited_object());
    }

    if (!base_node || !base_node->has_node(p)) {
        assign->set_button_icon(Ref<Texture>());
        assign->set_text((String)p);
        return;
    }

    Node *target_node = base_node->get_node(p);
    ERR_FAIL_COND(!target_node);

    assign->set_text(target_node->get_name());
    assign->set_button_icon(EditorNode::get_singleton()->get_object_icon(target_node, "Node"));
}

void EditorPropertyRootMotion::setup(const NodePath &p_base_hint) {

    base_hint = p_base_hint;
}

void EditorPropertyRootMotion::_notification(int p_what) {

    if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
        Ref<Texture> t = get_theme_icon("Clear", "EditorIcons");
        clear->set_button_icon(t);
    }
}

EditorPropertyRootMotion::EditorPropertyRootMotion() {

    HBoxContainer *hbc = memnew(HBoxContainer);
    add_child(hbc);
    assign = memnew(Button);
    assign->set_flat(true);
    assign->set_h_size_flags(SIZE_EXPAND_FILL);
    assign->set_clip_text(true);
    assign->connect("pressed",callable_mp(this, &ClassName::_node_assign));
    hbc->add_child(assign);

    clear = memnew(Button);
    clear->set_flat(true);
    clear->connect("pressed",callable_mp(this, &ClassName::_node_clear));
    hbc->add_child(clear);

    filter_dialog = memnew(ConfirmationDialog);
    add_child(filter_dialog);
    filter_dialog->set_title(TTR("Edit Filtered Tracks:"));
    filter_dialog->connect("confirmed",callable_mp(this, &ClassName::_confirmed));

    filters = memnew(Tree);
    filter_dialog->add_child(filters);
    filters->set_v_size_flags(SIZE_EXPAND_FILL);
    filters->set_hide_root(true);
    filters->connect("item_activated",callable_mp(this, &ClassName::_confirmed));
    //filters->connect("item_edited",callable_mp(this, &ClassName::_filter_edited));
}
//////////////////////////

bool EditorInspectorRootMotionPlugin::can_handle(Object *p_object) {
    return true; //can handle everything
}

void EditorInspectorRootMotionPlugin::parse_begin(Object *p_object) {
    //do none
}

bool EditorInspectorRootMotionPlugin::parse_property(Object *p_object, VariantType p_type, StringView p_path, PropertyHint p_hint, StringView p_hint_text, int p_usage) {

    if (p_path == StringView("root_motion_track") && p_object->is_class("AnimationTree") &&
            p_type == VariantType::NODE_PATH) {
        EditorPropertyRootMotion *editor = memnew(EditorPropertyRootMotion);
        if (p_hint == PropertyHint::NodePathToEditedNode && !p_hint_text.empty()) {
            editor->setup(NodePath(p_hint_text));
        }
        add_property_editor(p_path, editor);
        return true;
    }

    return false; //can be overridden, although it will most likely be last anyway
}

void EditorInspectorRootMotionPlugin::parse_end() {
    //do none
}
