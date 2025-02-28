/*************************************************************************/
/*  sprite_editor_plugin.cpp                                             */
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

#include "sprite_editor_plugin.h"

#include "core/callable_method_pointer.h"
#include "core/math/geometry.h"
#include "core/method_bind.h"
#include "core/translation_helpers.h"
#include "canvas_item_editor_plugin.h"
#include "editor/editor_scale.h"
#include "editor/scene_tree_dock.h"
#include "scene/2d/collision_polygon_2d.h"
#include "scene/2d/light_occluder_2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "scene/2d/polygon_2d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering_server.h"
#include "thirdparty/misc/clipper.hpp"

IMPL_GDCLASS(SpriteEditor)
IMPL_GDCLASS(SpriteEditorPlugin)

void SpriteEditor::_node_removed(Node *p_node) {

    if (p_node == node) {
        node = nullptr;
        options->hide();
    }
}

void SpriteEditor::edit(Sprite2D *p_sprite) {

    node = p_sprite;
}

#define PRECISION 10.0

Vector<Vector2> expand(const Vector<Vector2> &points, const Rect2i &rect, float epsilon = 2.0) {
    int size = points.size();
    ERR_FAIL_COND_V(size < 2, Vector<Vector2>());

    ClipperLib::Path subj;
    ClipperLib::PolyTree solution;
    ClipperLib::PolyTree out;

    for (auto point : points) {

        subj << ClipperLib::IntPoint(point.x * PRECISION, point.y * PRECISION);
    }
    ClipperLib::ClipperOffset co;
    co.AddPath(subj, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    co.Execute(solution, epsilon * PRECISION);

    ClipperLib::PolyNode *p = solution.GetFirst();

    ERR_FAIL_COND_V(!p, points);

    while (p->IsHole()) {
        p = p->GetNext();
    }

    //turn the result into simply polygon (AKA, fix overlap)

    //clamp into the specified rect
    ClipperLib::Clipper cl;
    cl.StrictlySimple(true);
    cl.AddPath(p->Contour, ClipperLib::ptSubject, true);
    //create the clipping rect
    ClipperLib::Path clamp {
        ClipperLib::IntPoint(0, 0),
        ClipperLib::IntPoint(rect.size.width * PRECISION, 0),
        ClipperLib::IntPoint(rect.size.width * PRECISION, rect.size.height * PRECISION),
        ClipperLib::IntPoint(0, rect.size.height * PRECISION)
    };
    cl.AddPath(clamp, ClipperLib::ptClip, true);
    cl.Execute(ClipperLib::ctIntersection, out);

    Vector<Vector2> outPoints;
    ClipperLib::PolyNode *p2 = out.GetFirst();
    ERR_FAIL_COND_V(!p2, points);

    while (p2->IsHole()) {
        p2 = p2->GetNext();
    }

    int lasti = p2->Contour.size() - 1;
    Vector2 prev = Vector2(p2->Contour[lasti].X / PRECISION, p2->Contour[lasti].Y / PRECISION);
    for (ClipperLib::IntPoint c_p : p2->Contour) {

        Vector2 cur = Vector2(c_p.X / PRECISION, c_p.Y / PRECISION);
        if (cur.distance_to(prev) > 0.5f) {
            outPoints.push_back(cur);
            prev = cur;
        }
    }
    return outPoints;
}

void SpriteEditor::_menu_option(int p_option) {

    if (!node) {
        return;
    }

    selected_menu_item = (Menu)p_option;

    switch (p_option) {
        case MENU_OPTION_CONVERT_TO_MESH_2D: {

            debug_uv_dialog->get_ok()->set_text(TTR("Create Mesh2D"));
            debug_uv_dialog->set_title("Mesh2D Preview");

            _update_mesh_data();
            debug_uv_dialog->popup_centered();
            debug_uv->update();

        } break;
        case MENU_OPTION_CONVERT_TO_POLYGON_2D: {

            debug_uv_dialog->get_ok()->set_text(TTR("Create Polygon2D"));
            debug_uv_dialog->set_title("Polygon2D Preview");

            _update_mesh_data();
            debug_uv_dialog->popup_centered();
            debug_uv->update();
        } break;
        case MENU_OPTION_CREATE_COLLISION_POLY_2D: {

            debug_uv_dialog->get_ok()->set_text(TTR("Create CollisionPolygon2D"));
            debug_uv_dialog->set_title("CollisionPolygon2D Preview");

            _update_mesh_data();
            debug_uv_dialog->popup_centered();
            debug_uv->update();

        } break;
        case MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D: {

            debug_uv_dialog->get_ok()->set_text(TTR("Create LightOccluder2D"));
            debug_uv_dialog->set_title("LightOccluder2D Preview");

            _update_mesh_data();
            debug_uv_dialog->popup_centered();
            debug_uv->update();

        } break;
    }
}

void SpriteEditor::_update_mesh_data() {

    Ref<Texture> texture = node->get_texture();
    if (not texture) {
        err_dialog->set_text(TTR("Sprite2D is empty!"));
        err_dialog->popup_centered_minsize();
        return;
    }

    if (node->get_hframes() > 1 || node->get_vframes() > 1) {
        err_dialog->set_text(TTR("Can't convert a sprite using animation frames to mesh."));
        err_dialog->popup_centered_minsize();
        return;
    }
    Ref<Image> image = texture->get_data();
    ERR_FAIL_COND(not image);

    if (image->is_compressed()) {
        image->decompress();
    }

    Rect2 rect;
    if (node->is_region())
        rect = node->get_region_rect();
    else
        rect.size = Size2(image->get_width(), image->get_height());

    Ref<BitMap> bm(make_ref_counted<BitMap>());
    bm->create_from_image_alpha(image);

    int shrink = shrink_pixels->get_value();
    if (shrink > 0) {
        bm->shrink_mask(shrink, rect);
    }

    int grow = grow_pixels->get_value();
    if (grow > 0) {
        bm->grow_mask(grow, rect);
    }

    float epsilon = simplification->get_value();

    Vector<Vector<Vector2> > lines = bm->clip_opaque_to_polygons(rect, epsilon);

    uv_lines.clear();

    computed_vertices.clear();
    computed_uv.clear();
    computed_indices.clear();

    Size2 img_size = Vector2(image->get_width(), image->get_height());
    for (int i = 0; i < lines.size(); i++) {
        lines[i] = expand(lines[i], rect, epsilon);
    }

    if (selected_menu_item == MENU_OPTION_CONVERT_TO_MESH_2D) {

        for (int j = 0; j < lines.size(); j++) {
            int index_ofs = computed_vertices.size();

            for (int i = 0; i < lines[j].size(); i++) {
                Vector2 vtx = lines[j][i];
                computed_uv.push_back(vtx / img_size);

                vtx -= rect.position; //offset by rect position

                //flip if flipped
                if (node->is_flipped_h())
                    vtx.x = rect.size.x - vtx.x - 1.0f;
                if (node->is_flipped_v())
                    vtx.y = rect.size.y - vtx.y - 1.0f;

                if (node->is_centered())
                    vtx -= rect.size / 2.0;

                computed_vertices.push_back(vtx);
            }

            Vector<int> poly = Geometry::triangulate_polygon(lines[j]);

            for (int i = 0; i < poly.size(); i += 3) {
                for (int k = 0; k < 3; k++) {
                    int idx = i + k;
                    int idxn = i + (k + 1) % 3;
                    uv_lines.push_back(lines[j][poly[idx]]);
                    uv_lines.push_back(lines[j][poly[idxn]]);

                    computed_indices.push_back(poly[idx] + index_ofs);
                }
            }
        }
    }

    outline_lines.clear();
    computed_outline_lines.clear();

    if (selected_menu_item == MENU_OPTION_CONVERT_TO_POLYGON_2D || selected_menu_item == MENU_OPTION_CREATE_COLLISION_POLY_2D || selected_menu_item == MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D) {
        outline_lines.reserve(lines.size());
        computed_outline_lines.reserve(lines.size());
        for (int pi = 0; pi < lines.size(); pi++) {

            Vector<Vector2> ol;
            Vector<Vector2> col;

            ol.reserve(lines[pi].size());
            col.reserve(lines[pi].size());

            for (size_t i = 0; i < lines[pi].size(); i++) {
                Vector2 vtx = lines[pi][i];

                ol.emplace_back(vtx);

                vtx -= rect.position; //offset by rect position

                //flip if flipped
                if (node->is_flipped_h())
                    vtx.x = rect.size.x - vtx.x - 1.0f;
                if (node->is_flipped_v())
                    vtx.y = rect.size.y - vtx.y - 1.0f;

                if (node->is_centered())
                    vtx -= rect.size / 2.0;

                col.emplace_back(vtx);
            }

            outline_lines.emplace_back(eastl::move(ol));
            computed_outline_lines.emplace_back(eastl::move(col));
        }
    }

    debug_uv->update();
}

void SpriteEditor::_create_node() {
    switch (selected_menu_item) {
        case MENU_OPTION_CONVERT_TO_MESH_2D: {
            _convert_to_mesh_2d_node();
        } break;
        case MENU_OPTION_CONVERT_TO_POLYGON_2D: {
            _convert_to_polygon_2d_node();
        } break;
        case MENU_OPTION_CREATE_COLLISION_POLY_2D: {
            _create_collision_polygon_2d_node();
        } break;
        case MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D: {
            _create_light_occluder_2d_node();
        } break;
    }
}

void SpriteEditor::_convert_to_mesh_2d_node() {

    if (computed_vertices.size() < 3) {
        err_dialog->set_text(TTR("Invalid geometry, can't replace by mesh."));
        err_dialog->popup_centered_minsize();
        return;
    }

    Ref<ArrayMesh> mesh(make_ref_counted<ArrayMesh>());

    SurfaceArrays a(eastl::move(Vector<Vector2>(computed_vertices)));
    a.m_uv_1 = computed_uv;
    a.m_indices = computed_indices;

    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, eastl::move(a), {}, Mesh::ARRAY_FLAG_USE_2D_VERTICES);

    MeshInstance2D *mesh_instance = memnew(MeshInstance2D);
    mesh_instance->set_mesh(mesh);

    UndoRedo *ur = EditorNode::get_undo_redo();
    ur->create_action(TTR("Convert to Mesh2D"));
    ur->add_do_method(EditorNode::get_singleton()->get_scene_tree_dock(), "replace_node", Variant(node), Variant(mesh_instance), true, false);
    ur->add_do_reference(mesh_instance);
    ur->add_undo_method(EditorNode::get_singleton()->get_scene_tree_dock(), "replace_node", Variant(mesh_instance), Variant(node), false, false);
    ur->add_undo_reference(node);
    ur->commit_action();
}

void SpriteEditor::_convert_to_polygon_2d_node() {

    if (computed_outline_lines.empty()) {
        err_dialog->set_text(TTR("Invalid geometry, can't create polygon."));
        err_dialog->popup_centered_minsize();
        return;
    }

    Polygon2D *polygon_2d_instance = memnew(Polygon2D);

    int total_point_count = 0;
    for (int i = 0; i < computed_outline_lines.size(); i++)
        total_point_count += computed_outline_lines[i].size();

    PoolVector2Array polygon;
    polygon.resize(total_point_count);
    PoolVector2Array::Write polygon_write = polygon.write();

    PoolVector2Array uvs;
    uvs.resize(total_point_count);
    PoolVector2Array::Write uvs_write = uvs.write();

    int current_point_index = 0;

    Array polys;
    polys.resize(computed_outline_lines.size());

    for (int i = 0; i < computed_outline_lines.size(); i++) {

        const Vector<Vector2> &outline = computed_outline_lines[i];
        const Vector<Vector2> &uv_outline = outline_lines[i];

        PoolIntArray pia;
        pia.resize(outline.size());
        PoolIntArray::Write pia_write = pia.write();

        for (int pi = 0; pi < outline.size(); pi++) {
            polygon_write[current_point_index] = outline[pi];
            uvs_write[current_point_index] = uv_outline[pi];
            pia_write[pi] = current_point_index;
            current_point_index++;
        }

        polys[i] = pia;
    }

    polygon_2d_instance->set_uv(uvs);
    polygon_2d_instance->set_polygon(polygon);
    polygon_2d_instance->set_polygons(polys);

    UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
    ur->create_action(TTR("Convert to Polygon2D"));
    ur->add_do_method(EditorNode::get_singleton()->get_scene_tree_dock(), "replace_node", Variant(node), Variant(polygon_2d_instance), true, false);
    ur->add_do_reference(polygon_2d_instance);
    ur->add_undo_method(EditorNode::get_singleton()->get_scene_tree_dock(), "replace_node", Variant(polygon_2d_instance), Variant(node), false, false);
    ur->add_undo_reference(node);
    ur->commit_action();
}

void SpriteEditor::_create_collision_polygon_2d_node() {

    if (computed_outline_lines.empty()) {
        err_dialog->set_text(TTR("Invalid geometry, can't create collision polygon."));
        err_dialog->popup_centered_minsize();
        return;
    }

    for (int i = 0; i < computed_outline_lines.size(); i++) {

        Vector<Vector2> outline = computed_outline_lines[i];

        CollisionPolygon2D *collision_polygon_2d_instance = memnew(CollisionPolygon2D);
        collision_polygon_2d_instance->set_polygon(eastl::move(outline));

        UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
        ur->create_action(TTR("Create CollisionPolygon2D Sibling"));
        ur->add_do_method(this, "_add_as_sibling_or_child", Variant(node), Variant(collision_polygon_2d_instance));
        ur->add_do_reference(collision_polygon_2d_instance);
        ur->add_undo_method(node != this->get_tree()->get_edited_scene_root() ? node->get_parent() : this->get_tree()->get_edited_scene_root(), "remove_child", Variant(collision_polygon_2d_instance));
        ur->commit_action();
    }
}

void SpriteEditor::_create_light_occluder_2d_node() {

    if (computed_outline_lines.empty()) {
        err_dialog->set_text(TTR("Invalid geometry, can't create light occluder."));
        err_dialog->popup_centered_minsize();
        return;
    }

    for (int i = 0; i < computed_outline_lines.size(); i++) {

        Vector<Vector2> outline = computed_outline_lines[i];

        Ref<OccluderPolygon2D> polygon(make_ref_counted<OccluderPolygon2D>());

        polygon->set_polygon(eastl::move(outline));

        LightOccluder2D *light_occluder_2d_instance = memnew(LightOccluder2D);
        light_occluder_2d_instance->set_occluder_polygon(polygon);

        UndoRedo *ur = EditorNode::get_singleton()->get_undo_redo();
        ur->create_action(TTR("Create LightOccluder2D Sibling"));
        ur->add_do_method(this, "_add_as_sibling_or_child", Variant(node), Variant(light_occluder_2d_instance));
        ur->add_do_reference(light_occluder_2d_instance);
        ur->add_undo_method(node != this->get_tree()->get_edited_scene_root() ? node->get_parent() : this->get_tree()->get_edited_scene_root(), "remove_child", Variant(light_occluder_2d_instance));
        ur->commit_action();
    }
}

void SpriteEditor::_add_as_sibling_or_child(Node *p_own_node, Node *p_new_node) {
    // Can't make sibling if own node is scene root
    if (p_own_node != this->get_tree()->get_edited_scene_root()) {
        p_own_node->get_parent()->add_child(p_new_node, true);
        object_cast<Node2D>(p_new_node)->set_transform(object_cast<Node2D>(p_own_node)->get_transform());
    } else {
        p_own_node->add_child(p_new_node, true);
    }

    p_new_node->set_owner(this->get_tree()->get_edited_scene_root());
}

void SpriteEditor::_debug_uv_draw() {

    Ref<Texture> tex = node->get_texture();
    ERR_FAIL_COND(not tex);

    Point2 draw_pos_offset = Point2(1.0f, 1.0f);
    Size2 draw_size_offset = Size2(2.0f, 2.0f);

    debug_uv->set_clip_contents(true);
    debug_uv->draw_texture(tex, draw_pos_offset);
    debug_uv->set_custom_minimum_size(tex->get_size() + draw_size_offset);
    debug_uv->draw_set_transform(draw_pos_offset, 0, Size2(1.0f, 1.0f));

    Color color = Color(1.0, 0.8f, 0.7f);

    if (selected_menu_item == MENU_OPTION_CONVERT_TO_MESH_2D && !uv_lines.empty()) {
        debug_uv->draw_multiline(uv_lines, color);

    } else if ((selected_menu_item == MENU_OPTION_CONVERT_TO_POLYGON_2D || selected_menu_item == MENU_OPTION_CREATE_COLLISION_POLY_2D || selected_menu_item == MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D) && !outline_lines.empty()) {
        for (int i = 0; i < outline_lines.size(); i++) {
            Vector<Vector2> outline = outline_lines[i];

            debug_uv->draw_polyline(outline, color);
            debug_uv->draw_line(outline.front(), outline.back(), color);
        }
    }
}

void SpriteEditor::_bind_methods() {
    MethodBinder::bind_method("_add_as_sibling_or_child", &SpriteEditor::_add_as_sibling_or_child);
}

SpriteEditor::SpriteEditor() {

    options = memnew(MenuButton);

    CanvasItemEditor::get_singleton()->add_control_to_menu_panel(options);

    options->set_text(TTR("Sprite2D"));
    options->set_button_icon(EditorNode::get_singleton()->get_gui_base()->get_theme_icon("Sprite2D", "EditorIcons"));

    options->get_popup()->add_item(TTR("Convert to Mesh2D"), MENU_OPTION_CONVERT_TO_MESH_2D);
    options->get_popup()->add_item(TTR("Convert to Polygon2D"), MENU_OPTION_CONVERT_TO_POLYGON_2D);
    options->get_popup()->add_item(TTR("Create CollisionPolygon2D Sibling"), MENU_OPTION_CREATE_COLLISION_POLY_2D);
    options->get_popup()->add_item(TTR("Create LightOccluder2D Sibling"), MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D);
    options->set_switch_on_hover(true);

    options->get_popup()->connect("id_pressed",callable_mp(this, &ClassName::_menu_option));

    err_dialog = memnew(AcceptDialog);
    add_child(err_dialog);

    debug_uv_dialog = memnew(ConfirmationDialog);
    debug_uv_dialog->get_ok()->set_text(TTR("Create Mesh2D"));
    debug_uv_dialog->set_title("Mesh 2D Preview");
    VBoxContainer *vb = memnew(VBoxContainer);
    debug_uv_dialog->add_child(vb);
    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_custom_minimum_size(Size2(800, 500) * EDSCALE);
    scroll->set_enable_h_scroll(true);
    scroll->set_enable_v_scroll(true);
    vb->add_margin_child(TTR("Preview:"), scroll, true);
    debug_uv = memnew(Control);
    debug_uv->connect("draw",callable_mp(this, &ClassName::_debug_uv_draw));
    scroll->add_child(debug_uv);
    debug_uv_dialog->connect("confirmed",callable_mp(this, &ClassName::_create_node));

    HBoxContainer *hb = memnew(HBoxContainer);
    hb->add_child(memnew(Label(TTR("Simplification: "))));
    simplification = memnew(SpinBox);
    simplification->set_min(0.01f);
    simplification->set_max(10.00);
    simplification->set_step(0.01f);
    simplification->set_value(2);
    hb->add_child(simplification);
    hb->add_spacer();
    hb->add_child(memnew(Label(TTR("Shrink (Pixels): "))));
    shrink_pixels = memnew(SpinBox);
    shrink_pixels->set_min(0);
    shrink_pixels->set_max(10);
    shrink_pixels->set_step(1);
    shrink_pixels->set_value(0);
    hb->add_child(shrink_pixels);
    hb->add_spacer();
    hb->add_child(memnew(Label(TTR("Grow (Pixels): "))));
    grow_pixels = memnew(SpinBox);
    grow_pixels->set_min(0);
    grow_pixels->set_max(10);
    grow_pixels->set_step(1);
    grow_pixels->set_value(2);
    hb->add_child(grow_pixels);
    hb->add_spacer();
    update_preview = memnew(Button);
    update_preview->set_text(TTR("Update Preview"));
    update_preview->connect("pressed",callable_mp(this, &ClassName::_update_mesh_data));
    hb->add_child(update_preview);
    vb->add_margin_child(TTR("Settings:"), hb);

    add_child(debug_uv_dialog);
}

void SpriteEditorPlugin::edit(Object *p_object) {

    sprite_editor->edit(object_cast<Sprite2D>(p_object));
}

bool SpriteEditorPlugin::handles(Object *p_object) const {

    return p_object->is_class("Sprite2D");
}

void SpriteEditorPlugin::make_visible(bool p_visible) {

    if (p_visible) {
        sprite_editor->options->show();
    } else {

        sprite_editor->options->hide();
        sprite_editor->edit(nullptr);
    }
}

SpriteEditorPlugin::SpriteEditorPlugin(EditorNode *p_node) {

    editor = p_node;
    sprite_editor = memnew(SpriteEditor);
    editor->get_viewport()->add_child(sprite_editor);
    make_visible(false);

    //sprite_editor->options->hide();
}

SpriteEditorPlugin::~SpriteEditorPlugin() = default;
