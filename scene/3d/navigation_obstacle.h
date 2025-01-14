/*************************************************************************/
/*  navigation_obstacle.h                                                */
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

#pragma once

#include "scene/main/node.h"
#include "core/rid.h"

class Navigation3D;
class Node3D;

class GODOT_EXPORT NavigationObstacle : public Node {
    GDCLASS(NavigationObstacle, Node)

    Node3D *parent_spatial = nullptr;
    Navigation3D *navigation;

    RID agent;
    bool estimate_radius = true;
    real_t radius = 1.0f;

protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &property) const override;
    void _notification(int p_what);

public:
    NavigationObstacle();
    ~NavigationObstacle() override;

    void set_navigation(Navigation3D *p_nav);
    const Navigation3D *get_navigation() const {
        return navigation;
    }

    void set_navigation_node(Node *p_nav);
    Node *get_navigation_node() const;

    RID get_rid() const {
        return agent;
    }

    void set_estimate_radius(bool p_estimate_radius);
    bool is_radius_estimated() const {
        return estimate_radius;
    }
    void set_radius(real_t p_radius);
    real_t get_radius() const {
        return radius;
    }
    String get_configuration_warning() const override;

private:
    void initialize_agent();
    void reevaluate_agent_radius();
    real_t estimate_agent_radius() const;
};
