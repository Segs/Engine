/*************************************************************************/
/*  main_class.h                                                         */
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

#include "core/error_list.h"
#include "core/typedefs.h"
#include "core/forward_decls.h"
#include "EASTL/functional.h"

using OptionAvailableCb = eastl::function<bool(const UIString &)>;
using OptionMissingCb = eastl::function<bool()>;

class GODOT_EXPORT Main {

    static void collect_options();
    static uint64_t last_ticks;
    static uint32_t frames;
    static uint32_t frame;
    static bool force_redraw_requested;
    static int iterating;
    static bool agile_input_event_flushing;

public:
    static bool is_project_manager();

    static void dumpReflectedTypes();
    static Error setup(bool p_second_phase = true);
    static Error setup2();
    static bool start();

    static bool iteration();
    static void force_redraw();

    static bool is_iterating();

    static void cleanup(bool p_force = false);
    static void add_cli_option(StringView short_form,StringView long_form,StringView desc,OptionAvailableCb available,OptionMissingCb missing=[]()->bool{ return true; });
    static void add_cli_value_option(StringView short_form,StringView long_form,StringView desc,StringView value_name,OptionAvailableCb available,OptionMissingCb missing=[]()->bool{ return true; });
private:
    static void init_rendering_options();
    static void print_debug_render_options();
};
