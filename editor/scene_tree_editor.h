/*************************************************************************/
/*  scene_tree_editor.h                                                  */
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

#pragma once

#include "core/undo_redo.h"
#include "core/ustring.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/tree.h"

class EditorSelection;

class SceneTreeEditor : public Control {

    GDCLASS(SceneTreeEditor,Control)
    friend struct SceneTreeEditorImpl;
public:
    enum ButtonId {
        BUTTON_SUBSCENE = 0,
        BUTTON_VISIBILITY = 1,
        BUTTON_SCRIPT = 2,
        BUTTON_LOCK = 3,
        BUTTON_GROUP = 4,
        BUTTON_WARNING = 5,
        BUTTON_SIGNALS = 6,
        BUTTON_GROUPS = 7,
        BUTTON_PIN = 8,
        BUTTON_UNIQUE = 9,
    };
private:

    EditorSelection* editor_selection;
    Tree *tree;
    Node *selected;
    GameEntity instance_node;

    UIString filter;

    AcceptDialog *error;
    AcceptDialog *warning;

    bool connect_to_script_mode;
    bool connecting_signal;

    int blocked;
    uint64_t last_hash;

    bool can_rename;
    bool can_open_instance;
    bool updating_tree;
    bool show_enabled_subscene;

    UndoRedo *undo_redo;

    HashSet<Node *> marked;
    bool marked_selectable;
    bool marked_children_selectable;
    bool display_foreign;
    bool tree_dirty;
    bool pending_test_update;
    Timer *update_timer;

    Vector<StringName> *script_types;
    Vector<StringName> valid_types;

    void _renamed();

    bool _add_nodes(Node *p_node, TreeItem *p_parent, bool p_scroll_to_selected = false);
    void _test_update_tree();
public:
    //used as a signal target.
    void _update_tree(bool p_scroll_to_selected = false);
    //used by undo/redo
    void _rename_node(GameEntity p_node, StringView p_name);
protected:
    void _tree_changed();
    void _node_removed(Node *p_node);
    void _node_renamed(Node *p_node);

    TreeItem *_find(TreeItem *p_node, const NodePath &p_path);
    void _notification(int p_what);
    void _selected_changed();
    void _deselect_items();

    void _cell_collapsed(Object *p_obj);

    static void _bind_methods();

    void _cell_button_pressed(Object *p_item, int p_column, int p_id);
    void _cell_multi_selected(Object *p_object, int p_cell, bool p_selected);
    void _update_selection(TreeItem *item);
    void _node_script_changed(Node *p_node);
    void _node_visibility_changed(Node *p_node);


    void _selection_changed();

    Variant get_drag_data_fw(const Point2 &p_point, Control *p_from);
    bool can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
    void drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);

    void _rmb_select(const Vector2 &p_pos);

    void _warning_changed(Node *p_for_node);

    bool _is_script_type(const StringName &p_type) const;

public:
    bool can_open_instances() const { return can_open_instance; }

    void set_filter(const UIString &p_filter);
    UIString get_filter() const;

    void set_undo_redo(UndoRedo *p_undo_redo) { undo_redo = p_undo_redo; }
    void set_display_foreign_nodes(bool p_display);
    bool get_display_foreign_nodes() const { return display_foreign; }

    void set_marked(const HashSet<Node *> &p_marked, bool p_selectable = false, bool p_children_selectable = true);
    void set_marked(Node *p_marked, bool p_selectable = false, bool p_children_selectable = true);
    void set_selected(Node *p_node, bool p_emit_selected = true);
    Node *get_selected() const { return selected; }
    void set_can_rename(bool p_can_rename) { can_rename = p_can_rename; }
    void set_editor_selection(EditorSelection *p_selection);

    void set_show_enabled_subscene(bool p_show) { show_enabled_subscene = p_show; }
    void set_valid_types(const Vector<StringName> &p_valid);

    void update_tree() { _update_tree(); }

    void set_connect_to_script_mode(bool p_enable);
    void set_connecting_signal(bool p_enable);

    Tree *get_scene_tree() const { return tree; }

    SceneTreeEditor(bool p_label = true, bool p_can_rename = false, bool p_can_open_instance = false);
    ~SceneTreeEditor() override;
};

class SceneTreeDialog : public ConfirmationDialog {

    GDCLASS(SceneTreeDialog,ConfirmationDialog)

    SceneTreeEditor *tree;
    //Button *select;
    //Button *cancel;
    LineEdit *filter;

    void _select();
    void _cancel();
    void _selected_changed();
    void _filter_changed(StringView p_filter);

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    SceneTreeEditor *get_scene_tree() { return tree; }
    LineEdit *get_filter_line_edit() { return filter; }
    SceneTreeDialog();
    ~SceneTreeDialog() override;
};
