/*************************************************************************/
/*  import_state.h                                                       */
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

#include "core/io/resource_importer.h"
#include "core/vector.h"
#include "editor/import/resource_importer_scene.h"
#include "editor/project_settings_editor.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/surface_tool.h"

#include "EASTL/deque.h"

#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
class AnimationPlayer;

namespace AssimpImporter {
/** Import state is for global scene import data
     * This makes the code simpler and contains useful lookups.
     */
struct ImportState {

    String path;
    Node3D *root;
    const aiScene *assimp_scene;
    uint32_t max_bone_weights;

    HashMap<String, Ref<Mesh> > mesh_cache;
    HashMap<int, Ref<Material> > material_cache;
    HashMap<String, int> light_cache;
    HashMap<String, int> camera_cache;
    // very useful for when you need to ask assimp for the bone mesh

    HashMap<const aiNode *, Node *> assimp_node_map;
    Map<String, Ref<Image> > path_to_image_cache;

    // Generation 3 - determinisitic iteration
    // to lower potential recursion errors
    Vector<const aiNode *> nodes;
    HashMap<const aiNode *, Node3D *> flat_node_map;
    AnimationPlayer *animation_player;

    // Generation 3 - deterministic armatures
    // list of armature nodes - flat and simple to parse
    // assimp node, node in godot
    Vector<aiNode *> armature_nodes;
    HashMap<const aiNode *, Skeleton *> armature_skeletons;
    HashMap<aiBone *, Skeleton *> skeleton_bone_map;
    // Generation 3 - deterministic bone handling
    // bones from the stack are popped when found
    // this means we can detect
    // what bones are for other armatures
    Vector<aiBone *> bone_stack;
};

struct AssimpImageData {
    Ref<Image> raw_image;
    Ref<ImageTexture> texture;
    aiTextureMapMode map_mode[3];
};

/** Recursive state is used to push state into functions instead of specifying them
    * This makes the code easier to handle too and add extra arguments without breaking things
    */
struct RecursiveState {
    RecursiveState() = delete;
    RecursiveState(
            Transform &_node_transform,
            Skeleton *_skeleton,
            Node3D *_new_node,
            const String &_node_name,
            const aiNode *_assimp_node,
            Node *_parent_node,
            aiBone *_bone) :
            node_transform(_node_transform),
            skeleton(_skeleton),
            new_node(_new_node),
            node_name(_node_name),
            assimp_node(_assimp_node),
            parent_node(_parent_node),
            bone(_bone) {}

    Transform &node_transform;
    Skeleton *skeleton;
    Node3D *new_node;
    const String &node_name;
    const aiNode *assimp_node;
    Node *parent_node;
    aiBone *bone;
};
} // namespace AssimpImporter
