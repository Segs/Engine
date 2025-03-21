/*************************************************************************/
/*  shape.h                                                              */
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

#include "core/resource.h"
#include "core/rid.h"
class ArrayMesh;

class GODOT_EXPORT Shape : public Resource {

    GDCLASS(Shape,Resource)

    OBJ_SAVE_TYPE(Shape)
    RES_BASE_EXTENSION("shape")
    RID shape;
    real_t margin;

    Ref<ArrayMesh> debug_mesh_cache;

protected:
    static void _bind_methods();

    _FORCE_INLINE_ RID get_shape() const { return shape; }
    Shape(RID p_shape);

    virtual void _update_shape();

public:
    virtual Vector<Vector3> get_debug_mesh_lines() = 0; // { return Vector<Vector3>(); }

    RID get_phys_rid() const override { return shape; }

    Ref<ArrayMesh> get_debug_mesh();

    /// Returns the radius of a sphere that fully enclose this shape
    virtual real_t get_enclosing_radius() const = 0;

    void add_vertices_to_array(Vector<Vector3> &array, const Transform &p_xform);

    real_t get_margin() const;
    void set_margin(real_t p_margin);

    Shape();
    ~Shape() override;
};
