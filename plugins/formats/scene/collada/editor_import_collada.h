/*************************************************************************/
/*  editor_import_collada.h                                              */
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

#include "editor/plugin_interfaces/PluginDeclarations.h"

class EditorSceneImporterCollada : public QObject, public EditorSceneImporterInterface {

    Q_PLUGIN_METADATA(IID "org.segs_engine.editor.SceneImporterCollada")
    Q_INTERFACES(EditorSceneImporterInterface)
    Q_OBJECT

public:
    uint32_t get_import_flags() const override;
    void get_extensions(Vector<String> &r_extensions) const override;
    Node *import_scene(StringView p_path, uint32_t p_flags, int p_bake_fps, uint32_t p_compress_flags, Vector<String> *r_missing_deps = nullptr, Error *r_err = nullptr) override;
    Ref<Animation> import_animation(StringView p_path, uint32_t p_flags, int p_bake_fps) override;

    EditorSceneImporterCollada();
};
