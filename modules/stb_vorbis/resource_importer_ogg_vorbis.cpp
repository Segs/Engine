/*************************************************************************/
/*  resource_importer_ogg_vorbis.cpp                                     */
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

#include "resource_importer_ogg_vorbis.h"

#include "core/class_db.h"
#include "core/os/file_access.h"
#include "core/resource/resource_manager.h"

IMPL_GDCLASS(ResourceImporterOGGVorbis)

const char *ResourceImporterOGGVorbis::get_importer_name() const {

    return "ogg_vorbis";
}

const char *ResourceImporterOGGVorbis::get_visible_name() const {

    return "OGGVorbis";
}
void ResourceImporterOGGVorbis::get_recognized_extensions(Vector<String> &p_extensions) const {

    p_extensions.emplace_back("ogg");
}

StringName ResourceImporterOGGVorbis::get_save_extension() const {
    return "oggstr";
}

StringName ResourceImporterOGGVorbis::get_resource_type() const {

    return "AudioStreamOGGVorbis";
}

bool ResourceImporterOGGVorbis::get_option_visibility(const StringName &/*p_option*/, const HashMap<StringName, Variant> &/*p_options*/) const {

    return true;
}

int ResourceImporterOGGVorbis::get_preset_count() const {
    return 0;
}
StringName ResourceImporterOGGVorbis::get_preset_name(int p_idx) const {

    return StringName();
}

void ResourceImporterOGGVorbis::get_import_options(Vector<ResourceImporterInterface::ImportOption> *r_options, int p_preset) const {

    r_options->push_back(ImportOption(PropertyInfo(VariantType::BOOL, "loop"), true));
    r_options->push_back(ImportOption(PropertyInfo(VariantType::FLOAT, "loop_offset"), 0));
}

Error ResourceImporterOGGVorbis::import(StringView p_source_file, StringView p_save_path, const HashMap<StringName, Variant> &p_options, Vector<String> &r_missing_deps,
                                        Vector<String> *r_platform_variants, Vector<String> *r_gen_files, Variant *r_metadata) {

    bool loop = p_options.at("loop").as<bool>();
    float loop_offset = p_options.at("loop_offset").as<float>();

    FileAccess *f = FileAccess::open(p_source_file, FileAccess::READ);

    ERR_FAIL_COND_V_MSG(!f, ERR_CANT_OPEN, "Cannot open file '" + String(p_source_file) + "'.");

    int len = int(f->get_len());

    PoolVector<uint8_t> data;
    data.resize(len);
    PoolVector<uint8_t>::Write w = data.write();

    f->get_buffer(w.ptr(), len);

    memdelete(f);

    Ref<AudioStreamOGGVorbis> ogg_stream(make_ref_counted<AudioStreamOGGVorbis>());

    ogg_stream->set_data(data);
    ERR_FAIL_COND_V_MSG(!ogg_stream->get_data().size(), ERR_FILE_CORRUPT, String("Couldn't import file as AudioStreamOGGVorbis: ") + p_source_file);
    ogg_stream->set_loop(loop);
    ogg_stream->set_loop_offset(loop_offset);

    return gResourceManager().save(String(p_save_path) + ".oggstr", ogg_stream);
}

ResourceImporterOGGVorbis::ResourceImporterOGGVorbis() {
}
