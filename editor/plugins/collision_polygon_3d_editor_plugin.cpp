/*************************************************************************/
/*  collision_polygon_3d_editor_plugin.cpp                               */
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

#include "collision_polygon_3d_editor_plugin.h"

#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "canvas_item_editor_plugin.h"
#include "core/math/geometry.h"
#include "core/os/file_access.h"
#include "core/os/input.h"
#include "core/os/keyboard.h"
#include "core/translation_helpers.h"
#include "editor/editor_settings.h"
#include "scene/main/scene_tree.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/collision_polygon_3d.h"
#include "scene/3d/immediate_geometry_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/separator.h"

#include "node_3d_editor_plugin.h"


IMPL_GDCLASS(CollisionPolygon3DEditor)
IMPL_GDCLASS(CollisionPolygon3DEditorPlugin)

void CollisionPolygon3DEditor::_notification(int p_what) {

    switch (p_what) {

        case NOTIFICATION_READY: {

            button_create->set_button_icon(get_theme_icon("Edit", "EditorIcons"));
            button_edit->set_button_icon(get_theme_icon("MovePoint", "EditorIcons"));
            button_edit->set_pressed(true);
            get_tree()->connect("node_removed",callable_mp(this, &ClassName::_node_removed));

        } break;
        case NOTIFICATION_PROCESS: {
            if (!node) {
                return;
            }

            if (_get_depth() != prev_depth) {
                _polygon_draw();
                prev_depth = _get_depth();
            }

        } break;
    }
}
void CollisionPolygon3DEditor::_node_removed(Node *p_node) {

    if (p_node == node) {
        node = nullptr;
        if (imgeom->get_parent() == p_node)
            p_node->remove_child(imgeom);
        hide();
        set_process(false);
    }
}

void CollisionPolygon3DEditor::_menu_option(int p_option) {

    switch (p_option) {

        case MODE_CREATE: {

            mode = MODE_CREATE;
            button_create->set_pressed(true);
            button_edit->set_pressed(false);
        } break;
        case MODE_EDIT: {

            mode = MODE_EDIT;
            button_create->set_pressed(false);
            button_edit->set_pressed(true);
        } break;
    }
}

void CollisionPolygon3DEditor::_wip_close() {

    undo_redo->create_action(TTR("Create Polygon3D"));
    undo_redo->add_undo_method(node, "set_polygon", node->call_va("get_polygon"));
    undo_redo->add_do_method(node, "set_polygon", Variant::from(wip));
    undo_redo->add_do_method(this, "_polygon_draw");
    undo_redo->add_undo_method(this, "_polygon_draw");
    wip.clear();
    wip_active = false;
    mode = MODE_EDIT;
    button_edit->set_pressed(true);
    button_create->set_pressed(false);
    edited_point = -1;
    undo_redo->commit_action();
}

bool CollisionPolygon3DEditor::forward_spatial_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {

    if (!node)
        return false;

    Transform gt = node->get_global_transform();
    Transform gi = gt.affine_inverse();
    float depth = _get_depth() * 0.5f;
    Vector3 n = gt.basis.get_axis(2).normalized();
    Plane p(gt.origin + n * depth, n);

    Ref<InputEventMouseButton> mb = dynamic_ref_cast<InputEventMouseButton>(p_event);

    if (mb) {

        Vector2 gpoint = mb->get_position();
        Vector3 ray_from = p_camera->project_ray_origin(gpoint);
        Vector3 ray_dir = p_camera->project_ray_normal(gpoint);

        Vector3 spoint;

        if (!p.intersects_ray(ray_from, ray_dir, &spoint))
            return false;

        spoint = gi.xform(spoint);

        Vector2 cpoint(spoint.x, spoint.y);

        //DO NOT snap here, it's confusing in 3D for adding points.
        //Let the snap happen when the point is being moved, instead.
        //cpoint = CanvasItemEditor::get_singleton()->snap_point(cpoint);

        Vector<Vector2> poly = node->call_va("get_polygon").as<Vector<Vector2>>();

        //first check if a point is to be added (segment split)
        real_t grab_threshold = EDITOR_GET_T<float>("editors/poly_editor/point_grab_radius");

        switch (mode) {

            case MODE_CREATE: {

                if (mb->get_button_index() == BUTTON_LEFT && mb->is_pressed()) {

                    if (!wip_active) {

                        wip.clear();
                        wip.push_back(cpoint);
                        wip_active = true;
                        edited_point_pos = cpoint;
                        snap_ignore = false;
                        _polygon_draw();
                        edited_point = 1;
                        return true;
                    } else {

                        if (wip.size() > 1 && p_camera->unproject_position(gt.xform(Vector3(wip[0].x, wip[0].y, depth))).distance_to(gpoint) < grab_threshold) {
                            //wip closed
                            _wip_close();

                            return true;
                        } else {

                            wip.push_back(cpoint);
                            edited_point = wip.size();
                            snap_ignore = false;
                            _polygon_draw();
                            return true;
                        }
                    }
                } else if (mb->get_button_index() == BUTTON_RIGHT && mb->is_pressed() && wip_active) {
                    _wip_close();
                }

            } break;

            case MODE_EDIT: {

                if (mb->get_button_index() == BUTTON_LEFT) {
                    if (mb->is_pressed()) {

                        if (mb->get_control()) {

                            if (poly.size() < 3) {

                                undo_redo->create_action(TTR("Edit Poly"));
                                undo_redo->add_undo_method(node, "set_polygon", Variant::from(poly));
                                poly.push_back(cpoint);
                                undo_redo->add_do_method(node, "set_polygon", Variant::from(poly));
                                undo_redo->add_do_method(this, "_polygon_draw");
                                undo_redo->add_undo_method(this, "_polygon_draw");
                                undo_redo->commit_action();
                                return true;
                            }

                            //search edges
                            int closest_idx = -1;
                            Vector2 closest_pos;
                            real_t closest_dist = 1e10;
                            for (int i = 0; i < poly.size(); i++) {

                                Vector2 points[2] = {
                                    p_camera->unproject_position(gt.xform(Vector3(poly[i].x, poly[i].y, depth))),
                                    p_camera->unproject_position(gt.xform(Vector3(poly[(i + 1) % poly.size()].x, poly[(i + 1) % poly.size()].y, depth)))
                                };

                                Vector2 cp = Geometry::get_closest_point_to_segment_2d(gpoint, points);
                                if (cp.distance_squared_to(points[0]) < CMP_EPSILON2 || cp.distance_squared_to(points[1]) < CMP_EPSILON2)
                                    continue; //not valid to reuse point

                                real_t d = cp.distance_to(gpoint);
                                if (d < closest_dist && d < grab_threshold) {
                                    closest_dist = d;
                                    closest_pos = cp;
                                    closest_idx = i;
                                }
                            }

                            if (closest_idx >= 0) {

                                pre_move_edit = poly;
                                poly.insert_at(closest_idx + 1, cpoint);
                                edited_point = closest_idx + 1;
                                edited_point_pos = cpoint;
                                node->call_va("set_polygon", Variant::from(poly));
                                _polygon_draw();
                                snap_ignore = true;

                                return true;
                            }
                        } else {

                            //look for points to move

                            int closest_idx = -1;
                            Vector2 closest_pos;
                            real_t closest_dist = 1e10f;
                            for (int i = 0; i < poly.size(); i++) {

                                Vector2 cp = p_camera->unproject_position(gt.xform(Vector3(poly[i].x, poly[i].y, depth)));

                                real_t d = cp.distance_to(gpoint);
                                if (d < closest_dist && d < grab_threshold) {
                                    closest_dist = d;
                                    closest_pos = cp;
                                    closest_idx = i;
                                }
                            }

                            if (closest_idx >= 0) {

                                pre_move_edit = poly;
                                edited_point = closest_idx;
                                edited_point_pos = poly[closest_idx];
                                _polygon_draw();
                                snap_ignore = false;
                                return true;
                            }
                        }
                    } else {

                        snap_ignore = false;

                        if (edited_point != -1) {

                            //apply

                            ERR_FAIL_INDEX_V(edited_point, poly.size(), false);
                            poly[edited_point] = edited_point_pos;
                            undo_redo->create_action(TTR("Edit Poly"));
                            undo_redo->add_do_method(node, "set_polygon", Variant::from(poly));
                            undo_redo->add_undo_method(node, "set_polygon", Variant::from(pre_move_edit));
                            undo_redo->add_do_method(this, "_polygon_draw");
                            undo_redo->add_undo_method(this, "_polygon_draw");
                            undo_redo->commit_action();

                            edited_point = -1;
                            return true;
                        }
                    }
                }
                if (mb->get_button_index() == BUTTON_RIGHT && mb->is_pressed() && edited_point == -1) {

                    int closest_idx = -1;
                    Vector2 closest_pos;
                    real_t closest_dist = 1e10;
                    for (int i = 0; i < poly.size(); i++) {

                        Vector2 cp = p_camera->unproject_position(gt.xform(Vector3(poly[i].x, poly[i].y, depth)));

                        real_t d = cp.distance_to(gpoint);
                        if (d < closest_dist && d < grab_threshold) {
                            closest_dist = d;
                            closest_pos = cp;
                            closest_idx = i;
                        }
                    }

                    if (closest_idx >= 0) {

                        undo_redo->create_action(TTR("Edit Poly (Remove Point)"));
                        undo_redo->add_undo_method(node, "set_polygon", Variant::from(poly));
                        poly.erase_at(closest_idx);
                        undo_redo->add_do_method(node, "set_polygon", Variant::from(poly));
                        undo_redo->add_do_method(this, "_polygon_draw");
                        undo_redo->add_undo_method(this, "_polygon_draw");
                        undo_redo->commit_action();
                        return true;
                    }
                }

            } break;
        }
    }

    Ref<InputEventMouseMotion> mm = dynamic_ref_cast<InputEventMouseMotion>(p_event);

    if (mm) {
        if (edited_point != -1 && (wip_active || mm->get_button_mask() & BUTTON_MASK_LEFT)) {

            Vector2 gpoint = mm->get_position();

            Vector3 ray_from = p_camera->project_ray_origin(gpoint);
            Vector3 ray_dir = p_camera->project_ray_normal(gpoint);

            Vector3 spoint;

            if (!p.intersects_ray(ray_from, ray_dir, &spoint))
                return false;

            spoint = gi.xform(spoint);

            Vector2 cpoint(spoint.x, spoint.y);

            if (snap_ignore && !Input::get_singleton()->is_key_pressed(KEY_CONTROL)) {
                snap_ignore = false;
            }

            if (!snap_ignore && Node3DEditor::get_singleton()->is_snap_enabled()) {
                cpoint = cpoint.snapped(Vector2(
                        Node3DEditor::get_singleton()->get_translate_snap(),
                        Node3DEditor::get_singleton()->get_translate_snap()));
            }
            edited_point_pos = cpoint;

            _polygon_draw();
        }
    }

    return false;
}

float CollisionPolygon3DEditor::_get_depth() {

    if (node->call_va("_has_editable_3d_polygon_no_depth").as<bool>())
        return 0;

    return node->call_va("get_depth").as<float>();
}

void CollisionPolygon3DEditor::_polygon_draw() {

    if (!node)
        return;

    Vector<Vector2> poly;

    if (wip_active)
        poly = wip;
    else
        poly = node->call_va("get_polygon").as<Vector<Vector2>>();

    float depth = _get_depth() * 0.5f;

    imgeom->clear();
    imgeom->set_material_override(line_material);
    imgeom->begin(Mesh::PRIMITIVE_LINES, Ref<Texture>());

    Rect2 rect;

    for (int i = 0; i < poly.size(); i++) {

        Vector2 p, p2;
        p = i == edited_point ? edited_point_pos : poly[i];
        if ((wip_active && i == poly.size() - 1) || (i + 1) % poly.size() == edited_point)
            p2 = edited_point_pos;
        else
            p2 = poly[(i + 1) % poly.size()];

        if (i == 0)
            rect.position = p;
        else
            rect.expand_to(p);

        Vector3 point = Vector3(p.x, p.y, depth);
        Vector3 next_point = Vector3(p2.x, p2.y, depth);

        imgeom->set_color(Color(1, 0.3f, 0.1f, 0.8f));
        imgeom->add_vertex(point);
        imgeom->set_color(Color(1, 0.3f, 0.1f, 0.8f));
        imgeom->add_vertex(next_point);

        //Color col=Color(1,0.3f,0.1f,0.8f);
        //vpc->draw_line(point,next_point,col,2);
        //vpc->draw_texture(handle,point-handle->get_size()*0.5f);
    }

    rect = rect.grow(1);

    AABB r;
    r.position.x = rect.position.x;
    r.position.y = rect.position.y;
    r.position.z = depth;
    r.size.x = rect.size.x;
    r.size.y = rect.size.y;
    r.size.z = 0;

    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position);
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(0.3f, 0, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position);
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(0.0f, 0.3f, 0));

    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(r.size.x, 0, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(r.size.x, 0, 0) - Vector3(0.3f, 0, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(r.size.x, 0, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(r.size.x, 0, 0) + Vector3(0, 0.3f, 0));

    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(0, r.size.y, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(0, r.size.y, 0) - Vector3(0, 0.3f, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(0, r.size.y, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + Vector3(0, r.size.y, 0) + Vector3(0.3f, 0, 0));

    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + r.size);
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + r.size - Vector3(0.3f, 0, 0));
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + r.size);
    imgeom->set_color(Color(0.8f, 0.8f, 0.8f, 0.2f));
    imgeom->add_vertex(r.position + r.size - Vector3(0.0f, 0.3f, 0));

    imgeom->end();

    while (m->get_surface_count()) {
        m->surface_remove(0);
    }

    if (poly.empty())
        return;

    Vector<Vector3> va;
    {

        va.resize(poly.size());
        for (int i = 0; i < poly.size(); i++) {

            Vector2 p = i == edited_point ? edited_point_pos : poly[i];

            Vector3 point = Vector3(p.x, p.y, depth);
            va[i] = point;
        }
    }
    SurfaceArrays a(eastl::move(va));
    m->add_surface_from_arrays(Mesh::PRIMITIVE_POINTS, eastl::move(a));
    m->surface_set_material(0, handle_material);
}

void CollisionPolygon3DEditor::edit(Node *p_collision_polygon) {

    if (p_collision_polygon) {

        node = object_cast<Node3D>(p_collision_polygon);
        //Enable the pencil tool if the polygon is empty
        if (node->call_va("get_polygon").as<PoolVector<Vector2>>().empty()) {
            _menu_option(MODE_CREATE);
        }
        wip.clear();
        wip_active = false;
        edited_point = -1;
        p_collision_polygon->add_child(imgeom);
        _polygon_draw();
        set_process(true);
        prev_depth = -1;

    } else {
        node = nullptr;

        if (imgeom->get_parent())
            imgeom->get_parent()->remove_child(imgeom);

        set_process(false);
    }
}

void CollisionPolygon3DEditor::_bind_methods() {

    SE_BIND_METHOD(CollisionPolygon3DEditor,_polygon_draw);
}

CollisionPolygon3DEditor::CollisionPolygon3DEditor(EditorNode *p_editor) {

    node = nullptr;
    editor = p_editor;
    undo_redo = EditorNode::get_undo_redo();

    add_child(memnew(VSeparator));
    button_create = memnew(ToolButton);
    add_child(button_create);
    button_create->connectF("pressed",this,[=]() { _menu_option(MODE_CREATE); });
    button_create->set_toggle_mode(true);

    button_edit = memnew(ToolButton);
    add_child(button_edit);
    button_edit->connectF("pressed",this,[=]() { _menu_option(MODE_EDIT); });
    button_edit->set_toggle_mode(true);

    mode = MODE_EDIT;
    wip_active = false;
    imgeom = memnew(ImmediateGeometry3D);
    imgeom->set_transform(Transform(Basis(), Vector3(0, 0, 0.00001f)));

    line_material = make_ref_counted<SpatialMaterial>();
    line_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
    line_material->set_line_width(3.0f);
    line_material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
    line_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    line_material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
    line_material->set_albedo(Color(1, 1, 1));

    handle_material = make_ref_counted<SpatialMaterial>();
    handle_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
    handle_material->set_flag(SpatialMaterial::FLAG_USE_POINT_SIZE, true);
    handle_material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
    handle_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    handle_material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
    Ref<Texture> handle = editor->get_gui_base()->get_theme_icon("Editor3DHandle", "EditorIcons");
    handle_material->set_point_size(handle->get_width());
    handle_material->set_texture(SpatialMaterial::TEXTURE_ALBEDO, handle);

    pointsm = memnew(MeshInstance3D);
    imgeom->add_child(pointsm);
    m = make_ref_counted<ArrayMesh>();
    pointsm->set_mesh(m);
    pointsm->set_transform(Transform(Basis(), Vector3(0, 0, 0.00001f)));

    snap_ignore = false;
}

CollisionPolygon3DEditor::~CollisionPolygon3DEditor() {

    memdelete(imgeom);
}

void CollisionPolygon3DEditorPlugin::edit(Object *p_object) {

    collision_polygon_editor->edit(object_cast<Node>(p_object));
}

bool CollisionPolygon3DEditorPlugin::handles(Object *p_object) const {

    return object_cast<Node3D>(p_object) && p_object->call_va("_is_editable_3d_polygon").as<bool>();
}

void CollisionPolygon3DEditorPlugin::make_visible(bool p_visible) {

    if (p_visible) {
        collision_polygon_editor->show();
    } else {

        collision_polygon_editor->hide();
        collision_polygon_editor->edit(nullptr);
    }
}

CollisionPolygon3DEditorPlugin::CollisionPolygon3DEditorPlugin(EditorNode *p_node) {

    editor = p_node;
    collision_polygon_editor = memnew(CollisionPolygon3DEditor(p_node));
    Node3DEditor::get_singleton()->add_control_to_menu_panel(collision_polygon_editor);

    collision_polygon_editor->hide();
}

CollisionPolygon3DEditorPlugin::~CollisionPolygon3DEditorPlugin() = default;
