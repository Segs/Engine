/*************************************************************************/
/*  rigid_body_bullet.h                                                  */
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

#ifndef BODYBULLET_H
#define BODYBULLET_H

#include "collision_object_bullet.h"
#include "space_bullet.h"

#include <BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h>
#include <LinearMath/btTransform.h>

/**
    @author AndreaCatania
*/

class AreaBullet;
class SpaceBullet;
class btRigidBody;
class GodotMotionState;

class BulletPhysicsDirectBodyState : public PhysicsDirectBodyState3D {
    GDCLASS(BulletPhysicsDirectBodyState,PhysicsDirectBodyState3D)


public:
    RigidBodyBullet *body = nullptr;

    BulletPhysicsDirectBodyState() {}

    Vector3 get_total_gravity() const override;
    float get_total_angular_damp() const override;
    float get_total_linear_damp() const override;

    Vector3 get_center_of_mass() const override;
    Basis get_principal_inertia_axes() const override;
    // get the mass
    float get_inverse_mass() const override;
    // get density of this body space
    Vector3 get_inverse_inertia() const override;
    // get density of this body space
    Basis get_inverse_inertia_tensor() const override;

    void set_linear_velocity(const Vector3 &p_velocity) override;
    Vector3 get_linear_velocity() const override;

    void set_angular_velocity(const Vector3 &p_velocity) override;
    Vector3 get_angular_velocity() const override;

    void set_transform(const Transform &p_transform) override;
    Transform get_transform() const override;
    Vector3 get_velocity_at_local_position(const Vector3 &p_position) const override;

    void add_central_force(const Vector3 &p_force) override;
    void add_force(const Vector3 &p_force, const Vector3 &p_pos) override;
    void add_torque(const Vector3 &p_torque) override;
    void apply_central_impulse(const Vector3 &p_impulse) override;
    void apply_impulse(const Vector3 &p_pos, const Vector3 &p_impulse) override;
    void apply_torque_impulse(const Vector3 &p_impulse) override;

    void set_sleep_state(bool p_enable) override;
    bool is_sleeping() const override;

    int get_contact_count() const override;

    Vector3 get_contact_local_position(int p_contact_idx) const override;
    Vector3 get_contact_local_normal(int p_contact_idx) const override;
    float get_contact_impulse(int p_contact_idx) const override;
    int get_contact_local_shape(int p_contact_idx) const override;

    RID get_contact_collider(int p_contact_idx) const override;
    Vector3 get_contact_collider_position(int p_contact_idx) const override;
    GameEntity get_contact_collider_id(int p_contact_idx) const override;
    int get_contact_collider_shape(int p_contact_idx) const override;
    Vector3 get_contact_collider_velocity_at_position(int p_contact_idx) const override;

    real_t get_step() const override;
    void integrate_forces() override {
        // Skip the execution of this function
    }

    PhysicsDirectSpaceState3D *get_space_state() override;
};

class RigidBodyBullet : public RigidCollisionObjectBullet {

public:
    struct CollisionData {
        RigidBodyBullet *otherObject;
        int other_object_shape;
        int local_shape;
        Vector3 hitLocalLocation;
        Vector3 hitWorldLocation;
        Vector3 hitNormal;
        float appliedImpulse;
    };

    struct ForceIntegrationCallback {
        GameEntity id;
        StringName method;
        Variant udata;
    };

    /// Used to hold shapes
    struct KinematicShape {
        class btConvexShape *shape;
        btTransform transform;

        KinematicShape() :
                shape(nullptr) {}
        bool is_active() const { return shape; }
    };

    struct KinematicUtilities {
        RigidBodyBullet *owner;
        btScalar safe_margin;
        Vector<KinematicShape> shapes;

        KinematicUtilities(RigidBodyBullet *p_owner);
        ~KinematicUtilities();

        void setSafeMargin(btScalar p_margin);
        /// Used to set the default shape to ghost
        void copyAllOwnerShapes();

    private:
        void just_delete_shapes(int new_size);
    };

private:
    BulletPhysicsDirectBodyState *direct_access = nullptr;
    friend class BulletPhysicsDirectBodyState;

    // This is required only for Kinematic movement
    KinematicUtilities *kinematic_utilities;

    PhysicsServer3D::BodyMode mode;
    GodotMotionState *godotMotionState;
    btRigidBody *btBody;
    uint16_t locked_axis;
    real_t mass;
    real_t gravity_scale;
    real_t linearDamp;
    real_t angularDamp;
    bool can_sleep;
    bool omit_forces_integration;
    bool can_integrate_forces;

    Vector<CollisionData> collisions;
    Vector<RigidBodyBullet *> collision_traces_1;
    Vector<RigidBodyBullet *> collision_traces_2;
    Vector<RigidBodyBullet *> *prev_collision_traces;
    Vector<RigidBodyBullet *> *curr_collision_traces;

    // these parameters are used to avoid vector resize
    int maxCollisionsDetection;
    int collisionsCount;
    int prev_collision_count;

    Vector<AreaBullet *> areasWhereIam;
    // these parameters are used to avoid vector resize
    int maxAreasWhereIam;
    int areaWhereIamCount;
    // Used to know if the area is used as gravity point
    int countGravityPointSpaces;
    bool isScratchedSpaceOverrideModificator;

    bool previousActiveState; // Last check state

    Callable force_integration_callback;

public:
    RigidBodyBullet();
    ~RigidBodyBullet() override;
    BulletPhysicsDirectBodyState *get_direct_state() const { return direct_access; }

    void init_kinematic_utilities();
    void destroy_kinematic_utilities();
    _FORCE_INLINE_ KinematicUtilities *get_kinematic_utilities() const { return kinematic_utilities; }

    _FORCE_INLINE_ btRigidBody *get_bt_rigid_body() { return btBody; }

    void main_shape_changed() override;
    void reload_body() override;
    void set_space(SpaceBullet *p_space) override;

    void dispatch_callbacks() override;
    void set_force_integration_callback(Callable &&callback);
    void scratch_space_override_modificator();

    void on_collision_filters_change() override;
    void on_collision_checker_start() override;
    void on_collision_checker_end() override;

    void set_max_collisions_detection(int p_maxCollisionsDetection) {

        ERR_FAIL_COND(0 > p_maxCollisionsDetection);

        maxCollisionsDetection = p_maxCollisionsDetection;

        collisions.resize(p_maxCollisionsDetection);
        collision_traces_1.resize(p_maxCollisionsDetection);
        collision_traces_2.resize(p_maxCollisionsDetection);

        collisionsCount = 0;
        prev_collision_count = MIN(prev_collision_count, p_maxCollisionsDetection);
    }
    int get_max_collisions_detection() {
        return maxCollisionsDetection;
    }

    bool can_add_collision() { return collisionsCount < maxCollisionsDetection; }
    bool add_collision_object(RigidBodyBullet *p_otherObject, const Vector3 &p_hitWorldLocation, const Vector3 &p_hitLocalLocation, const Vector3 &p_hitNormal, const float &p_appliedImpulse, int p_other_shape_index, int p_local_shape_index);
    bool was_colliding(RigidBodyBullet *p_other_object);

    void set_activation_state(bool p_active);
    bool is_active() const;

    void set_omit_forces_integration(bool p_omit);
    _FORCE_INLINE_ bool get_omit_forces_integration() const { return omit_forces_integration; }

    void set_param(PhysicsServer3D::BodyParameter p_param, real_t);
    real_t get_param(PhysicsServer3D::BodyParameter p_param) const;

    void set_mode(PhysicsServer3D::BodyMode p_mode);
    PhysicsServer3D::BodyMode get_mode() const;

    void set_state(PhysicsServer3D::BodyState p_state, const Variant &p_variant);
    Variant get_state(PhysicsServer3D::BodyState p_state) const;

    void apply_impulse(const Vector3 &p_pos, const Vector3 &p_impulse);
    void apply_central_impulse(const Vector3 &p_impulse);
    void apply_torque_impulse(const Vector3 &p_impulse);

    void apply_force(const Vector3 &p_force, const Vector3 &p_pos);
    void apply_central_force(const Vector3 &p_force);
    void apply_torque(const Vector3 &p_torque);

    void set_applied_force(const Vector3 &p_force);
    Vector3 get_applied_force() const;
    void set_applied_torque(const Vector3 &p_torque);
    Vector3 get_applied_torque() const;

    void set_axis_lock(PhysicsServer3D::BodyAxis p_axis, bool lock);
    bool is_axis_locked(PhysicsServer3D::BodyAxis p_axis) const;
    void reload_axis_lock();

    /// Doc:
    /// https://web.archive.org/web/20180404091446/http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Anti_tunneling_by_Motion_Clamping
    void set_continuous_collision_detection(bool p_enable);
    bool is_continuous_collision_detection_enabled() const;

    void set_linear_velocity(const Vector3 &p_velocity);
    Vector3 get_linear_velocity() const;

    void set_angular_velocity(const Vector3 &p_velocity);
    Vector3 get_angular_velocity() const;

    void set_transform__bullet(const btTransform &p_global_transform) override;
    const btTransform &get_transform__bullet() const override;

    void reload_shapes() override;

    void on_enter_area(AreaBullet *p_area) override;
    void on_exit_area(AreaBullet *p_area) override;
    void reload_space_override_modificator();

    /// Kinematic
    void reload_kinematic_shapes();

    void notify_transform_changed() override;

private:
    void _internal_set_mass(real_t p_mass);
};

#endif
