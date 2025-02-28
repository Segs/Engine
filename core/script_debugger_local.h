/*************************************************************************/
/*  script_debugger_local.h                                              */
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

#include "core/list.h"
#include "core/script_language.h"
#include "core/debugger/script_debugger.h"

class GODOT_EXPORT ScriptDebuggerLocal : public ScriptDebugger {

    bool profiling;
    float frame_time, process_time, physics_time, physics_frame_time;
    uint64_t idle_accum;
    String target_function;
    Map<String, String> options;

    Vector<ScriptLanguage::ProfilingInfo> pinfo;

    Pair<String, int> to_breakpoint(const String &p_line);
    void print_variables(const Vector<String> &names, const Vector<Variant> &values, StringView variable_prefix);

public:
    void debug(ScriptLanguage *p_script, bool p_can_continue, bool p_is_error_breakpoint) override;
    void send_message(const String &p_message, const Array &p_args) override;
    void send_error(StringView p_func, StringView p_file, int p_line, StringView p_err, StringView p_descr, ErrorHandlerType p_type, const Vector<ScriptLanguage::StackInfo> &p_stack_info) override;

    bool is_profiling() const override { return profiling; }
    void add_profiling_frame_data(const StringName &p_name, const Array &p_data) override {}

    void idle_poll() override;

    void profiling_start() override;
    void profiling_end() override;
    void profiling_set_frame_times(float p_frame_time, float p_process_time, float p_physics_time, float p_physics_frame_time) override;

    ScriptDebuggerLocal();
};
