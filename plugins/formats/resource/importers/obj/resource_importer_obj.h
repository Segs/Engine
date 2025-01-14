/*************************************************************************/
/*  resource_importer_obj.h                                              */
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

#include "core/plugin_interfaces/PluginDeclarations.h"
#include "editor/plugin_interfaces/PluginDeclarations.h"

class ResourceImporterOBJ : public QObject, public EditorSceneImporterInterface,public ResourceImporterInterface {
    Q_PLUGIN_METADATA(IID "org.segs_engine.OBJImporter")
    Q_INTERFACES(EditorSceneImporterInterface ResourceImporterInterface)
    Q_OBJECT

    //EditorSceneImporterInterface
public:
    uint32_t get_import_flags() const override;
    void get_extensions(Vector<String> &r_extensions) const override;
    Node *import_scene(StringView p_path, uint32_t p_flags, int p_bake_fps, uint32_t p_compress_flags, Vector<String> *r_missing_deps, Error *r_err = nullptr) override;
    Ref<Animation> import_animation(StringView p_path, uint32_t p_flags, int p_bake_fps) override;

    // ResourceImporterInterface
public:
    const char *get_importer_name() const override;
    const char *get_visible_name() const override;
    void get_recognized_extensions(Vector<String> &p_extensions) const override;
    StringName get_save_extension() const override;
    StringName get_resource_type() const override;

    int get_preset_count() const override;
    StringName get_preset_name(int p_idx) const override;

    void get_import_options(Vector<ImportOption> *r_options, int p_preset = 0) const override;
    bool get_option_visibility(const StringName &p_option, const HashMap<StringName, Variant> &p_options) const override;

    Error import(StringView p_source_file, StringView p_save_path, const HashMap<StringName, Variant> &p_options, Vector<String> &r_missing_deps,
            Vector<String> *r_platform_variants, Vector<String> *r_gen_files = nullptr,
            Variant *r_metadata = nullptr) override;

    // ResourceImporterInterface defaults
public:
    float get_priority() const override {return 6.0f;}
    int get_import_order() const override {return 0;}
    StringName get_option_group_file() const override { return StringName(); }
    Error import_group_file(StringView /*p_group_file*/, const Map<String, HashMap<StringName, Variant> > & /*p_source_file_options*/, const Map<String, String> & /*p_base_paths*/) override {
        return ERR_UNAVAILABLE;
    }
    bool are_import_settings_valid(StringView  /*p_path*/) const override { return true; }
    String get_import_settings_string() const override { return String(); }

public:
    ResourceImporterOBJ();

};
