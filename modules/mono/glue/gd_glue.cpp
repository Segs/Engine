/*************************************************************************/
/*  gd_glue.cpp                                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "gd_glue.h"

#include "core/array.h"
#include "core/class_db.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/ustring.h"
#include "core/object_db.h"
#include "core/pool_vector.h"
#include "core/print_string.h"
#include "core/variant.h"
#include "core/variant_parser.h"
#include "core/translation_helpers.h"

#include "../mono_gd/gd_mono_cache.h"
#include "../mono_gd/gd_mono_utils.h"
#include "EASTL/unique_ptr.h"

MonoObject *godot_icall_GD_bytes2var(MonoArray *p_bytes, MonoBoolean p_allow_objects) {
    Variant ret;
    PoolByteArray varr = GDMonoMarshal::mono_array_to_pool_vec<uint8_t>(p_bytes);
    PoolByteArray::Read r = varr.read();
    Error err = decode_variant(ret, r.ptr(), varr.size(), nullptr, p_allow_objects);
    if (err != OK) {
        ret = RTR("Not enough bytes for decoding bytes, or invalid format.");
    }
    return GDMonoMarshal::variant_to_mono_object(ret);
}

MonoObject *godot_icall_GD_convert(MonoObject *p_what, int32_t p_type) {
    Variant what = GDMonoMarshal::mono_object_to_variant(p_what);
    Callable::CallError ce;
    Variant ret = Variant::construct(VariantType(p_type), what, ce);
    ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
    return GDMonoMarshal::variant_to_mono_object(ret);
}

int godot_icall_GD_hash(MonoObject *p_var) {
    return GDMonoMarshal::mono_object_to_variant(p_var).hash();
}

MonoObject *godot_icall_GD_instance_from_id(uint64_t p_instance_id) {
    return GDMonoUtils::unmanaged_get_managed(object_for_entity(GE(p_instance_id)));
}

void godot_icall_GD_print(MonoArray *p_what) {
    String str;
    const uintptr_t length = mono_array_length(p_what);

    for (int i = 0; i < length; i++) {
        MonoObject *elem = mono_array_get(p_what, MonoObject *, i);

        MonoException *exc = nullptr;
        String elem_str = GDMonoMarshal::mono_object_to_variant_string(elem, &exc);

        if (exc) {
            GDMonoUtils::set_pending_exception(exc);
            return;
        }

        str += elem_str;
    }

    print_line(str);
}

void godot_icall_GD_printerr(MonoArray *p_what) {

    String str;
    const uintptr_t length = mono_array_length(p_what);

    for (int i = 0; i < length; i++) {
        MonoObject *elem = mono_array_get(p_what, MonoObject *, i);

        MonoException *exc = nullptr;
        String elem_str = GDMonoMarshal::mono_object_to_variant_string(elem, &exc);

        if (exc) {
            GDMonoUtils::set_pending_exception(exc);
            return;
        }

        str += elem_str;
    }

    print_error(str);
}

void godot_icall_GD_printraw(MonoArray *p_what) {
    String str;
    const uintptr_t length = mono_array_length(p_what);

    for (int i = 0; i < length; i++) {
        MonoObject *elem = mono_array_get(p_what, MonoObject *, i);

        MonoException *exc = nullptr;
        String elem_str = GDMonoMarshal::mono_object_to_variant_string(elem, &exc);

        if (exc) {
            GDMonoUtils::set_pending_exception(exc);
            return;
        }

        str += elem_str;
    }

    OS::get_singleton()->print(str);
}

void godot_icall_GD_prints(MonoArray *p_what) {
    String str;
    const uintptr_t length = mono_array_length(p_what);

    for (int i = 0; i < length; i++) {
        MonoObject *elem = mono_array_get(p_what, MonoObject *, i);

        MonoException *exc = nullptr;
        String elem_str = GDMonoMarshal::mono_object_to_variant_string(elem, &exc);

        if (exc) {
            GDMonoUtils::set_pending_exception(exc);
            return;
        }

        if (i) {
            str += " ";
        }

        str += elem_str;
    }

    print_line(str);
}

void godot_icall_GD_printt(MonoArray *p_what) {
    String str;
    const uintptr_t length = mono_array_length(p_what);

    for (uintptr_t i = 0; i < length; i++) {
        MonoObject *elem = mono_array_get(p_what, MonoObject *, i);

        MonoException *exc = nullptr;
        String elem_str = GDMonoMarshal::mono_object_to_variant_string(elem, &exc);

        if (exc) {
            GDMonoUtils::set_pending_exception(exc);
            return;
        }

        if (i) {
            str += "\t";
        }

        str += elem_str;
    }

    print_line(str);
}

float godot_icall_GD_randf() {
    return Math::randf();
}

uint32_t godot_icall_GD_randi() {
    return Math::rand();
}

void godot_icall_GD_randomize() {
    Math::randomize();
}

double godot_icall_GD_rand_range(double from, double to) {
    return Math::random(from, to);
}

uint32_t godot_icall_GD_rand_seed(uint64_t seed, uint64_t *newSeed) {
    uint32_t ret = Math::rand_from_seed(&seed);
    *newSeed = seed;
    return ret;
}

void godot_icall_GD_seed(uint64_t p_seed) {
    Math::seed(p_seed);
}

MonoString *godot_icall_GD_str(MonoArray *p_what) {
    String str;
    Array what = GDMonoMarshal::mono_array_to_Array(p_what);

    for (int i = 0; i < what.size(); i++) {
        String os = what[i].as<String>();

        if (i == 0) {
            str = os;
        } else {
            str += os;
        }
    }

    return GDMonoMarshal::mono_string_from_godot(str);
}

MonoObject *godot_icall_GD_str2var(MonoString *p_str) {
    Variant ret;

    auto ss= eastl::unique_ptr<VariantParserStream,wrap_deleter>(VariantParser::get_string_stream(eastl::move(GDMonoMarshal::mono_string_to_godot(p_str))));

    String errs;
    int line;
    Error err = VariantParser::parse(ss.get(), ret, errs, line);
    if (err != OK) {
        String err_str = "Parse error at line " + itos(line) + ": " + errs + ".";
        ERR_PRINT(err_str);
        ret = err_str;
    }

    return GDMonoMarshal::variant_to_mono_object(ret);
}

MonoBoolean godot_icall_GD_type_exists(StringName *p_type) {
    StringName type = p_type ? *p_type : StringName();
    return ClassDB::class_exists(type);
}

void godot_icall_GD_pusherror(MonoString *p_str) {
    ERR_PRINT(GDMonoMarshal::mono_string_to_godot(p_str));
}

void godot_icall_GD_pushwarning(MonoString *p_str) {
    WARN_PRINT(GDMonoMarshal::mono_string_to_godot(p_str));
}

MonoArray *godot_icall_GD_var2bytes(MonoObject *p_var, MonoBoolean p_full_objects) {
    Variant var = GDMonoMarshal::mono_object_to_variant(p_var);

    PoolByteArray barr;
    int len;
    Error err = encode_variant(var, nullptr, len, p_full_objects);
    ERR_FAIL_COND_V_MSG(err != OK, nullptr, "Unexpected error encoding variable to bytes, likely unserializable type found (Object or RID).");

    barr.resize(len);
    {
        PoolByteArray::Write w = barr.write();
        encode_variant(var, w.ptr(), len, p_full_objects);
    }

    return GDMonoMarshal::container_to_mono_array(barr);
}

MonoString *godot_icall_GD_var2str(MonoObject *p_var) {
    String vars;
    VariantWriter::write_to_string(GDMonoMarshal::mono_object_to_variant(p_var), vars);
    return GDMonoMarshal::mono_string_from_godot(vars);
}

uint32_t godot_icall_TypeToVariantType(MonoReflectionType *p_refl_type) {
    return (uint32_t)GDMonoMarshal::managed_to_variant_type(ManagedType::from_reftype(p_refl_type));
}

MonoObject *godot_icall_DefaultGodotTaskScheduler() {
    return GDMonoCache::cached_data.task_scheduler_handle->get_target();
}

void godot_register_gd_icalls() {
    mono_add_internal_call("Godot.GD::godot_icall_GD_bytes2var", (void *)godot_icall_GD_bytes2var);
    mono_add_internal_call("Godot.GD::godot_icall_GD_convert", (void *)godot_icall_GD_convert);
    mono_add_internal_call("Godot.GD::godot_icall_GD_hash", (void *)godot_icall_GD_hash);
    mono_add_internal_call("Godot.GD::godot_icall_GD_instance_from_id", (void *)godot_icall_GD_instance_from_id);
    mono_add_internal_call("Godot.GD::godot_icall_GD_pusherror", (void *)godot_icall_GD_pusherror);
    mono_add_internal_call("Godot.GD::godot_icall_GD_pushwarning", (void *)godot_icall_GD_pushwarning);
    mono_add_internal_call("Godot.GD::godot_icall_GD_print", (void *)godot_icall_GD_print);
    mono_add_internal_call("Godot.GD::godot_icall_GD_printerr", (void *)godot_icall_GD_printerr);
    mono_add_internal_call("Godot.GD::godot_icall_GD_printraw", (void *)godot_icall_GD_printraw);
    mono_add_internal_call("Godot.GD::godot_icall_GD_prints", (void *)godot_icall_GD_prints);
    mono_add_internal_call("Godot.GD::godot_icall_GD_printt", (void *)godot_icall_GD_printt);
    mono_add_internal_call("Godot.GD::godot_icall_GD_randf", (void *)godot_icall_GD_randf);
    mono_add_internal_call("Godot.GD::godot_icall_GD_randi", (void *)godot_icall_GD_randi);
    mono_add_internal_call("Godot.GD::godot_icall_GD_randomize", (void *)godot_icall_GD_randomize);
    mono_add_internal_call("Godot.GD::godot_icall_GD_rand_range", (void *)godot_icall_GD_rand_range);
    mono_add_internal_call("Godot.GD::godot_icall_GD_rand_seed", (void *)godot_icall_GD_rand_seed);
    mono_add_internal_call("Godot.GD::godot_icall_GD_seed", (void *)godot_icall_GD_seed);
    mono_add_internal_call("Godot.GD::godot_icall_GD_str", (void *)godot_icall_GD_str);
    mono_add_internal_call("Godot.GD::godot_icall_GD_str2var", (void *)godot_icall_GD_str2var);
    mono_add_internal_call("Godot.GD::godot_icall_GD_type_exists", (void *)godot_icall_GD_type_exists);
    mono_add_internal_call("Godot.GD::godot_icall_GD_var2bytes", (void *)godot_icall_GD_var2bytes);
    mono_add_internal_call("Godot.GD::godot_icall_GD_var2str", (void *)godot_icall_GD_var2str);
    mono_add_internal_call("Godot.GD::godot_icall_TypeToVariantType", (void *)godot_icall_TypeToVariantType);

    // Dispatcher
    mono_add_internal_call("Godot.Dispatcher::godot_icall_DefaultGodotTaskScheduler", (void *)godot_icall_DefaultGodotTaskScheduler);
}
