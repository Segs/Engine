/*************************************************************************/
/*  navigation_2d_server.cpp                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "servers/navigation_2d_server.h"
#include "core/math/transform.h"
#include "core/math/transform_2d.h"
#include "servers/navigation_server.h"
#include "core/method_bind.h"
#include "core/method_bind_interface.h"


IMPL_GDCLASS(Navigation2DServer)

/**
    @author AndreaCatania
*/

Navigation2DServer *Navigation2DServer::singleton = nullptr;

#define FORWARD_0_C(FUNC_NAME)                                 \
    Navigation2DServer::FUNC_NAME() const {                    \
        return NavigationServer::get_singleton()->FUNC_NAME(); \
    }

#define FORWARD_1(FUNC_NAME, T_0, D_0, CONV_0)                                \
    Navigation2DServer::FUNC_NAME(T_0 D_0) {                                  \
        return NavigationServer::get_singleton_mut()->FUNC_NAME(CONV_0(D_0)); \
    }

#define FORWARD_1_C(FUNC_NAME, T_0, D_0, CONV_0)                          \
    Navigation2DServer::FUNC_NAME(T_0 D_0)                                \
            const {                                                       \
        return NavigationServer::get_singleton()->FUNC_NAME(CONV_0(D_0)); \
    }

#define FORWARD_2_C(FUNC_NAME, T_0, D_0, T_1, D_1, CONV_0, CONV_1)                     \
    Navigation2DServer::FUNC_NAME(T_0 D_0, T_1 D_1)                                    \
            const {                                                                    \
        return NavigationServer::get_singleton()->FUNC_NAME(CONV_0(D_0), CONV_1(D_1)); \
    }

#define FORWARD_2_R_C(CONV_R, FUNC_NAME, T_0, D_0, T_1, D_1, CONV_0, CONV_1)                                           \
    Navigation2DServer::FUNC_NAME(T_0 D_0, T_1 D_1) const {                                                            \
        return CONV_R(NavigationServer::get_singleton()->FUNC_NAME(CONV_0(D_0), CONV_1(D_1)));                         \
    }
#define FORWARD_4_R_C(CONV_R, FUNC_NAME, T_0, D_0, T_1, D_1, T_2, D_2, T_3, D_3, CONV_0, CONV_1, CONV_2, CONV_3)         \
    Navigation2DServer::FUNC_NAME(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3) const {                                          \
        return CONV_R(                                                                                                 \
                NavigationServer::get_singleton()->FUNC_NAME(CONV_0(D_0), CONV_1(D_1), CONV_2(D_2), CONV_3(D_3)));     \
    }

#define FORWARD_4_C(FUNC_NAME, T_0, D_0, T_1, D_1, T_2, D_2, T_3, D_3, CONV_0, CONV_1, CONV_2, CONV_3)           \
    Navigation2DServer::FUNC_NAME(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3) const {                                          \
        return NavigationServer::get_singleton()->FUNC_NAME(CONV_0(D_0), CONV_1(D_1), CONV_2(D_2), CONV_3(D_3)); \
    }

static RID rid_to_rid(const RID d) {
    return d;
}
static bool bool_to_bool(const bool d) {
    return d;
}
static int int_to_int(const int d) {
    return d;
}
static real_t real_to_real(const real_t d) {
    return d;
}
static Vector3 v2_to_v3(const Vector2 d) {
    return Vector3(d.x, 0.0, d.y);
}
static Vector2 v3_to_v2(const Vector3 &d) {
    return Vector2(d.x, d.z);
}
static Vector<Vector2> vector_v3_to_v2(const Vector<Vector3> &d) {
    Vector<Vector2> nd;
    nd.reserve(d.size());
    for (Vector3 v : d) {
        nd.emplace_back(v3_to_v2(v));
    }
    return nd;
}
Transform trf2_to_trf3(const Transform2D &d) {
    Vector3 o(v2_to_v3(d.get_origin()));
    Basis b;
    b.rotate(Vector3(0, 1, 0), d.get_rotation());
    return Transform(b, o);
}
Object *obj_to_obj(Object *d) {
    return d;
}
StringName sn_to_sn(StringName &d) {
    return d;
}
Variant var_to_var(Variant &d) {
    return d;
}
Ref<NavigationMesh> poly_to_mesh(Ref<NavigationPolygon> d) {
    if (d) {
        return d->get_mesh();
    } else {
        return Ref<NavigationMesh>();
    }
}

void Navigation2DServer::_bind_methods() {
    SE_BIND_METHOD(Navigation2DServer,map_create);
    SE_BIND_METHOD(Navigation2DServer,map_set_active);
    SE_BIND_METHOD(Navigation2DServer,map_is_active);
    SE_BIND_METHOD(Navigation2DServer,map_set_cell_size);
    SE_BIND_METHOD(Navigation2DServer,map_get_cell_size);
    SE_BIND_METHOD(Navigation2DServer,map_set_edge_connection_margin);
    SE_BIND_METHOD(Navigation2DServer,map_get_edge_connection_margin);
    SE_BIND_METHOD(Navigation2DServer,map_get_path);

    SE_BIND_METHOD(Navigation2DServer,map_get_closest_point);
    SE_BIND_METHOD(Navigation2DServer,map_get_closest_point_owner);
    SE_BIND_METHOD(Navigation2DServer,region_create);
    SE_BIND_METHOD(Navigation2DServer,region_set_map);
    SE_BIND_METHOD(Navigation2DServer,region_set_transform);
    SE_BIND_METHOD(Navigation2DServer,region_set_navpoly);

    SE_BIND_METHOD(Navigation2DServer,agent_create);
    SE_BIND_METHOD(Navigation2DServer,agent_set_map);
    SE_BIND_METHOD(Navigation2DServer,agent_set_neighbor_dist);
    SE_BIND_METHOD(Navigation2DServer,agent_set_max_neighbors);
    SE_BIND_METHOD(Navigation2DServer,agent_set_time_horizon);
    SE_BIND_METHOD(Navigation2DServer,agent_set_radius);
    SE_BIND_METHOD(Navigation2DServer,agent_set_max_speed);
    SE_BIND_METHOD(Navigation2DServer,agent_set_velocity);
    SE_BIND_METHOD(Navigation2DServer,agent_set_target_velocity);
    SE_BIND_METHOD(Navigation2DServer,agent_set_position);
    SE_BIND_METHOD(Navigation2DServer,agent_is_map_changed);
    SE_BIND_METHOD(Navigation2DServer,agent_set_callback);

    SE_BIND_METHOD(Navigation2DServer,free_rid);
}

Navigation2DServer::Navigation2DServer() {
    singleton = this;
}

Navigation2DServer::~Navigation2DServer() {
    singleton = nullptr;
}

RID FORWARD_0_C(map_create)

void FORWARD_2_C(map_set_active, RID, p_map, bool, p_active, rid_to_rid, bool_to_bool)

bool FORWARD_1_C(map_is_active, RID, p_map, rid_to_rid)

void FORWARD_2_C(map_set_cell_size, RID, p_map, real_t, p_cell_size, rid_to_rid, real_to_real)
real_t FORWARD_1_C(map_get_cell_size, RID, p_map, rid_to_rid)

void FORWARD_2_C(map_set_cell_height, RID, p_map, real_t, p_cell_height, rid_to_rid, real_to_real);
real_t FORWARD_1_C(map_get_cell_height, RID, p_map, rid_to_rid);
void FORWARD_2_C(map_set_edge_connection_margin, RID, p_map, real_t, p_connection_margin, rid_to_rid, real_to_real)
real_t FORWARD_1_C(map_get_edge_connection_margin, RID, p_map, rid_to_rid)

Vector<Vector2> FORWARD_4_R_C(vector_v3_to_v2, map_get_path, RID, p_map, Vector2, p_origin, Vector2, p_destination, bool, p_optimize, rid_to_rid, v2_to_v3, v2_to_v3, bool_to_bool)
Vector2 FORWARD_2_R_C(v3_to_v2, map_get_closest_point, RID, p_map, const Vector2 &, p_point, rid_to_rid, v2_to_v3);
RID FORWARD_2_C(map_get_closest_point_owner, RID, p_map, const Vector2 &, p_point, rid_to_rid, v2_to_v3);

RID FORWARD_0_C(region_create)
void FORWARD_2_C(region_set_map, RID, p_region, RID, p_map, rid_to_rid, rid_to_rid)

void FORWARD_2_C(region_set_transform, RID, p_region, Transform2D, p_transform, rid_to_rid, trf2_to_trf3)

void Navigation2DServer::region_set_navpoly(RID p_region, Ref<NavigationPolygon> p_nav_mesh) const {
    NavigationServer::get_singleton()->region_set_navmesh(p_region, poly_to_mesh(p_nav_mesh));
}

RID Navigation2DServer::agent_create() const {
    RID agent = NavigationServer::get_singleton()->agent_create();
    NavigationServer::get_singleton()->agent_set_ignore_y(agent, true);
    return agent;
}

void FORWARD_2_C(agent_set_map, RID, p_agent, RID, p_map, rid_to_rid, rid_to_rid)

void FORWARD_2_C(agent_set_neighbor_dist, RID, p_agent, real_t, p_dist, rid_to_rid, real_to_real)

void FORWARD_2_C(agent_set_max_neighbors, RID, p_agent, int, p_count, rid_to_rid, int_to_int)

void FORWARD_2_C(agent_set_time_horizon, RID, p_agent, real_t, p_time, rid_to_rid, real_to_real)

void FORWARD_2_C(agent_set_radius, RID, p_agent, real_t, p_radius, rid_to_rid, real_to_real)

void FORWARD_2_C(agent_set_max_speed, RID, p_agent, real_t, p_max_speed, rid_to_rid, real_to_real)

void FORWARD_2_C(agent_set_velocity, RID, p_agent, Vector2, p_velocity, rid_to_rid, v2_to_v3)

void FORWARD_2_C(agent_set_target_velocity, RID, p_agent, Vector2, p_velocity, rid_to_rid, v2_to_v3)

void FORWARD_2_C(agent_set_position, RID, p_agent, Vector2, p_position, rid_to_rid, v2_to_v3)

void FORWARD_2_C(agent_set_ignore_y, RID, p_agent, bool, p_ignore, rid_to_rid, bool_to_bool)

bool FORWARD_1_C(agent_is_map_changed, RID, p_agent, rid_to_rid)

void Navigation2DServer::agent_set_callback(RID p_agent, Callable&& cb) const
{
    return NavigationServer::get_singleton()->agent_set_callback(rid_to_rid(p_agent), eastl::move(cb));
}

void FORWARD_1_C(free_rid, RID, p_object, rid_to_rid)
