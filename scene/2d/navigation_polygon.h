/*************************************************************************/
/*  navigation_polygon.h                                                 */
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

#include "core/rid.h"
#include "scene/2d/node_2d.h"
#include "core/os/mutex.h"
#include "core/resource.h"

class NavigationMesh;
class GODOT_EXPORT NavigationPolygon : public Resource {

    GDCLASS(NavigationPolygon,Resource)

    Vector<Vector2> vertices;
    struct Polygon {
        Vector<int> indices;
    };
    Vector<Polygon> polygons;
    Vector<PoolVector<Vector2> > outlines;

    mutable Rect2 item_rect;
    mutable bool rect_cache_dirty=true;

    Mutex navmesh_generation;
    // Navigation mesh
    Ref<NavigationMesh> navmesh;
protected:
    static void _bind_methods();
public:
    void _set_polygons(const Array &p_array);
    Array _get_polygons() const;

    void _set_outlines(const Array &p_array);
    Array _get_outlines() const;

public:
#ifdef TOOLS_ENABLED
    Rect2 _edit_get_rect() const;
    bool _edit_is_selected_on_click(const Point2 &p_point, float p_tolerance) const;
#endif

    void set_vertices(Vector<Vector2> &&p_vertices);
    const Vector<Vector2> &get_vertices() const { return vertices; }

    void add_polygon(Vector<int> &&p_polygon);
    int get_polygon_count() const;

    void add_outline(const PoolVector<Vector2> &p_outline);
    void add_outline_at_index(const PoolVector<Vector2> &p_outline, int p_index);
    void set_outline(int p_idx, const PoolVector<Vector2> &p_outline);
    PoolVector<Vector2> get_outline(int p_idx) const;
    void remove_outline(int p_idx);
    int get_outline_count() const;

    void clear_outlines();
    void make_polygons_from_outlines();

    const Vector<int> &get_polygon(int p_idx);
    void clear_polygons();

    const Ref<NavigationMesh> &get_mesh();

    NavigationPolygon();
    ~NavigationPolygon() override;
};

class Navigation2D;

class GODOT_EXPORT NavigationPolygonInstance : public Node2D {

    GDCLASS(NavigationPolygonInstance,Node2D)

    bool enabled;
    RID region;
    Navigation2D *navigation;
    Ref<NavigationPolygon> navpoly;

    void _navpoly_changed();

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
#ifdef TOOLS_ENABLED
    Rect2 _edit_get_rect() const override;
    bool _edit_is_selected_on_click(const Point2 &p_point, float p_tolerance) const override;
#endif

    void set_enabled(bool p_enabled);
    bool is_enabled() const;

    void set_navigation_polygon(const Ref<NavigationPolygon> &p_navpoly);
    Ref<NavigationPolygon> get_navigation_polygon() const;

    String get_configuration_warning() const override;

    NavigationPolygonInstance();
    ~NavigationPolygonInstance() override;
};
