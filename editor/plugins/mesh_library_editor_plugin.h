/*************************************************************************/
/*  mesh_library_editor_plugin.h                                         */
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

#include "editor/editor_node.h"
#include "scene/resources/mesh_library.h"

class MeshLibraryEditor : public Control {

    GDCLASS(MeshLibraryEditor,Control)

    Ref<MeshLibrary> mesh_library;

    EditorNode *editor;
    MenuButton *menu;
    ConfirmationDialog *cd_remove;
    ConfirmationDialog *cd_update;
    EditorFileDialog *file;
    int to_erase;
    bool apply_xforms;

    enum {

        MENU_OPTION_ADD_ITEM,
        MENU_OPTION_REMOVE_ITEM,
        MENU_OPTION_UPDATE_FROM_SCENE,
        MENU_OPTION_IMPORT_FROM_SCENE,
        MENU_OPTION_IMPORT_FROM_SCENE_APPLY_XFORMS
    };

    int option;
    void _import_scene_cbk(StringView p_str);
    void _menu_cbk(int p_option);
    void _menu_remove_confirm();
    void _menu_update_confirm(bool p_apply_xforms);

    static void _import_scene(Node *p_scene, const Ref<MeshLibrary> &p_library, bool p_merge, bool p_apply_xforms);

protected:
    static void _bind_methods() {}

public:
    MenuButton *get_menu_button() const { return menu; }

    void edit(const Ref<MeshLibrary> &p_mesh_library);
    static Error update_library_file(Node *p_base_scene, const Ref<MeshLibrary>& ml, bool p_merge = true, bool p_apply_xforms = false);

    MeshLibraryEditor(EditorNode *p_editor);
};

class MeshLibraryEditorPlugin : public EditorPlugin {

    GDCLASS(MeshLibraryEditorPlugin,EditorPlugin)

    MeshLibraryEditor *mesh_library_editor;
    EditorNode *editor;

public:
    StringView get_name() const override { return "MeshLibrary"; }
    bool has_main_screen() const override { return false; }
    void edit(Object *p_node) override;
    bool handles(Object *p_node) const override;
    void make_visible(bool p_visible) override;

    MeshLibraryEditorPlugin(EditorNode *p_node);
};
