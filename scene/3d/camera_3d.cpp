/*************************************************************************/
/*  camera.cpp                                                           */
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

#include "camera_3d.h"

#include "core/engine.h"
#include "core/math/camera_matrix.h"
#include "core/method_bind.h"
#include "core/object_tooling.h"
#include "scene/3d/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_3d.h"
#include "servers/physics_server_3d.h"
#include "servers/rendering_server.h"

IMPL_GDCLASS(Camera3D)
IMPL_GDCLASS(ClippedCamera3D)

VARIANT_ENUM_CAST(Camera3D::Projection);
VARIANT_ENUM_CAST(Camera3D::KeepAspect);
VARIANT_ENUM_CAST(Camera3D::DopplerTracking);
VARIANT_ENUM_CAST(ClippedCamera3D::ProcessMode);

void Camera3D::_update_audio_listener_state() {
}

void Camera3D::_request_camera_update() {

    _update_camera();
}

void Camera3D::_update_camera_mode() {

    force_change = true;
    switch (mode) {
        case PROJECTION_PERSPECTIVE: {

            set_perspective(fov, near, far);

        } break;
        case PROJECTION_ORTHOGONAL: {
            set_orthogonal(size, near, far);
        } break;
        case PROJECTION_FRUSTUM: {
            set_frustum(size, frustum_offset, near, far);
        } break;
    }
}

void Camera3D::_validate_property(PropertyInfo &p_property) const {
    if (p_property.name == "fov") {
        if (mode != PROJECTION_PERSPECTIVE) {
            p_property.usage = PROPERTY_USAGE_NOEDITOR;
        }
    } else if (p_property.name == "size") {
        if (mode != PROJECTION_ORTHOGONAL && mode != PROJECTION_FRUSTUM) {
            p_property.usage = PROPERTY_USAGE_NOEDITOR;
        }
    } else if (p_property.name == "frustum_offset") {
        if (mode != PROJECTION_FRUSTUM) {
            p_property.usage = PROPERTY_USAGE_NOEDITOR;
        }
    }
}

void Camera3D::_update_camera() {

    if (!is_inside_tree())
        return;

    RenderingServer::get_singleton()->camera_set_transform(camera, get_camera_transform());

    // here goes listener stuff
    /*
    if (viewport_ptr && is_inside_scene() && is_current())
        get_viewport()->_camera_transform_changed_notify();
    */

    if (get_tree()->is_node_being_edited(this) || !is_current())
        return;

    get_viewport()->_camera_transform_changed_notify();

    if (get_world_3d()) {
        get_world_3d()->_update_camera(this);
    }
}

void Camera3D::_notification(int p_what) {

    switch (p_what) {

        case NOTIFICATION_ENTER_WORLD: {

            // Needs to track the Viewport  because it's needed on NOTIFICATION_EXIT_WORLD
            // and Node3D will handle it first, including clearing its reference to the Viewport,
            // therefore making it impossible to subclasses to access it
            viewport = get_viewport();
            ERR_FAIL_COND(!viewport);

            bool first_camera = viewport->_camera_add(this);
            if (current || first_camera)
                viewport->_camera_set(this);

        } break;
        case NOTIFICATION_TRANSFORM_CHANGED: {

            _request_camera_update();
            if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
                velocity_tracker->update_position(get_global_transform().origin);
            }
        } break;
        case NOTIFICATION_EXIT_WORLD: {

            if (!get_tree()->is_node_being_edited(this)) {
                if (is_current()) {
                    clear_current();
                    current = true; //keep it true

                } else {
                    current = false;
                }
            }

            if (viewport) {
                viewport->_camera_remove(this);
                viewport = nullptr;
            }

        } break;
        case NOTIFICATION_BECAME_CURRENT: {
            if (viewport) {
                viewport->find_world_3d()->_register_camera(this);
            }
        } break;
        case NOTIFICATION_LOST_CURRENT: {
            if (viewport) {
                viewport->find_world_3d()->_remove_camera(this);
            }
        } break;
    }
}

Transform Camera3D::get_camera_transform() const {

    Transform tr = get_global_transform().orthonormalized();
    tr.origin += tr.basis.get_axis(1) * v_offset;
    tr.origin += tr.basis.get_axis(0) * h_offset;
    return tr;
}

void Camera3D::set_perspective(float p_fovy_degrees, float p_z_near, float p_z_far) {

    if (!force_change && fov == p_fovy_degrees && p_z_near == near && p_z_far == far && mode == PROJECTION_PERSPECTIVE)
        return;

    fov = p_fovy_degrees;
    near = p_z_near;
    far = p_z_far;
    mode = PROJECTION_PERSPECTIVE;

    RenderingServer::get_singleton()->camera_set_perspective(camera, fov, near, far);
    update_gizmo();
    force_change = false;
}
void Camera3D::set_orthogonal(float p_size, float p_z_near, float p_z_far) {

    if (!force_change && size == p_size && p_z_near == near && p_z_far == far && mode == PROJECTION_ORTHOGONAL)
        return;

    size = p_size;

    near = p_z_near;
    far = p_z_far;
    mode = PROJECTION_ORTHOGONAL;
    force_change = false;

    RenderingServer::get_singleton()->camera_set_orthogonal(camera, size, near, far);
    update_gizmo();
}

void Camera3D::set_frustum(float p_size, Vector2 p_offset, float p_z_near, float p_z_far) {
    if (!force_change && size == p_size && frustum_offset == p_offset && p_z_near == near && p_z_far == far && mode == PROJECTION_FRUSTUM)
        return;

    size = p_size;
    frustum_offset = p_offset;

    near = p_z_near;
    far = p_z_far;
    mode = PROJECTION_FRUSTUM;
    force_change = false;

    RenderingServer::get_singleton()->camera_set_frustum(camera, size, frustum_offset, near, far);
    update_gizmo();
}

void Camera3D::set_projection(Camera3D::Projection p_mode) {
    if (p_mode == PROJECTION_PERSPECTIVE || p_mode == PROJECTION_ORTHOGONAL || p_mode == PROJECTION_FRUSTUM) {
        mode = p_mode;
        _update_camera_mode();
        Object_change_notify(this);
    }
}

RenderingEntity Camera3D::get_camera_rid() const {

    return camera;
}

void Camera3D::make_current() {

    current = true;

    if (!is_inside_tree())
        return;

    get_viewport()->_camera_set(this);

    //get_scene()->call_group(SceneMainLoop::GROUP_CALL_REALTIME,camera_group,"_camera_make_current",this);
}

void Camera3D::clear_current(bool p_enable_next) {

    current = false;
    if (!is_inside_tree())
        return;

    if (get_viewport()->get_camera() == this) {
        get_viewport()->_camera_set(nullptr);

        if (p_enable_next) {
            get_viewport()->_camera_make_next_current(this);
        }
    }
}

void Camera3D::set_current(bool p_current) {
    if (p_current) {
        make_current();
    } else {
        clear_current();
    }
}

bool Camera3D::is_current() const {

    if (is_inside_tree() && !get_tree()->is_node_being_edited(this)) {

        return get_viewport()->get_camera() == this;
    } else
        return current;
}


Vector3 Camera3D::project_ray_normal(const Point2 &p_pos) const {

    Vector3 ray = project_local_ray_normal(p_pos);
    return get_camera_transform().basis.xform(ray).normalized();
}

Vector3 Camera3D::project_local_ray_normal(const Point2 &p_pos) const {

    ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera3D is not inside scene.");

    Size2 viewport_size = get_viewport()->get_camera_rect_size();
    Vector2 cpos = get_viewport()->get_camera_coords(p_pos);
    Vector3 ray;

    if (mode == PROJECTION_ORTHOGONAL) {

        ray = Vector3(0, 0, -1);
    } else {
        CameraMatrix cm;
        cm.set_perspective(fov, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);

        Vector2 screen_he = cm.get_viewport_half_extents();
        ray = Vector3(((cpos.x / viewport_size.width) * 2.0f - 1.0f) * screen_he.x,
                ((1.0f - (cpos.y / viewport_size.height)) * 2.0f - 1.0f) * screen_he.y, -near)
                      .normalized();
    }

    return ray;
}

Vector3 Camera3D::project_ray_origin(const Point2 &p_pos) const {

    ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera3D is not inside scene.");

    Size2 viewport_size = get_viewport()->get_camera_rect_size();
    Vector2 cpos = get_viewport()->get_camera_coords(p_pos);
    ERR_FAIL_COND_V(viewport_size.y == 0, Vector3());

    if (mode == PROJECTION_PERSPECTIVE) {

        return get_camera_transform().origin;
    }
    Vector2 pos = cpos / viewport_size;
    float vsize, hsize;
    if (keep_aspect == KEEP_WIDTH) {
        vsize = size / viewport_size.aspect();
        hsize = size;
    } else {
        hsize = size * viewport_size.aspect();
        vsize = size;
    }

    Vector3 ray;
    ray.x = pos.x * (hsize)-hsize / 2;
    ray.y = (1.0 - pos.y) * (vsize)-vsize / 2;
    ray.z = -near;
    ray = get_camera_transform().xform(ray);
    return ray;
}

bool Camera3D::is_position_behind(const Vector3 &p_pos) const {

    Transform t = get_global_transform();
    Vector3 eyedir = -t.basis.get_axis(2).normalized();
    return eyedir.dot(p_pos - t.origin) < near;
}

Vector<Vector3> Camera3D::get_near_plane_points() const {
    ERR_FAIL_COND_V_MSG(!is_inside_tree(),{}, "Camera3D is not inside scene.");

    Size2 viewport_size = get_viewport()->get_visible_rect().size;

    CameraMatrix cm;

    if (mode == PROJECTION_ORTHOGONAL)
        cm.set_orthogonal(size, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);
    else
        cm.set_perspective(fov, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);

    Vector3 endpoints[8];
    cm.get_endpoints(Transform(), endpoints);

    Vector<Vector3> points = { Vector3(0,0,0) };

    for (int i = 0; i < 4; i++) {
        points.push_back(endpoints[i + 4]);
    }
    return points;
}

Point2 Camera3D::unproject_position(const Vector3 &p_pos) const {

    ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector2(), "Camera3D is not inside scene.");

    Size2 viewport_size = get_viewport()->get_visible_rect().size;

    CameraMatrix cm;

    if (mode == PROJECTION_ORTHOGONAL)
        cm.set_orthogonal(size, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);
    else
        cm.set_perspective(fov, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);

    Plane p(get_camera_transform().xform_inv(p_pos), 1.0);

    p = cm.xform4(p);
    p.normal /= p.d;

    Point2 res;
    res.x = (p.normal.x * 0.5f + 0.5f) * viewport_size.x;
    res.y = (-p.normal.y * 0.5f + 0.5f) * viewport_size.y;

    return res;
}

Vector3 Camera3D::project_position(const Point2 &p_point, float p_z_depth) const {

    ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera3D is not inside scene.");

    if (p_z_depth == 0.0f && mode != PROJECTION_ORTHOGONAL) {
        return get_global_transform().origin;
    }

    Size2 viewport_size = get_viewport()->get_visible_rect().size;

    CameraMatrix cm;

    if (mode == PROJECTION_ORTHOGONAL)
        cm.set_orthogonal(size, viewport_size.aspect(), p_z_depth, far, keep_aspect == KEEP_WIDTH);
    else
        cm.set_perspective(fov, viewport_size.aspect(), p_z_depth, far, keep_aspect == KEEP_WIDTH);

    Vector2 vp_he = cm.get_viewport_half_extents();

    Vector2 point;
    point.x = (p_point.x / viewport_size.x) * 2.0f - 1.0f;
    point.y = (1.0 - (p_point.y / viewport_size.y)) * 2.0f - 1.0f;
    point *= vp_he;

    Vector3 p(point.x, point.y, -p_z_depth);

    return get_camera_transform().xform(p);
}

/*
void Camera3D::_camera_make_current(Node *p_camera) {


    if (p_camera==this) {
        RenderingServer::get_singleton()->viewport_attach_camera(viewport_id,camera);
        active=true;
    } else {
        if (active && p_camera==NULL) {
            //detech camera because no one else will claim it
            RenderingServer::get_singleton()->viewport_attach_camera(viewport_id,RID());
        }
        active=false;
    }
}
*/

void Camera3D::set_environment(const Ref<Environment> &p_environment) {

    environment = p_environment;
    if (environment)
        RenderingServer::get_singleton()->camera_set_environment(camera, environment->get_rid());
    else
        RenderingServer::get_singleton()->camera_set_environment(camera, entt::null);
    _update_camera_mode();
}

Ref<Environment> Camera3D::get_environment() const {

    return environment;
}

void Camera3D::set_keep_aspect_mode(KeepAspect p_aspect) {
    keep_aspect = p_aspect;
    RenderingServer::get_singleton()->camera_set_use_vertical_aspect(camera, p_aspect == KEEP_WIDTH);
    _update_camera_mode();
    Object_change_notify(this);
}

Camera3D::KeepAspect Camera3D::get_keep_aspect_mode() const {

    return keep_aspect;
}

void Camera3D::set_doppler_tracking(DopplerTracking p_tracking) {

    if (doppler_tracking == p_tracking)
        return;

    doppler_tracking = p_tracking;
    if (p_tracking != DOPPLER_TRACKING_DISABLED) {
        velocity_tracker->set_track_physics_step(doppler_tracking == DOPPLER_TRACKING_PHYSICS_STEP);
        if (is_inside_tree()) {
            velocity_tracker->reset(get_global_transform().origin);
        }
    }
    _update_camera_mode();
}

Camera3D::DopplerTracking Camera3D::get_doppler_tracking() const {
    return doppler_tracking;
}

void Camera3D::_bind_methods() {

    SE_BIND_METHOD(Camera3D,project_ray_normal);
    SE_BIND_METHOD(Camera3D,project_local_ray_normal);
    SE_BIND_METHOD(Camera3D,project_ray_origin);
    SE_BIND_METHOD(Camera3D,unproject_position);
    SE_BIND_METHOD(Camera3D,is_position_behind);
    SE_BIND_METHOD(Camera3D,project_position);
    SE_BIND_METHOD(Camera3D,set_perspective);
    SE_BIND_METHOD(Camera3D,set_orthogonal);
    SE_BIND_METHOD(Camera3D,set_frustum);
    SE_BIND_METHOD(Camera3D,make_current);
    MethodBinder::bind_method(D_METHOD("clear_current", {"enable_next"}), &Camera3D::clear_current, {DEFVAL(true)});
    SE_BIND_METHOD(Camera3D,set_current);
    SE_BIND_METHOD(Camera3D,is_current);
    SE_BIND_METHOD(Camera3D,get_camera_transform);
    SE_BIND_METHOD(Camera3D,get_fov);
    SE_BIND_METHOD(Camera3D,get_frustum_offset);
    SE_BIND_METHOD(Camera3D,get_size);
    SE_BIND_METHOD(Camera3D,get_zfar);
    SE_BIND_METHOD(Camera3D,get_znear);
    SE_BIND_METHOD(Camera3D,set_fov);
    SE_BIND_METHOD(Camera3D,set_frustum_offset);
    SE_BIND_METHOD(Camera3D,set_size);
    SE_BIND_METHOD(Camera3D,set_zfar);
    SE_BIND_METHOD(Camera3D,set_znear);
    SE_BIND_METHOD(Camera3D,get_projection);
    SE_BIND_METHOD(Camera3D,set_projection);
    SE_BIND_METHOD(Camera3D,set_h_offset);
    SE_BIND_METHOD(Camera3D,get_h_offset);
    SE_BIND_METHOD(Camera3D,set_v_offset);
    SE_BIND_METHOD(Camera3D,get_v_offset);
    SE_BIND_METHOD(Camera3D,set_cull_mask);
    SE_BIND_METHOD(Camera3D,get_cull_mask);
    SE_BIND_METHOD(Camera3D,set_environment);
    SE_BIND_METHOD(Camera3D,get_environment);
    SE_BIND_METHOD(Camera3D,set_keep_aspect_mode);
    SE_BIND_METHOD(Camera3D,get_keep_aspect_mode);
    SE_BIND_METHOD(Camera3D,set_doppler_tracking);
    SE_BIND_METHOD(Camera3D,get_doppler_tracking);
    SE_BIND_METHOD(Camera3D,get_frustum);
    SE_BIND_METHOD(Camera3D,get_camera_rid);

    SE_BIND_METHOD(Camera3D,set_cull_mask_bit);
    SE_BIND_METHOD(Camera3D,get_cull_mask_bit);

    //MethodBinder::bind_method(D_METHOD("_camera_make_current"),&Camera3D::_camera_make_current );

    ADD_PROPERTY(PropertyInfo(VariantType::INT, "keep_aspect", PropertyHint::Enum, "Keep Width,Keep Height"), "set_keep_aspect_mode", "get_keep_aspect_mode");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "cull_mask", PropertyHint::Layers3DRenderer), "set_cull_mask", "get_cull_mask");
    ADD_PROPERTY(PropertyInfo(VariantType::OBJECT, "environment", PropertyHint::ResourceType, "Environment"), "set_environment", "get_environment");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "h_offset"), "set_h_offset", "get_h_offset");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "v_offset"), "set_v_offset", "get_v_offset");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "doppler_tracking", PropertyHint::Enum, "Disabled,Idle,Physics"), "set_doppler_tracking", "get_doppler_tracking");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "projection", PropertyHint::Enum, "Perspective,Orthogonal,Frustum"), "set_projection", "get_projection");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "current"), "set_current", "is_current");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "fov", PropertyHint::Range, "1,179,0.1"), "set_fov", "get_fov");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "size", PropertyHint::Range, "0.001,16384,0.001"), "set_size", "get_size");
    ADD_PROPERTY(PropertyInfo(VariantType::VECTOR2, "frustum_offset"), "set_frustum_offset", "get_frustum_offset");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "near", PropertyHint::ExpRange, "0.01,8192,0.01,or_greater"), "set_znear", "get_znear");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "far", PropertyHint::ExpRange, "0.1,8192,0.1,or_greater"), "set_zfar", "get_zfar");

    BIND_ENUM_CONSTANT(PROJECTION_PERSPECTIVE);
    BIND_ENUM_CONSTANT(PROJECTION_ORTHOGONAL);
    BIND_ENUM_CONSTANT(PROJECTION_FRUSTUM);

    BIND_ENUM_CONSTANT(KEEP_WIDTH);
    BIND_ENUM_CONSTANT(KEEP_HEIGHT);

    BIND_ENUM_CONSTANT(DOPPLER_TRACKING_DISABLED);
    BIND_ENUM_CONSTANT(DOPPLER_TRACKING_IDLE_STEP);
    BIND_ENUM_CONSTANT(DOPPLER_TRACKING_PHYSICS_STEP);
}

float Camera3D::get_fov() const {

    return fov;
}

float Camera3D::get_size() const {

    return size;
}

float Camera3D::get_znear() const {

    return near;
}

Vector2 Camera3D::get_frustum_offset() const {
    return frustum_offset;
}

float Camera3D::get_zfar() const {

    return far;
}

Camera3D::Projection Camera3D::get_projection() const {

    return mode;
}

void Camera3D::set_fov(float p_fov) {
    ERR_FAIL_COND(p_fov < 1 || p_fov > 179);

    fov = p_fov;
    _update_camera_mode();
    Object_change_notify(this,"fov");
}

void Camera3D::set_size(float p_size) {
    ERR_FAIL_COND(p_size < 0.1f || p_size > 16384);

    size = p_size;
    _update_camera_mode();
    Object_change_notify(this,"size");
}

void Camera3D::set_znear(float p_znear) {
    near = p_znear;
    _update_camera_mode();
}

void Camera3D::set_frustum_offset(Vector2 p_offset) {
    frustum_offset = p_offset;
    _update_camera_mode();
}

void Camera3D::set_zfar(float p_zfar) {
    far = p_zfar;
    _update_camera_mode();
}

void Camera3D::set_cull_mask(uint32_t p_layers) {
    layers = p_layers;
    RenderingServer::get_singleton()->camera_set_cull_mask(camera, layers);
    _update_camera_mode();
}

uint32_t Camera3D::get_cull_mask() const {

    return layers;
}

void Camera3D::set_cull_mask_bit(int p_layer, bool p_enable) {
    ERR_FAIL_INDEX(p_layer, 32);
    if (p_enable) {
        set_cull_mask(layers | (1 << p_layer));
    } else {
        set_cull_mask(layers & (~(1 << p_layer)));
    }
}

bool Camera3D::get_cull_mask_bit(int p_layer) const {
    ERR_FAIL_INDEX_V(p_layer, 32, false);
    return (layers & (1 << p_layer));
}

Frustum Camera3D::get_frustum() const {

    ERR_FAIL_COND_V(!is_inside_world(), Frustum());

    Size2 viewport_size = get_viewport()->get_visible_rect().size;
    CameraMatrix cm;
    if (mode == PROJECTION_PERSPECTIVE)
        cm.set_perspective(fov, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);
    else
        cm.set_orthogonal(size, viewport_size.aspect(), near, far, keep_aspect == KEEP_WIDTH);

    return cm.get_projection_planes(get_camera_transform());
}

void Camera3D::set_v_offset(float p_offset) {

    v_offset = p_offset;
    _update_camera();
}

float Camera3D::get_v_offset() const {

    return v_offset;
}

void Camera3D::set_h_offset(float p_offset) {
    h_offset = p_offset;
    _update_camera();
}

float Camera3D::get_h_offset() const {

    return h_offset;
}

Vector3 Camera3D::get_doppler_tracked_velocity() const {

    if (doppler_tracking == DOPPLER_TRACKING_DISABLED) {
        return Vector3();
    }
    return velocity_tracker->get_tracked_linear_velocity();
}
Camera3D::Camera3D() {

    camera = RenderingServer::get_singleton()->camera_create();
    frustum_offset = Vector2();
    set_perspective(70.0f, 0.05f, 100.0f);
    RenderingServer::get_singleton()->camera_set_cull_mask(camera, layers);
    velocity_tracker = make_ref_counted<VelocityTracker3D>();
    set_notify_transform(true);
    set_disable_scale(true);
}

Camera3D::~Camera3D() {
    RenderingServer::get_singleton()->free_rid(camera);
}

////////////////////////////////////////

void ClippedCamera3D::set_margin(float p_margin) {
    margin = p_margin;
}
float ClippedCamera3D::get_margin() const {
    return margin;
}
void ClippedCamera3D::set_process_mode(ProcessMode p_mode) {

    if (process_mode == p_mode) {
        return;
    }
    process_mode = p_mode;
    set_process_internal(process_mode == CLIP_PROCESS_IDLE);
    set_physics_process_internal(process_mode == CLIP_PROCESS_PHYSICS);
}
ClippedCamera3D::ProcessMode ClippedCamera3D::get_process_mode() const {
    return process_mode;
}

Transform ClippedCamera3D::get_camera_transform() const {

    Transform t = Camera3D::get_camera_transform();
    t.origin += -t.basis.get_axis(Vector3::AXIS_Z).normalized() * clip_offset;
    return t;
}

void ClippedCamera3D::_notification(int p_what) {
    if (p_what == NOTIFICATION_INTERNAL_PROCESS || p_what == NOTIFICATION_INTERNAL_PHYSICS_PROCESS) {

        Node3D *parent = object_cast<Node3D>(get_parent());
        if (!parent) {
            return;
        }

        PhysicsDirectSpaceState3D *dspace = get_world_3d()->get_direct_space_state();
        ERR_FAIL_COND(!dspace); // most likely physics set to threads

        Vector3 cam_fw = -get_global_transform().basis.get_axis(Vector3::AXIS_Z).normalized();
        Vector3 cam_pos = get_global_transform().origin;
        Vector3 parent_pos = parent->get_global_transform().origin;

        Plane parent_plane(parent_pos, cam_fw);

        if (parent_plane.is_point_over(cam_pos)) {
            //cam is beyond parent plane
            return;
        }

        Vector3 ray_from = parent_plane.project(cam_pos);

        clip_offset = 0; //reset by defau;t

        { //check if points changed
            Vector<Vector3> local_points = get_near_plane_points();

            bool all_equal = true;

            for (int i = 0; i < 5; i++) {
                if (points[i] != local_points[i]) {
                    all_equal = false;
                    break;
                }
            }

            if (!all_equal) {
                PhysicsServer3D::get_singleton()->shape_set_data(pyramid_shape, Variant::from(local_points));
                points = local_points;
            }
        }

        Transform xf = get_global_transform();
        xf.origin = ray_from;
        xf.orthonormalize();

        float csafe, cunsafe;
        if (dspace->cast_motion(pyramid_shape, xf, cam_pos - ray_from, margin, csafe, cunsafe, exclude, collision_mask, clip_to_bodies, clip_to_areas)) {
            clip_offset = cam_pos.distance_to(ray_from + (cam_pos - ray_from) * csafe);
        }

        _update_camera();
    }

    if (p_what == NOTIFICATION_LOCAL_TRANSFORM_CHANGED) {
        update_gizmo();
    }
}

void ClippedCamera3D::set_collision_mask(uint32_t p_mask) {

    collision_mask = p_mask;
}

uint32_t ClippedCamera3D::get_collision_mask() const {

    return collision_mask;
}

void ClippedCamera3D::set_collision_mask_bit(int p_bit, bool p_value) {

    ERR_FAIL_INDEX_MSG(p_bit, 32, "Collision layer bit must be between 0 and 31 inclusive.");
    uint32_t mask = get_collision_mask();
    if (p_value)
        mask |= 1 << p_bit;
    else
        mask &= ~(1 << p_bit);
    set_collision_mask(mask);
}

bool ClippedCamera3D::get_collision_mask_bit(int p_bit) const {

    ERR_FAIL_INDEX_V_MSG(p_bit, 32, false, "Collision layer bit must be between 0 and 31 inclusive.");
    return get_collision_mask() & (1 << p_bit);
}

void ClippedCamera3D::add_exception_rid(const RID &p_rid) {

    exclude.insert(p_rid);
}

void ClippedCamera3D::add_exception(const Object *p_object) {

    ERR_FAIL_NULL(p_object);
    const CollisionObject3D *co = object_cast<CollisionObject3D>(p_object);
    if (!co)
        return;
    add_exception_rid(co->get_rid());
}

void ClippedCamera3D::remove_exception_rid(const RID &p_rid) {

    exclude.erase(p_rid);
}

void ClippedCamera3D::remove_exception(const Object *p_object) {

    ERR_FAIL_NULL(p_object);
    const CollisionObject3D *co = object_cast<CollisionObject3D>(p_object);
    if (!co)
        return;
    remove_exception_rid(co->get_rid());
}

void ClippedCamera3D::clear_exceptions() {

    exclude.clear();
}

float ClippedCamera3D::get_clip_offset() const {

    return clip_offset;
}

void ClippedCamera3D::set_clip_to_areas(bool p_clip) {

    clip_to_areas = p_clip;
}

bool ClippedCamera3D::is_clip_to_areas_enabled() const {

    return clip_to_areas;
}

void ClippedCamera3D::set_clip_to_bodies(bool p_clip) {

    clip_to_bodies = p_clip;
}

bool ClippedCamera3D::is_clip_to_bodies_enabled() const {

    return clip_to_bodies;
}

void ClippedCamera3D::_bind_methods() {

    SE_BIND_METHOD(ClippedCamera3D,set_margin);
    SE_BIND_METHOD(ClippedCamera3D,get_margin);

    SE_BIND_METHOD(ClippedCamera3D,set_process_mode);
    SE_BIND_METHOD(ClippedCamera3D,get_process_mode);

    SE_BIND_METHOD(ClippedCamera3D,set_collision_mask);
    SE_BIND_METHOD(ClippedCamera3D,get_collision_mask);

    SE_BIND_METHOD(ClippedCamera3D,set_collision_mask_bit);
    SE_BIND_METHOD(ClippedCamera3D,get_collision_mask_bit);

    SE_BIND_METHOD(ClippedCamera3D,add_exception_rid);
    SE_BIND_METHOD(ClippedCamera3D,add_exception);

    SE_BIND_METHOD(ClippedCamera3D,remove_exception_rid);
    SE_BIND_METHOD(ClippedCamera3D,remove_exception);

    SE_BIND_METHOD(ClippedCamera3D,set_clip_to_areas);
    SE_BIND_METHOD(ClippedCamera3D,is_clip_to_areas_enabled);

    SE_BIND_METHOD(ClippedCamera3D,get_clip_offset);

    SE_BIND_METHOD(ClippedCamera3D,set_clip_to_bodies);
    SE_BIND_METHOD(ClippedCamera3D,is_clip_to_bodies_enabled);

    SE_BIND_METHOD(ClippedCamera3D,clear_exceptions);

    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "margin", PropertyHint::Range, "0,32,0.01"), "set_margin", "get_margin");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "process_mode", PropertyHint::Enum, "Physics,Idle"), "set_process_mode", "get_process_mode");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "collision_mask", PropertyHint::Layers3DPhysics), "set_collision_mask", "get_collision_mask");

    ADD_GROUP("Clip To", "clip_to");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "clip_to_areas", PropertyHint::Layers3DPhysics), "set_clip_to_areas", "is_clip_to_areas_enabled");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "clip_to_bodies", PropertyHint::Layers3DPhysics), "set_clip_to_bodies", "is_clip_to_bodies_enabled");

    BIND_ENUM_CONSTANT(CLIP_PROCESS_PHYSICS);
    BIND_ENUM_CONSTANT(CLIP_PROCESS_IDLE);
}
ClippedCamera3D::ClippedCamera3D() {
    margin = 0;
    clip_offset = 0;
    process_mode = CLIP_PROCESS_PHYSICS;
    set_physics_process_internal(true);
    collision_mask = 1;
    set_notify_local_transform(Engine::get_singleton()->is_editor_hint());
    points.resize(5);
    pyramid_shape = PhysicsServer3D::get_singleton()->shape_create(PhysicsServer3D::SHAPE_CONVEX_POLYGON);
    clip_to_areas = false;
    clip_to_bodies = true;
}
ClippedCamera3D::~ClippedCamera3D() {
    PhysicsServer3D::get_singleton()->free_rid(pyramid_shape);
}
