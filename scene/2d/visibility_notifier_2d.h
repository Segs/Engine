/*************************************************************************/
/*  visibility_notifier_2d.h                                             */
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

#include "core/hash_set.h"
#include "scene/2d/node_2d.h"

class Viewport;
class GODOT_EXPORT VisibilityNotifier2D : public Node2D {

    GDCLASS(VisibilityNotifier2D,Node2D)

    HashSet<Viewport *> viewports;

    Rect2 rect;

protected:
    friend struct SpatialIndexer2D;

    void _enter_viewport(Viewport *p_viewport);
    void _exit_viewport(Viewport *p_viewport);

    virtual void _screen_enter() {}
    virtual void _screen_exit() {}

    void _notification(int p_what);
    static void _bind_methods();

public:
#ifdef TOOLS_ENABLED
    Rect2 _edit_get_rect() const override;
    bool _edit_use_rect() const override;
#endif
    void set_rect(const Rect2 &p_rect);
    Rect2 get_rect() const;

    bool is_on_screen() const;

    VisibilityNotifier2D();
};

class GODOT_EXPORT VisibilityEnabler2D : public VisibilityNotifier2D {

    GDCLASS(VisibilityEnabler2D,VisibilityNotifier2D)

public:
    enum Enabler {
        ENABLER_PAUSE_ANIMATIONS,
        ENABLER_FREEZE_BODIES,
        ENABLER_PAUSE_PARTICLES,
        ENABLER_PARENT_PROCESS,
        ENABLER_PARENT_PHYSICS_PROCESS,
        ENABLER_PAUSE_ANIMATED_SPRITES,
        ENABLER_MAX
    };

protected:
    void _screen_enter() override;
    void _screen_exit() override;

    bool visible;

    void _find_nodes(Node *p_node);

    HashMap<Node *, Variant> nodes;
    void _node_removed(Node *p_node);
    bool enabler[ENABLER_MAX];

    void _change_node_state(Node *p_node, bool p_enabled);

    void _notification(int p_what);
    static void _bind_methods();

public:
    void set_enabler(Enabler p_enabler, bool p_enable);
    bool is_enabler_enabled(Enabler p_enabler) const;

    String get_configuration_warning() const override;

    VisibilityEnabler2D();
};
