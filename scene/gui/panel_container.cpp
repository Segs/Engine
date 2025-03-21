/*************************************************************************/
/*  panel_container.cpp                                                  */
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

#include "panel_container.h"

#include "scene/resources/style_box.h"
#include "core/class_db.h"
#include "core/property_info.h"


IMPL_GDCLASS(PanelContainer)

Size2 PanelContainer::get_minimum_size() const {

    Ref<StyleBox> style;

    if (has_stylebox("panel"))
        style = get_theme_stylebox("panel");
    else
        style = get_theme_stylebox("panel", "PanelContainer");

    Size2 ms;
    for (int i = 0; i < get_child_count(); i++) {

        Control *c = object_cast<Control>(get_child(i));
        if (!c || !c->is_visible_in_tree())
            continue;
        if (c->is_set_as_top_level())
            continue;

        Size2 minsize = c->get_combined_minimum_size();
        ms.width = M_MAX(ms.width, minsize.width);
        ms.height = M_MAX(ms.height, minsize.height);
    }

    if (style)
        ms += style->get_minimum_size();
    return ms;
}

void PanelContainer::_notification(int p_what) {

    if (p_what == NOTIFICATION_DRAW) {

        RenderingEntity ci = get_canvas_item();
        Ref<StyleBox> style;

        if (has_stylebox("panel"))
            style = get_theme_stylebox("panel");
        else
            style = get_theme_stylebox("panel", "PanelContainer");

        style->draw(ci, Rect2(Point2(), get_size()));
    }

    if (p_what == NOTIFICATION_SORT_CHILDREN) {

        Ref<StyleBox> style;

        if (has_stylebox("panel"))
            style = get_theme_stylebox("panel");
        else
            style = get_theme_stylebox("panel", "PanelContainer");

        Size2 size = get_size();
        Point2 ofs;
        if (style) {
            size -= style->get_minimum_size();
            ofs += style->get_offset();
        }

        for (int i = 0; i < get_child_count(); i++) {

            Control *c = object_cast<Control>(get_child(i));
            if (!c || !c->is_visible_in_tree())
                continue;
            if (c->is_set_as_top_level())
                continue;

            fit_child_in_rect(c, Rect2(ofs, size));
        }
    }
}

PanelContainer::PanelContainer() {
    // Has visible stylebox, so stop by default.
    set_mouse_filter(MOUSE_FILTER_STOP);
}
