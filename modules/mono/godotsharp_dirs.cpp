/*************************************************************************/
/*  godotsharp_dirs.cpp                                                  */
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

#include "godotsharp_dirs.h"

#include "core/os/dir_access.h"
#include "core/os/os.h"
#include "core/project_settings.h"
#include "core/string.h"
#include "core/string_utils.h"

#ifdef TOOLS_ENABLED
#include "core/version.h"
#include "editor/editor_settings.h"
#endif

#ifdef ANDROID_ENABLED
#include "mono_gd/gd_mono_android.h"
#endif

#include "mono_gd/gd_mono.h"

namespace GodotSharpDirs {

String _get_expected_build_config() {
#ifdef TOOLS_ENABLED
    return "Debug";
#else

#ifdef DEBUG_ENABLED
    return "ExportDebug";
#else
    return "ExportRelease";
#endif

#endif
}

String _get_mono_user_dir() {
    using namespace PathUtils;
#ifdef TOOLS_ENABLED
    if (EditorSettings::get_singleton()) {
        return plus_file(EditorSettings::get_singleton()->get_data_dir(),"mono");
    } else {
        String settings_path;

        String exe_dir = get_base_dir(OS::get_singleton()->get_executable_path());
        DirAccessRef d = DirAccess::create_for_path(exe_dir);

        if (d->file_exists("._sc_") || d->file_exists("_sc_")) {
            // contain yourself
            settings_path = plus_file(exe_dir,"editor_data");
        } else {
            settings_path = OS::get_singleton()->get_data_path(); //plus_file(,OS::get_singleton()->get_godot_dir_name());
        }

        return plus_file(settings_path,"mono");
    }
#else
    return plus_file(OS::get_singleton()->get_user_data_dir(),"mono");
#endif
}
static String get_parent_dir_for_buildfile(ProjectSettings *settings, const char *project_filetype) {
    String parent_dir = settings->getT<String>(StringName(String("mono/project/") + project_filetype));
    if (parent_dir.empty()) {
        parent_dir = "res://";
    }
    return settings->globalize_path(parent_dir);
}

class _GodotSharpDirs {

public:
    String res_data_dir;
    String res_metadata_dir;
    String res_assemblies_base_dir;
    String res_assemblies_dir;
    String res_config_dir;
    String res_temp_dir;
    String res_temp_assemblies_base_dir;
    String res_temp_assemblies_dir;
    String mono_user_dir;
    String mono_logs_dir;

#ifdef TOOLS_ENABLED
    String mono_solutions_dir;
    String build_logs_dir;

    String project_assembly_name;
    String cmake_filepath;
    String sln_filepath;
    String csproj_filepath;

    String data_editor_tools_dir;
    String data_editor_prebuilt_api_dir;
#else
    // Equivalent of res_assemblies_dir, but in the data directory rather than in 'res://'.
    // Only defined on export templates. Used when exporting assemblies outside of PCKs.
    String data_game_assemblies_dir;
#endif

    String data_mono_etc_dir;
    String data_mono_lib_dir;

#ifdef WINDOWS_ENABLED
    String data_mono_bin_dir;
#endif
    _GodotSharpDirs(const _GodotSharpDirs &) = delete;
    _GodotSharpDirs &operator=(const _GodotSharpDirs &)=delete;

private:
    _GodotSharpDirs() {
        using namespace PathUtils;
        res_data_dir = "res://.csharp";
        res_metadata_dir = plus_file(res_data_dir,"metadata");
        res_assemblies_base_dir = plus_file(res_data_dir,"assemblies");
        res_assemblies_dir = plus_file(res_assemblies_base_dir,GDMono::get_expected_api_build_config());
        res_config_dir = plus_file(plus_file(res_data_dir,"etc"),"mono");

        // TODO use paths from csproj
        res_temp_dir = plus_file(res_data_dir,"temp");
        res_temp_assemblies_base_dir = plus_file(res_temp_dir,"bin");
        res_temp_assemblies_dir = plus_file(res_temp_assemblies_base_dir,_get_expected_build_config());

        mono_user_dir = _get_mono_user_dir();
        mono_logs_dir = plus_file(mono_user_dir,"mono_logs");

        ProjectSettings * settings = ProjectSettings::get_singleton();
        String appname = settings->get("application/config/name").as<String>();
#ifdef TOOLS_ENABLED
        mono_solutions_dir = plus_file(mono_user_dir,"solutions");
        build_logs_dir = plus_file(mono_user_dir,"build_logs");

        GLOBAL_DEF("mono/project/assembly_name", "");
        GLOBAL_DEF("mono/project/solution_directory", "");
        GLOBAL_DEF("mono/project/c#_project_directory", "");


        String appname_safe = OS::get_singleton()->get_safe_dir_name(appname);
        if (appname_safe.empty()) {
            appname_safe = "UnnamedProject";
        }

        project_assembly_name = settings->getT<String>("mono/project/assembly_name");
        if (project_assembly_name.empty()) {
            project_assembly_name = appname_safe;
            settings->set("mono/project/assembly_name", project_assembly_name);
        }
        String sln_parent_dir = get_parent_dir_for_buildfile(settings, "solution_directory");
        String csproj_parent_dir = get_parent_dir_for_buildfile(settings, "c#_project_directory");
        String cmake_parent_dir = get_parent_dir_for_buildfile(settings, "cmake_project_directory");

        sln_filepath = plus_file(sln_parent_dir,project_assembly_name + ".sln");
        csproj_filepath = plus_file(csproj_parent_dir,project_assembly_name + ".csproj");
        cmake_filepath = plus_file(cmake_parent_dir, "CMakeLists.txt");
#endif

        String exe_dir(path(from_native_path(OS::get_singleton()->get_executable_path())));

        String data_dir_root = plus_file(exe_dir,"csharp");
        String data_mono_root_dir = plus_file(data_dir_root,"mono");
#ifdef TOOLS_ENABLED
        data_editor_tools_dir = plus_file(data_dir_root,"assemblies");
        data_editor_prebuilt_api_dir = plus_file(data_dir_root,"assemblies");
#ifdef OSX_ENABLED
        if (!DirAccess::exists(data_editor_tools_dir)) {
            data_editor_tools_dir = plus_file(exe_dir,"../Resources/GodotSharp/Tools");
        }

        if (!DirAccess::exists(data_editor_prebuilt_api_dir)) {
            data_editor_prebuilt_api_dir = plus_file(exe_dir,"../Resources/GodotSharp/Api");
        }
#endif
#else

        data_game_assemblies_dir = plus_file(data_dir_root,"assemblies");
#ifdef OSX_ENABLED
        if (!DirAccess::exists(data_game_assemblies_dir)) {
            data_game_assemblies_dir = exe_dir.plus_file("../Resources/GodotSharp/Assemblies");
        }
#endif

#endif
#ifdef WINDOWS_ENABLED
        data_mono_bin_dir = plus_file(data_mono_root_dir,"bin");
#endif
        data_mono_etc_dir = plus_file(data_mono_root_dir,"etc");
        data_mono_lib_dir = plus_file(data_mono_root_dir,"lib");
#ifdef OSX_ENABLED
        if (!DirAccess::exists(data_mono_root_dir)) {
            data_mono_etc_dir = plus_file(exe_dir,"../Resources/GodotSharp/Mono/etc");
            data_mono_lib_dir = plus_file(exe_dir,"../Resources/GodotSharp/Mono/lib");
        }
#endif
    }
public:
    static _GodotSharpDirs &get_singleton() {
        static _GodotSharpDirs singleton;
        return singleton;
    }
};

String get_res_data_dir() {
    return _GodotSharpDirs::get_singleton().res_data_dir;
}

String get_res_metadata_dir() {
    return _GodotSharpDirs::get_singleton().res_metadata_dir;
}

String get_res_assemblies_base_dir() {
    return _GodotSharpDirs::get_singleton().res_assemblies_base_dir;
}

String get_res_assemblies_dir() {
    return _GodotSharpDirs::get_singleton().res_assemblies_dir;
}

String get_res_config_dir() {
    return _GodotSharpDirs::get_singleton().res_config_dir;
}

String get_res_temp_dir() {
    return _GodotSharpDirs::get_singleton().res_temp_dir;
}

String get_res_temp_assemblies_base_dir() {
    return _GodotSharpDirs::get_singleton().res_temp_assemblies_base_dir;
}

String get_res_temp_assemblies_dir() {
    return _GodotSharpDirs::get_singleton().res_temp_assemblies_dir;
}

String get_mono_user_dir() {
    return _GodotSharpDirs::get_singleton().mono_user_dir;
}

String get_mono_logs_dir() {
    return _GodotSharpDirs::get_singleton().mono_logs_dir;
}

#ifdef TOOLS_ENABLED
String get_mono_solutions_dir() {
    return _GodotSharpDirs::get_singleton().mono_solutions_dir;
}

String get_build_logs_dir() {
    return _GodotSharpDirs::get_singleton().build_logs_dir;
}

String get_project_assembly_name() {
    return _GodotSharpDirs::get_singleton().project_assembly_name;
}

String get_project_sln_path() {
    return _GodotSharpDirs::get_singleton().sln_filepath;
}

String get_project_csproj_path() {
    return _GodotSharpDirs::get_singleton().csproj_filepath;
}

String get_data_editor_tools_dir() {
    return _GodotSharpDirs::get_singleton().data_editor_tools_dir;
}

String get_data_editor_prebuilt_api_dir() {
    return _GodotSharpDirs::get_singleton().data_editor_prebuilt_api_dir;
}
#else
String get_data_game_assemblies_dir() {
    return _GodotSharpDirs::get_singleton().data_game_assemblies_dir;
}
#endif

String get_data_mono_etc_dir() {
    return _GodotSharpDirs::get_singleton().data_mono_etc_dir;
}

String get_data_mono_lib_dir() {
    return _GodotSharpDirs::get_singleton().data_mono_lib_dir;
}

#ifdef WINDOWS_ENABLED
String get_data_mono_bin_dir() {
    return _GodotSharpDirs::get_singleton().data_mono_bin_dir;
}
#endif

} // namespace GodotSharpDirs
