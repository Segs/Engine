
/*************************************************************************/
/*  vehicle_body_3d.h                                                    */
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

#include "scene/3d/physics_body_3d.h"

class VehicleBody3D;

class GODOT_EXPORT VehicleWheel3D : public Node3D {

    GDCLASS(VehicleWheel3D,Node3D)

    friend class VehicleBody3D;

    Transform m_worldTransform;
    Transform local_xform;
    bool engine_traction=false;
    bool steers=false;

    Vector3 m_chassisConnectionPointCS; //const
    Vector3 m_wheelDirectionCS; //const
    Vector3 m_wheelAxleCS; // const or modified by steering

    real_t m_suspensionRestLength = 0.15f;
    real_t m_maxSuspensionTravelCm = 500;
    real_t m_wheelRadius = 0.5f;

    real_t m_suspensionStiffness = 5.88f;
    real_t m_wheelsDampingCompression = 0.83f;
    real_t m_wheelsDampingRelaxation = 0.88f;
    real_t m_frictionSlip = 10.5;
    real_t m_maxSuspensionForce = 6000;
    bool m_bIsFrontWheel = false;

    VehicleBody3D *body=nullptr;

    //btVector3    m_wheelAxleCS; // const or modified by steering ?

    real_t m_steering=0;
    real_t m_rotation=0;
    real_t m_deltaRotation=0;
    real_t m_rpm=0;
    real_t m_rollInfluence=0.1f;
    real_t m_engineForce=0;
    real_t m_brake=0;

    real_t m_clippedInvContactDotSuspension = 1.0;
    real_t m_suspensionRelativeVelocity = 0;
    //calculated by suspension
    real_t m_wheelsSuspensionForce=0;
    real_t m_skidInfo=0;

    struct RaycastInfo {
        //set by raycaster
        Vector3 m_contactNormalWS; //contactnormal
        Vector3 m_contactPointWS; //raycast hitpoint
        real_t m_suspensionLength=0;
        Vector3 m_hardPointWS; //raycast starting point
        Vector3 m_wheelDirectionWS; //direction in worldspace
        Vector3 m_wheelAxleWS; // axle in worldspace
        bool m_isInContact = false;
        PhysicsBody3D *m_groundObject = nullptr; //could be general void* ptr
    } m_raycastInfo;

    void _update(PhysicsDirectBodyState3D *s);

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    void set_radius(float p_radius);
    float get_radius() const;

    void set_suspension_rest_length(float p_length);
    float get_suspension_rest_length() const;

    void set_suspension_travel(float p_length);
    float get_suspension_travel() const;

    void set_suspension_stiffness(float p_value);
    float get_suspension_stiffness() const;

    void set_suspension_max_force(float p_value);
    float get_suspension_max_force() const;

    void set_damping_compression(float p_value);
    float get_damping_compression() const;

    void set_damping_relaxation(float p_value);
    float get_damping_relaxation() const;

    void set_friction_slip(float p_value);
    float get_friction_slip() const;

    void set_use_as_traction(bool p_enable);
    bool is_used_as_traction() const;

    void set_use_as_steering(bool p_enabled);
    bool is_used_as_steering() const;

    bool is_in_contact() const;

    Node3D *get_contact_body() const;

    void set_roll_influence(float p_value);
    float get_roll_influence() const;

    float get_skidinfo() const;

    float get_rpm() const;

    void set_engine_force(float p_engine_force);
    float get_engine_force() const;

    void set_brake(float p_brake);
    float get_brake() const;

    void set_steering(float p_steering);
    float get_steering() const;

    String get_configuration_warning() const override;

    VehicleWheel3D();
};

class GODOT_EXPORT VehicleBody3D : public RigidBody {

    GDCLASS(VehicleBody3D,RigidBody)

    float engine_force;
    float brake;

    real_t m_pitchControl;
    real_t m_steeringValue;
    real_t m_currentVehicleSpeedKmHour;

    HashSet<RID> exclude;

    Vector<Vector3> m_forwardWS;
    Vector<Vector3> m_axle;
    Vector<real_t> m_forwardImpulse;
    Vector<real_t> m_sideImpulse;

    struct btVehicleWheelContactPoint {
        PhysicsDirectBodyState3D *m_s;
        PhysicsBody3D *m_body1;
        Vector3 m_frictionPositionWorld;
        Vector3 m_frictionDirectionWorld;
        real_t m_jacDiagABInv;
        real_t m_maxImpulse;

        btVehicleWheelContactPoint(PhysicsDirectBodyState3D *s, PhysicsBody3D *body1, const Vector3 &frictionPosWorld, const Vector3 &frictionDirectionWorld, real_t maxImpulse);
    };

    void _resolve_single_bilateral(PhysicsDirectBodyState3D *s, const Vector3 &pos1, PhysicsBody3D *body2, const Vector3 &pos2, const Vector3 &normal, real_t &impulse, const real_t p_rollInfluence);
    real_t _calc_rolling_friction(btVehicleWheelContactPoint &contactPoint);

    void _update_friction(PhysicsDirectBodyState3D *s);
    void _update_suspension(PhysicsDirectBodyState3D *s);
    real_t _ray_cast(int p_idx, PhysicsDirectBodyState3D *s);
    void _update_wheel_transform(VehicleWheel3D &wheel, PhysicsDirectBodyState3D *s);
    void _update_wheel(int p_idx, PhysicsDirectBodyState3D *s);

    friend class VehicleWheel3D;
    Vector<VehicleWheel3D *> wheels;

    static void _bind_methods();

    void _direct_state_changed(Object *p_state) override;

public:
    void set_engine_force(float p_engine_force);
    float get_engine_force() const;

    void set_brake(float p_brake);
    float get_brake() const;

    void set_steering(float p_steering);
    float get_steering() const;

    VehicleBody3D();
};
