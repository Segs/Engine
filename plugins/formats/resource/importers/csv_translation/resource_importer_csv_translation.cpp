/*************************************************************************/
/*  resource_importer_csv_translation.cpp                                */
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

#include "resource_importer_csv_translation.h"

#include "core/compressed_translation.h"
#include "core/io/resource_saver.h"
#include "core/os/file_access.h"
#include "core/resource/resource_manager.h"
#include "core/string_utils.h"
#include "core/translation.h"

const char *ResourceImporterCSVTranslation::get_importer_name() const {

    return "csv_translation";
}

const char *ResourceImporterCSVTranslation::get_visible_name() const {

    return "CSV Translation";
}
void ResourceImporterCSVTranslation::get_recognized_extensions(Vector<String> &p_extensions) const {

    p_extensions.push_back("csv");
}

StringName ResourceImporterCSVTranslation::get_save_extension() const {
    return ""; //does not save a single resource
}

StringName ResourceImporterCSVTranslation::get_resource_type() const {

    return "Translation";
}

bool ResourceImporterCSVTranslation::get_option_visibility(
        const StringName &p_option, const HashMap<StringName, Variant> &p_options) const {

    return true;
}

int ResourceImporterCSVTranslation::get_preset_count() const {
    return 0;
}
StringName ResourceImporterCSVTranslation::get_preset_name(int p_idx) const {

    return "";
}

void ResourceImporterCSVTranslation::get_import_options(Vector<ImportOption> *r_options, int p_preset) const {

    r_options->push_back(ImportOption(PropertyInfo(VariantType::BOOL, "compress"), true));
    r_options->push_back(
            ImportOption(PropertyInfo(VariantType::INT, "delimiter", PropertyHint::Enum, "Comma,Semicolon,Tab"), 0));
}

Error ResourceImporterCSVTranslation::import(StringView p_source_file, StringView p_save_path,
        const HashMap<StringName, Variant> &p_options, Vector<String> &r_missing_deps,
    Vector<String> *r_platform_variants, Vector<String> *r_gen_files, Variant *r_metadata) {

    bool compress = p_options.at("compress").as<bool>();

    char delimiter;
    switch (p_options.at("delimiter").as<int>()) {
        case 0:
            delimiter = ',';
            break;
        case 1:
            delimiter = ';';
            break;
        case 2:
            delimiter = '\t';
            break;
    }

    FileAccessRef f = FileAccess::open(p_source_file, FileAccess::READ);

    ERR_FAIL_COND_V_MSG(!f, ERR_INVALID_PARAMETER, "Cannot open file from path '" + p_source_file + "'.");

    Vector<String> line = f->get_csv_line(delimiter);
    ERR_FAIL_COND_V(line.size() <= 1, ERR_PARSE_ERROR);

    Vector<String> locales;
    Vector<Ref<Translation> > translations;

    for (int i = 1; i < line.size(); i++) {

       String locale = TranslationServer::get_singleton()->standardize_locale(line[i]);

        locales.push_back(String(locale));
        Ref<Translation> translation=make_ref_counted<Translation>();
        translation->set_locale(locale);
        translations.emplace_back(eastl::move(translation));
    }

    line = f->get_csv_line(delimiter);

    while (line.size() == locales.size() + 1) {

        StringView  key = line[0];
        if (!key.empty()) {

            for (int i = 1; i < line.size(); i++) {
                translations[i - 1]->add_message(StringName(key), StringName(StringUtils::c_unescape(line[i])));
            }
        }

        line = f->get_csv_line(delimiter);
    }

    for (auto & translation : translations) {
        Ref<Translation> xlt = translation;

        if (compress) {
            Ref cxl(make_ref_counted<PHashTranslation>());
            cxl->generate(xlt);
            xlt = cxl;
        }

        String save_path =
                String(PathUtils::get_basename(p_source_file)) + "." + translation->get_locale() + ".translation";

        gResourceManager().save(save_path, xlt);
        if (r_gen_files) {
            r_gen_files->push_back(save_path);
        }
    }

    return OK;
}

ResourceImporterCSVTranslation::ResourceImporterCSVTranslation() {}
