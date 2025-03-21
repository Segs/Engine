/*************************************************************************/
/*  shader_editor_plugin.cpp                                             */
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

#include "shader_editor_plugin.h"

#include "core/io/resource_loader.h"
#include "core/callable_method_pointer.h"
#include "core/method_bind.h"
#include "core/object_tooling.h"
#include "core/string_formatter.h"
#include "core/resource/resource_tools.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/os/file_access.h"
#include "core/resource/resource_manager.h"
#include "core/translation_helpers.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "editor/property_editor.h"
#include "scene/resources/style_box.h"
#include "servers/rendering/shader_types.h"

/*** SHADER SCRIPT EDITOR ****/
IMPL_GDCLASS(ShaderTextEditor)
IMPL_GDCLASS(ShaderEditor)
IMPL_GDCLASS(ShaderEditorPlugin)

Ref<Shader> ShaderTextEditor::get_edited_shader() const {

    return shader;
}
void ShaderTextEditor::set_edited_shader(const Ref<Shader> &p_shader) {

    if (shader == p_shader) {
        return;
    }
    shader = p_shader;

    _load_theme_settings();

    get_text_edit()->set_text(p_shader->get_code());
    get_text_edit()->clear_undo_history();

    get_text_edit()->call_deferred("set_h_scroll", 0);
    get_text_edit()->call_deferred("set_v_scroll", 0);
    _validate_script();
    _line_col_changed();
}

void ShaderTextEditor::reload_text() {
    ERR_FAIL_COND(not shader);

    TextEdit *te = get_text_edit();
    int column = te->cursor_get_column();
    int row = te->cursor_get_line();
    int h = te->get_h_scroll();
    int v = te->get_v_scroll();

    te->set_text(shader->get_code());
    te->cursor_set_line(row);
    te->cursor_set_column(column);
    te->set_h_scroll(h);
    te->set_v_scroll(v);

    te->tag_saved_version();

    update_line_and_column();
}

void ShaderTextEditor::_load_theme_settings() {

    get_text_edit()->clear_colors();

    Color background_color = EDITOR_GET_T<Color>("text_editor/highlighting/background_color");
    Color completion_background_color = EDITOR_GET_T<Color>("text_editor/highlighting/completion_background_color");
    Color completion_selected_color = EDITOR_GET_T<Color>("text_editor/highlighting/completion_selected_color");
    Color completion_existing_color = EDITOR_GET_T<Color>("text_editor/highlighting/completion_existing_color");
    Color completion_scroll_color = EDITOR_GET_T<Color>("text_editor/highlighting/completion_scroll_color");
    Color completion_font_color = EDITOR_GET_T<Color>("text_editor/highlighting/completion_font_color");
    Color text_color = EDITOR_GET_T<Color>("text_editor/highlighting/text_color");
    Color line_number_color = EDITOR_GET_T<Color>("text_editor/highlighting/line_number_color");
    Color caret_color = EDITOR_GET_T<Color>("text_editor/highlighting/caret_color");
    Color caret_background_color = EDITOR_GET_T<Color>("text_editor/highlighting/caret_background_color");
    Color text_selected_color = EDITOR_GET_T<Color>("text_editor/highlighting/text_selected_color");
    Color selection_color = EDITOR_GET_T<Color>("text_editor/highlighting/selection_color");
    Color brace_mismatch_color = EDITOR_GET_T<Color>("text_editor/highlighting/brace_mismatch_color");
    Color current_line_color = EDITOR_GET_T<Color>("text_editor/highlighting/current_line_color");
    Color line_length_guideline_color = EDITOR_GET_T<Color>("text_editor/highlighting/line_length_guideline_color");
    Color word_highlighted_color = EDITOR_GET_T<Color>("text_editor/highlighting/word_highlighted_color");
    Color number_color = EDITOR_GET_T<Color>("text_editor/highlighting/number_color");
    Color function_color = EDITOR_GET_T<Color>("text_editor/highlighting/function_color");
    Color member_variable_color = EDITOR_GET_T<Color>("text_editor/highlighting/member_variable_color");
    Color mark_color = EDITOR_GET_T<Color>("text_editor/highlighting/mark_color");
    Color bookmark_color = EDITOR_GET_T<Color>("text_editor/highlighting/bookmark_color");
    Color breakpoint_color = EDITOR_GET_T<Color>("text_editor/highlighting/breakpoint_color");
    Color executing_line_color = EDITOR_GET_T<Color>("text_editor/highlighting/executing_line_color");
    Color code_folding_color = EDITOR_GET_T<Color>("text_editor/highlighting/code_folding_color");
    Color search_result_color = EDITOR_GET_T<Color>("text_editor/highlighting/search_result_color");
    Color search_result_border_color = EDITOR_GET_T<Color>("text_editor/highlighting/search_result_border_color");
    Color symbol_color = EDITOR_GET_T<Color>("text_editor/highlighting/symbol_color");
    Color keyword_color = EDITOR_GET_T<Color>("text_editor/highlighting/keyword_color");
    Color control_flow_keyword_color = EDITOR_GET_T<Color>("text_editor/highlighting/control_flow_keyword_color");
    Color comment_color = EDITOR_GET_T<Color>("text_editor/highlighting/comment_color");

    get_text_edit()->add_theme_color_override("background_color", background_color);
    get_text_edit()->add_theme_color_override("completion_background_color", completion_background_color);
    get_text_edit()->add_theme_color_override("completion_selected_color", completion_selected_color);
    get_text_edit()->add_theme_color_override("completion_existing_color", completion_existing_color);
    get_text_edit()->add_theme_color_override("completion_scroll_color", completion_scroll_color);
    get_text_edit()->add_theme_color_override("completion_font_color", completion_font_color);
    get_text_edit()->add_theme_color_override("font_color", text_color);
    get_text_edit()->add_theme_color_override("line_number_color", line_number_color);
    get_text_edit()->add_theme_color_override("caret_color", caret_color);
    get_text_edit()->add_theme_color_override("caret_background_color", caret_background_color);
    get_text_edit()->add_theme_color_override("font_color_selected", text_selected_color);
    get_text_edit()->add_theme_color_override("selection_color", selection_color);
    get_text_edit()->add_theme_color_override("brace_mismatch_color", brace_mismatch_color);
    get_text_edit()->add_theme_color_override("current_line_color", current_line_color);
    get_text_edit()->add_theme_color_override("line_length_guideline_color", line_length_guideline_color);
    get_text_edit()->add_theme_color_override("word_highlighted_color", word_highlighted_color);
    get_text_edit()->add_theme_color_override("number_color", number_color);
    get_text_edit()->add_theme_color_override("function_color", function_color);
    get_text_edit()->add_theme_color_override("member_variable_color", member_variable_color);
    get_text_edit()->add_theme_color_override("mark_color", mark_color);
    get_text_edit()->add_theme_color_override("bookmark_color", bookmark_color);
    get_text_edit()->add_theme_color_override("breakpoint_color", breakpoint_color);
    get_text_edit()->add_theme_color_override("executing_line_color", executing_line_color);
    get_text_edit()->add_theme_color_override("code_folding_color", code_folding_color);
    get_text_edit()->add_theme_color_override("search_result_color", search_result_color);
    get_text_edit()->add_theme_color_override("search_result_border_color", search_result_border_color);
    get_text_edit()->add_theme_color_override("symbol_color", symbol_color);

    Vector<StringView> keywords;
    ShaderLanguage::get_keyword_list(&keywords);
    for (const auto &E : keywords) {
        if (ShaderLanguage::is_control_flow_keyword(E)) {
            get_text_edit()->add_keyword_color(E, control_flow_keyword_color);
        } else {
            get_text_edit()->add_keyword_color(E, keyword_color);
        }
    }
    // Colorize built-ins like `COLOR` differently to make them easier
    // to distinguish from keywords at a quick glance.

    if (shader) {

        for (const eastl::pair<const StringName, ShaderLanguage::FunctionInfo> &E : ShaderTypes::get_singleton()->get_functions(RS::ShaderMode(shader->get_mode()))) {

            for (const eastl::pair<const StringName, ShaderLanguage::BuiltInInfo> &F : E.second.built_ins) {
                keywords.push_back(F.first.asCString());
            }
        }

        for (int i = 0; i < ShaderTypes::get_singleton()->get_modes(RS::ShaderMode(shader->get_mode())).size(); i++) {

            keywords.push_back(ShaderTypes::get_singleton()->get_modes(RS::ShaderMode(shader->get_mode()))[i].asCString());
        }
    }

    for (StringView E : keywords) {

        get_text_edit()->add_keyword_color(E, keyword_color);
    }

    // Colorize built-ins like `COLOR` differently to make them easier
    // to distinguish from keywords at a quick glance.

    Vector<StringName> built_ins;
    if (shader) {
        for (const auto &E : ShaderTypes::get_singleton()->get_functions(RS::ShaderMode(shader->get_mode()))) {
            for (const auto &F : E.second.built_ins) {
                built_ins.push_back(F.first);
            }
        }
        const auto &modes(ShaderTypes::get_singleton()->get_modes(RS::ShaderMode(shader->get_mode())));
        built_ins.insert(built_ins.end(),modes.begin(),modes.end());
    }

    const Color user_type_color = EDITOR_GET_T<Color>("text_editor/highlighting/user_type_color");

    for (const StringName &E : built_ins) {
        get_text_edit()->add_keyword_color(E, user_type_color);
    }

    // Colorize comments.
    get_text_edit()->add_color_region("/*", "*/", comment_color, false);
    get_text_edit()->add_color_region("//", "", comment_color, false);
}

void ShaderTextEditor::_check_shader_mode() {

    String type = ShaderLanguage::get_shader_type(get_text_edit()->get_text());
    using namespace RenderingServerEnums;
    ShaderMode mode;

    if (type == "canvas_item") {
        mode = ShaderMode::CANVAS_ITEM;
    } else if (type == "particles") {
        mode = ShaderMode::PARTICLES;
    } else {
        mode = ShaderMode::SPATIAL;
    }

    if (shader->get_mode() != mode) {
        shader->set_code(get_text_edit()->get_text_utf8());
        _load_theme_settings();
    }
}

void ShaderTextEditor::_code_complete_script(const String &p_code, Vector<ScriptCodeCompletionOption> *r_options) {

    _check_shader_mode();

    ShaderLanguage sl;
    String calltip;

    sl.complete(p_code,
            ShaderTypes::get_singleton()->get_functions(RS::ShaderMode(shader->get_mode())),
            ShaderTypes::get_singleton()->get_modes(RS::ShaderMode(shader->get_mode())),
            ShaderTypes::get_singleton()->get_types(), r_options, calltip);

    get_text_edit()->set_code_hint(calltip);
}

void ShaderTextEditor::_validate_script() {

    _check_shader_mode();

    String code = get_text_edit()->get_text_utf8();
    // List<StringName> params;
    // shader->get_param_list(&params);

    ShaderLanguage sl;

    Error err = sl.compile(code, ShaderTypes::get_singleton()->get_functions(RS::ShaderMode(shader->get_mode())),
            ShaderTypes::get_singleton()->get_modes(RS::ShaderMode(shader->get_mode())),
            ShaderTypes::get_singleton()->get_types());

    if (err != OK) {
        String error_text = "error(" + itos(sl.get_error_line()) + "): " + sl.get_error_text();
        set_error(StringName(error_text));
        set_error_pos(sl.get_error_line() - 1, 0);
        for (int i = 0; i < get_text_edit()->get_line_count(); i++)
            get_text_edit()->set_line_as_marked(i, false);
        get_text_edit()->set_line_as_marked(sl.get_error_line() - 1, true);

    } else {
        for (int i = 0; i < get_text_edit()->get_line_count(); i++)
            get_text_edit()->set_line_as_marked(i, false);
        set_error(StringName());
    }

    emit_signal("script_changed");
}

void ShaderTextEditor::_bind_methods() {
}

ShaderTextEditor::ShaderTextEditor() {
}

/*** SCRIPT EDITOR ******/

void ShaderEditor::_menu_option(int p_option) {

    switch (p_option) {
        case EDIT_UNDO: {
            shader_editor->get_text_edit()->undo();
        } break;
        case EDIT_REDO: {
            shader_editor->get_text_edit()->redo();
        } break;
        case EDIT_CUT: {
            shader_editor->get_text_edit()->cut();
        } break;
        case EDIT_COPY: {
            shader_editor->get_text_edit()->copy();
        } break;
        case EDIT_PASTE: {
            shader_editor->get_text_edit()->paste();
        } break;
        case EDIT_SELECT_ALL: {
            shader_editor->get_text_edit()->select_all();
        } break;
        case EDIT_MOVE_LINE_UP: {
            shader_editor->move_lines_up();
        } break;
        case EDIT_MOVE_LINE_DOWN: {
            shader_editor->move_lines_down();
        } break;
        case EDIT_INDENT_LEFT: {

            if (not shader) {
                return;
            }

            TextEdit *tx = shader_editor->get_text_edit();
            tx->indent_left();

        } break;
        case EDIT_INDENT_RIGHT: {

            if (not shader) {
                return;
            }

            TextEdit *tx = shader_editor->get_text_edit();
            tx->indent_right();

        } break;
        case EDIT_DELETE_LINE: {
            shader_editor->delete_lines();
        } break;
        case EDIT_DUPLICATE_SELECTION: {
            shader_editor->duplicate_selection();
        } break;
        case EDIT_TOGGLE_COMMENT: {

            if (not shader) {
                return;
            }

            shader_editor->toggle_inline_comment("//");

        } break;
        case EDIT_COMPLETE: {

            shader_editor->get_text_edit()->query_code_comple();
        } break;
        case SEARCH_FIND: {

            shader_editor->get_find_replace_bar()->popup_search();
        } break;
        case SEARCH_FIND_NEXT: {

            shader_editor->get_find_replace_bar()->search_next();
        } break;
        case SEARCH_FIND_PREV: {

            shader_editor->get_find_replace_bar()->search_prev();
        } break;
        case SEARCH_REPLACE: {

            shader_editor->get_find_replace_bar()->popup_replace();
        } break;
        case SEARCH_GOTO_LINE: {

            goto_line_dialog->popup_find_line(shader_editor->get_text_edit());
        } break;
        case BOOKMARK_TOGGLE: {

            shader_editor->toggle_bookmark();
        } break;
        case BOOKMARK_GOTO_NEXT: {

            shader_editor->goto_next_bookmark();
        } break;
        case BOOKMARK_GOTO_PREV: {

            shader_editor->goto_prev_bookmark();
        } break;
        case BOOKMARK_REMOVE_ALL: {

            shader_editor->remove_all_bookmarks();
        } break;
        case HELP_DOCS: {
            OS::get_singleton()->shell_open(FormatVE("%s/tutorials/shaders/shader_reference/index.html",VERSION_DOCS_URL));
        } break;
    }
    if (p_option != SEARCH_FIND && p_option != SEARCH_REPLACE && p_option != SEARCH_GOTO_LINE) {
        auto te=shader_editor->get_text_edit();
        te->call_deferred([te] { te->grab_focus(); });
    }
}

void ShaderEditor::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE:
        case NOTIFICATION_THEME_CHANGED: {
            PopupMenu *popup = help_menu->get_popup();
            popup->set_item_icon(popup->get_item_index(HELP_DOCS), get_theme_icon("Instance", "EditorIcons"));
        } break;

        case MainLoop::NOTIFICATION_WM_FOCUS_IN: {
        _check_for_external_edit();
        } break;
    }
}

void ShaderEditor::_params_changed() {

    shader_editor->_validate_script();
}

void ShaderEditor::_editor_settings_changed() {
    shader_editor->update_editor_settings();

    TextEdit *text_editor = shader_editor->get_text_edit();

    EditorSettings *editor_settings = EditorSettings::get_singleton();

    text_editor->add_constant_override("line_spacing", editor_settings->getT<int>("text_editor/theme/line_spacing"));
    text_editor->cursor_set_blink_enabled(EditorSettings::get_singleton()->is_caret_blink_active());
    text_editor->cursor_set_blink_speed(editor_settings->getT<float>("text_editor/cursor/caret_blink_speed"));
    text_editor->cursor_set_block_mode(editor_settings->getT<bool>("text_editor/cursor/block_caret"));

    text_editor->set_auto_brace_completion(editor_settings->getT<bool>("text_editor/completion/auto_brace_complete"));
    text_editor->set_auto_indent(editor_settings->getT<bool>("text_editor/indent/auto_indent"));
    text_editor->set_draw_minimap(editor_settings->getT<bool>("text_editor/navigation/show_minimap"));
    text_editor->set_draw_spaces(editor_settings->getT<bool>("text_editor/indent/draw_spaces"));
    text_editor->set_draw_tabs(editor_settings->getT<bool>("text_editor/indent/draw_tabs"));
    text_editor->set_highlight_all_occurrences(editor_settings->getT<bool>("text_editor/highlighting/highlight_all_occurrences"));
    text_editor->set_highlight_current_line(editor_settings->getT<bool>("text_editor/highlighting/highlight_current_line"));
    text_editor->set_indent_size(editor_settings->getT<int>("text_editor/indent/size"));
    text_editor->set_indent_using_spaces(editor_settings->getT<bool>("text_editor/indent/type"));
    text_editor->set_minimap_width(editor_settings->getT<int>("text_editor/navigation/minimap_width"));
    text_editor->set_scroll_pass_end_of_file(editor_settings->getT<bool>("text_editor/cursor/scroll_past_end_of_file"));
    text_editor->set_show_line_numbers(editor_settings->getT<bool>("text_editor/appearance/show_line_numbers"));
    text_editor->set_smooth_scroll_enabled(editor_settings->getT<bool>("text_editor/navigation/smooth_scrolling"));
    text_editor->set_syntax_coloring(editor_settings->getT<bool>("text_editor/highlighting/syntax_highlighting"));
    text_editor->set_v_scroll_speed(editor_settings->getT<float>("text_editor/navigation/v_scroll_speed"));
    text_editor->set_show_line_length_guidelines(editor_settings->getT<bool>("text_editor/appearance/show_line_length_guidelines"));
    text_editor->set_line_length_guideline_soft_column(editor_settings->getT<int>("text_editor/appearance/line_length_guideline_soft_column"));
    text_editor->set_line_length_guideline_hard_column(editor_settings->getT<int>("text_editor/appearance/line_length_guideline_hard_column"));

    text_editor->set_breakpoint_gutter_enabled(false);
}

void ShaderEditor::_bind_methods() {
    MethodBinder::bind_method("apply_shaders", &ShaderEditor::apply_shaders);
    MethodBinder::bind_method("save_external_data", &ShaderEditor::save_external_data);
}

void ShaderEditor::ensure_select_current() {

    /*
    if (tab_container->get_child_count() && tab_container->get_current_tab()>=0) {

        ShaderTextEditor *ste = object_cast<ShaderTextEditor>(tab_container->get_child(tab_container->get_current_tab()));
        if (!ste)
            return;
        Ref<Shader> shader = ste->get_edited_shader();
        get_scene()->get_root_node()->call("_resource_selected",shader);
    }*/
}

void ShaderEditor::goto_line_selection(int p_line, int p_begin, int p_end) {

    shader_editor->goto_line_selection(p_line, p_begin, p_end);
}

void ShaderEditor::_check_for_external_edit() {

    if (not shader) {
        return;
    }

    // internal shader.
    if (shader->get_path().empty() || PathUtils::is_internal_path(shader->get_path()) ) {
        return;
    }

    bool use_autoreload = EDITOR_DEF_T("text_editor/files/auto_reload_scripts_on_external_change", false);
    if (ResourceTooling::get_last_modified_time(shader.get()) != FileAccess::get_modified_time(shader->get_path())) {
        if (use_autoreload) {
            _reload_shader_from_disk();
        } else {
            auto dc=disk_changed;
            dc->call_deferred([dc] { dc->popup_centered(); });
        }
    }
}

void ShaderEditor::_reload_shader_from_disk() {

    Ref<Shader> rel_shader = dynamic_ref_cast<Shader>(gResourceManager().load(shader->get_path(), shader->get_class(), true));
    ERR_FAIL_COND(not rel_shader);

    shader->set_code(rel_shader->get_code());
    ResourceTooling::set_last_modified_time_from_another(shader.get(), rel_shader.get());
    shader_editor->reload_text();
}

void ShaderEditor::edit(const Ref<Shader> &p_shader) {

    if (not p_shader || !p_shader->is_text_shader()) {
        return;
    }

    if (shader == p_shader) {
        return;
    }

    shader = p_shader;

    shader_editor->set_edited_shader(p_shader);

    //vertex_editor->set_edited_shader(shader,ShaderLanguage::SHADER_MATERIAL_VERTEX);
    // see if already has it
}

void ShaderEditor::save_external_data(StringView p_str) {

    if (not shader) {
        disk_changed->hide();
        return;
    }

    apply_shaders();
    if (not shader->get_path().empty() && not PathUtils::is_internal_path(shader->get_path()) ) {
        //external shader, save it
        gResourceManager().save(shader->get_path(), shader);
    }

    disk_changed->hide();
}

void ShaderEditor::apply_shaders() {

    if (shader) {
        String shader_code = shader->get_code();
        String editor_code = shader_editor->get_text_edit()->get_text_utf8();
        if (shader_code != editor_code) {
            shader->set_code(editor_code);
            Object_set_edited(shader.get(),true);
        }
    }
}

void ShaderEditor::_text_edit_gui_input(const Ref<InputEvent> &ev) {

    Ref<InputEventMouseButton> mb = dynamic_ref_cast<InputEventMouseButton>(ev);

    if (mb) {

        if (mb->get_button_index() == BUTTON_RIGHT && mb->is_pressed()) {

            int col, row;
            TextEdit *tx = shader_editor->get_text_edit();
            tx->_get_mouse_pos(mb->get_global_position() - tx->get_global_position(), row, col);
            tx->set_right_click_moves_caret(EditorSettings::get_singleton()->getT<bool>("text_editor/cursor/right_click_moves_caret"));

            if (tx->is_right_click_moving_caret()) {
                if (tx->is_selection_active()) {

                    int from_line = tx->get_selection_from_line();
                    int to_line = tx->get_selection_to_line();
                    int from_column = tx->get_selection_from_column();
                    int to_column = tx->get_selection_to_column();

                    if (row < from_line || row > to_line || (row == from_line && col < from_column) || (row == to_line && col > to_column)) {
                        // Right click is outside the selected text
                        tx->deselect();
                    }
                }
                if (!tx->is_selection_active()) {
                    tx->cursor_set_line(row, true, false);
                    tx->cursor_set_column(col);
                }
            }
            _make_context_menu(tx->is_selection_active(), get_local_mouse_position());
        }
    }
    Ref<InputEventKey> k = dynamic_ref_cast<InputEventKey>(ev);
    if (k && k->is_pressed() && k->get_keycode() == KEY_MENU) {
        TextEdit *tx = shader_editor->get_text_edit();
        _make_context_menu(tx->is_selection_active(), (get_global_transform().inverse() * tx->get_global_transform()).xform(tx->_get_cursor_pixel_pos()));
        context_menu->grab_focus();
    }
}

void ShaderEditor::_update_bookmark_list() {

    bookmarks_menu->clear();

    bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_bookmark"), BOOKMARK_TOGGLE);
    bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/remove_all_bookmarks"), BOOKMARK_REMOVE_ALL);
    bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_next_bookmark"), BOOKMARK_GOTO_NEXT);
    bookmarks_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_previous_bookmark"), BOOKMARK_GOTO_PREV);

    Array bookmark_list = shader_editor->get_text_edit()->get_bookmarks_array();
    if (bookmark_list.empty()) {
        return;
    }

    bookmarks_menu->add_separator();

    for (int i = 0; i < bookmark_list.size(); i++) {
        String line(StringUtils::strip_edges(shader_editor->get_text_edit()->get_line(bookmark_list[i].as<int>())));
        // Limit the size of the line if too big.
        if (line.length() > 50) {
            line = StringUtils::substr(line,0, 50);
        }

        bookmarks_menu->add_item(StringName(StringUtils::num(bookmark_list[i].as<int>() + 1) + " - \"" + line + "\""));
        bookmarks_menu->set_item_metadata(bookmarks_menu->get_item_count() - 1, bookmark_list[i]);
    }
}

void ShaderEditor::_bookmark_item_pressed(int p_idx) {

    if (p_idx < 4) { // Any item before the separator.
        _menu_option(bookmarks_menu->get_item_id(p_idx));
    } else {
        shader_editor->goto_line(bookmarks_menu->get_item_metadata(p_idx).as<int>());
    }
}

void ShaderEditor::_make_context_menu(bool p_selection, Vector2 p_position) {

    context_menu->clear();
    if (p_selection) {
        context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/cut"), EDIT_CUT);
        context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/copy"), EDIT_COPY);
    }

    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/paste"), EDIT_PASTE);
    context_menu->add_separator();
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/select_all"), EDIT_SELECT_ALL);
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/undo"), EDIT_UNDO);
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/redo"), EDIT_REDO);

    context_menu->add_separator();
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_left"), EDIT_INDENT_LEFT);
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_right"), EDIT_INDENT_RIGHT);
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
    context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_bookmark"), BOOKMARK_TOGGLE);

    context_menu->set_position(get_global_transform().xform(p_position));
    context_menu->set_size(Vector2(1, 1));
    context_menu->popup();
}

ShaderEditor::ShaderEditor(EditorNode *p_node) {

    shader_editor = memnew(ShaderTextEditor);
    shader_editor->set_v_size_flags(SIZE_EXPAND_FILL);
    shader_editor->add_constant_override("separation", 0);
    shader_editor->set_anchors_and_margins_preset(Control::PRESET_WIDE);

    shader_editor->connect("script_changed",callable_mp(this, &ClassName::apply_shaders));
    EditorSettings::get_singleton()->connect("settings_changed",callable_mp(this, &ClassName::_editor_settings_changed));

    shader_editor->get_text_edit()->set_callhint_settings(
            EditorSettings::get_singleton()->getT<bool>("text_editor/completion/put_callhint_tooltip_below_current_line"),
            EditorSettings::get_singleton()->getT<Vector2>("text_editor/completion/callhint_tooltip_offset"));

    shader_editor->get_text_edit()->set_select_identifiers_on_hover(true);
    shader_editor->get_text_edit()->set_context_menu_enabled(false);
    shader_editor->get_text_edit()->connect("gui_input",callable_mp(this, &ClassName::_text_edit_gui_input));

    shader_editor->update_editor_settings();

    context_menu = memnew(PopupMenu);
    add_child(context_menu);
    context_menu->connect("id_pressed",callable_mp(this, &ClassName::_menu_option));
    context_menu->set_hide_on_window_lose_focus(true);

    VBoxContainer *main_container = memnew(VBoxContainer);
    HBoxContainer *hbc = memnew(HBoxContainer);

    edit_menu = memnew(MenuButton);
    edit_menu->set_text(TTR("Edit"));
    edit_menu->set_switch_on_hover(true);
    edit_menu->get_popup()->set_hide_on_window_lose_focus(true);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/undo"), EDIT_UNDO);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/redo"), EDIT_REDO);
    edit_menu->get_popup()->add_separator();
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/cut"), EDIT_CUT);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/copy"), EDIT_COPY);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/paste"), EDIT_PASTE);
    edit_menu->get_popup()->add_separator();
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/select_all"), EDIT_SELECT_ALL);
    edit_menu->get_popup()->add_separator();
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/move_up"), EDIT_MOVE_LINE_UP);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/move_down"), EDIT_MOVE_LINE_DOWN);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_left"), EDIT_INDENT_LEFT);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent_right"), EDIT_INDENT_RIGHT);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/delete_line"), EDIT_DELETE_LINE);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/duplicate_selection"), EDIT_DUPLICATE_SELECTION);
    edit_menu->get_popup()->add_separator();
    edit_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/complete_symbol"), EDIT_COMPLETE);
    edit_menu->get_popup()->connect("id_pressed",callable_mp(this, &ClassName::_menu_option));

    search_menu = memnew(MenuButton);
    search_menu->set_text(TTR("Search"));
    search_menu->set_switch_on_hover(true);
    search_menu->get_popup()->set_hide_on_window_lose_focus(true);
    search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find"), SEARCH_FIND);
    search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find_next"), SEARCH_FIND_NEXT);
    search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/find_previous"), SEARCH_FIND_PREV);
    search_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/replace"), SEARCH_REPLACE);
    search_menu->get_popup()->connect("id_pressed",callable_mp(this, &ShaderEditor::_menu_option));

    MenuButton *goto_menu = memnew(MenuButton);
    goto_menu->set_text(TTR("Go To"));
    goto_menu->set_switch_on_hover(true);
    goto_menu->get_popup()->connect("id_pressed",callable_mp(this, &ShaderEditor::_menu_option));

    goto_menu->get_popup()->add_shortcut(ED_GET_SHORTCUT("script_text_editor/goto_line"), SEARCH_GOTO_LINE);
    goto_menu->get_popup()->add_separator();

    bookmarks_menu = memnew(PopupMenu);
    bookmarks_menu->set_name("Bookmarks");
    goto_menu->get_popup()->add_child(bookmarks_menu);
    goto_menu->get_popup()->add_submenu_item(TTR("Bookmarks"), StringName("Bookmarks"));
    _update_bookmark_list();
    bookmarks_menu->connect("about_to_show",callable_mp(this, &ShaderEditor::_update_bookmark_list));
    bookmarks_menu->connect("index_pressed",callable_mp(this, &ShaderEditor::_bookmark_item_pressed));

    help_menu = memnew(MenuButton);
    help_menu->set_text(TTR("Help"));
    help_menu->set_switch_on_hover(true);
    help_menu->get_popup()->add_item(TTR("Online Docs"), HELP_DOCS);
    help_menu->get_popup()->connect("id_pressed",callable_mp(this, &ShaderEditor::_menu_option));

    add_child(main_container);
    main_container->add_child(hbc);
    hbc->add_child(search_menu);
    hbc->add_child(edit_menu);
    hbc->add_child(goto_menu);
    hbc->add_child(help_menu);
    hbc->add_theme_style_override("panel", p_node->get_gui_base()->get_theme_stylebox("ScriptEditorPanel", "EditorStyles"));
    main_container->add_child(shader_editor);

    goto_line_dialog = memnew(GotoLineDialog);
    add_child(goto_line_dialog);

    disk_changed = memnew(ConfirmationDialog);

    VBoxContainer *vbc = memnew(VBoxContainer);
    disk_changed->add_child(vbc);

    Label *dl = memnew(Label);
    dl->set_text(TTR("This shader has been modified on on disk.\nWhat action should be taken?"));
    vbc->add_child(dl);

    disk_changed->connect("confirmed",callable_mp(this, &ShaderEditor::_reload_shader_from_disk));
    disk_changed->get_ok()->set_text(TTR("Reload"));

    disk_changed->add_button(TTR("Resave"), !OS::get_singleton()->get_swap_ok_cancel(), "resave");
    disk_changed->connect("custom_action",callable_mp(this, &ShaderEditor::save_external_data));

    add_child(disk_changed);

    _editor_settings_changed();
}

void ShaderEditorPlugin::edit(Object *p_object) {

    Shader *s = object_cast<Shader>(p_object);
    shader_editor->edit(Ref<Shader>(s));
}

bool ShaderEditorPlugin::handles(Object *p_object) const {

    Shader *shader = object_cast<Shader>(p_object);
    return shader != nullptr && shader->is_text_shader();
}

void ShaderEditorPlugin::make_visible(bool p_visible) {

    if (p_visible) {
        button->show();
        editor->make_bottom_panel_item_visible(shader_editor);

    } else {

        button->hide();
        if (shader_editor->is_visible_in_tree()) {
            editor->hide_bottom_panel();
        }
        shader_editor->apply_shaders();
    }
}

void ShaderEditorPlugin::selected_notify() {

    shader_editor->ensure_select_current();
}

void ShaderEditorPlugin::save_external_data() {

    shader_editor->save_external_data();
}

void ShaderEditorPlugin::apply_changes() {

    shader_editor->apply_shaders();
}

ShaderEditorPlugin::ShaderEditorPlugin(EditorNode *p_node) {

    editor = p_node;
    shader_editor = memnew(ShaderEditor(p_node));

    shader_editor->set_custom_minimum_size(Size2(0, 300)* EDSCALE);
    button = editor->add_bottom_panel_item(TTR("Shader"), shader_editor);
    button->hide();
}

ShaderEditorPlugin::~ShaderEditorPlugin() {}
