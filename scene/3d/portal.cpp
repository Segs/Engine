/*************************************************************************/
/*  portal.cpp                                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "portal.h"

#include "core/engine.h"
#include "core/method_bind.h"
#include "core/class_db.h"
#include "core/string_formatter.h"
#include "core/translation_helpers.h"
#include "mesh_instance_3d.h"
#include "room.h"
#include "room_group.h"
#include "room_manager.h"
#include "scene/main/viewport.h"
#include "servers/rendering_server.h"
#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering/rendering_server_scene.h"
#include "scene/main/scene_tree.h"

IMPL_GDCLASS(Portal)
bool Portal::_portal_plane_convention = false;
bool Portal::_settings_gizmo_show_margins = true;

Portal::Portal() {
    clear();

    _settings_active = true;
    _settings_two_way = true;
    _internal = false;
    _linkedroom_ID[0] = -1;
    _linkedroom_ID[1] = -1;
    _pts_world.clear();
    _pts_local.clear();
    _pts_local_raw.resize(0);
    _pt_center_world = Vector3();
    _plane = Plane();
    _margin = 1.0;
    _use_default_margin = true;

    // the visual server portal lifetime is linked to the lifetime of this object
    _portal_rid = VSG::scene->portal_create();

#ifdef TOOLS_ENABLED
    _room_manager_godot_ID = entt::null;
#endif

    // portals are defined COUNTER clockwise,
    // because they point OUTWARD from the room in the direction
    // of the normal
    PoolVector<Vector2> points;
    points.resize(4);
    points.set(0, Vector2(1, -1));
    points.set(1, Vector2(1, 1));
    points.set(2, Vector2(-1, 1));
    points.set(3, Vector2(-1, -1));

    set_points(points); // default shape
}

Portal::~Portal() {
    if (_portal_rid != entt::null) {
        RenderingServer::get_singleton()->free_rid(_portal_rid);
    }
}

String Portal::get_configuration_warning() const {
    String warning = Node3D::get_configuration_warning();

    auto lambda = [](const Node *p_node) {
        return static_cast<bool>((object_cast<RoomManager>(p_node) || object_cast<Room>(p_node) || object_cast<RoomGroup>(p_node)));
    };

    if (Room::detect_nodes_using_lambda(this, lambda)) {
        if (Room::detect_nodes_of_type<RoomManager>(this)) {
            if (!warning.empty()) {
                warning += "\n\n";
            }
            warning += TTR("The RoomManager should not be a child or grandchild of a Portal.");
        }
        if (Room::detect_nodes_of_type<Room>(this)) {
            if (!warning.empty()) {
                warning += "\n\n";
            }
            warning += TTR("A Room should not be a child or grandchild of a Portal.");
        }
        if (Room::detect_nodes_of_type<RoomGroup>(this)) {
            if (!warning.empty()) {
                warning += "\n\n";
            }
            warning += TTR("A RoomGroup should not be a child or grandchild of a Portal.");
        }
    }

    return warning;
}

void Portal::set_point(int p_idx, const Vector2 &p_point) {
    if (p_idx >= _pts_local_raw.size()) {
        return;
    }

    _pts_local_raw.set(p_idx, p_point);
    _sanitize_points();
    update_gizmo();
}

void Portal::set_points(const PoolVector<Vector2> &p_points) {
    _pts_local_raw = p_points;
    _sanitize_points();

    if (is_inside_tree()) {
        portal_update();
        update_gizmo();
    }
}

PoolVector<Vector2> Portal::get_points() const {
    return _pts_local_raw;
}

// extra editor links to the room manager to allow unloading
// on change, or re-converting
void Portal::_changed() {
#ifdef TOOLS_ENABLED
    RoomManager *rm = RoomManager::active_room_manager;
    if (!rm) {
        return;
    }

    rm->_rooms_changed(String("changed Portal ") + get_name());
#endif
}

void Portal::clear() {
    _internal = false;
    _linkedroom_ID[0] = -1;
    _linkedroom_ID[1] = -1;
    _importing_portal = false;
}

void Portal::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            ERR_FAIL_COND(!get_world_3d());

            // defer full creation of the visual server portal to when the editor portal is in the scene tree
            VSG::scene->portal_set_scenario(_portal_rid, get_world_3d()->get_scenario());

            // we can't calculate world points until we have entered the tree
            portal_update();
            update_gizmo();

        } break;
        case NOTIFICATION_EXIT_WORLD: {
            // partially destroy  the visual server portal when the editor portal exits the scene tree
            VSG::scene->portal_set_scenario(_portal_rid, entt::null);
        } break;
        case NOTIFICATION_TRANSFORM_CHANGED: {
            // keep the world points and the visual server up to date
            portal_update();

            // In theory we shouldn't need to update the gizmo when the transform
            // changes .. HOWEVER, the portal margin is displayed in world space units,
            // back transformed to model space.
            // If the Z scale is changed by the user, the portal margin length can become incorrect
            // and needs 'resyncing' to the global scale of the portal node.
            // We really only need to do this when Z scale is changed, but it is easier codewise
            // to always change it, unless we have evidence this is a performance problem.
            update_gizmo();
        } break;
    }
}

void Portal::set_portal_active(bool p_active) {
    _settings_active = p_active;
    VSG::scene->portal_set_active(_portal_rid, p_active);
}

bool Portal::get_portal_active() const {
    return _settings_active;
}

void Portal::set_use_default_margin(bool p_use) {
    _use_default_margin = p_use;
    update_gizmo();
}

bool Portal::get_use_default_margin() const {
    return _use_default_margin;
}

void Portal::set_portal_margin(real_t p_margin) {
    _margin = p_margin;

    if (!_use_default_margin) {
        // give visual feedback in the editor for the portal margin zone
        update_gizmo();
    }
}

real_t Portal::get_portal_margin() const {
    return _margin;
}

void Portal::resolve_links(const Vector<Room *> &p_rooms, RenderingEntity p_from_room_rid) {
    Room *linkedroom = nullptr;
    if (has_node(_settings_path_linkedroom)) {
        linkedroom = object_cast<Room>(get_node(_settings_path_linkedroom));

        // only allow linking to rooms that are part of the roomlist
        // (already recognised).
        // If we don't check this, it will start trying to link to Room nodes that are invalid,
        // and crash.
        if (linkedroom && (p_rooms.find(linkedroom) == p_rooms.end())) {
            // invalid room
            WARN_PRINT("Portal attempting to link to Room outside the roomlist : " + linkedroom->get_name());
            linkedroom = nullptr;
        }

        // this should not happen, but just in case
        if (linkedroom && (linkedroom->_room_ID >= p_rooms.size())) {
            WARN_PRINT("Portal attempting to link to invalid Room : " + linkedroom->get_name());
            linkedroom = nullptr;
        }
    }

    if (linkedroom) {
        _linkedroom_ID[1] = linkedroom->_room_ID;

        // send to visual server
        VSG::scene->portal_link(_portal_rid, p_from_room_rid, linkedroom->_room_rid, _settings_two_way);
    } else {
        _linkedroom_ID[1] = -1;
    }
}

void Portal::set_linked_room_internal(const NodePath &link_path) {
    _settings_path_linkedroom = link_path;
}

bool Portal::try_set_unique_name(const String &p_name) {
    SceneTree *scene_tree = get_tree();
    if (!scene_tree) {
        // should not happen in the editor
        return false;
    }

    Viewport *root = scene_tree->get_root();
    if (!root) {
        return false;
    }

    Node *found = root->find_node(p_name, true, false);

    // if the name does not already exist in the scene tree, we can use it
    if (!found) {
        set_name(p_name);
        return true;
    }

    // we are trying to set the same name this node already has...
    if (found == this) {
        // noop
        return true;
    }

    return false;
}

void Portal::set_linked_room(const NodePath &link_path) {
    _settings_path_linkedroom = link_path;

    // see if the link looks legit
    Room *linkedroom = nullptr;
    if (has_node(link_path)) {
        linkedroom = object_cast<Room>(get_node(link_path));

        if (linkedroom) {
            if (linkedroom != get_parent()) {
                // was ok
            } else {
                WARN_PRINT("Linked room cannot be the parent room of a portal.");
            }
        } else {
            WARN_PRINT("Linked room path is not a room.");
        }
    }

    _changed();
}

NodePath Portal::get_linked_room() const {
    return _settings_path_linkedroom;
}

void Portal::flip() {
    // flip portal
    Transform tr = get_transform();
    Basis flip_basis = Basis(Vector3(0, Math_PI, 0));
    tr.basis *= flip_basis;
    set_transform(tr);

    _pts_local.clear();
    _pts_world.clear();

    // flip the raw verts
    Vector<Vector2> raw;
    raw.resize(_pts_local_raw.size());
    for (int n = 0; n < _pts_local_raw.size(); n++) {
        const Vector2 &pt = _pts_local_raw[n];
        raw[n] = Vector2(-pt.x, pt.y);
    }

    // standardize raw verts winding
    Geometry::sort_polygon_winding(raw, false);

    for (int n = 0; n < raw.size(); n++) {
        _pts_local_raw.set(n, raw[n]);
    }

    _sanitize_points();
    portal_update();

    update_gizmo();
}

bool Portal::create_from_mesh_instance(const MeshInstance3D *p_mi) {
    ERR_FAIL_COND_V(!p_mi, false);

    _pts_local.clear();
    _pts_world.clear();

    Ref<Mesh> rmesh = p_mi->get_mesh();
    ERR_FAIL_COND_V(!rmesh, false);

    if (rmesh->get_surface_count() == 0) {
        WARN_PRINT(FormatVE("Portal '%s' has no surfaces, ignoring", get_name().asCString()));
        return false;
    }

    SurfaceArrays arrays = rmesh->surface_get_arrays(0);
    Span<const Vector3> vertices = arrays.positions3();
    const Vector<int> &indices = arrays.m_indices;

    // get the model space verts and find center
    int num_source_points = vertices.size();
    ERR_FAIL_COND_V(num_source_points < 3, false);

    const Transform &tr_source = p_mi->get_global_transform();

    Vector<Vector3> pts_world;

    for (int n = 0; n < num_source_points; n++) {
        Vector3 pt = tr_source.xform(vertices[n]);

        // test for duplicates.
        // Some geometry may contain duplicate verts in portals
        // which will muck up the winding etc...
        bool duplicate = false;

        for (int m = 0; m < pts_world.size(); m++) {
            Vector3 diff = pt - pts_world[m];
            // hopefully this epsilon will do in nearly all cases
            if (diff.length() < 0.001) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            pts_world.push_back(pt);
        }
    }

    ERR_FAIL_COND_V(pts_world.size() < 3, false);

    // create the normal from 3 vertices .. either indexed, or use the first 3
    Vector3 three_pts[3];
    if (indices.size() >= 3) {
        for (int n = 0; n < 3; n++) {
            ERR_FAIL_COND_V(indices[n] >= num_source_points, false);
            three_pts[n] = tr_source.xform(vertices[indices[n]]);
        }
    } else {
        for (int n = 0; n < 3; n++) {
            three_pts[n] = pts_world[n];
        }
    }
    Vector3 normal = Plane(three_pts[0], three_pts[1], three_pts[2]).normal;
    if (_portal_plane_convention) {
        normal = -normal;
    }

    // get the verts sorted with winding, assume that the triangle initial winding
    // tells us the normal and hence which way the world space portal should be facing
    _sort_verts_clockwise(normal, pts_world);

    // back calculate the plane from *all* the portal points, this will give us a nice average plane
    // (in case of wonky portals where artwork isn't bang on)
    _plane = _plane_from_points_newell(pts_world);

    // change the portal transform to match our plane and the center of the portal
    Transform tr_global;

    // prevent warnings when poly normal matches the up vector
    Vector3 up(0, 1, 0);
    if (Math::abs(_plane.normal.dot(up)) > 0.9) {
        up = Vector3(1, 0, 0);
    }

    tr_global.set_look_at(Vector3(0, 0, 0), _plane.normal, up);
    tr_global.origin = _pt_center_world;

    // We can't directly set this global transform on the portal, because the parent node may already
    // have a transform applied, so we need to account for this and give a corrected local transform
    // for the portal, such that the end result global transform will be correct.

    // find the difference between this new global transform and the transform of the parent
    // then use this for the new local transform of the portal
    Node3D *parent = object_cast<Node3D>(get_parent());
    ERR_FAIL_COND_V(!parent, false);

    Transform tr_inverse_parent = parent->get_global_transform().affine_inverse();
    Transform new_local_transform = tr_inverse_parent * tr_global;
    set_transform(new_local_transform);

    // now back calculate the local space coords of the portal from the world space coords.
    // The local space will be used in future for editing and as a 'master' store of the verts.
    _pts_local_raw.resize(pts_world.size());

    // back transform from global space to local space
    Transform tr = tr_global.affine_inverse();

    for (int n = 0; n < pts_world.size(); n++) {
        // pt3 is now in local space
        Vector3 pt3 = tr.xform(pts_world[n]);

        // only the x and y required
        _pts_local_raw.set(n, Vector2(pt3.x, pt3.y));

        // The z coordinate should be approx zero
        // DEV_ASSERT(Math::abs(pt3.z) < 0.1);
    }

    _sanitize_points();
    portal_update();

    return true;
}

void Portal::_update_aabb() {
    _aabb_local = AABB();

    if (_pts_local.size()) {
        Vector3 begin = _vec2to3(_pts_local[0]);
        Vector3 end = begin;

        for (int n = 1; n < _pts_local.size(); n++) {
            Vector3 pt = _vec2to3(_pts_local[n]);

            if (pt.x < begin.x) {
                begin.x = pt.x;
            }
            if (pt.y < begin.y) {
                begin.y = pt.y;
            }
            if (pt.z < begin.z) {
                begin.z = pt.z;
            }

            if (pt.x > end.x) {
                end.x = pt.x;
            }
            if (pt.y > end.y) {
                end.y = pt.y;
            }
            if (pt.z > end.z) {
                end.z = pt.z;
            }
        }

        _aabb_local.position = begin;
        _aabb_local.size = end - begin;
    }
}

void Portal::portal_update() {
    // first calculate the plane from the transform
    // (portals are standardized outward from source room once sanitized,
    // irrespective of the user portal plane convention)
    const Transform &tr = get_global_transform();
    _plane = Plane(0.0, 0.0, -1.0, 0.0);
    _plane = tr.xform(_plane);

    // after becoming a portal, the centre world IS the transform origin
    _pt_center_world = tr.origin;

    // recalculates world points from the local space
    int num_points = _pts_local.size();
    if (_pts_world.size() != num_points) {
        _pts_world.resize(num_points);
    }

    for (int n = 0; n < num_points; n++) {
        _pts_world[n] = tr.xform(_vec2to3(_pts_local[n]));
    }

    // no need to check winding order, the points are pre-sanitized only when they change

    // extension margin to prevent objects too easily sprawling
    real_t margin = get_active_portal_margin();
    VSG::scene->portal_set_geometry(_portal_rid, _pts_world, margin);
}

real_t Portal::get_active_portal_margin() const {
    if (_use_default_margin) {
        return RoomManager::_get_default_portal_margin();
    }
    return _margin;
}

void Portal::_sanitize_points() {
    // remove duplicates? NYI maybe not necessary
    Vector<Vector2> raw;
    raw.resize(_pts_local_raw.size());
    for (int n = 0; n < _pts_local_raw.size(); n++) {
        raw[n]=_pts_local_raw[n];
    }

    // this function may get rid of some concave points due to user editing ..
    // may not be necessary, no idea how fast it is
    _pts_local = Geometry::convex_hull_2d(raw);

    // some peculiarity of convex_hull_2d function, it duplicates the last point for some reason
    if (_pts_local.size() > 1) {
        _pts_local.resize(_pts_local.size() - 1);
    }

    // sort winding, the system expects counter clockwise polys
    Geometry::sort_polygon_winding(_pts_local, false);

    // a bit of a bodge, but a small epsilon pulling in the portal edges towards the center
    // can hide walls in the opposite room that abutt the portal (due to floating point error)
    // find 2d center
    Vector2 center;
    for (int n = 0; n < _pts_local.size(); n++) {
        center += _pts_local[n];
    }
    center /= _pts_local.size();

    const real_t pull_in = 0.0001;

    for (int n = 0; n < _pts_local.size(); n++) {
        Vector2 offset = _pts_local[n] - center;
        real_t l = offset.length();

        // don't apply the pull in for tiny holes
        if (l > (pull_in * 2.0f)) {
            real_t fract = (l - pull_in) / l;
            offset *= fract;
            _pts_local[n] = center + offset;
        }
    }

    _update_aabb();
}

void Portal::_sort_verts_clockwise(const Vector3 &p_portal_normal, Vector<Vector3> &r_verts) {
    // cannot sort less than 3 verts
    if (r_verts.size() < 3) {
        return;
    }

    // find centroid
    int num_points = r_verts.size();
    _pt_center_world = Vector3(0, 0, 0);

    for (int n = 0; n < num_points; n++) {
        _pt_center_world += r_verts[n];
    }
    _pt_center_world /= num_points;
    /////////////////////////////////////////

    // now algorithm
    for (int n = 0; n < num_points - 2; n++) {
        Vector3 a = r_verts[n] - _pt_center_world;
        a.normalize();

        Plane p = Plane(r_verts[n], _pt_center_world, _pt_center_world + p_portal_normal);

        double smallest_angle = -1;
        int smallest = -1;

        for (int m = n + 1; m < num_points; m++) {
            if (p.distance_to(r_verts[m]) > 0.0) {
                Vector3 b = r_verts[m] - _pt_center_world;
                b.normalize();

                double angle = a.dot(b);

                if (angle > smallest_angle) {
                    smallest_angle = angle;
                    smallest = m;
                }
            } // which side

        } // for m

        // swap smallest and n+1 vert
        if (smallest != -1) {
            eastl::swap(r_verts[smallest],r_verts[n + 1]);
        }
    } // for n

    // the vertices are now sorted, but may be in the opposite order to that wanted.
    // we detect this by calculating the normal of the poly, then flipping the order if the normal is pointing
    // the wrong way.
    Plane plane = Plane(r_verts[0], r_verts[1], r_verts[2]);

    if (p_portal_normal.dot(plane.normal) < 0.0) {
        // reverse winding order of verts
        eastl::reverse(r_verts.begin(),r_verts.end());
    }
}

Plane Portal::_plane_from_points_newell(const Vector<Vector3> &p_pts) {
    int num_points = p_pts.size();

    if (num_points < 3) {
        return Plane();
    }

    Vector3 normal;
    Vector3 center;

    for (int i = 0; i < num_points; i++) {
        int j = (i + 1) % num_points;

        const Vector3 &pi = p_pts[i];
        const Vector3 &pj = p_pts[j];

        center += pi;

        normal.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
        normal.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
        normal.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
    }

    normal.normalize();
    center /= num_points;

    _pt_center_world = center;

    // point and normal
    return Plane(center, normal);
}

void Portal::_bind_methods() {

    SE_BIND_METHOD(Portal,set_portal_active);
    SE_BIND_METHOD(Portal,get_portal_active);

    SE_BIND_METHOD(Portal,set_two_way);
    SE_BIND_METHOD(Portal,is_two_way);

    SE_BIND_METHOD(Portal,set_use_default_margin);
    SE_BIND_METHOD(Portal,get_use_default_margin);

    SE_BIND_METHOD(Portal,set_portal_margin);
    SE_BIND_METHOD(Portal,get_portal_margin);

    SE_BIND_METHOD(Portal,set_linked_room);
    SE_BIND_METHOD(Portal,get_linked_room);

    SE_BIND_METHOD(Portal,set_points);
    SE_BIND_METHOD(Portal,get_points);

    SE_BIND_METHOD(Portal,set_point);

    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "portal_active"), "set_portal_active", "get_portal_active");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "two_way"), "set_two_way", "is_two_way");
    ADD_PROPERTY(PropertyInfo(VariantType::NODE_PATH, "linked_room", PropertyHint::NodePathValidTypes, "Room"), "set_linked_room", "get_linked_room");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "use_default_margin"), "set_use_default_margin", "get_use_default_margin");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "portal_margin", PropertyHint::Range, "0.0,10.0,0.01"), "set_portal_margin", "get_portal_margin");
    ADD_PROPERTY(PropertyInfo(VariantType::POOL_VECTOR2_ARRAY, "points"), "set_points", "get_points");
}
