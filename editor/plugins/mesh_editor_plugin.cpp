/*************************************************************************/
/*  mesh_editor_plugin.cpp                                               */
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

#include "mesh_editor_plugin.h"

#include "editor/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/texture_button.h"
#include "scene/resources/world_3d.h"
#include "core/method_bind.h"
#include "core/callable_method_pointer.h"

IMPL_GDCLASS(MeshEditor)
IMPL_GDCLASS(EditorInspectorPluginMesh)
IMPL_GDCLASS(MeshEditorPlugin)

void MeshEditor::_gui_input(const Ref<InputEvent>& p_event) {

    Ref<InputEventMouseMotion> mm = dynamic_ref_cast<InputEventMouseMotion>(p_event);
    if (mm && mm->get_button_mask() & BUTTON_MASK_LEFT) {

        rot_x -= mm->get_relative().y * 0.01f;
        rot_y -= mm->get_relative().x * 0.01f;
        if (rot_x < -Math_PI / 2)
            rot_x = -Math_PI / 2;
        else if (rot_x > Math_PI / 2) {
            rot_x = Math_PI / 2;
        }
        _update_rotation();
    }
}

void MeshEditor::_notification(int p_what) {

    if (p_what == NOTIFICATION_READY) {

        //get_scene()->connect("node_removed",this,"_node_removed");

        if (first_enter) {
            //it's in propertyeditor so. could be moved around

            light_1_switch->set_normal_texture(get_theme_icon("MaterialPreviewLight1", "EditorIcons"));
            light_1_switch->set_pressed_texture(get_theme_icon("MaterialPreviewLight1Off", "EditorIcons"));
            light_2_switch->set_normal_texture(get_theme_icon("MaterialPreviewLight2", "EditorIcons"));
            light_2_switch->set_pressed_texture(get_theme_icon("MaterialPreviewLight2Off", "EditorIcons"));
            first_enter = false;
        }
    }
}

void MeshEditor::_update_rotation() {

    Transform t;
    t.basis.rotate(Vector3(0, 1, 0), -rot_y);
    t.basis.rotate(Vector3(1, 0, 0), -rot_x);
    rotation->set_transform(t);
}

void MeshEditor::edit(const Ref<Mesh>& p_mesh) {

    mesh = p_mesh;
    mesh_instance->set_mesh(mesh);

    rot_x = Math::deg2rad(-15.0f);
    rot_y = Math::deg2rad(30.0f);
    _update_rotation();

    AABB aabb = mesh->get_aabb();
    Vector3 ofs = aabb.position + aabb.size * 0.5;
    float m = aabb.get_longest_axis_size();
    if (m != 0.0f) {
        m = 1.0f / m;
        m *= 0.5f;
        Transform xform;
        xform.basis.scale(Vector3(m, m, m));
        xform.origin = -xform.basis.xform(ofs); //-ofs*m;
        //xform.origin.z -= aabb.get_longest_axis_size() * 2;
        mesh_instance->set_transform(xform);
    }
}

void MeshEditor::_button_pressed(Node *p_button) {

    if (p_button == light_1_switch) {
        light1->set_visible(!light_1_switch->is_pressed());
    }

    if (p_button == light_2_switch) {
        light2->set_visible(!light_2_switch->is_pressed());
    }
}

void MeshEditor::_bind_methods() {

    SE_BIND_METHOD(MeshEditor,_gui_input);
}

MeshEditor::MeshEditor() {

    viewport = memnew(Viewport);
    Ref<World3D> world(make_ref_counted<World3D>());
    viewport->set_world_3d(world); //use own world
    add_child(viewport);
    viewport->set_disable_input(true);
    viewport->set_msaa(Viewport::MSAA_2X);
    set_stretch(true);
    camera = memnew(Camera3D);
    camera->set_transform(Transform(Basis(), Vector3(0, 0, 1.1f)));
    camera->set_perspective(45, 0.1f, 10);
    viewport->add_child(camera);

    light1 = memnew(DirectionalLight3D);
    light1->set_transform(Transform().looking_at(Vector3(-1, -1, -1), Vector3(0, 1, 0)));
    viewport->add_child(light1);

    light2 = memnew(DirectionalLight3D);
    light2->set_transform(Transform().looking_at(Vector3(0, 1, 0), Vector3(0, 0, 1)));
    light2->set_color(Color(0.7f, 0.7f, 0.7f));
    viewport->add_child(light2);

    rotation = memnew(Node3D);
    viewport->add_child(rotation);
    mesh_instance = memnew(MeshInstance3D);
    rotation->add_child(mesh_instance);

    set_custom_minimum_size(Size2(1, 150) * EDSCALE);

    HBoxContainer *hb = memnew(HBoxContainer);
    add_child(hb);
    hb->set_anchors_and_margins_preset(Control::PRESET_WIDE, Control::PRESET_MODE_MINSIZE, 2);

    hb->add_spacer();

    VBoxContainer *vb_light = memnew(VBoxContainer);
    hb->add_child(vb_light);

    light_1_switch = memnew(TextureButton);
    light_1_switch->set_toggle_mode(true);
    vb_light->add_child(light_1_switch);
    light_1_switch->connectF("pressed",this,[this]() { _button_pressed(light_1_switch); });

    light_2_switch = memnew(TextureButton);
    light_2_switch->set_toggle_mode(true);
    vb_light->add_child(light_2_switch);
    light_2_switch->connectF("pressed",this,[this]() { _button_pressed(light_2_switch); });

    first_enter = true;

    rot_x = 0;
    rot_y = 0;
}

///////////////////////

bool EditorInspectorPluginMesh::can_handle(Object *p_object) {

    return object_cast<Mesh>(p_object) != nullptr;
}

void EditorInspectorPluginMesh::parse_begin(Object *p_object) {

    Mesh *mesh = object_cast<Mesh>(p_object);
    if (!mesh) {
        return;
    }
    Ref<Mesh> m(mesh);

    MeshEditor *editor = memnew(MeshEditor);
    editor->edit(m);
    add_custom_control(editor);
}

MeshEditorPlugin::MeshEditorPlugin(EditorNode *p_node) {

    Ref<EditorInspectorPluginMesh> plugin(make_ref_counted<EditorInspectorPluginMesh>());
    add_inspector_plugin(plugin);
}
