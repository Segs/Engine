/*************************************************************************/
/*  engine.cpp                                                           */
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

#include "engine.h"

#include "engine_tooling.h"
#include "core/reference.h"
#include "core/dictionary.h"
#include "core/print_string.h"
#include "core/variant.h"
#include "core/ustring.h"
#include "core/string.h"
#include "authors.gen.h"
#include "donors.gen.h"
#include "license.gen.h"
#include "core/version.h"
#include "core/version_generated.gen.h"

void Engine::set_iterations_per_second(int p_ips) {

    ERR_FAIL_COND_MSG(p_ips <= 0, "Engine iterations per second must be greater than 0.");
    ips = p_ips;
}
int Engine::get_iterations_per_second() const {

    return ips;
}

void Engine::set_physics_jitter_fix(float p_threshold) {
    if (p_threshold < 0) {
        p_threshold = 0;
    }
    physics_jitter_fix = p_threshold;
}

float Engine::get_physics_jitter_fix() const {
    return physics_jitter_fix;
}

void Engine::set_target_fps(int p_fps) {
    _target_fps = p_fps > 0 ? p_fps : 0;
}

int Engine::get_target_fps() const {
    return _target_fps;
}

uint64_t Engine::get_frames_drawn() {

    return frames_drawn;
}

void Engine::set_frame_delay(uint32_t p_msec) {

    _frame_delay = p_msec;
}

uint32_t Engine::get_frame_delay() const {

    return _frame_delay;
}

void Engine::set_print_error_messages(bool p_enabled)
{
    _print_error_enabled = p_enabled;
}

bool Engine::is_printing_error_messages() const
{
    return _print_error_enabled;
}
void Engine::set_time_scale(float p_scale) {

    _time_scale = p_scale;
}

float Engine::get_time_scale() const {

    return _time_scale;
}

void Engine::set_portals_active(bool p_active) {
    _portals_active = p_active;
}

void Engine::set_editor_hint(bool p_enabled) {
    EngineTooling::set_editor_hint(p_enabled);
}

bool Engine::is_editor_hint() const {
    return EngineTooling::is_editor_hint();
}

Dictionary Engine::get_version_info() const {
    Dictionary dict;
    dict["major"] = VERSION_MAJOR;
    dict["minor"] = VERSION_MINOR;
    dict["patch"] = VERSION_PATCH;
    dict["hex"] = VERSION_HEX;
    dict["status"] = VERSION_STATUS;
    dict["build"] = VERSION_BUILD;
    dict["year"] = VERSION_YEAR;

    String hash(VERSION_HASH);
    dict["hash"] = hash.length() == 0 ? Variant("unknown") : Variant(hash);

    String stringver = dict["major"].as<String>() + "." + dict["minor"].as<String>();
    if (dict["patch"].as<int>() != 0) {
        stringver += "." + dict["patch"].as<String>();
    }
    stringver += "-" + dict["status"].as<String>() + " (" + dict["build"].as<String>() + ")";
    dict["string"] = Variant(stringver);

    return dict;
}

static Array array_from_info(const char *const *info_list) {
    Array arr;
    for (int i = 0; info_list[i] != nullptr; i++) {
        arr.push_back(info_list[i]);
    }
    return arr;
}

static Array array_from_info_count(const char *const *info_list, int info_count) {
    Array arr;
    for (int i = 0; i < info_count; i++) {
        arr.push_back(info_list[i]);
    }
    return arr;
}

Dictionary Engine::get_author_info() const {
    Dictionary dict;

    dict["lead_developers"] = array_from_info(AUTHORS_LEAD_DEVELOPERS);
    dict["project_managers"] = array_from_info(AUTHORS_PROJECT_MANAGERS);
    dict["founders"] = array_from_info(AUTHORS_FOUNDERS);
    dict["developers"] = array_from_info(AUTHORS_DEVELOPERS);

    return dict;
}

Array Engine::get_copyright_info() const {
    Array components;
    for (const ComponentCopyright &cp_info : COPYRIGHT_INFO) {
        Dictionary component_dict;
        component_dict["name"] = cp_info.name;
        Array parts;
        for (int i = 0; i < cp_info.part_count; i++) {
            const ComponentCopyrightPart &cp_part = cp_info.parts[i];
            Dictionary part_dict;
            part_dict["files"] = array_from_info_count(cp_part.files, cp_part.file_count);
            part_dict["copyright"] = array_from_info_count(cp_part.copyright_statements, cp_part.copyright_count);
            part_dict["license"] = cp_part.license;
            parts.push_back(part_dict);
        }
        component_dict["parts"] = parts;

        components.push_back(component_dict);
    }
    return components;
}

Dictionary Engine::get_donor_info() const {
    Dictionary donors;
    donors["platinum_sponsors"] = array_from_info(DONORS_SPONSOR_PLATINUM);
    donors["gold_sponsors"] = array_from_info(DONORS_SPONSOR_GOLD);
    donors["silver_sponsors"] = array_from_info(DONORS_SPONSOR_SILVER);
    donors["bronze_sponsors"] = array_from_info(DONORS_SPONSOR_BRONZE);
    donors["mini_sponsors"] = array_from_info(DONORS_SPONSOR_MINI);
    donors["gold_donors"] = array_from_info(DONORS_GOLD);
    donors["silver_donors"] = array_from_info(DONORS_SILVER);
    donors["bronze_donors"] = array_from_info(DONORS_BRONZE);
    return donors;
}

Dictionary Engine::get_license_info() const {
    Dictionary licenses;
    for (int i = 0; i < LICENSE_COUNT; i++) {
        licenses[StaticCString(LICENSE_NAMES[i],true)] = LICENSE_BODIES[i];
    }
    return licenses;
}

String Engine::get_license_text() const {
    return String(GODOT_LICENSE_TEXT);
}

void Engine::add_singleton(const Singleton &p_singleton) {

    singletons.emplace_back(p_singleton);
    singleton_ptrs[p_singleton.name] = p_singleton.ptr;
}

Object *Engine::get_named_singleton(const StringName &p_name) const {
    auto E = singleton_ptrs.find(p_name);
    ERR_FAIL_COND_V_MSG(E==singleton_ptrs.end(), nullptr, "Failed to retrieve non-existent singleton '" + String(p_name) + "'.");
    return E->second;
}

bool Engine::has_singleton(const StringName &p_name) const {
    return singleton_ptrs.contains(p_name);
}
Engine *Engine::singleton = nullptr;

Engine *Engine::get_singleton() {
    return singleton;
}

Engine::Engine() {

    singleton = this;
}

#include "service_interfaces/CoreInterface.h"
#include "core/io/file_access_memory.h"
class CoreInterfaceImpl final : public CoreInterface
{
    // CoreInterface interface
public:
    FileAccess * wrapMemoryAsFileAccess(const uint8_t *data,int sz) final
    {
        auto result = new FileAccessMemory;
        if(OK!=result->open_custom(data,sz))
        {
            delete result;
            result = nullptr;
        }
        return result;
    }
    void  releaseFileAccess(FileAccess *d) final
    {
        delete d;
    }
    void reportError(StringView msg, const char *retval, const char *funcstr,const char *file, int line) override
    {
        _err_print_error(funcstr, file, line, String("Method/Function Failed, returning: ")+ retval,msg);
    }
    void clearLastError() override
    {
    }

    // CoreInterface interface
public:
    void fillVersion(uint32_t &major, uint32_t &minor, uint32_t &patch) override
    {
        major = VERSION_MAJOR;
        minor = VERSION_MINOR;
#ifdef VERSION_PATCH
        patch = VERSION_PATCH;
#else
        patch = 0;
#endif
    }
};
extern CoreInterface *getCoreInterface() {
    static CoreInterfaceImpl impl;
    return &impl;
}

Engine::Singleton::Singleton(StringName p_name, Object *p_ptr) :
    name(eastl::move(p_name)),
    ptr(p_ptr) {
#ifdef DEBUG_ENABLED
    RefCounted *ref = object_cast<RefCounted>(p_ptr);
    if (ref && !ref->is_referenced()) {
        WARN_PRINT("You must use Ref<> to ensure the lifetime of a Reference object intended to be used as a singleton.");
    }
#endif
}
