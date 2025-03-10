/*************************************************************************/
/*  editor_autoload_settings.h                                           */
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

#ifndef EDITOR_AUTOLOAD_SETTINGS_H
#define EDITOR_AUTOLOAD_SETTINGS_H

#include "scene/gui/tree.h"

#include "editor_file_dialog.h"

class EditorAutoloadSettings : public VBoxContainer {

    GDCLASS(EditorAutoloadSettings,VBoxContainer)

    enum {
        BUTTON_OPEN,
        BUTTON_MOVE_UP,
        BUTTON_MOVE_DOWN,
        BUTTON_DELETE
    };


    struct AutoLoadInfo {
        StringName name;
        String path;
        Node *node;
        int order;
        bool is_singleton;
        bool in_editor;

        bool operator==(const AutoLoadInfo &p_info) const {
            return order == p_info.order;
        }

        AutoLoadInfo() {
            is_singleton = false;
            in_editor = false;
            node = nullptr;
        }
    };

    Vector<AutoLoadInfo> autoload_cache;
    String autoload_changed;
    Tree *tree;
    EditorLineEditFileChooser *autoload_add_path;
    LineEdit *autoload_add_name;
    Button *add_autoload;
    Label *error_message;

    String selected_autoload;
    int number_of_autoloads;
    bool updating_autoload;


    bool _autoload_name_is_valid(const StringName &p_name, String *r_error = nullptr);

    void _autoload_add();
    void _autoload_selected();
    void _autoload_edited();
    void _autoload_button_pressed(Object *p_item, int p_column, int p_button);
    void _autoload_activated();
    void _autoload_path_text_changed(StringView p_path);
    void _autoload_text_entered(StringView p_name);
    void _autoload_text_changed(StringView p_name);
    void _autoload_open(StringView path);
    void _autoload_file_callback(StringView p_path);
    Node *_create_autoload(StringView p_path);

    Variant get_drag_data_fw(const Point2 &p_point, Control *p_control);
    bool can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_control) const;
    void drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_control);

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    void update_autoload();
    bool autoload_add(const StringName &p_name, StringView p_path);
    void autoload_remove(const StringName &p_name);

    EditorAutoloadSettings();
    ~EditorAutoloadSettings() override;
};

#endif
