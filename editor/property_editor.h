/*************************************************************************/
/*  property_editor.h                                                    */
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

#include "editor/editor_file_dialog.h"
#include "editor/scene_tree_editor.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/split_container.h"

#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

class EditorLocaleDialog;
class PropertyValueEvaluator;
class CreateDialog;
class PropertySelector;
class TextEdit;

class EditorResourceConversionPlugin : public RefCounted {

    GDCLASS(EditorResourceConversionPlugin,RefCounted)

protected:
    static void _bind_methods();

public:
    virtual StringName converts_to() const;
    virtual bool handles(const Ref<Resource> &p_resource) const;
    virtual Ref<Resource> convert(const Ref<Resource> &p_resource) const;
};

class CustomPropertyEditor : public Popup {

    GDCLASS(CustomPropertyEditor,Popup)

    enum {
        MAX_VALUE_EDITORS = 12,
        MAX_ACTION_BUTTONS = 5,
        OBJ_MENU_LOAD = 0,
        OBJ_MENU_EDIT = 1,
        OBJ_MENU_CLEAR = 2,
        OBJ_MENU_MAKE_UNIQUE = 3,
        OBJ_MENU_COPY = 4,
        OBJ_MENU_PASTE = 5,
        OBJ_MENU_NEW_SCRIPT = 6,
        OBJ_MENU_EXTEND_SCRIPT = 7,
        OBJ_MENU_SHOW_IN_FILE_SYSTEM = 8,
        TYPE_BASE_ID = 100,
        CONVERT_BASE_ID = 1000
    };

    enum {
        EASING_LINEAR,
        EASING_EASE_IN,
        EASING_EASE_OUT,
        EASING_ZERO,
        EASING_IN_OUT,
        EASING_OUT_IN
    };

    PopupMenu *menu;
    SceneTreeDialog *scene_tree;
    EditorLocaleDialog* locale;
    EditorFileDialog *file;
    ConfirmationDialog *error;
    UIString name;
    VariantType type;
    Variant val_variant;
    Vector<StringView> field_names;
    PropertyHint hint;
    String hint_text;
    LineEdit *value_editor[MAX_VALUE_EDITORS];
    int focused_value_editor;
    Label *value_label[MAX_VALUE_EDITORS];
    HScrollBar *scroll[4];
    Button *action_buttons[MAX_ACTION_BUTTONS];
    MenuButton *type_button;
    Vector<StringName> inheritors_array;
    TextureRect *texture_preview;
    ColorPicker *color_picker;
    TextEdit *text_edit;
    bool read_only;
    bool picking_viewport;
    GridContainer *checks20gc;
    CheckBox *checks20[20];
    SpinBox *spinbox;
    HSlider *slider;

    Control *easing_draw;
    CreateDialog *create_dialog;
    PropertySelector *property_select;

    Object *owner;

    bool updating;

    PropertyValueEvaluator *evaluator;

    void _text_edit_changed();
    void _locale_selected(StringView p_locale);
    void _file_selected(StringView p_file);
    void _modified(StringView p_string);

    real_t _parse_real_expression(StringView text);

    void _range_modified(double p_value);
    void _focus_enter();
    void _focus_exit();
    void _action_pressed(int p_which);
    void _type_create_selected(int p_idx);
    void _create_dialog_callback();
    void _create_selected_property(StringView p_prop);

    void _color_changed(const Color &p_color);
    void _draw_easing();
    void _menu_option(int p_which);

    void _drag_easing(const Ref<InputEvent> &p_ev);

    void _node_path_selected(NodePath p_path);
    void config_value_editors(int p_amount, int p_columns, int p_label_w, const Vector<StringName> &p_strings);
    void config_value_editors_utf8(int p_amount, int p_columns, int p_label_w, const Vector<StringView> &p_strings);
    void config_action_buttons(Span<const StringName> p_strings);

    void _emit_changed_whole_or_field();

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    void hide_menu();

    Variant get_variant() const;
    UIString get_name() const;

    void set_read_only(bool p_read_only) { read_only = p_read_only; }

    bool edit(Object *p_owner, StringView p_name, VariantType p_type, const Variant &p_variant, PropertyHint p_hint, StringView p_hint_text);

    CustomPropertyEditor();
};
