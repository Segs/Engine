/*************************************************************************/
/*  shader_gles3.h                                                       */
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
 
#include "drivers/gles3/rasterizer_gl_unique_handle.h"
#include "core/hashfuncs.h"
#include "core/hash_map.h"
#include "core/map.h"
#include "core/math/camera_matrix.h"
#include "core/color.h"
#include "core/threaded_callable_queue.h"
#include "core/variant.h"
#include "core/string.h"
#include "core/vector.h"
#include "core/set.h"
#include "core/math/aabb.h"
#include "core/math/basis.h"
#include "core/math/face3.h"
#include "core/math/plane.h"
#include "core/math/quat.h"
#include "core/math/transform.h"
#include "core/math/transform_2d.h"
#include "core/math/vector3.h"

#include "platform_config.h"
#include "thirdparty/glad/glad/glad.h"
#include "EASTL/intrusive_list.h"

class ShaderCacheGLES3;

union ShaderVersionKey {

    static constexpr uint32_t UBERSHADER_FLAG = ((uint32_t)1) << 31;
    struct {
        uint32_t version;
        uint32_t code_version;
    };
    uint64_t key;
    bool operator==(ShaderVersionKey p_key) const { return key == p_key.key; }
    bool operator<(ShaderVersionKey p_key) const { return key < p_key.key; }
    constexpr ShaderVersionKey() : key(0) {}
    constexpr ShaderVersionKey(uint64_t p_key) : key(p_key) {}

    _FORCE_INLINE_ bool is_subject_to_caching() const { return (version & UBERSHADER_FLAG); }
};

namespace eastl {
    template<>
    struct hash<ShaderVersionKey> {
        size_t operator()(ShaderVersionKey np) const {
            return eastl::hash<uint64_t>()(np.key);
        }

    };
}


class ShaderGLES3 {
protected:
    struct Enum {

        uint64_t mask;
        uint64_t shift;
        const char *defines[16];
    };

    struct EnumValue {

        uint64_t set_mask;
        uint64_t clear_mask;
    };

    struct AttributePair {

        const char *name;
        int index;
    };

    struct UniformPair {
        const char *name;
        VariantType type_hint;
    };

    struct TexUnitPair {

        const char *name;
        int index;
    };

    struct UBOPair {

        const char *name;
        int index;
    };

    struct Feedback {

        const char *name;
        int conditional;
    };

    virtual int get_ubershader_flags_uniform() const { return -1; }

private:
    //@TODO Optimize to a fixed set of shader pools and use a LRU
    int uniform_count;
    int texunit_pair_count;
    int conditional_count;
    int ubo_count;
    int feedback_count;
    int vertex_code_start;
    int fragment_code_start;
    int attribute_pair_count;
public:
    enum AsyncMode : int8_t {
        ASYNC_MODE_VISIBLE,
        ASYNC_MODE_HIDDEN,
        };

private:
    struct CustomCode {

        String vertex;
        String vertex_globals;
        String fragment;
        String fragment_globals;
        String light;
        String uniforms;
        uint32_t version;
        Vector<StringName> texture_uniforms;
        Vector<String> custom_defines;
        Set<uint32_t> versions;
        AsyncMode async_mode;
    };
public:
    static ShaderCacheGLES3 *shader_cache;
    static ThreadedCallableQueue<GLuint> *cache_write_queue;

    static ThreadedCallableQueue<GLuint> *compile_queue; // Non-null if using queued asynchronous compilation (via secondary context)
    static bool parallel_compile_supported; // True if using natively supported asyncrhonous compilation

    static bool async_hidden_forbidden;
    static uint32_t *compiles_started_this_frame;
    static uint32_t *max_frame_compiles_in_progress;
    static uint32_t max_simultaneous_compiles;
    static uint32_t active_compiles_count;
#ifdef DEBUG_ENABLED
    static bool log_active_async_compiles_count;
#endif
    static uint64_t current_frame;
    static void advance_async_shaders_compilation();
private:

    struct Version  : public eastl::intrusive_list_node {
        ShaderVersionKey version_key = 0;

        // Set by the render thread upfront; the compile thread (for queued async.) reads them
        struct Ids {
            GLuint main;
            GLuint vert;
            GLuint frag;
        } ids;

        ShaderGLES3 *shader = nullptr;
        uint32_t code_version = 0;

        AsyncMode async_mode = ASYNC_MODE_VISIBLE;
        GLint *uniform_location=nullptr;
        Vector<GLint> texture_uniform_locations;
        uint64_t last_frame_processed = UINT64_MAX;
        enum CompileStatus : int8_t {
            COMPILE_STATUS_PENDING,
            COMPILE_STATUS_SOURCE_PROVIDED,
            COMPILE_STATUS_COMPILING_VERTEX,
            COMPILE_STATUS_COMPILING_FRAGMENT,
            COMPILE_STATUS_COMPILING_VERTEX_AND_FRAGMENT,
            COMPILE_STATUS_PROCESSING_AT_QUEUE,
            COMPILE_STATUS_BINARY_READY,
            COMPILE_STATUS_BINARY_READY_FROM_CACHE,
            COMPILE_STATUS_LINKING,
            COMPILE_STATUS_ERROR,
            COMPILE_STATUS_RESTART_NEEDED,
            COMPILE_STATUS_OK,
    };
        CompileStatus compile_status=COMPILE_STATUS_PENDING;

        struct ProgramBinary {
            String cache_hash;
            Vector<uint8_t> data;
            SafeNumeric<int> result_from_queue;
            GLenum format;
            enum Source : int8_t {
                SOURCE_NONE,
                SOURCE_LOCAL, // Binary data will only be available if cache enabled
                SOURCE_QUEUE,
                SOURCE_CACHE,
            } source;
            // Shared with the compile thread (for queued async.); otherwise render thread only
        } program_binary;

        bool uniforms_ready=false;
    };
    static eastl::intrusive_list<Version> versions_compiling;

    Version *version;



    //this should use a way more cachefriendly version..
    HashMap<ShaderVersionKey, Version> version_map;

    HashMap<uint32_t, CustomCode> custom_code_map;
    uint32_t last_custom_code;

    ShaderVersionKey conditional_version;
    ShaderVersionKey new_conditional_version;

    virtual const char *get_shader_name() const = 0;

    const char **conditional_defines;
    const char **uniform_names;
    const AttributePair *attribute_pairs;
    const TexUnitPair *texunit_pairs;
    const UBOPair *ubo_pairs;
    const Feedback *feedbacks;
    const char *vertex_code;
    const char *fragment_code;
    String fragment_code0;
    String fragment_code1;
    String fragment_code2;
    String fragment_code3;
    String fragment_code4;

    String vertex_code_before_mats;
    String vertex_code_before_globals;
    String vertex_code_before_custom;
    String vertex_code_after_custom;

    Vector<String> custom_defines;

    int base_material_tex_index;

    Version *get_current_version(bool &r_async_forbidden);
    // These will run on the shader compile thread if using que compile queue approach to async.
    void _set_source(Version::Ids p_ids, Span<const char *> p_vertex_strings, Span<const char *> p_fragment_strings) const;
    bool _complete_compile(Version::Ids p_ids, bool p_retrievable) const;
    bool _complete_link(Version::Ids p_ids, GLenum *r_program_format = nullptr, Vector<uint8_t> *r_program_binary = nullptr) const;
    // ---
    static void _log_active_compiles();
    static bool _process_program_state(Version *p_version, bool p_async_forbidden);
    void _setup_uniforms(CustomCode *p_cc) const;
    void _dispose_program(Version *p_version);

    static ShaderGLES3 *active;

    int max_image_units;

    void _set_uniform_variant(GLint p_uniform, const Variant &p_value) {

        if (p_uniform < 0) {
            return; // do none
        }
        switch (p_value.get_type()) {

            case VariantType::BOOL:
            case VariantType::INT: {

                int val = p_value.as<int>();
                glUniform1i(p_uniform, val);
            } break;
            case VariantType::FLOAT: {

                real_t val = p_value.as<real_t>();
                glUniform1f(p_uniform, val);
            } break;
            case VariantType::COLOR: {

                Color val = p_value.as<Color>();
                glUniform4f(p_uniform, val.r, val.g, val.b, val.a);
            } break;
            case VariantType::VECTOR2: {

                Vector2 val = p_value.as<Vector2>();
                glUniform2f(p_uniform, val.x, val.y);
            } break;
            case VariantType::VECTOR3: {

                Vector3 val = p_value.as<Vector3>();
                glUniform3f(p_uniform, val.x, val.y, val.z);
            } break;
            case VariantType::PLANE: {

                Plane val = p_value.as<Plane>();
                glUniform4f(p_uniform, val.normal.x, val.normal.y, val.normal.z, val.d);
            } break;
            case VariantType::QUAT: {

                Quat val = p_value.as<Quat>();
                glUniform4f(p_uniform, val.x, val.y, val.z, val.w);
            } break;

            case VariantType::TRANSFORM2D: {

                Transform2D tr = p_value.as<Transform2D>();
                GLfloat matrix[16] = { /* build a 16x16 matrix */
                    tr.elements[0][0], tr.elements[0][1], 0, 0,
                    tr.elements[1][0], tr.elements[1][1], 0, 0,
                    0                , 0                , 1, 0,
                    tr.elements[2][0], tr.elements[2][1], 0, 1
                };

                glUniformMatrix4fv(p_uniform, 1, false, matrix);

            } break;
            case VariantType::BASIS:
            case VariantType::TRANSFORM: {

                Transform tr = p_value.as<Transform>();
                GLfloat matrix[16] = { /* build a 16x16 matrix */
                    tr.basis.elements[0][0], tr.basis.elements[1][0], tr.basis.elements[2][0], 0,
                    tr.basis.elements[0][1], tr.basis.elements[1][1], tr.basis.elements[2][1], 0,
                    tr.basis.elements[0][2], tr.basis.elements[1][2], tr.basis.elements[2][2], 0,
                    tr.origin.x            , tr.origin.y            , tr.origin.z            , 1
                };

                glUniformMatrix4fv(p_uniform, 1, false, matrix);
            } break;
            default: {
                ERR_FAIL();
            } // do nothing
        }
    }

    bool _bind(bool p_binding_fallback);
    bool _bind_ubershader(bool p_for_warmup=false);

protected:
    _FORCE_INLINE_ int _get_uniform(int p_which) const;
    _FORCE_INLINE_ void _set_conditional(int p_which, bool p_value);

    void setup(const char **p_conditional_defines, int p_conditional_count, const char **p_uniform_names,
            int p_uniform_count, const AttributePair *p_attribute_pairs, int p_attribute_count,
            const TexUnitPair *p_texunit_pairs, int p_texunit_pair_count, const UBOPair *p_ubo_pairs,
            int p_ubo_pair_count, const Feedback *p_feedback, int p_feedback_count, const char *p_vertex_code,
            const char *p_fragment_code, int p_vertex_code_start, int p_fragment_code_start);

    ShaderGLES3();

public:
    enum {
        CUSTOM_SHADER_DISABLED = 0
    };

    GLint get_uniform_location(StringView p_name) const;
    GLint get_uniform_location(int p_index) const;

    static _FORCE_INLINE_ ShaderGLES3 *get_active() { return active; }
    bool bind();
    void unbind();

    void clear_caches();

    uint32_t create_custom_shader();
    void set_custom_shader_code(uint32_t p_code_id, const String &p_vertex, const String &p_vertex_globals,
            const String &p_fragment, const String &p_light, const String &p_fragment_globals, const String &p_uniforms,
            const Vector<StringName> &p_texture_uniforms, const Vector<String> &p_custom_defines, AsyncMode p_async_mode);
    void set_custom_shader(uint32_t p_code_id);
    void free_custom_shader(uint32_t p_code_id);

    bool is_custom_code_ready_for_render(uint32_t p_code_id);

    uint32_t get_version() const { return new_conditional_version.version; }
    _FORCE_INLINE_ bool is_version_valid() const { return version && version->compile_status == Version::COMPILE_STATUS_OK; }

    virtual void init() = 0;
    void init_async_compilation();
    bool is_async_compilation_supported();
    void finish();

    void set_base_material_tex_index(int p_idx);

    void add_custom_define(StringView p_define) {
        custom_defines.emplace_back(p_define);
    }
    void get_custom_defines(Vector<StringView> *p_defines) {
        p_defines->reserve(p_defines->size()+custom_defines.size());
        for (int i = 0; i < custom_defines.size(); i++) {
            p_defines->push_back(custom_defines[i]);
        }
    }
    //NOTE: If remove_custom_define is called frequently, consider changing custom_defines type to vector_map?
    void remove_custom_define(StringView p_define) {
        for(auto iter=custom_defines.begin(),fin=custom_defines.end();iter!=fin; ++iter) {
            if(*iter==p_define) {
                custom_defines.erase(iter);
                return;
            }
        }
    }
    virtual ~ShaderGLES3();
};

// called a lot, made inline

int ShaderGLES3::_get_uniform(int p_which) const {

    ERR_FAIL_INDEX_V(p_which, uniform_count, -1);
    ERR_FAIL_COND_V(!version, -1);
    return version->uniform_location[p_which];
}

void ShaderGLES3::_set_conditional(int p_which, bool p_value) {

    ERR_FAIL_INDEX(p_which, conditional_count);
    if (p_value) {
        new_conditional_version.version |= (1 << p_which);
    } else {
        new_conditional_version.version &= ~(1 << p_which);
    }
}
