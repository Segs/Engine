/*************************************************************************/
/*  split_container.h                                                    */
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

#ifndef SPLIT_CONTAINER_H
#define SPLIT_CONTAINER_H

#include "scene/gui/container.h"

class GODOT_EXPORT SplitContainer : public Container {

    GDCLASS(SplitContainer,Container)

public:
    enum DraggerVisibility : uint8_t {
        DRAGGER_VISIBLE,
        DRAGGER_HIDDEN,
        DRAGGER_HIDDEN_COLLAPSED
    };

private:
    int split_offset;
    int middle_sep;
    int drag_from;
    int drag_ofs;
    DraggerVisibility dragger_visibility;
    bool should_clamp_split_offset;
    bool vertical;
    bool dragging;
    bool collapsed;
    bool mouse_inside;

    Control *_getch(int p_idx) const;

    void _resort();

protected:
    void _gui_input(const Ref<InputEvent> &p_event);
    void _notification(int p_what);
    static void _bind_methods();

public:
    void set_split_offset(int p_offset);
    int get_split_offset() const;
    void clamp_split_offset();

    void set_collapsed(bool p_collapsed);
    bool is_collapsed() const;

    void set_dragger_visibility(DraggerVisibility p_visibility);
    DraggerVisibility get_dragger_visibility() const;

    CursorShape get_cursor_shape(const Point2 &p_pos = Point2i()) const override;

    Size2 get_minimum_size() const override;

    SplitContainer(bool p_vertical = false);
};


class HSplitContainer : public SplitContainer {

    GDCLASS(HSplitContainer,SplitContainer)

public:
    HSplitContainer() :
            SplitContainer(false) {}
};

class VSplitContainer : public SplitContainer {

    GDCLASS(VSplitContainer,SplitContainer)

public:
    VSplitContainer() :
            SplitContainer(true) {}
};

#endif // SPLIT_CONTAINER_H
