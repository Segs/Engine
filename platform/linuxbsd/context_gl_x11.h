/*************************************************************************/
/*  context_gl_x11.h                                                     */
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

#ifdef X11_ENABLED

#if defined(OPENGL_ENABLED)

#include "core/os/os.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

struct ContextGL_X11_Private;

class ContextGL_X11 {


private:
    ContextGL_X11_Private *p;
    OS::VideoMode default_video_mode;
    //::Colormap x11_colormap;
    ::Display *x11_display;
    ::Window &x11_window;
    bool double_buffer;
    bool direct_render;
    int glx_minor, glx_major;
    bool use_vsync;

public:
    void release_current();
    void make_current();
    void swap_buffers();
    int get_window_width();
    int get_window_height();
    void *get_glx_context();
    bool is_offscreen_available() const;
    void make_offscreen_current();
    void release_offscreen_current();

    Error initialize();

    void set_use_vsync(bool p_use);
    bool is_using_vsync() const;

    ContextGL_X11(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode);
    ~ContextGL_X11();
};

#endif

#endif
