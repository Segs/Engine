/*************************************************************************/
/*  baked_lightmap_editor_plugin.cpp                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "baked_lightmap_editor_plugin.h"

#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "core/string_formatter.h"
#include "core/translation_helpers.h"
#include "editor/editor_file_dialog.h"
#include "scene/main/scene_tree.h"

IMPL_GDCLASS(BakedLightmapEditorPlugin)

void BakedLightmapEditorPlugin::_bake_select_file(StringView p_file) {

    if (!lightmap)
        return;

    BakedLightmap::BakeError err;
    if (get_tree()->get_edited_scene_root() && get_tree()->get_edited_scene_root() == lightmap) {
        err = lightmap->bake(lightmap, p_file);
    } else {
        err = lightmap->bake(lightmap->get_parent(), p_file);
    }

    switch (err) {
        case BakedLightmap::BAKE_ERROR_NO_SAVE_PATH: {
            String scene_path(lightmap->get_filename());
            if (scene_path.empty()) {
                scene_path = lightmap->get_owner()->get_filename();
            }
            if (scene_path.empty()) {
                EditorNode::get_singleton()->show_warning(TTR("Can't determine a save path for lightmap images.\nSave your scene and try again."));
            break;
            }
            scene_path = String(PathUtils::get_basename(scene_path)) + ".lmbake";

            file_dialog->set_current_path(scene_path);
            file_dialog->popup_centered_ratio();

        } break;
        case BakedLightmap::BAKE_ERROR_NO_MESHES:
            EditorNode::get_singleton()->show_warning(TTR("No meshes to bake. Make sure they contain an UV2 channel and that the 'Use In Baked Light' and 'Generate Lightmap' flags are on."));
            break;
        case BakedLightmap::BAKE_ERROR_CANT_CREATE_IMAGE:
            EditorNode::get_singleton()->show_warning(TTR("Failed creating lightmap images, make sure path is writable."));
            break;
        case BakedLightmap::BAKE_ERROR_LIGHTMAP_SIZE:
            EditorNode::get_singleton()->show_warning(TTR("Failed determining lightmap size. Maximum lightmap size too small?"));
            break;
        case BakedLightmap::BAKE_ERROR_INVALID_MESH:
            EditorNode::get_singleton()->show_warning(TTR("Some mesh is invalid. Make sure the UV2 channel values are contained within the [0.0,1.0] square region."));
            break;
        case BakedLightmap::BAKE_ERROR_NO_LIGHTMAPPER:
            EditorNode::get_singleton()->show_warning(TTR("Godot editor was built without ray tracing support, lightmaps can't be baked."));
            break;
        default: {
        }
    }
}

void BakedLightmapEditorPlugin::_bake() {
    _bake_select_file("");
}
void BakedLightmapEditorPlugin::edit(Object *p_object) {

    BakedLightmap *s = object_cast<BakedLightmap>(p_object);
    if (!s)
        return;

    lightmap = s;
}

bool BakedLightmapEditorPlugin::handles(Object *p_object) const {

    return p_object->is_class("BakedLightmap");
}

void BakedLightmapEditorPlugin::make_visible(bool p_visible) {

    if (p_visible) {
        bake->show();
    } else {

        bake->hide();
    }
}

EditorProgress *BakedLightmapEditorPlugin::tmp_progress = nullptr;
EditorProgress *BakedLightmapEditorPlugin::tmp_subprogress = nullptr;

bool BakedLightmapEditorPlugin::bake_func_step(float p_progress, StringView p_description, void *, bool p_force_refresh) {
    if (!tmp_progress) {
        tmp_progress = memnew(EditorProgress("bake_lightmaps", TTR("Bake Lightmaps"), 1000, true));
        ERR_FAIL_COND_V(tmp_progress == nullptr, false);
    }
    return tmp_progress->step(p_description, p_progress * 1000, p_force_refresh);
}

bool BakedLightmapEditorPlugin::bake_func_substep(float p_progress,StringView p_description, void *, bool p_force_refresh) {
    if (!tmp_subprogress) {
        tmp_subprogress = memnew(EditorProgress("bake_lightmaps_substep", "", 1000, true));
        ERR_FAIL_COND_V(tmp_subprogress == nullptr, false);
    }
    return tmp_subprogress->step(p_description, p_progress * 1000, p_force_refresh);
}

void BakedLightmapEditorPlugin::bake_func_end(uint32_t p_time_started) {
    memdelete(tmp_progress);
    memdelete(tmp_subprogress);

    const int time_taken = (OS::get_singleton()->get_ticks_msec() - p_time_started) * 0.001;
    if (time_taken >= 1) {
        // Only print a message and request attention if baking lightmaps took at least 1 second.
        // Otherwise, attempting to bake in an erroneous situation (e.g. no meshes to bake)
        // would print the "done baking lightmaps" message and request attention for no good reason.
        print_line(FormatVE("Done baking lightmaps in %02d:%02d:%02d.", time_taken / 3600, (time_taken % 3600) / 60, time_taken % 60));

        // Request attention in case the user was doing something else.
        // Baking lightmaps is likely the editor task that can take the most time,
        // so only request the attention for baking lightmaps.
        OS::get_singleton()->request_attention();
    }
}

BakedLightmapEditorPlugin::BakedLightmapEditorPlugin(EditorNode *p_node) {

    editor = p_node;
    bake = memnew(ToolButton);
    bake->set_button_icon(editor->get_gui_base()->get_theme_icon("Bake", "EditorIcons"));
    bake->set_text(TTR("Bake Lightmaps"));
    bake->hide();
    bake->connect("pressed", callable_mp(this, &BakedLightmapEditorPlugin::_bake));

    file_dialog = memnew(EditorFileDialog);
    file_dialog->set_mode(EditorFileDialog::MODE_SAVE_FILE);
    file_dialog->add_filter("*.lmbake ; " + TTR("LightMap Bake"));
    file_dialog->set_title(TTR("Select lightmap bake file:"));
    file_dialog->connect("file_selected", callable_mp(this, &BakedLightmapEditorPlugin::_bake_select_file));
    bake->add_child(file_dialog);
    add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, bake);
    lightmap = nullptr;

    BakedLightmap::bake_step_function = bake_func_step;
    BakedLightmap::bake_substep_function = bake_func_substep;
    BakedLightmap::bake_end_function = bake_func_end;
}

BakedLightmapEditorPlugin::~BakedLightmapEditorPlugin() = default;
