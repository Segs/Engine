/*************************************************************************/
/*  camera_3d.h                                                             */
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

#include "scene/3d/node_3d.h"
#include "scene/3d/velocity_tracker_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/environment.h"

struct Frustum;

class GODOT_EXPORT Camera3D : public Node3D {

    GDCLASS(Camera3D,Node3D)

public:
    enum Projection {

        PROJECTION_PERSPECTIVE,
        PROJECTION_ORTHOGONAL,
        PROJECTION_FRUSTUM
    };

    enum KeepAspect {
        KEEP_WIDTH,
        KEEP_HEIGHT
    };

    enum DopplerTracking {
        DOPPLER_TRACKING_DISABLED,
        DOPPLER_TRACKING_IDLE_STEP,
        DOPPLER_TRACKING_PHYSICS_STEP
    };

private:
    friend class Viewport;

    bool force_change = false;
    bool current = false;
    Viewport *viewport = nullptr;

    Projection mode = PROJECTION_PERSPECTIVE;

    float fov = 0.0f;
    float size = 1.0f;
    Vector2 frustum_offset;
    float near = 0.0f;
    float far = 0.0f;
    float v_offset = 0.0f;
    float h_offset = 0.0f;
    KeepAspect keep_aspect = KEEP_HEIGHT;

    RenderingEntity camera;
    RenderingEntity scenario_id;

    //String camera_group;

    uint32_t layers = 0xfffff;

    Ref<Environment> environment;
    DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;
    Ref<VelocityTracker3D> velocity_tracker;

    void _update_audio_listener_state();


protected:
    void _update_camera();
    virtual void _request_camera_update();
    void _update_camera_mode();

    void _notification(int p_what);
    void _validate_property(PropertyInfo &p_property) const override;

    static void _bind_methods();

public:
    enum {

        NOTIFICATION_BECAME_CURRENT = 50,
        NOTIFICATION_LOST_CURRENT = 51
    };

    void set_perspective(float p_fovy_degrees, float p_z_near, float p_z_far);
    void set_orthogonal(float p_size, float p_z_near, float p_z_far);
    void set_frustum(float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
    void set_projection(Camera3D::Projection p_mode);

    void make_current();
    void clear_current(bool p_enable_next = true);
    void set_current(bool p_current);
    bool is_current() const;

    RenderingEntity get_camera_rid() const;

    float get_fov() const;
    float get_size() const;
    float get_zfar() const;
    float get_znear() const;
    Vector2 get_frustum_offset() const;

    Projection get_projection() const;

    void set_fov(float p_fov);
    void set_size(float p_size);
    void set_zfar(float p_zfar);
    void set_znear(float p_znear);
    void set_frustum_offset(Vector2 p_offset);

    virtual Transform get_camera_transform() const;

    virtual Vector3 project_ray_normal(const Point2 &p_pos) const;
    virtual Vector3 project_ray_origin(const Point2 &p_pos) const;
    virtual Vector3 project_local_ray_normal(const Point2 &p_pos) const;
    virtual Point2 unproject_position(const Vector3 &p_pos) const;
    bool is_position_behind(const Vector3 &p_pos) const;
    virtual Vector3 project_position(const Point2 &p_point, float p_z_depth) const;

    Vector<Vector3> get_near_plane_points() const;

    void set_cull_mask(uint32_t p_layers);
    uint32_t get_cull_mask() const;

    void set_cull_mask_bit(int p_layer, bool p_enable);
    bool get_cull_mask_bit(int p_layer) const;

    virtual Frustum get_frustum() const;

    void set_environment(const Ref<Environment> &p_environment);
    Ref<Environment> get_environment() const;

    void set_keep_aspect_mode(KeepAspect p_aspect);
    KeepAspect get_keep_aspect_mode() const;

    void set_v_offset(float p_offset);
    float get_v_offset() const;

    void set_h_offset(float p_offset);
    float get_h_offset() const;

    void set_doppler_tracking(DopplerTracking p_tracking);
    DopplerTracking get_doppler_tracking() const;

    Vector3 get_doppler_tracked_velocity() const;

    Camera3D();
    ~Camera3D() override;
};

class GODOT_EXPORT ClippedCamera3D : public Camera3D {

    GDCLASS(ClippedCamera3D,Camera3D)

public:
    enum ProcessMode {
        CLIP_PROCESS_PHYSICS,
        CLIP_PROCESS_IDLE,
    };

private:
    ProcessMode process_mode;
    RID pyramid_shape;
    float margin;
    float clip_offset;
    uint32_t collision_mask;
    bool clip_to_areas;
    bool clip_to_bodies;

    HashSet<RID> exclude;

    Vector<Vector3> points;

protected:
    void _notification(int p_what);
    static void _bind_methods();
    Transform get_camera_transform() const override;

public:
    void set_clip_to_areas(bool p_clip);
    bool is_clip_to_areas_enabled() const;

    void set_clip_to_bodies(bool p_clip);
    bool is_clip_to_bodies_enabled() const;

    void set_margin(float p_margin);
    float get_margin() const;

    void set_process_mode(ProcessMode p_mode);
    ProcessMode get_process_mode() const;

    void set_collision_mask(uint32_t p_mask);
    uint32_t get_collision_mask() const;

    void set_collision_mask_bit(int p_bit, bool p_value);
    bool get_collision_mask_bit(int p_bit) const;

    void add_exception_rid(const RID &p_rid);
    void add_exception(const Object *p_object);
    void remove_exception_rid(const RID &p_rid);
    void remove_exception(const Object *p_object);
    void clear_exceptions();

    float get_clip_offset() const;

    ClippedCamera3D();
    ~ClippedCamera3D() override;
};

