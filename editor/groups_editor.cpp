/*************************************************************************/
/*  groups_editor.cpp                                                    */
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

#include "groups_editor.h"
#include "editor/scene_tree_dock.h"
#include "editor/scene_tree_editor.h"
#include "editor_node.h"
#include "editor_scale.h"
#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"

#include "EASTL/sort.h"

IMPL_GDCLASS(GroupDialog)
IMPL_GDCLASS(GroupsEditor)

// ACTION DEFINITIONS

class AddSelectedToGroupAction final : public UndoableAction {
    Dequeue<GameEntity> m_selected_nodes;
    GameEntity m_node_handle;
    StringName m_group_name;
public:
    AddSelectedToGroupAction(GroupDialog *dlg,StringName group_name,Tree *nodes_to_add) : m_node_handle(dlg->get_instance_id()), m_group_name(group_name) {
        auto *sc = SceneTree::get_singleton();
        TreeItem *selected = nodes_to_add->get_selected();
        while(selected) {
            Node *node = sc->get_edited_scene_root()->get_node(selected->get_metadata(0).as<NodePath>());
            m_selected_nodes.push_back(node->get_instance_id());
            selected = nodes_to_add->get_next_selected(selected);
        }
    }
    StringName name() const override { return TTR("Add to Group"); }
    void redo() override {

        for (GameEntity E : m_selected_nodes) {
            auto *n = object_cast<Node>(object_for_entity(E));
            assert(n);
            n->add_to_group(m_group_name);
        }
        finish_processing();
    }
    void undo() override {
        for (GameEntity E : m_selected_nodes) {
            auto *n = object_cast<Node>(object_for_entity(E));
            assert(n);
            n->remove_from_group(m_group_name);
        }
        finish_processing();
    }
    bool can_apply() override { return object_for_entity(m_node_handle)!= nullptr; }
    ~AddSelectedToGroupAction() override = default;
    void finish_processing() {
        auto *dlg = object_cast<GroupDialog>(object_for_entity(m_node_handle));
        assert(dlg);
        dlg->_group_selected();
        dlg->emit_signal("group_edited");
        // To force redraw of scene tree.
        EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor()->update_tree();
    }
};

class RemoveSelectedFromGroupAction final: public UndoableAction {
    Dequeue<GameEntity> m_selected_nodes;
    GameEntity m_node_handle;
    StringName m_group_name;
public:
    RemoveSelectedFromGroupAction(GroupDialog *dlg,StringName group_name,Tree *nodes_to_add) : m_node_handle(dlg->get_instance_id()), m_group_name(group_name) {
        auto *sc = SceneTree::get_singleton();
        TreeItem *selected = nodes_to_add->get_selected();
        while(selected) {
            Node *node = sc->get_edited_scene_root()->get_node(selected->get_metadata(0).as<NodePath>());
            m_selected_nodes.push_back(node->get_instance_id());
            selected = nodes_to_add->get_next_selected(selected);
        }
    }
    StringName name() const override { return TTR("Remove from Group"); }
    void redo() override {

        for (GameEntity E : m_selected_nodes) {
            auto *n = object_cast<Node>(object_for_entity(E));
            assert(n);
            n->remove_from_group(m_group_name);
        }
        finish_processing();
    }
    void undo() override {
        for (GameEntity E : m_selected_nodes) {
            auto *n = object_cast<Node>(object_for_entity(E));
            assert(n);
            n->add_to_group(m_group_name);
        }
        finish_processing();
    }
    bool can_apply() override { return object_for_entity(m_node_handle)!= nullptr; }
    ~RemoveSelectedFromGroupAction() override = default;
    void finish_processing() {
        auto *dlg = object_cast<GroupDialog>(object_for_entity(m_node_handle));
        assert(dlg);
        dlg->_group_selected();
        dlg->emit_signal("group_edited");
        // To force redraw of scene tree.
        EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor()->update_tree();
    }
};

class DeleteGroupAction final: public UndoableAction {
    GameEntity m_node_handle;
    Dequeue<GameEntity> m_nodes_to_remove;
    bool m_removed_all = true;
    StringName m_group_name;

public:
    DeleteGroupAction(GroupDialog *dlg,StringName name) : m_node_handle(dlg->get_instance_id()),m_group_name(name) {
        auto *sc = SceneTree::get_singleton();
        Dequeue<Node *> nodes;
        sc->get_nodes_in_group(m_group_name, &nodes);

        for (Node *E : nodes) {
            if (dlg->_can_edit(E, m_group_name)) {
                m_nodes_to_remove.push_back(E->get_instance_id());
            } else {
                m_removed_all = false;
            }
        }
    }
    StringName name() const override { return TTR("Delete Group"); }
    void redo() override {
        GroupDialog *dlg = object_cast<GroupDialog>(object_for_entity(m_node_handle));
        assert(dlg);
        for (GameEntity node_obj : m_nodes_to_remove) {
            Node *node = object_cast<Node>(object_for_entity(node_obj));
            assert(node);
            node->remove_from_group(m_group_name);
        }
        if (m_removed_all) {
            dlg->_delete_group_item(m_group_name);
        }
        dlg->_group_selected();
        dlg->emit_signal("group_edited");
        // To force redraw of scene tree.
        EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor()->update_tree();
    }
    void undo() override {
        GroupDialog *dlg = object_cast<GroupDialog>(object_for_entity(m_node_handle));
        assert(dlg);
        for (GameEntity node_obj : m_nodes_to_remove) {
            Node *node = object_cast<Node>(object_for_entity(node_obj));
            assert(node);
            node->add_to_group(m_group_name, true);
        }
        if (m_removed_all) {
            dlg->_add_group(m_group_name);
        }

        dlg->_group_selected();
        dlg->emit_signal("group_edited");
        // To force redraw of scene tree.
        EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor()->update_tree();
    }
    // checks if operation is still applicable.
    bool can_apply() override { return object_for_entity(m_node_handle) != nullptr; }
};

class RenameGroupAction final : public UndoableAction {
    GameEntity m_node_handle;
    Dequeue<GameEntity> m_nodes_to_move;
    bool m_removed_all = true;
    StringName m_old_group_name;
    StringName m_new_group_name;

public:
    RenameGroupAction(GroupDialog *dlg,StringName old_name,StringName new_name) :  m_node_handle(dlg->get_instance_id()),m_old_group_name(old_name),m_new_group_name(new_name) {
        auto *sc = SceneTree::get_singleton();
        Dequeue<Node *> nodes;
        sc->get_nodes_in_group(m_old_group_name, &nodes);

        for (Node *E : nodes) {
            if (dlg->_can_edit(E, m_old_group_name)) {
                m_nodes_to_move.push_back(E->get_instance_id());
            } else {
                m_removed_all = false;
            }
        }
    }
    StringName name() const override { return TTR("Rename Group"); }
    void redo() override {
        GroupDialog *dlg = object_cast<GroupDialog>(object_for_entity(m_node_handle));
        assert(dlg);
        for (GameEntity node_obj : m_nodes_to_move) {
            Node *node = object_cast<Node>(object_for_entity(node_obj));
            assert(node);
            node->remove_from_group(m_old_group_name);
            node->add_to_group(m_new_group_name, true);
        }
        if( m_removed_all ) { // all nodes were copied, we can just rename the group
            dlg->_rename_group_item(m_old_group_name, m_new_group_name);
        }
        dlg->_group_selected();
        dlg->emit_signal("group_edited");
    }
    void undo() override {
        GroupDialog *dlg = object_cast<GroupDialog>(object_for_entity(m_node_handle));
        assert(dlg);
        for (GameEntity node_obj : m_nodes_to_move) {
            Node *node = object_cast<Node>(object_for_entity(node_obj));
            assert(node);
            node->remove_from_group(m_new_group_name);
            node->add_to_group(m_old_group_name, true);
        }
        if( m_removed_all ) { // all nodes were copied, we can just rename the group
            dlg->_rename_group_item(m_new_group_name, m_old_group_name);
        }
        dlg->_group_selected();
        dlg->emit_signal("group_edited");
    }
    bool can_apply() override { return object_for_entity(m_node_handle)!= nullptr; }

    ~RenameGroupAction() override = default;
};

void GroupDialog::_group_selected() {
    nodes_to_add->clear();
    add_node_root = nodes_to_add->create_item();

    nodes_to_remove->clear();
    remove_node_root = nodes_to_remove->create_item();

    if (!groups->is_anything_selected()) {
        group_empty->hide();
        return;
    }

    selected_group = StringName(groups->get_selected()->get_text(0));
    _load_nodes(scene_tree->get_edited_scene_root());

    group_empty->set_visible(!remove_node_root->get_children());
}

void GroupDialog::_load_nodes(Node *p_current) {
    StringName item_name = p_current->get_name();
    if (p_current != scene_tree->get_edited_scene_root()) {
        item_name = StringName(String(p_current->get_parent()->get_name()) + "/" + item_name);
    }

    bool keep = true;
    Node *root = scene_tree->get_edited_scene_root();
    Node *owner = p_current->get_owner();
    if (owner != root && p_current != root && !owner && !root->is_editable_instance(owner)) {
        keep = false;
    }

    TreeItem *node = nullptr;
    NodePath path = scene_tree->get_edited_scene_root()->get_path_to(p_current);
    if (keep && p_current->is_in_group(selected_group)) {
        if (StringUtils::is_subsequence_of(
                    remove_filter->get_text(), p_current->get_name(), StringUtils::CaseInsensitive)) {
            node = nodes_to_remove->create_item(remove_node_root);
            keep = true;
        } else {
            keep = false;
        }
    } else if (keep && StringUtils::is_subsequence_of(
                               add_filter->get_text(), p_current->get_name(), StringUtils::CaseInsensitive)) {
        node = nodes_to_add->create_item(add_node_root);
        keep = true;
    } else {
        keep = false;
    }

    if (keep) {
        node->set_text(0, item_name);
        node->set_metadata(0, path);
        node->set_tooltip(0, StringName((String)path));

        Ref<Texture> icon = EditorNode::get_singleton()->get_object_icon(p_current, "Node");
        node->set_icon(0, icon);

        if (!_can_edit(p_current, selected_group)) {
            node->set_selectable(0, false);
            node->set_custom_color(0, get_theme_color("disabled_font_color", "Editor"));
        }
    }

    for (int i = 0; i < p_current->get_child_count(); i++) {
        _load_nodes(p_current->get_child(i));
    }
}

bool GroupDialog::_can_edit(Node *p_node, const StringName &p_group) {
    Node *n = p_node;
    bool can_edit = true;
    while (n) {
        Ref<SceneState> ss = n == EditorNode::get_singleton()->get_edited_scene() ? n->get_scene_inherited_state() :
                                                                                      n->get_scene_instance_state();
        if (ss) {
            int path = ss->find_node_by_path(n->get_path_to(p_node));
            if (path != -1) {
                if (ss->is_node_in_group(path, p_group)) {
                    can_edit = false;
                }
            }
        }
        n = n->get_owner();
    }
    return can_edit;
}

void GroupDialog::_add_pressed() {
    TreeItem *selected = nodes_to_add->get_selected();

    if (!selected) {
        return;
    }

    undo_redo->add_action(new AddSelectedToGroupAction(this,selected_group,nodes_to_add));
    undo_redo->commit_action();
}

void GroupDialog::_removed_pressed() {
    TreeItem *selected = nodes_to_remove->get_selected();

    if (!selected) {
        return;
    }
    undo_redo->add_action(new RemoveSelectedFromGroupAction(this,selected_group,nodes_to_add));
    undo_redo->commit_action();
}

void GroupDialog::_remove_filter_changed(StringView p_filter) {
    _group_selected();
}

void GroupDialog::_add_filter_changed(StringView p_filter) {
    _group_selected();
}

void GroupDialog::_add_group_pressed(StringView p_name) {
    _add_group(StringName(add_group_text->get_text()));
    add_group_text->clear();
}

void GroupDialog::_add_group_text_changed(StringView p_new_text) {
    add_group_button->set_disabled(StringUtils::strip_edges(p_new_text).empty());
}

void GroupDialog::_add_group(const StringName& p_name) {
    if (!is_visible()) {
        return; // No need to edit the dialog if it's not being used.
    }

    StringName name(StringUtils::strip_edges(p_name));
    if (name.empty() || groups->search_item_text(name)) {
        return;
    }

    TreeItem *new_group = groups->create_item(groups_root);
    new_group->set_text_utf8(0, name);
    new_group->add_button(0, get_theme_icon("Remove", "EditorIcons"), DELETE_GROUP);
    new_group->add_button(0, get_theme_icon("ActionCopy", "EditorIcons"), COPY_GROUP);
    new_group->set_editable(0, true);
    new_group->select(0);
    groups->ensure_cursor_is_visible();
}

void GroupDialog::_group_renamed() {
    TreeItem *renamed_group = groups->get_edited();
    if (!renamed_group) {
        return;
    }

    const StringName name(StringUtils::strip_edges(renamed_group->get_text(0)));
    for (TreeItem *E = groups_root->get_children(); E; E = E->get_next()) {
        if (E != renamed_group && E->get_text(0) == name) {
            renamed_group->set_text(0, selected_group);
            error->set_text(TTR("Group name already exists."));
            error->popup_centered();
            return;
        }
    }

    if (name.empty()) {
        renamed_group->set_text(0, selected_group);
        error->set_text(TTR("Invalid group name."));
        error->popup_centered();
        return;
    }

    renamed_group->set_text_utf8(0, name); // Spaces trimmed.
    undo_redo->add_action(new RenameGroupAction(this, selected_group, name));
    undo_redo->commit_action();
}

void GroupDialog::_rename_group_item(StringView p_old_name, StringView p_new_name) {
    if (!is_visible()) {
        return; // No need to edit the dialog if it's not being used.
    }

    selected_group = StringName(p_new_name);

    for (TreeItem *E = groups_root->get_children(); E; E = E->get_next()) {
        if (E->get_text(0) == p_old_name) {
            E->set_text_utf8(0, p_new_name);
            return;
        }
    }
}

void GroupDialog::_load_groups(Node *p_current) {
    Vector<Node::GroupInfo> gi;
    p_current->get_groups(&gi);

    for (const Node::GroupInfo &E : gi) {
        if (!E.persistent) {
            continue;
        }
        _add_group(E.name);
    }

    for (int i = 0; i < p_current->get_child_count(); i++) {
        _load_groups(p_current->get_child(i));
    }
}

void GroupDialog::_modify_group_pressed(Object *p_item, int p_column, int p_id) {
    TreeItem *ti = object_cast<TreeItem>(p_item);
    if (!ti) {
        return;
    }

    switch (p_id) {
        case DELETE_GROUP: {
            StringName name(ti->get_text(0));
            undo_redo->add_action(new DeleteGroupAction(this,name));
            undo_redo->commit_action();
        } break;
        case COPY_GROUP: {
            OS::get_singleton()->set_clipboard(ti->get_text(p_column));
        } break;
    }
}

void GroupDialog::_delete_group_item(StringView p_name) {
    if (!is_visible()) {
        return; // No need to edit the dialog if it's not being used.
    }

    if (selected_group == p_name) {
            add_filter->clear();
            remove_filter->clear();
            nodes_to_remove->clear();
            nodes_to_add->clear();
            groups->deselect_all();
            selected_group = "";
        }
    for (TreeItem *E = groups_root->get_children(); E; E = E->get_next()) {
        if (E->get_text(0) == p_name) {
            groups_root->remove_child(E);
            return;
    }
    }
}

void GroupDialog::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED:
        case NOTIFICATION_ENTER_TREE: {
            add_button->set_button_icon(get_theme_icon("Forward", "EditorIcons"));
            remove_button->set_button_icon(get_theme_icon("Back", "EditorIcons"));

            add_filter->set_right_icon(get_theme_icon("Search", "EditorIcons"));
            add_filter->set_clear_button_enabled(true);
            remove_filter->set_right_icon(get_theme_icon("Search", "EditorIcons"));
            remove_filter->set_clear_button_enabled(true);
        } break;
    }
}

void GroupDialog::edit() {

    popup_centered();

    groups->clear();
    groups_root = groups->create_item();

    nodes_to_add->clear();
    nodes_to_remove->clear();

    add_group_text->clear();
    add_filter->clear();
    remove_filter->clear();

    _load_groups(scene_tree->get_edited_scene_root());
}

void GroupDialog::_bind_methods() {
    ADD_SIGNAL(MethodInfo("group_edited"));
}

GroupDialog::GroupDialog() {
    set_custom_minimum_size(Size2(600, 400) * EDSCALE);

    scene_tree = SceneTree::get_singleton();

    VBoxContainer *vbc = memnew(VBoxContainer);
    add_child(vbc);
    vbc->set_anchors_and_margins_preset(PRESET_WIDE, PRESET_MODE_KEEP_SIZE, 8 * EDSCALE);

    HBoxContainer *hbc = memnew(HBoxContainer);
    vbc->add_child(hbc);
    hbc->set_v_size_flags(SIZE_EXPAND_FILL);

    VBoxContainer *vbc_left = memnew(VBoxContainer);
    hbc->add_child(vbc_left);
    vbc_left->set_h_size_flags(SIZE_EXPAND_FILL);

    Label *group_title = memnew(Label);
    group_title->set_text(TTR("Groups"));
    vbc_left->add_child(group_title);

    groups = memnew(Tree);
    vbc_left->add_child(groups);
    groups->set_hide_root(true);
    groups->set_select_mode(Tree::SELECT_SINGLE);
    groups->set_allow_reselect(true);
    groups->set_allow_rmb_select(true);
    groups->set_v_size_flags(SIZE_EXPAND_FILL);
    groups->add_constant_override("draw_guides", 1);
    groups->connect("item_selected",callable_mp(this, &ClassName::_group_selected));
    groups->connect("button_pressed", callable_mp(this, &ClassName::_modify_group_pressed));
    groups->connect("item_edited",callable_mp(this, &ClassName::_group_renamed));

    HBoxContainer *chbc = memnew(HBoxContainer);
    vbc_left->add_child(chbc);
    chbc->set_h_size_flags(SIZE_EXPAND_FILL);

    add_group_text = memnew(LineEdit);
    chbc->add_child(add_group_text);
    add_group_text->set_h_size_flags(SIZE_EXPAND_FILL);
    add_group_text->connect("text_entered",callable_mp(this, &ClassName::_add_group_pressed));
    add_group_text->connect("text_changed", callable_mp(this, &ClassName::_add_group_text_changed));

    add_group_button = memnew(Button);
    add_group_button->set_text("Add");
    chbc->add_child(add_group_button);
    add_group_button->connectF("pressed", this, [=]() { _add_group_pressed(StringView()); });

    VBoxContainer *vbc_add = memnew(VBoxContainer);
    hbc->add_child(vbc_add);
    vbc_add->set_h_size_flags(SIZE_EXPAND_FILL);

    Label *out_of_group_title = memnew(Label);
    out_of_group_title->set_text(TTR("Nodes Not in Group"));
    vbc_add->add_child(out_of_group_title);

    nodes_to_add = memnew(Tree);
    vbc_add->add_child(nodes_to_add);
    nodes_to_add->set_hide_root(true);
    nodes_to_add->set_hide_folding(true);
    nodes_to_add->set_select_mode(Tree::SELECT_MULTI);
    nodes_to_add->set_v_size_flags(SIZE_EXPAND_FILL);
    nodes_to_add->add_constant_override("draw_guides", 1);

    HBoxContainer *add_filter_hbc = memnew(HBoxContainer);
    add_filter_hbc->add_constant_override("separate", 0);
    vbc_add->add_child(add_filter_hbc);

    add_filter = memnew(LineEdit);
    add_filter->set_h_size_flags(SIZE_EXPAND_FILL);
    add_filter->set_placeholder(TTR("Filter nodes"));
    add_filter_hbc->add_child(add_filter);
    add_filter->connect("text_changed",callable_mp(this, &ClassName::_add_filter_changed));

    VBoxContainer *vbc_buttons = memnew(VBoxContainer);
    hbc->add_child(vbc_buttons);
    vbc_buttons->set_h_size_flags(SIZE_SHRINK_CENTER);
    vbc_buttons->set_v_size_flags(SIZE_SHRINK_CENTER);

    add_button = memnew(ToolButton);
    add_button->set_text(TTR("Add"));
    add_button->connect("pressed",callable_mp(this, &ClassName::_add_pressed));

    vbc_buttons->add_child(add_button);
    vbc_buttons->add_spacer();
    vbc_buttons->add_spacer();
    vbc_buttons->add_spacer();

    remove_button = memnew(ToolButton);
    remove_button->set_text(TTR("Remove"));
    remove_button->connect("pressed",callable_mp(this, &ClassName::_removed_pressed));

    vbc_buttons->add_child(remove_button);

    VBoxContainer *vbc_remove = memnew(VBoxContainer);
    hbc->add_child(vbc_remove);
    vbc_remove->set_h_size_flags(SIZE_EXPAND_FILL);

    Label *in_group_title = memnew(Label);
    in_group_title->set_text(TTR("Nodes in Group"));
    vbc_remove->add_child(in_group_title);

    nodes_to_remove = memnew(Tree);
    vbc_remove->add_child(nodes_to_remove);
    nodes_to_remove->set_v_size_flags(SIZE_EXPAND_FILL);
    nodes_to_remove->set_hide_root(true);
    nodes_to_remove->set_hide_folding(true);
    nodes_to_remove->set_select_mode(Tree::SELECT_MULTI);
    nodes_to_remove->add_constant_override("draw_guides", 1);

    HBoxContainer *remove_filter_hbc = memnew(HBoxContainer);
    remove_filter_hbc->add_constant_override("separate", 0);
    vbc_remove->add_child(remove_filter_hbc);

    remove_filter = memnew(LineEdit);
    remove_filter->set_h_size_flags(SIZE_EXPAND_FILL);
    remove_filter->set_placeholder(TTR("Filter nodes"));
    remove_filter_hbc->add_child(remove_filter);
    remove_filter->connect("text_changed",callable_mp(this, &ClassName::_remove_filter_changed));

    group_empty = memnew(Label());
    group_empty->set_text(TTR("Empty groups will be automatically removed."));
    group_empty->set_valign(Label::VALIGN_CENTER);
    group_empty->set_align(Label::ALIGN_CENTER);
    group_empty->set_autowrap(true);
    group_empty->set_custom_minimum_size(Size2(100 * EDSCALE, 0));
    nodes_to_remove->add_child(group_empty);
    group_empty->set_anchors_and_margins_preset(PRESET_WIDE, PRESET_MODE_KEEP_SIZE, 8 * EDSCALE);

    set_title(TTR("Group Editor"));
    set_as_top_level(true);
    set_resizable(true);

    error = memnew(ConfirmationDialog);
    add_child(error);
    error->get_ok()->set_text(TTR("Close"));
    _add_group_text_changed("");
}

////////////////////////////////////////////////////////////////////////////////

void GroupsEditor::_add_group(StringView p_group) {

    if (!node) {
        return;
    }

    const StringName name(StringUtils::strip_edges(p_group));
    if (name.empty()) {
        return;
    }
    group_name->clear();

    if (node->is_in_group(name)){
        return;
    }

    undo_redo->create_action(TTR("Add to Group"));

    undo_redo->add_do_method(node, "add_to_group", name, true);
    undo_redo->add_undo_method(node, "remove_from_group", name);
    undo_redo->add_do_method(this, "update_tree");
    undo_redo->add_undo_method(this, "update_tree");
    // To force redraw of scene tree.
    undo_redo->add_do_method(EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor(), "update_tree");
    undo_redo->add_undo_method(EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor(), "update_tree");

    undo_redo->commit_action();

}

void GroupsEditor::_modify_group(Object *p_item, int p_column, int p_id) {

    if (!node)
        return;

    TreeItem *ti = object_cast<TreeItem>(p_item);
    if (!ti)
        return;

    switch (p_id) {
        case DELETE_GROUP: {
    StringName name(ti->get_text(0));

    undo_redo->create_action(TTR("Remove from Group"));

    undo_redo->add_do_method(node, "remove_from_group", name);
    undo_redo->add_undo_method(node, "add_to_group", name, true);
    undo_redo->add_do_method(this, "update_tree");
    undo_redo->add_undo_method(this, "update_tree");
    // To force redraw of scene tree.
            undo_redo->add_do_method(
                    EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor(), "update_tree");
            undo_redo->add_undo_method(
                    EditorNode::get_singleton()->get_scene_tree_dock()->get_tree_editor(), "update_tree");

    undo_redo->commit_action();
        } break;
        case COPY_GROUP: {
            OS::get_singleton()->set_clipboard(ti->get_text(p_column));
        } break;
    }
}

void GroupsEditor::_group_name_changed(StringView p_new_text) {
    add->set_disabled(StringUtils::strip_edges(p_new_text).empty());
}

struct _GroupInfoComparator {

    bool operator()(const Node::GroupInfo &p_a, const Node::GroupInfo &p_b) const {
        return StringView(p_a.name) < StringView(p_b.name);
    }
};

void GroupsEditor::update_tree() {

    tree->clear();

    if (!node)
        return;

    Vector<Node::GroupInfo> groups;
    node->get_groups(&groups);
    eastl::sort(groups.begin(),groups.end(),_GroupInfoComparator());

    TreeItem *root = tree->create_item();

    for (const GroupInfo &gi : groups) {

        if (!gi.persistent)
            continue;

        Node *n = node;
        bool can_be_deleted = true;

        while (n) {

            Ref<SceneState> ss = n == EditorNode::get_singleton()->get_edited_scene() ? n->get_scene_inherited_state() :
                                                                                        n->get_scene_instance_state();

            if (ss) {

                int path = ss->find_node_by_path(n->get_path_to(node));
                if (path != -1) {
                    if (ss->is_node_in_group(path, gi.name)) {
                        can_be_deleted = false;
                    }
                }
            }

            n = n->get_owner();
        }

        TreeItem *item = tree->create_item(root);
        item->set_text(0, gi.name);
        if (can_be_deleted) {
            item->add_button(0, get_theme_icon("Remove", "EditorIcons"), DELETE_GROUP);
            item->add_button(0, get_theme_icon("ActionCopy", "EditorIcons"), COPY_GROUP);
        } else {
            item->set_selectable(0, false);
        }
    }
}

void GroupsEditor::set_current(Node *p_node) {

    node = p_node;
    update_tree();
}

void GroupsEditor::_show_group_dialog() {
    group_dialog->edit();
    group_dialog->set_undo_redo(undo_redo);
}

void GroupsEditor::_bind_methods() {
    MethodBinder::bind_method("update_tree", &GroupsEditor::update_tree);

    MethodBinder::bind_method("_show_group_dialog", &GroupsEditor::_show_group_dialog);
}

GroupsEditor::GroupsEditor() {

    node = nullptr;

    VBoxContainer *vbc = this;

    group_dialog = memnew(GroupDialog);
    group_dialog->set_as_top_level(true);
    add_child(group_dialog);
    group_dialog->connect("group_edited",callable_mp(this, &ClassName::update_tree));

    Button *group_dialog_button = memnew(Button);
    group_dialog_button->set_text(TTR("Manage Groups"));
    vbc->add_child(group_dialog_button);
    group_dialog_button->connect("pressed",callable_mp(this, &ClassName::_show_group_dialog));

    HBoxContainer *hbc = memnew(HBoxContainer);
    vbc->add_child(hbc);

    group_name = memnew(LineEdit);
    group_name->set_h_size_flags(SIZE_EXPAND_FILL);
    hbc->add_child(group_name);
    group_name->connect("text_entered",callable_mp(this, &ClassName::_add_group));
    group_name->connect("text_changed", callable_mp(this, &ClassName::_group_name_changed));

    add = memnew(Button);
    add->set_text(TTR("Add"));
    hbc->add_child(add);
    add->connectF("pressed", this, [=]() { _add_group(group_name->get_text()); });

    tree = memnew(Tree);
    tree->set_hide_root(true);
    tree->set_v_size_flags(SIZE_EXPAND_FILL);
    vbc->add_child(tree);
    tree->connect("button_pressed", callable_mp(this, &ClassName::_modify_group));
    tree->add_constant_override("draw_guides", 1);
    add_constant_override("separation", 3 * EDSCALE);
    _group_name_changed("");
}

GroupsEditor::~GroupsEditor() = default;
