/*************************************************************************/
/*  editor_run.h                                                         */
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

#include "core/os/os.h"

class EditorRun {
public:
    enum Status {

        STATUS_PLAY,
        STATUS_PAUSED,
        STATUS_STOP
    };

    OS::ProcessID pid;

private:
    bool debug_collisions;
    bool debug_navigation;
    Status status;

public:
    Status get_status() const;
    Error run(se_string_view p_scene, se_string_view p_custom_args, const Vector<String> &p_breakpoints, const bool &p_skip_breakpoints = false);
    void run_native_notify() { status = STATUS_PLAY; }
    void stop();

    OS::ProcessID get_pid() const { return pid; }

    void set_debug_collisions(bool p_debug);
    bool get_debug_collisions() const;

    void set_debug_navigation(bool p_debug);
    bool get_debug_navigation() const;

    EditorRun();
};
