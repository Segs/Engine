/*************************************************************************/
/*  gradient_editor_plugin.cpp                                           */
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

#include "gradient_editor_plugin.h"

#include "canvas_item_editor_plugin.h"
#include "node_3d_editor_plugin.h"
#include "editor/editor_scale.h"
#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "core/translation_helpers.h"

IMPL_GDCLASS(GradientEditor)
IMPL_GDCLASS(EditorInspectorPluginGradient)
IMPL_GDCLASS(GradientEditorPlugin)

Size2 GradientEditor::get_minimum_size() const {
    return Size2(0, 60) * EDSCALE;
}
void GradientEditor::_gradient_changed() {

    if (editing) {
        return;
    }

    editing = true;
    const Vector<Gradient::Point> &points = gradient->get_points();
    set_points(points);
    editing = false;
}

void GradientEditor::_ramp_changed() {

    editing = true;
    UndoRedo *undo_redo = EditorNode::get_singleton()->get_undo_redo();
    undo_redo->create_action(TTR("Gradient Edited"));
    undo_redo->add_do_method(gradient.get(), "set_offsets", Variant::from(get_offsets()));
    undo_redo->add_do_method(gradient.get(), "set_colors", Variant::from(get_colors()));
    undo_redo->add_undo_method(gradient.get(), "set_offsets", Variant::from(gradient->get_offsets()));
    undo_redo->add_undo_method(gradient.get(), "set_colors", Variant::from(gradient->get_colors()));
    undo_redo->commit_action();
    editing = false;
}



void GradientEditor::set_gradient(const Ref<Gradient> &p_gradient) {
    gradient = p_gradient;
    connect("ramp_changed",callable_mp(this, &ClassName::_ramp_changed));
    gradient->connect("changed",callable_mp(this, &ClassName::_gradient_changed));
    set_points(gradient->get_points());
}

GradientEditor::GradientEditor() {
    editing = false;
}

///////////////////////

bool EditorInspectorPluginGradient::can_handle(Object *p_object) {

    return object_cast<Gradient>(p_object) != nullptr;
}

void EditorInspectorPluginGradient::parse_begin(Object *p_object) {

    Gradient *gradient = object_cast<Gradient>(p_object);
    Ref<Gradient> g(gradient);

    GradientEditor *editor = memnew(GradientEditor);
    editor->set_gradient(g);
    add_custom_control(editor);
}

GradientEditorPlugin::GradientEditorPlugin(EditorNode *p_node) {

    Ref<EditorInspectorPluginGradient> plugin(make_ref_counted<EditorInspectorPluginGradient>());
    add_inspector_plugin(plugin);
}
