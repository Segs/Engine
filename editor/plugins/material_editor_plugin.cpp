/*************************************************************************/
/*  material_editor_plugin.cpp                                           */
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

#include "material_editor_plugin.h"

#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "scene/2d/canvas_item_material.h"
#include "scene/gui/viewport_container.h"
#include "scene/resources/particles_material.h"
#include "scene/resources/shader.h"
#include "scene/resources/world_3d.h"

IMPL_GDCLASS(MaterialEditor)
IMPL_GDCLASS(EditorInspectorPluginMaterial)
IMPL_GDCLASS(MaterialEditorPlugin)
IMPL_GDCLASS(SpatialMaterialConversionPlugin)
IMPL_GDCLASS(ParticlesMaterialConversionPlugin)
IMPL_GDCLASS(CanvasItemMaterialConversionPlugin)

void MaterialEditor::_notification(int p_what) {

    if (p_what == NOTIFICATION_READY) {

        //get_scene()->connect("node_removed",this,"_node_removed");

        if (first_enter) {
            //it's in propertyeditor so.. could be moved around

            light_1_switch->set_normal_texture(get_theme_icon("MaterialPreviewLight1", "EditorIcons"));
            light_1_switch->set_pressed_texture(get_theme_icon("MaterialPreviewLight1Off", "EditorIcons"));
            light_2_switch->set_normal_texture(get_theme_icon("MaterialPreviewLight2", "EditorIcons"));
            light_2_switch->set_pressed_texture(get_theme_icon("MaterialPreviewLight2Off", "EditorIcons"));

            sphere_switch->set_normal_texture(get_theme_icon("MaterialPreviewSphereOff", "EditorIcons"));
            sphere_switch->set_pressed_texture(get_theme_icon("MaterialPreviewSphere", "EditorIcons"));
            box_switch->set_normal_texture(get_theme_icon("MaterialPreviewCubeOff", "EditorIcons"));
            box_switch->set_pressed_texture(get_theme_icon("MaterialPreviewCube", "EditorIcons"));

            first_enter = false;
        }
    }

    if (p_what == NOTIFICATION_DRAW) {

        Ref<Texture> checkerboard = get_theme_icon("Checkerboard", "EditorIcons");
        Size2 size = get_size();

        draw_texture_rect(checkerboard, Rect2(Point2(), size), true);
    }
}

void MaterialEditor::edit(const Ref<Material>& p_material, const Ref<Environment> &p_env) {

    material = p_material;
    camera->set_environment(p_env);
    if (material) {
        sphere_instance->set_material_override(material);
        box_instance->set_material_override(material);
    } else {

        hide();
    }
}

void MaterialEditor::_button_pressed(Node *p_button) {

    if (p_button == light_1_switch) {
        light1->set_visible(!light_1_switch->is_pressed());
    }

    if (p_button == light_2_switch) {
        light2->set_visible(!light_2_switch->is_pressed());
    }

    if (p_button == box_switch) {
        box_instance->show();
        sphere_instance->hide();
        box_switch->set_pressed(true);
        sphere_switch->set_pressed(false);
        EditorSettings::get_singleton()->set_project_metadata("inspector_options", "material_preview_on_sphere", false);
    }

    if (p_button == sphere_switch) {
        box_instance->hide();
        sphere_instance->show();
        box_switch->set_pressed(false);
        sphere_switch->set_pressed(true);
        EditorSettings::get_singleton()->set_project_metadata("inspector_options", "material_preview_on_sphere", true);
    }
}

MaterialEditor::MaterialEditor() {

    vc = memnew(ViewportContainer);
    vc->set_stretch(true);
    add_child(vc);
    vc->set_anchors_and_margins_preset(PRESET_WIDE);
    viewport = memnew(Viewport);
    Ref<World3D> world(make_ref_counted<World3D>());
    viewport->set_world_3d(world); //use own world
    vc->add_child(viewport);
    viewport->set_disable_input(true);
    viewport->set_transparent_background(true);
    viewport->set_msaa(Viewport::MSAA_4X);

    camera = memnew(Camera3D);
    camera->set_transform(Transform(Basis(), Vector3(0, 0, 3)));
    camera->set_perspective(45, 0.1f, 10);
    camera->make_current();
    viewport->add_child(camera);

    light1 = memnew(DirectionalLight3D);
    light1->set_transform(Transform().looking_at(Vector3(-1, -1, -1), Vector3(0, 1, 0)));
    viewport->add_child(light1);

    light2 = memnew(DirectionalLight3D);
    light2->set_transform(Transform().looking_at(Vector3(0, 1, 0), Vector3(0, 0, 1)));
    light2->set_color(Color(0.7f, 0.7f, 0.7f));
    viewport->add_child(light2);

    sphere_instance = memnew(MeshInstance3D);
    viewport->add_child(sphere_instance);

    box_instance = memnew(MeshInstance3D);
    viewport->add_child(box_instance);

    Transform box_xform;
    box_xform.basis.rotate(Vector3(1, 0, 0), Math::deg2rad(25.0f));
    box_xform.basis = box_xform.basis * Basis().rotated(Vector3(0, 1, 0), Math::deg2rad(-25.0f));
    box_xform.basis.scale(Vector3(0.8f, 0.8f, 0.8f));
    box_xform.origin.y = 0.2f;
    box_instance->set_transform(box_xform);

    sphere_mesh = make_ref_counted<SphereMesh>();
    sphere_instance->set_mesh(sphere_mesh);
    box_mesh = make_ref_counted<CubeMesh>();
    box_instance->set_mesh(box_mesh);


    set_custom_minimum_size(Size2(1, 150) * EDSCALE);

    HBoxContainer *hb = memnew(HBoxContainer);
    add_child(hb);
    hb->set_anchors_and_margins_preset(Control::PRESET_WIDE, Control::PRESET_MODE_MINSIZE, 2);

    VBoxContainer *vb_shape = memnew(VBoxContainer);
    hb->add_child(vb_shape);

    sphere_switch = memnew(TextureButton);
    sphere_switch->set_toggle_mode(true);
    sphere_switch->set_pressed(true);
    vb_shape->add_child(sphere_switch);
    sphere_switch->connectF("pressed",this, [this]() { _button_pressed(sphere_switch); });

    box_switch = memnew(TextureButton);
    box_switch->set_toggle_mode(true);
    box_switch->set_pressed(false);
    vb_shape->add_child(box_switch);
    box_switch->connectF("pressed",this, [this]() { _button_pressed(box_switch); });

    hb->add_spacer();

    VBoxContainer *vb_light = memnew(VBoxContainer);
    hb->add_child(vb_light);

    light_1_switch = memnew(TextureButton);
    light_1_switch->set_toggle_mode(true);
    vb_light->add_child(light_1_switch);
    light_1_switch->connectF("pressed",this, [this]() { _button_pressed(light_1_switch); });

    light_2_switch = memnew(TextureButton);
    light_2_switch->set_toggle_mode(true);
    vb_light->add_child(light_2_switch);
    light_2_switch->connectF("pressed",this, [this]() { _button_pressed(light_2_switch); });

    first_enter = true;

    if (EditorSettings::get_singleton()->get_project_metadataT("inspector_options", "material_preview_on_sphere", true)) {
        box_instance->hide();
    } else {
        box_instance->show();
        sphere_instance->hide();
        box_switch->set_pressed(true);
        sphere_switch->set_pressed(false);
    }
}

///////////////////////

bool EditorInspectorPluginMaterial::can_handle(Object *p_object) {

    Material *material = object_cast<Material>(p_object);
    if (!material)
        return false;

    return material->get_shader_mode() == RenderingServerEnums::ShaderMode::SPATIAL;
}

void EditorInspectorPluginMaterial::parse_begin(Object *p_object) {

    Material *material = object_cast<Material>(p_object);
    if (!material) {
        return;
    }
    Ref<Material> m(material);

    MaterialEditor *editor = memnew(MaterialEditor);
    editor->edit(m, env);
    add_custom_control(editor);
}

EditorInspectorPluginMaterial::EditorInspectorPluginMaterial() {
    env = make_ref_counted<Environment>();
    Ref<ProceduralSky> proc_sky(make_ref_counted<ProceduralSky>(true));
    env->set_sky(proc_sky);
    env->set_background(Environment::BG_COLOR_SKY);
}

MaterialEditorPlugin::MaterialEditorPlugin(EditorNode *p_node) {

    Ref<EditorInspectorPluginMaterial> plugin(make_ref_counted<EditorInspectorPluginMaterial>());
    add_inspector_plugin(plugin);
}

StringName SpatialMaterialConversionPlugin::converts_to() const {

    return StringName("ShaderMaterial");
}
bool SpatialMaterialConversionPlugin::handles(const Ref<Resource> &p_resource) const {

    Ref<SpatialMaterial> mat = dynamic_ref_cast<SpatialMaterial>(p_resource);
    return mat;
}
Ref<Resource> SpatialMaterialConversionPlugin::convert(const Ref<Resource> &p_resource) const {

    Ref<SpatialMaterial> mat = dynamic_ref_cast<SpatialMaterial>(p_resource);
    ERR_FAIL_COND_V(not mat, Ref<Resource>());

    Ref<ShaderMaterial> smat(make_ref_counted<ShaderMaterial>());

    Ref<Shader> shader(make_ref_counted<Shader>());

    String code(RenderingServer::get_singleton()->shader_get_code(mat->get_shader_rid()));

    shader->set_code(code);

    smat->set_shader(shader);

    Vector<PropertyInfo> params;
    RenderingServer::get_singleton()->shader_get_param_list(mat->get_shader_rid(), &params);

    for(const PropertyInfo & E : params) {

        // Texture parameter has to be treated specially since SpatialMaterial saved it
        // as RID but ShaderMaterial needs Texture itself
        Ref<Texture> texture = mat->get_texture_by_name(E.name);
        if (texture) {
            smat->set_shader_param(E.name, texture);
        } else {
            Variant value = RenderingServer::get_singleton()->material_get_param(mat->get_rid(), E.name);
            smat->set_shader_param(E.name, value);
        }
    }

    smat->set_render_priority(mat->get_render_priority());
    smat->set_local_to_scene(mat->is_local_to_scene());
    smat->set_name(mat->get_name());
    return smat;
}

StringName ParticlesMaterialConversionPlugin::converts_to() const {

    return StringName("ShaderMaterial");
}
bool ParticlesMaterialConversionPlugin::handles(const Ref<Resource> &p_resource) const {

    Ref<ParticlesMaterial> mat = dynamic_ref_cast<ParticlesMaterial>(p_resource);
    return mat;
}
Ref<Resource> ParticlesMaterialConversionPlugin::convert(const Ref<Resource> &p_resource) const {

    Ref<ParticlesMaterial> mat = dynamic_ref_cast<ParticlesMaterial>(p_resource);
    ERR_FAIL_COND_V(not mat, Ref<Resource>());

    Ref<ShaderMaterial> smat(make_ref_counted<ShaderMaterial>());

    Ref<Shader> shader(make_ref_counted<Shader>());

    String code(RenderingServer::get_singleton()->shader_get_code(mat->get_shader_rid()));

    shader->set_code(code);

    smat->set_shader(shader);

    Vector<PropertyInfo> params;
    RenderingServer::get_singleton()->shader_get_param_list(mat->get_shader_rid(), &params);

    for(const PropertyInfo & E : params) {
        Variant value = RenderingServer::get_singleton()->material_get_param(mat->get_rid(), E.name);
        smat->set_shader_param(E.name, value);
    }

    smat->set_render_priority(mat->get_render_priority());
    smat->set_local_to_scene(mat->is_local_to_scene());
    smat->set_name(mat->get_name());
    return smat;
}

StringName CanvasItemMaterialConversionPlugin::converts_to() const {

    return StringName("ShaderMaterial");
}
bool CanvasItemMaterialConversionPlugin::handles(const Ref<Resource> &p_resource) const {

    Ref<CanvasItemMaterial> mat = dynamic_ref_cast<CanvasItemMaterial>(p_resource);
    return mat;
}
Ref<Resource> CanvasItemMaterialConversionPlugin::convert(const Ref<Resource> &p_resource) const {

    Ref<CanvasItemMaterial> mat = dynamic_ref_cast<CanvasItemMaterial>(p_resource);
    ERR_FAIL_COND_V(not mat, Ref<Resource>());

    Ref<ShaderMaterial> smat(make_ref_counted<ShaderMaterial>());

    Ref<Shader> shader(make_ref_counted<Shader>());

    String code(RenderingServer::get_singleton()->shader_get_code(mat->get_shader_rid()));

    shader->set_code(code);

    smat->set_shader(shader);

    Vector<PropertyInfo> params;
    RenderingServer::get_singleton()->shader_get_param_list(mat->get_shader_rid(), &params);

    for(const PropertyInfo & E : params) {
        Variant value = RenderingServer::get_singleton()->material_get_param(mat->get_rid(), E.name);
        smat->set_shader_param(E.name, value);
    }

    smat->set_render_priority(mat->get_render_priority());
    smat->set_local_to_scene(mat->is_local_to_scene());
    smat->set_name(mat->get_name());
    return smat;
}
