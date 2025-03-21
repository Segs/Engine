/*************************************************************************/
/*  navigation_polygon.cpp                                               */
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

#include "navigation_polygon.h"

#include "core/callable_method_pointer.h"
#include "core/core_string_names.h"
#include "core/math/geometry.h"
#include "core/method_bind.h"
#include "core/object_tooling.h"
#include "core/os/mutex.h"
#include "core/engine.h"
#include "core/os/mutex.h"
#include "core/translation_helpers.h"
#include "navigation_2d.h"
#include "servers/navigation_2d_server.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/navigation_mesh.h"
#include "core/map.h"

#include "thirdparty/misc/triangulator.h"

IMPL_GDCLASS(NavigationPolygon)
IMPL_GDCLASS(NavigationPolygonInstance)

#ifdef TOOLS_ENABLED
Rect2 NavigationPolygon::_edit_get_rect() const {

    if (!rect_cache_dirty) {
        return item_rect;
    }
    item_rect = Rect2();
    bool first = true;

    for (int i = 0; i < outlines.size(); i++) {
        const PoolVector<Vector2> &outline = outlines[i];
        const int outline_size = outline.size();
        if (outline_size < 3)
            continue;
        PoolVector<Vector2>::Read p = outline.read();
        for (int j = 0; j < outline_size; j++) {
            if (first) {
                item_rect = Rect2(p[j], Vector2(0, 0));
                first = false;
            } else {
                item_rect.expand_to(p[j]);
            }
        }
    }

    rect_cache_dirty = false;
    return item_rect;
}

bool NavigationPolygon::_edit_is_selected_on_click(const Point2 &p_point, float p_tolerance) const {

    for (int i = 0; i < outlines.size(); i++) {
        const PoolVector<Vector2> &outline = outlines[i];
        const int outline_size = outline.size();
        if (outline_size < 3)
            continue;
        if (Geometry::is_point_in_polygon(p_point, outline.toSpan()))
            return true;
    }
    return false;
}
#endif

void NavigationPolygon::set_vertices(Vector<Vector2> &&p_vertices) {
    {
        MutexGuard guard(navmesh_generation);
        navmesh.unref();
    }
    vertices = eastl::move(p_vertices);
    rect_cache_dirty = true;
}

void NavigationPolygon::_set_polygons(const Array &p_array) {
    {
        MutexGuard guard(navmesh_generation);
        navmesh.unref();
    }
    polygons.resize(p_array.size());
    for (int i = 0; i < p_array.size(); i++) {
        polygons[i].indices = p_array[i].as<Vector<int>>();
    }
}

Array NavigationPolygon::_get_polygons() const {

    Array ret;
    ret.resize(polygons.size());
    for (int i = 0; i < ret.size(); i++) {
        ret[i] = Variant::from(polygons[i].indices);
    }

    return ret;
}

void NavigationPolygon::_set_outlines(const Array &p_array) {

    outlines.resize(p_array.size());
    for (int i = 0; i < p_array.size(); i++) {
        outlines[i] = p_array[i].as<PoolVector<Vector2>>();
    }
    rect_cache_dirty = true;
}

Array NavigationPolygon::_get_outlines() const {

    Array ret;
    ret.resize(outlines.size());
    for (int i = 0; i < ret.size(); i++) {
        ret[i] = Variant(outlines[i]);
    }

    return ret;
}

void NavigationPolygon::add_polygon(Vector<int> &&p_polygon) {

    Polygon polygon;
    polygon.indices = eastl::move(p_polygon);
    polygons.push_back(polygon);
    {
        MutexGuard guard(navmesh_generation);
        navmesh.unref();
    }
}

void NavigationPolygon::add_outline_at_index(const PoolVector<Vector2> &p_outline, int p_index) {

    outlines.insert_at(p_index, p_outline);
    rect_cache_dirty = true;
}

int NavigationPolygon::get_polygon_count() const {

    return polygons.size();
}
const Vector<int> &NavigationPolygon::get_polygon(int p_idx) {

    ERR_FAIL_INDEX_V(p_idx, polygons.size(), null_int_pvec);
    return polygons[p_idx].indices;
}
void NavigationPolygon::clear_polygons() {

    polygons.clear();
    {
        MutexGuard guard(navmesh_generation);
        navmesh.unref();
    }
}

const Ref<NavigationMesh> &NavigationPolygon::get_mesh() {
    MutexGuard guard(navmesh_generation);
    if (!navmesh) {
        navmesh = make_ref_counted<NavigationMesh>();
        Vector<Vector3> verts;
        verts.reserve(get_vertices().size());
        const Vector<Vector2> &r(get_vertices());

        for (int i(0); i < get_vertices().size(); i++) {
            verts.emplace_back(r[i].x, 0.0, r[i].y);
        }
        navmesh->set_vertices(eastl::move(verts));

        for (int i(0); i < get_polygon_count(); i++) {
            navmesh->add_polygon(Vector<int>(get_polygon(i)));
        }
    }
    return navmesh;
}

void NavigationPolygon::add_outline(const PoolVector<Vector2> &p_outline) {

    outlines.push_back(p_outline);
    rect_cache_dirty = true;
}

int NavigationPolygon::get_outline_count() const {

    return outlines.size();
}

void NavigationPolygon::set_outline(int p_idx, const PoolVector<Vector2> &p_outline) {
    ERR_FAIL_INDEX(p_idx, outlines.size());
    outlines[p_idx] = p_outline;
    rect_cache_dirty = true;
}

void NavigationPolygon::remove_outline(int p_idx) {

    ERR_FAIL_INDEX(p_idx, outlines.size());
    outlines.erase_at(p_idx);
    rect_cache_dirty = true;
}

PoolVector<Vector2> NavigationPolygon::get_outline(int p_idx) const {
    ERR_FAIL_INDEX_V(p_idx, outlines.size(), PoolVector<Vector2>());
    return outlines[p_idx];
}

void NavigationPolygon::clear_outlines() {

    outlines.clear();
    rect_cache_dirty = true;
}
void NavigationPolygon::make_polygons_from_outlines() {

    {
        MutexGuard guard(navmesh_generation);
        navmesh.unref();
    }

    eastl::list<TriangulatorPoly> in_poly, out_poly;

    Vector2 outside_point(-1e10, -1e10);

    for (int i = 0; i < outlines.size(); i++) {

        PoolVector<Vector2> ol = outlines[i];
        int olsize = ol.size();
        if (olsize < 3)
            continue;
        PoolVector<Vector2>::Read r = ol.read();
        for (int j = 0; j < olsize; j++) {
            outside_point.x = M_MAX(r[j].x, outside_point.x);
            outside_point.y = M_MAX(r[j].y, outside_point.y);
        }
    }

    outside_point += Vector2(0.7239784f, 0.819238f); //avoid precision issues

    for (int i = 0; i < outlines.size(); i++) {

        PoolVector<Vector2> ol = outlines[i];
        int olsize = ol.size();
        if (olsize < 3)
            continue;
        PoolVector<Vector2>::Read r = ol.read();

        int interscount = 0;
        //test if this is an outer outline
        for (int k = 0; k < outlines.size(); k++) {

            if (i == k)
                continue; //no self intersect

            PoolVector<Vector2> ol2 = outlines[k];
            int olsize2 = ol2.size();
            if (olsize2 < 3)
                continue;
            PoolVector<Vector2>::Read r2 = ol2.read();

            for (int l = 0; l < olsize2; l++) {

                if (Geometry::segment_intersects_segment_2d(r[0], outside_point, r2[l], r2[(l + 1) % olsize2], nullptr)) {
                    interscount++;
                }
            }
        }

        bool outer = (interscount % 2) == 0;
        static_assert(sizeof(Vector2)==sizeof(TriangulatorPoint));
        TriangulatorPoly tp((const TriangulatorPoint *)r.ptr(),olsize);

        if (outer)
            tp.SetOrientation(TRIANGULATOR_CCW);
        else {
            tp.SetOrientation(TRIANGULATOR_CW);
            tp.SetHole(true);
        }

        in_poly.push_back(tp);
    }

    TriangulatorPartition tpart;
    if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { //failed!
        ERR_PRINT("NavigationPolygon: Convex partition failed!");
        return;
    }

    polygons.clear();
    vertices.resize(0);
    //TODO: SEGS: consider faster data structure here.
    Map<Vector2, int> points;
    for (TriangulatorPoly &tp : out_poly) {
        struct Polygon p;

        for (long i = 0; i < tp.GetNumPoints(); i++) {
            Vector2 pt {tp[i].x,tp[i].y};
            Map<Vector2, int>::iterator E = points.find(pt);
            if (E==points.end()) {
                E = points.emplace(pt, vertices.size()).first;
                vertices.push_back(pt);
            }
            p.indices.push_back(E->second);
        }

        polygons.push_back(p);
    }

    emit_signal(CoreStringNames::get_singleton()->changed);
}

void NavigationPolygon::_bind_methods() {

    SE_BIND_METHOD(NavigationPolygon,set_vertices);
    SE_BIND_METHOD(NavigationPolygon,get_vertices);

    SE_BIND_METHOD(NavigationPolygon,add_polygon);
    SE_BIND_METHOD(NavigationPolygon,get_polygon_count);
    SE_BIND_METHOD(NavigationPolygon,get_polygon);
    SE_BIND_METHOD(NavigationPolygon,clear_polygons);

    SE_BIND_METHOD(NavigationPolygon,add_outline);
    SE_BIND_METHOD(NavigationPolygon,add_outline_at_index);
    SE_BIND_METHOD(NavigationPolygon,get_outline_count);
    SE_BIND_METHOD(NavigationPolygon,set_outline);
    SE_BIND_METHOD(NavigationPolygon,get_outline);
    SE_BIND_METHOD(NavigationPolygon,remove_outline);
    SE_BIND_METHOD(NavigationPolygon,clear_outlines);
    SE_BIND_METHOD(NavigationPolygon,make_polygons_from_outlines);

    SE_BIND_METHOD(NavigationPolygon,_set_polygons);
    SE_BIND_METHOD(NavigationPolygon,_get_polygons);

    SE_BIND_METHOD(NavigationPolygon,_set_outlines);
    SE_BIND_METHOD(NavigationPolygon,_get_outlines);

    ADD_PROPERTY(PropertyInfo(VariantType::POOL_VECTOR2_ARRAY, "vertices", PropertyHint::None, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_vertices", "get_vertices");
    ADD_PROPERTY(PropertyInfo(VariantType::ARRAY, "polygons", PropertyHint::None, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_polygons", "_get_polygons");
    ADD_PROPERTY(PropertyInfo(VariantType::ARRAY, "outlines", PropertyHint::None, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_outlines", "_get_outlines");
}

NavigationPolygon::NavigationPolygon() {
}

NavigationPolygon::~NavigationPolygon() {
}

void NavigationPolygonInstance::set_enabled(bool p_enabled) {

    if (enabled == p_enabled) {
        return;
    }
    enabled = p_enabled;

    if (!is_inside_tree()) {
        return;
    }

    if (!enabled) {

        Navigation2DServer::get_singleton()->region_set_map(region, RID());
    } else {

        if (navigation) {
            Navigation2DServer::get_singleton()->region_set_map(region, navigation->get_rid());
        }
    }

    if (Engine::get_singleton()->is_editor_hint() || get_tree()->is_debugging_navigation_hint())
        update();
}

bool NavigationPolygonInstance::is_enabled() const {

    return enabled;
}

/////////////////////////////

#ifdef TOOLS_ENABLED
Rect2 NavigationPolygonInstance::_edit_get_rect() const {

    return navpoly ? navpoly->_edit_get_rect() : Rect2();
}

bool NavigationPolygonInstance::_edit_is_selected_on_click(const Point2 &p_point, float p_tolerance) const {

    return navpoly ? navpoly->_edit_is_selected_on_click(p_point, p_tolerance) : false;
}
#endif
void NavigationPolygonInstance::_notification(int p_what) {

    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {

            Node2D *c = this;
            while (c) {

                navigation = object_cast<Navigation2D>(c);
                if (navigation) {

                    if (enabled) {
                        Navigation2DServer::get_singleton()->region_set_map(region, navigation->get_rid());
                    }
                    break;
                }

                c = object_cast<Node2D>(c->get_parent());
            }

        } break;
        case NOTIFICATION_TRANSFORM_CHANGED: {

            Navigation2DServer::get_singleton()->region_set_transform(region, get_global_transform());

        } break;
        case NOTIFICATION_EXIT_TREE: {

            if (navigation) {
                Navigation2DServer::get_singleton()->region_set_map(region, RID());
            }
            navigation = nullptr;
        } break;
        case NOTIFICATION_DRAW: {

            if (is_inside_tree() && (Engine::get_singleton()->is_editor_hint() || get_tree()->is_debugging_navigation_hint()) && navpoly) {

                const Vector<Vector2> &verts = navpoly->get_vertices();
                int vsize = verts.size();
                if (vsize < 3)
                    return;

                Color color;
                if (enabled) {
                    color = get_tree()->get_debug_navigation_color();
                } else {
                    color = get_tree()->get_debug_navigation_disabled_color();
                }
                Vector<Color> colors;
                Vector<Vector2> vertices;
                vertices = verts;
                colors.resize(vsize,color);

                Vector<int> indices;
                indices.reserve(navpoly->get_polygon_count()*2*3);
                for (int i = 0; i < navpoly->get_polygon_count(); i++) {
                    const Vector<int> &polygon = navpoly->get_polygon(i);

                    for (int j = 2; j < polygon.size(); j++) {

                        int kofs[3] = { 0, j - 1, j };
                        for (int k = 0; k < 3; k++) {

                            int idx = polygon[kofs[k]];
                            ERR_FAIL_INDEX(idx, vsize);
                            indices.push_back(idx);
                        }
                    }
                }
                RenderingServer::get_singleton()->canvas_item_add_triangle_array(get_canvas_item(), indices, vertices, colors);
            }
        } break;
    }
}

void NavigationPolygonInstance::set_navigation_polygon(const Ref<NavigationPolygon> &p_navpoly) {

    if (p_navpoly == navpoly) {
        return;
    }

    if (navpoly) {
        navpoly->disconnect(CoreStringNames::get_singleton()->changed, callable_mp(this, &NavigationPolygonInstance::_navpoly_changed));
    }
    navpoly = p_navpoly;
    Navigation2DServer::get_singleton()->region_set_navpoly(region, p_navpoly);
    if (navpoly) {
        navpoly->connect(CoreStringNames::get_singleton()->changed, callable_mp(this, &NavigationPolygonInstance::_navpoly_changed));
    }
    _navpoly_changed();

    Object_change_notify(this,"navpoly");
    update_configuration_warning();
}

Ref<NavigationPolygon> NavigationPolygonInstance::get_navigation_polygon() const {

    return navpoly;
}

void NavigationPolygonInstance::_navpoly_changed() {

    if (is_inside_tree() && (Engine::get_singleton()->is_editor_hint() || get_tree()->is_debugging_navigation_hint()))
        update();
}

String NavigationPolygonInstance::get_configuration_warning() const {

    if (!is_visible_in_tree() || !is_inside_tree())
        return String();

    String warning = BaseClassName::get_configuration_warning();

    if (not navpoly) {
        if(!warning.empty())
            warning.append("\n\n");
        return warning+TTRS("A NavigationPolygon resource must be set or created for this node to work. Please set a property or draw a polygon.");
    }
    const Node2D *c = this;
    while (c) {

        if (object_cast<Navigation2D>(c)) {
            return warning;
        }

        c = object_cast<Node2D>(c->get_parent());
    }

    if(!warning.empty())
        warning.append("\n\n");

    return warning+TTRS("NavigationPolygonInstance must be a child or grandchild to a Navigation2D node. It only provides navigation data.");
}

void NavigationPolygonInstance::_bind_methods() {

    SE_BIND_METHOD(NavigationPolygonInstance,set_navigation_polygon);
    SE_BIND_METHOD(NavigationPolygonInstance,get_navigation_polygon);

    SE_BIND_METHOD(NavigationPolygonInstance,set_enabled);
    SE_BIND_METHOD(NavigationPolygonInstance,is_enabled);

    SE_BIND_METHOD(NavigationPolygonInstance,_navpoly_changed);

    ADD_PROPERTY(PropertyInfo(VariantType::OBJECT, "navpoly", PropertyHint::ResourceType, "NavigationPolygon"), "set_navigation_polygon", "get_navigation_polygon");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "enabled"), "set_enabled", "is_enabled");
}

NavigationPolygonInstance::NavigationPolygonInstance() {

    enabled = true;
    set_notify_transform(true);
    region = Navigation2DServer::get_singleton()->region_create();

    navigation = nullptr;
}

NavigationPolygonInstance::~NavigationPolygonInstance() {
    Navigation2DServer::get_singleton()->free_rid(region);
}
