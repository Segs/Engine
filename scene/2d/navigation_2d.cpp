/*************************************************************************/
/*  navigation_2d.cpp                                                    */
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

#include "navigation_2d.h"
#include "core/method_bind.h"
#include "core/math/geometry.h"
#include "servers/navigation_2d_server.h"

#define USE_ENTRY_POINT

IMPL_GDCLASS(Navigation2D)

#include "servers/navigation_2d_server.h"

void Navigation2D::_bind_methods() {

    SE_BIND_METHOD(Navigation2D,get_rid);

    MethodBinder::bind_method(D_METHOD("get_simple_path", {"start", "end", "optimize"}),&Navigation2D::get_simple_path, {DEFVAL(true)});
    SE_BIND_METHOD(Navigation2D,get_closest_point);
    SE_BIND_METHOD(Navigation2D,get_closest_point_owner);

    SE_BIND_METHOD(Navigation2D,set_cell_size);
    SE_BIND_METHOD(Navigation2D,get_cell_size);

    SE_BIND_METHOD(Navigation2D,set_edge_connection_margin);
    SE_BIND_METHOD(Navigation2D,get_edge_connection_margin);

    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "cell_size"), "set_cell_size", "get_cell_size");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "edge_connection_margin"), "set_edge_connection_margin", "get_edge_connection_margin");
}

void Navigation2D::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            Navigation2DServer::get_singleton()->map_set_active(map, true);
        } break;
        case NOTIFICATION_EXIT_TREE: {

            Navigation2DServer::get_singleton()->map_set_active(map, false);
        } break;
    }
}

void Navigation2D::set_cell_size(float p_cell_size) {
    cell_size = p_cell_size;
    Navigation2DServer::get_singleton()->map_set_cell_size(map, cell_size);
}

void Navigation2D::set_edge_connection_margin(float p_edge_connection_margin) {
    edge_connection_margin = p_edge_connection_margin;
    Navigation2DServer::get_singleton()->map_set_edge_connection_margin(map, edge_connection_margin);
}

Vector<Vector2> Navigation2D::get_simple_path(const Vector2 &p_start, const Vector2 &p_end, bool p_optimize) const {
    return Navigation2DServer::get_singleton()->map_get_path(map, p_start, p_end, p_optimize);
}

Vector2 Navigation2D::get_closest_point(const Vector2 &p_point) const {
    return Navigation2DServer::get_singleton()->map_get_closest_point(map, p_point);
}

RID Navigation2D::get_closest_point_owner(const Vector2 &p_point) const {
    return Navigation2DServer::get_singleton()->map_get_closest_point_owner(map, p_point);
}
Navigation2D::Navigation2D() {
    map = Navigation2DServer::get_singleton()->map_create();
    set_cell_size(1); // One pixel
    set_edge_connection_margin(1);
}

Navigation2D::~Navigation2D() {
    Navigation2DServer::get_singleton()->free_rid(map);
}
