/*************************************************************************/
/*  world_3d.h                                                              */
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

#include "core/engine_entities.h"
#include "core/resource.h"
#include "scene/resources/environment.h"
#include "servers/physics_server_3d.h"

class Camera3D;
class VisibilityNotifier3D;
struct SpatialIndexer;

class GODOT_EXPORT World3D : public Resource {
    GDCLASS(World3D,Resource)

    RES_BASE_EXTENSION("world")

private:
    RID physics_space;
    RID navigation_map;
    RenderingEntity renderer_scene;
    SpatialIndexer *indexer;
    Ref<Environment> environment;
    Ref<Environment> fallback_environment;

protected:
    static void _bind_methods();

    friend class Camera3D;
    friend class VisibilityNotifier3D;

    void _register_camera(Camera3D *p_camera);
    void _update_camera(Camera3D *p_camera);
    void _remove_camera(Camera3D *p_camera);

    void _register_notifier(VisibilityNotifier3D *p_notifier, const AABB &p_rect);
    void _update_notifier(VisibilityNotifier3D *p_notifier, const AABB &p_rect);
    void _remove_notifier(VisibilityNotifier3D *p_notifier);
    friend class Viewport;
    void _update(uint64_t p_frame);

public:
    RID get_space() const;
    RenderingEntity get_scenario() const;
    RID get_navigation_map() const;

    void set_environment(const Ref<Environment> &p_environment);
    Ref<Environment> get_environment() const;

    void set_fallback_environment(const Ref<Environment> &p_environment);
    Ref<Environment> get_fallback_environment() const;

    void get_camera_list(Vector<Camera3D *> *r_cameras);

    PhysicsDirectSpaceState3D *get_direct_space_state();

    World3D();
    ~World3D() override;
};
