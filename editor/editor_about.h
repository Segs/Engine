/*************************************************************************/
/*  editor_about.h                                                       */
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

#include "scene/gui/dialogs.h"
#include "core/list.h"
#include "core/object.h"
#include "core/forward_decls.h"

class RichTextLabel;
class ScrollContainer;
class TextureRect;
class Tree;
class LinkButton;

/**
 * NOTE: Do not assume the EditorNode singleton to be available in this class' methods.
 * EditorAbout is also used from the project manager where EditorNode isn't initialized.
 */
class EditorAbout : public AcceptDialog {

    GDCLASS(EditorAbout,AcceptDialog)

private:
    void _license_tree_selected();
    ScrollContainer *_populate_list(StringView p_name, const Vector<StringName> &p_sections, const char *const *const p_src[], const int p_flag_single_column = 0);

    LinkButton *version_btn;
    Tree *_tpl_tree;
    RichTextLabel *_license_text;
    RichTextLabel *_tpl_text;
    TextureRect *_logo;

protected:
    void _version_button_pressed();
    void _notification(int p_what);
    static void _bind_methods();

public:
    TextureRect *get_logo() const;

    EditorAbout();
    ~EditorAbout() override;
};
