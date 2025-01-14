/*************************************************************************/
/*  shader_compiler_gles3.h                                              */
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
#include "core/pair.h"
#include "core/string.h"
#include "core/hash_map.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_types.h"
#include "servers/rendering_server_enums.h"
 
class ShaderCompilerGLES3 {
public:
    struct IdentifierActions {

        HashMap<StringName, Pair<int8_t *, int> > render_mode_values;
        HashMap<StringName, bool *> render_mode_flags;
        HashMap<StringName, bool *> usage_flag_pointers;
        HashMap<StringName, bool *> write_flag_pointers;

        HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> *uniforms;
    };

    struct GeneratedCode {

        Vector<String> defines;
        Vector<StringName> texture_uniforms;
        Vector<ShaderLanguage::DataType> texture_types;
        Vector<ShaderLanguage::ShaderNode::Uniform::Hint> texture_hints;

        Vector<uint32_t> uniform_offsets;
        uint32_t uniform_total_size;
        String uniforms;
        String vertex_global;
        String vertex;
        String fragment_global;
        String fragment;
        String light;

        bool uses_fragment_time;
        bool uses_vertex_time;
    };

private:
    ShaderLanguage parser;

    struct DefaultIdentifierActions {

        HashMap<StringName, String> renames;
        HashMap<StringName, String> render_mode_defines;
        HashMap<StringName, String> usage_defines;
    };

    void _dump_function_deps(const ShaderLanguage::ShaderNode *p_node, const StringName &p_for_func, const HashMap<StringName, String> &p_func_code, String &r_to_add, HashSet<StringName> &added);
    String _dump_node_code(const ShaderLanguage::Node *p_node, int p_level, GeneratedCode &r_gen_code, IdentifierActions &p_actions, const DefaultIdentifierActions &p_default_actions, bool p_assigning, bool p_use_scope = true);

    const ShaderLanguage::ShaderNode *shader;
    const ShaderLanguage::FunctionNode *function;
    StringName current_func_name;
    StringName vertex_name;
    StringName fragment_name;
    StringName light_name;
    StringName time_name;

    HashSet<StringName> used_name_defines;
    HashSet<StringName> used_flag_pointers;
    HashSet<StringName> used_rmode_defines;
    HashSet<StringName> internal_functions;
    HashSet<StringName> fragment_varyings;

    DefaultIdentifierActions actions[int(RenderingServerEnums::ShaderMode::MAX)];

public:
    Error compile(RS::ShaderMode p_mode, const String &p_code, IdentifierActions *p_actions, const String &p_path, GeneratedCode &r_gen_code);

    ShaderCompilerGLES3();
};
