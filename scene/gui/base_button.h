/*************************************************************************/
/*  base_button.h                                                        */
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
#include "scene/gui/control.h"

class ButtonGroup;
class ShortCut;

class GODOT_EXPORT BaseButton : public Control {

    GDCLASS(BaseButton,Control)

public:
    enum ActionMode {
        ACTION_MODE_BUTTON_PRESS,
        ACTION_MODE_BUTTON_RELEASE,
    };

private:
    int button_mask;
    bool toggle_mode = false;
    bool shortcut_in_tooltip = true;
    bool keep_pressed_outside = false;
    FocusMode enabled_focus_mode;
    Ref<ShortCut> shortcut;

    ActionMode action_mode;
    struct Status {

        bool pressed = false;
        bool hovering = false;
        bool press_attempt = false;
        bool pressing_inside = false;

        bool disabled = false;

    } status;

    Ref<ButtonGroup> button_group;

    void _unpress_group();
    void _pressed();
    void _toggled(bool p_pressed);

    void on_action_event(Ref<InputEvent> p_event);

protected:
    virtual void pressed() {}
    virtual void toggled(bool p_pressed) {}
    static void _bind_methods();
    virtual void _gui_input(Ref<InputEvent> p_event);
    virtual void _unhandled_input(Ref<InputEvent> p_event);
    void _notification(int p_what);

public:
    enum DrawMode {
        DRAW_NORMAL,
        DRAW_PRESSED,
        DRAW_HOVER,
        DRAW_DISABLED,
        DRAW_HOVER_PRESSED,
    };

    DrawMode get_draw_mode() const;

    /* Signals */

    bool is_pressed() const; ///< return whether button is pressed (toggled in)
    bool is_pressing() const; ///< return whether button is pressed (toggled in)
    bool is_hovered() const;

    void set_pressed(bool p_pressed); // Only works in toggle mode.
    void set_pressed_no_signal(bool p_pressed);
    void set_toggle_mode(bool p_on);
    bool is_toggle_mode() const;

    void set_shortcut_in_tooltip(bool p_on);
    bool is_shortcut_in_tooltip_enabled() const;

    void set_disabled(bool p_disabled);
    bool is_disabled() const;

    void set_action_mode(ActionMode p_mode);
    ActionMode get_action_mode() const;

    void set_keep_pressed_outside(bool p_on);
    bool is_keep_pressed_outside() const;

    void set_button_mask(int p_mask);
    int get_button_mask() const;

    void set_enabled_focus_mode(FocusMode p_mode);
    FocusMode get_enabled_focus_mode() const;

    void set_shortcut(const Ref<ShortCut> &p_shortcut);
    Ref<ShortCut> get_shortcut() const;

    const String & get_tooltip(const Point2 &p_pos) const override;

    void set_button_group(const Ref<ButtonGroup> &p_group);
    Ref<ButtonGroup> get_button_group() const;

    BaseButton();
    ~BaseButton() override;
};


#include "core/resource.h"

class GODOT_EXPORT ButtonGroup : public Resource {

    GDCLASS(ButtonGroup,Resource)

    friend class BaseButton;
    HashSet<BaseButton *> buttons;

protected:
    static void _bind_methods();

public:
    BaseButton *get_pressed_button();
    Vector<BaseButton *> get_buttons();
    ButtonGroup();
};
