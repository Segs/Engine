/*************************************************************************/
/*  os_server.h                                                          */
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

#ifndef OS_SERVER_H
#define OS_SERVER_H

#include "drivers/dummy/texture_loader_dummy.h"
#include "drivers/unix/os_unix.h"
#include "main/input_default.h"
#ifdef __APPLE__
#include "platform/osx/crash_handler_osx.h"
#else
#include "platform/linuxbsd/crash_handler_x11.h"
#endif
#include "servers/audio_server.h"
#include "servers/rendering/rasterizer.h"
#include "servers/rendering_server.h"

#undef CursorShape

class OS_Server : public OS_Unix {

    RenderingServer *rendering_server;
    VideoMode current_videomode;
    List<String> args;
    MainLoop *main_loop;

    bool grab;

    virtual void delete_main_loop();

    bool force_quit;

    InputDefault *input;

    CrashHandler crash_handler;

    int video_driver_index;

    Ref<ResourceFormatDummyTexture> resource_loader_dummy;

protected:
    virtual int get_video_driver_count() const;
    virtual const char *get_video_driver_name(int p_driver) const;
    virtual int get_current_video_driver() const;
    virtual int get_audio_driver_count() const;
    virtual const char *get_audio_driver_name(int p_driver) const;

    virtual void initialize_core();
    virtual Error initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver);
    virtual void finalize();

    virtual void set_main_loop(MainLoop *p_main_loop);

public:
    virtual String get_name() const;

    virtual void set_mouse_show(bool p_show);
    virtual void set_mouse_grab(bool p_grab);
    virtual bool is_mouse_grab_enabled() const;
    virtual Point2 get_mouse_position() const;
    virtual int get_mouse_button_state() const;
    virtual void set_window_title(const String &p_title);

    virtual MainLoop *get_main_loop() const;

    virtual bool can_draw() const;

    virtual void set_video_mode(const VideoMode &p_video_mode, int p_screen = 0);
    virtual VideoMode get_video_mode(int p_screen = 0) const;
    virtual void get_fullscreen_mode_list(Vector<VideoMode> *p_list, int p_screen = 0) const;

    virtual Size2 get_window_size() const;

    virtual void move_window_to_foreground();

    void run();

    virtual int get_power_seconds_left();
    virtual int get_power_percent_left();
    virtual bool _check_internal_feature_support(const String &p_feature);

    void disable_crash_handler();
    bool is_disable_crash_handler() const;

    OS_Server();
};

#endif
