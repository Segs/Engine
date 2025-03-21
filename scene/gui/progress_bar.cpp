/*************************************************************************/
/*  progress_bar.cpp                                                     */
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

#include "progress_bar.h"

#include "scene/resources/style_box.h"
#include "scene/resources/font.h"
#include "core/method_bind.h"

IMPL_GDCLASS(ProgressBar)

Size2 ProgressBar::get_minimum_size() const {

    Ref<StyleBox> bg = get_theme_stylebox("bg");
    Ref<StyleBox> fg = get_theme_stylebox("fg");
    Ref<Font> font = get_theme_font("font");

    Size2 minimum_size = bg->get_minimum_size();
    minimum_size.height = M_MAX(minimum_size.height, fg->get_minimum_size().height);
    minimum_size.width = M_MAX(minimum_size.width, fg->get_minimum_size().width);
    if (percent_visible) {
        minimum_size.height = M_MAX(minimum_size.height, bg->get_minimum_size().height + font->get_height());
    } else { // this is needed, else the progressbar will collapse
        minimum_size.width = M_MAX(minimum_size.width, 1);
        minimum_size.height = M_MAX(minimum_size.height, 1);
    }
    return minimum_size;
}

void ProgressBar::_notification(int p_what) {

    if (p_what == NOTIFICATION_DRAW) {

        Ref<StyleBox> bg = get_theme_stylebox("bg");
        Ref<StyleBox> fg = get_theme_stylebox("fg");
        Ref<Font> font = get_theme_font("font");
        Color font_color = get_theme_color("font_color");

        draw_style_box(bg, Rect2(Point2(), get_size()));
        float r = get_as_ratio();
        int mp = fg->get_minimum_size().width;
        int p = r * (get_size().width - mp);
        if (p > 0) {

            draw_style_box(fg, Rect2(Point2(), Size2(p + fg->get_minimum_size().width, get_size().height)));
        }

        if (percent_visible) {
            String txt = StringUtils::num(int(get_as_ratio() * 100)) + "%";
            font->draw_halign_utf8(get_canvas_item(), Point2(0, font->get_ascent() + (get_size().height - font->get_height()) / 2), HALIGN_CENTER, get_size().width, txt, font_color);
        }
    }
}

void ProgressBar::set_percent_visible(bool p_visible) {

    percent_visible = p_visible;
    update();
}

bool ProgressBar::is_percent_visible() const {

    return percent_visible;
}

void ProgressBar::_bind_methods() {

    SE_BIND_METHOD(ProgressBar,set_percent_visible);
    SE_BIND_METHOD(ProgressBar,is_percent_visible);
    ADD_GROUP("Percent", "percent_");
    ADD_PROPERTY(PropertyInfo(VariantType::BOOL, "percent_visible"), "set_percent_visible", "is_percent_visible");
}

ProgressBar::ProgressBar() {

    set_v_size_flags(0);
    set_step(0.01);
    percent_visible = true;
}
