/*************************************************************************/
/*  occluder.h                                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

class OccluderShape;
class Resource;

using RES = Ref<Resource>;

class Occluder : public Node3D {
    GDCLASS(Occluder, Node3D);

    friend class OccluderSpatialGizmo;
    friend class OccluderEditorPlugin;

    RenderingEntity _occluder_instance;
    Ref<OccluderShape> _shape;

    void resource_changed(RES res);

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    void set_shape(const Ref<OccluderShape> &p_shape);
    Ref<OccluderShape> get_shape() const;

    String get_configuration_warning() const override;
#ifdef TOOLS_ENABLED
    // for editor gizmo
    AABB get_fallback_gizmo_aabb() const override;
#endif
    Occluder();
    ~Occluder() override;
};
