/*************************************************************************/
/*  editor_import_collada.cpp                                            */
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

#include "editor_import_collada.h"

#include "collada.h"
#include "core/os/os.h"
#include "core/string.h"
#include "core/string_utils.h"
#include "core/ustring.h"
//#include "editor/editor_node.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/path_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/surface_tool.h"

#include "EASTL/sort.h"
#include <QHash>

#include "core/resource/resource_manager.h"

struct ColladaImport {

    Collada collada;
    Node3D *scene;

    Vector<Ref<Animation> > animations;

    struct NodeMap {
        //String path;
        Node3D *node = nullptr;
        int bone = -1;
        Vector<int> anim_tracks;
    };

    bool found_ambient;
    Color ambient;
    bool found_directional;
    bool force_make_tangents;
    bool apply_mesh_xform_to_vertices;
    bool use_mesh_builtin_materials;
    float bake_fps;

    HashMap<String, NodeMap,Hasher<String>> node_map; //map from collada node to engine node
    HashMap<String, String> node_name_map; //map from collada node to engine node
    HashMap<String, Ref<ArrayMesh> > mesh_cache;
    HashMap<String, Ref<Curve3D> > curve_cache;
    HashMap<String, Ref<Material> > material_cache;
    HashMap<Collada::Node *, Skeleton *> skeleton_map;

    HashMap<Skeleton *, HashMap<String, int> > skeleton_bone_map;

    Set<String> valid_animated_nodes;
    Vector<int> valid_animated_properties;
    HashMap<String, bool> bones_with_animation;

    Error _populate_skeleton(Skeleton *p_skeleton, Collada::Node *p_node, int &r_bone, int p_parent);
    Error _create_scene_skeletons(Collada::Node *p_node);
    Error _create_scene(Collada::Node *p_node, Node3D *p_parent);
    Error _create_resources(Collada::Node *p_node, uint32_t p_use_compression);
    Error _create_material(const String &p_target);
    Error _create_mesh_surfaces(bool p_optimize, Ref<ArrayMesh> &p_mesh,
            const HashMap<String, Collada::NodeGeometry::Material> &p_material_map, const Collada::MeshData &meshdata,
            const Transform &p_local_xform, const Vector<int> &bone_remap,
            const Collada::SkinControllerData *p_skin_controller, const Collada::MorphControllerData *p_morph_data,
            Vector<Ref<ArrayMesh>> p_morph_meshes = Vector<Ref<ArrayMesh>>(), uint32_t p_use_compression = 0,
            bool p_use_mesh_material = false);
    Error load(StringView p_path, int p_flags, bool p_force_make_tangents = false, uint32_t p_use_compression = 0);
    void _fix_param_animation_tracks();
    void create_animation(int p_clip, bool p_make_tracks_in_all_bones, bool p_import_value_tracks);
    void create_animations(bool p_make_tracks_in_all_bones, bool p_import_value_tracks);

    Set<String> tracks_in_clips;
    Vector<String> missing_textures;

    void _pre_process_lights(Collada::Node *p_node);

    ColladaImport() {

        found_ambient = false;
        found_directional = false;
        force_make_tangents = false;
        apply_mesh_xform_to_vertices = true;
        bake_fps = 15;
    }
};

Error ColladaImport::_populate_skeleton(Skeleton *p_skeleton, Collada::Node *p_node, int &r_bone, int p_parent) {

    if (p_node->type != Collada::Node::TYPE_JOINT)
        return OK;

    Collada::NodeJoint *joint = static_cast<Collada::NodeJoint *>(p_node);

    p_skeleton->add_bone(p_node->name);
    if (p_parent >= 0)
        p_skeleton->set_bone_parent(r_bone, p_parent);

    NodeMap nm;
    nm.node = p_skeleton;
    nm.bone = r_bone;
    node_map[p_node->id] = nm;
    node_name_map[p_node->name] = p_node->id;

    skeleton_bone_map[p_skeleton][joint->sid] = r_bone;

    if (collada.state.bone_rest_map.contains(joint->sid)) {

        p_skeleton->set_bone_rest(r_bone, collada.fix_transform(collada.state.bone_rest_map[joint->sid]));
        //should map this bone to something for animation?
    } else {
        WARN_PRINT("Collada: Joint has no rest.");
    }

    int id = r_bone++;
    for (int i = 0; i < p_node->children.size(); i++) {

        Error err = _populate_skeleton(p_skeleton, p_node->children[i], r_bone, id);
        if (err)
            return err;
    }

    return OK;
}

void ColladaImport::_pre_process_lights(Collada::Node *p_node) {

    if (p_node->type == Collada::Node::TYPE_LIGHT) {

        Collada::NodeLight *light = static_cast<Collada::NodeLight *>(p_node);
        if (collada.state.light_data_map.contains(light->light)) {

            Collada::LightData &ld = collada.state.light_data_map[light->light];
            if (ld.mode == Collada::LightData::MODE_AMBIENT) {
                found_ambient = true;
                ambient = ld.color;
            }
            if (ld.mode == Collada::LightData::MODE_DIRECTIONAL) {
                found_directional = true;
            }
        }
    }

    for (int i = 0; i < p_node->children.size(); i++)
        _pre_process_lights(p_node->children[i]);
}

Error ColladaImport::_create_scene_skeletons(Collada::Node *p_node) {

    if (p_node->type == Collada::Node::TYPE_SKELETON) {

        Skeleton *sk = memnew(Skeleton);
        int bone = 0;
        for (int i = 0; i < p_node->children.size(); i++) {

            _populate_skeleton(sk, p_node->children[i], bone, -1);
        }
        sk->localize_rests(); //after creating skeleton, rests must be localized...!
        skeleton_map[p_node] = sk;
    }

    for (int i = 0; i < p_node->children.size(); i++) {

        Error err = _create_scene_skeletons(p_node->children[i]);
        if (err)
            return err;
    }
    return OK;
}

Error ColladaImport::_create_scene(Collada::Node *p_node, Node3D *p_parent) {

    Node3D *node = nullptr;

    switch (p_node->type) {

        case Collada::Node::TYPE_NODE: {

            node = memnew(Node3D);
        } break;
        case Collada::Node::TYPE_JOINT: {

            return OK; // do nothing
        } break;
        case Collada::Node::TYPE_LIGHT: {

            // node = memnew( Light3D)
            Collada::NodeLight *light = static_cast<Collada::NodeLight *>(p_node);
            if (collada.state.light_data_map.contains(light->light)) {

                Collada::LightData &ld = collada.state.light_data_map[light->light];

                if (ld.mode == Collada::LightData::MODE_AMBIENT) {

                    if (found_directional)
                        return OK; //do nothing not needed

                    if (!T_GLOBAL_DEF("collada/use_ambient", false))
                        return OK;
                    //well, it's an ambient light..
                    Light3D *l = memnew(DirectionalLight3D);
                    // l->set_color(Light3D::COLOR_AMBIENT,ld.color);
                    // l->set_color(Light3D::COLOR_DIFFUSE,Color(0,0,0));
                    // l->set_color(Light3D::COLOR_SPECULAR,Color(0,0,0));
                    node = l;

                } else if (ld.mode == Collada::LightData::MODE_DIRECTIONAL) {

                    //well, it's an ambient light..
                    Light3D *l = memnew(DirectionalLight3D);
                    /*
                    if (found_ambient) //use it here
                        l->set_color(Light3D::COLOR_AMBIENT,ambient);

                    l->set_color(Light3D::COLOR_DIFFUSE,ld.color);
                    l->set_color(Light3D::COLOR_SPECULAR,Color(1,1,1));
                    */
                    node = l;
                } else {

                    Light3D *l;

                    if (ld.mode == Collada::LightData::MODE_OMNI)
                        l = memnew(OmniLight3D);
                    else {
                        l = memnew(SpotLight3D);
                        // l->set_parameter(Light3D::PARAM_SPOT_ANGLE,ld.spot_angle);
                        // l->set_parameter(Light3D::PARAM_SPOT_ATTENUATION,ld.spot_exp);
                    }

                    //
                    // l->set_color(Light3D::COLOR_DIFFUSE,ld.color);
                    // l->set_color(Light3D::COLOR_SPECULAR,Color(1,1,1));
                    //l->approximate_opengl_attenuation(ld.constant_att,ld.linear_att,ld.quad_att);
                    node = l;
                }

            } else {

                node = memnew(Node3D);
            }
        } break;
        case Collada::Node::TYPE_CAMERA: {

            Collada::NodeCamera *cam = static_cast<Collada::NodeCamera *>(p_node);
            Camera3D *camera = memnew(Camera3D);

            if (collada.state.camera_data_map.contains(cam->camera)) {

                const Collada::CameraData &cd = collada.state.camera_data_map[cam->camera];

                switch (cd.mode) {

                    case Collada::CameraData::MODE_ORTHOGONAL: {

                        if (cd.orthogonal.y_mag) {

                            camera->set_keep_aspect_mode(Camera3D::KEEP_HEIGHT);
                            camera->set_orthogonal(cd.orthogonal.y_mag * 2.0f, cd.z_near, cd.z_far);

                        } else if (!cd.orthogonal.y_mag && cd.orthogonal.x_mag) {

                            camera->set_keep_aspect_mode(Camera3D::KEEP_WIDTH);
                            camera->set_orthogonal(cd.orthogonal.x_mag * 2.0f, cd.z_near, cd.z_far);
                        }

                    } break;
                    case Collada::CameraData::MODE_PERSPECTIVE: {

                        if (cd.perspective.y_fov) {

                            camera->set_perspective(cd.perspective.y_fov, cd.z_near, cd.z_far);

                        } else if (!cd.perspective.y_fov && cd.perspective.x_fov) {

                            camera->set_perspective(cd.perspective.x_fov / cd.aspect, cd.z_near, cd.z_far);
                        }

                    } break;
                }
            }

            node = camera;

        } break;
        case Collada::Node::TYPE_GEOMETRY: {

            Collada::NodeGeometry *ng = static_cast<Collada::NodeGeometry *>(p_node);

            if (collada.state.curve_data_map.contains(ng->source)) {

                node = memnew(Path3D);
            } else {
                //mesh since nothing else
                node = memnew(MeshInstance3D);
                //object_cast<MeshInstance3D>(node)->set_flag(GeometryInstance::FLAG_USE_BAKED_LIGHT, true);
            }
        } break;
        case Collada::Node::TYPE_SKELETON: {

            ERR_FAIL_COND_V(!skeleton_map.contains(p_node), ERR_CANT_CREATE);
            Skeleton *sk = skeleton_map[p_node];
            node = sk;
        } break;
    }

    if (!p_node->name.empty())
        node->set_name(p_node->name);
    NodeMap nm;
    nm.node = node;
    node_map[p_node->id] = nm;
    node_name_map[node->get_name().asCString()] = p_node->id;
    Transform xf = p_node->default_transform;

    xf = collada.fix_transform(xf) * p_node->post_transform;
    node->set_transform(xf);
    p_parent->add_child(node);
    node->set_owner(scene);

    if (!p_node->empty_draw_type.empty()) {
        node->set_meta("empty_draw_type", Variant(p_node->empty_draw_type));
    }

    for (int i = 0; i < p_node->children.size(); i++) {

        Error err = _create_scene(p_node->children[i], node);
        if (err)
            return err;
    }
    return OK;
}

Error ColladaImport::_create_material(const String &p_target) {

    ERR_FAIL_COND_V(material_cache.contains(p_target), ERR_ALREADY_EXISTS);
    ERR_FAIL_COND_V(!collada.state.material_map.contains(p_target), ERR_INVALID_PARAMETER);
    Collada::Material &src_mat = collada.state.material_map[p_target];
    ERR_FAIL_COND_V(!collada.state.effect_map.contains(src_mat.instance_effect), ERR_INVALID_PARAMETER);
    Collada::Effect &effect = collada.state.effect_map[src_mat.instance_effect];

    Ref<SpatialMaterial> material(make_ref_counted<SpatialMaterial>());

    if (!src_mat.name.empty())
        material->set_name(src_mat.name);
    else if (!effect.name.empty())
        material->set_name(effect.name);

    // DIFFUSE

    if (!effect.diffuse.texture.empty()) {

        String texfile = effect.get_texture_path(effect.diffuse.texture, collada);
        if (!texfile.empty()) {

            if (StringUtils::begins_with(texfile,"/")) {
                texfile = StringUtils::replace_first(texfile,"/", "res://");
            }
            Ref<Texture> texture(dynamic_ref_cast<Texture>(gResourceManager().load(texfile, "Texture")));
            if (texture) {

                material->set_texture(SpatialMaterial::TEXTURE_ALBEDO, texture);
                material->set_albedo(Color(1, 1, 1, 1));
                //material->set_parameter(SpatialMaterial::PARAM_DIFFUSE,Color(1,1,1,1));
            } else {
                missing_textures.emplace_back(PathUtils::get_file(texfile));
            }
        }
    } else {
        material->set_albedo(effect.diffuse.color);
    }

    // SPECULAR

    if (!effect.specular.texture.empty()) {

        String texfile = effect.get_texture_path(effect.specular.texture, collada);
        if (!texfile.empty()) {

            if (StringUtils::begins_with(texfile,"/")) {
                texfile = StringUtils::replace_first(texfile,"/", "res://");
            }

            Ref<Texture> texture = dynamic_ref_cast<Texture>(gResourceManager().load(texfile, "Texture"));
            if (texture) {
                material->set_texture(SpatialMaterial::TEXTURE_METALLIC, texture);
                material->set_specular(1.0);

                //material->set_texture(SpatialMaterial::PARAM_SPECULAR,texture);
                //material->set_parameter(SpatialMaterial::PARAM_SPECULAR,Color(1,1,1,1));
            } else {
                missing_textures.emplace_back(PathUtils::get_file(texfile));
            }
        }

    } else {
        material->set_metallic(effect.specular.color.get_v());
    }

    // EMISSION

    if (!effect.emission.texture.empty()) {

        String texfile = effect.get_texture_path(effect.emission.texture, collada);
        if (!texfile.empty()) {

            if (StringUtils::begins_with(texfile,"/")) {
                texfile = StringUtils::replace_first(texfile,"/", "res://");
            }

            Ref<Texture> texture(dynamic_ref_cast<Texture>(gResourceManager().load(texfile, "Texture")));
            if (texture) {

                material->set_feature(SpatialMaterial::FEATURE_EMISSION, true);
                material->set_texture(SpatialMaterial::TEXTURE_EMISSION, texture);
                material->set_emission(Color(1, 1, 1, 1));

                //material->set_parameter(SpatialMaterial::PARAM_EMISSION,Color(1,1,1,1));
            } else {
                missing_textures.emplace_back(PathUtils::get_file(texfile));
            }
        }
    } else {
        if (effect.emission.color != Color()) {
            material->set_feature(SpatialMaterial::FEATURE_EMISSION, true);
            material->set_emission(effect.emission.color);
        }
    }

    // NORMAL

    if (!effect.bump.texture.empty()) {

        String texfile = effect.get_texture_path(effect.bump.texture, collada);
        if (!texfile.empty()) {

            if (StringUtils::begins_with(texfile,"/")) {
                texfile = StringUtils::replace_first(texfile,"/", "res://");
            }

            Ref<Texture> texture(dynamic_ref_cast<Texture>(gResourceManager().load(texfile, "Texture")));
            if (texture) {
                material->set_feature(SpatialMaterial::FEATURE_NORMAL_MAPPING, true);
                material->set_texture(SpatialMaterial::TEXTURE_NORMAL, texture);
                //material->set_emission(Color(1,1,1,1));

                //material->set_texture(SpatialMaterial::PARAM_NORMAL,texture);
            } else {
                //missing_textures.push_back(PathUtils::get_file(texfile));
            }
        }
    }

    float roughness = (effect.shininess - 1.0f) / 510;
    material->set_roughness(roughness);

    if (effect.double_sided) {
        material->set_cull_mode(SpatialMaterial::CULL_DISABLED);
    }
    material->set_flag(SpatialMaterial::FLAG_UNSHADED, effect.unshaded);

    material_cache[p_target] = material;
    return OK;
}

Error ColladaImport::_create_mesh_surfaces(bool p_optimize, Ref<ArrayMesh> &p_mesh,
        const HashMap<String, Collada::NodeGeometry::Material> &p_material_map, const Collada::MeshData &meshdata,
        const Transform &p_local_xform, const Vector<int> &bone_remap,
        const Collada::SkinControllerData *p_skin_controller, const Collada::MorphControllerData *p_morph_data,
        Vector<Ref<ArrayMesh>> p_morph_meshes, uint32_t p_use_compression, bool p_use_mesh_material) {

    bool local_xform_mirror = p_local_xform.basis.determinant() < 0;

    if (p_morph_data) {

        //add morphie target
        ERR_FAIL_COND_V(!p_morph_data->targets.contains("MORPH_TARGET"), ERR_INVALID_DATA);
        String mt = p_morph_data->targets.at("MORPH_TARGET");
        ERR_FAIL_COND_V(!p_morph_data->sources.contains(mt), ERR_INVALID_DATA);
        int morph_targets = p_morph_data->sources.at(mt).sarray.size();
        for (int i = 0; i < morph_targets; i++) {

            String target = p_morph_data->sources.at(mt).sarray[i];
            ERR_FAIL_COND_V(!collada.state.mesh_data_map.contains(target), ERR_INVALID_DATA);
            String name = collada.state.mesh_data_map[target].name;

            p_mesh->add_blend_shape(StringName(name));
        }
        if (p_morph_data->mode == "RELATIVE")
            p_mesh->set_blend_shape_mode(Mesh::BLEND_SHAPE_MODE_RELATIVE);
        else if (p_morph_data->mode == "NORMALIZED")
            p_mesh->set_blend_shape_mode(Mesh::BLEND_SHAPE_MODE_NORMALIZED);
    }

    int surface = 0;
    for (int p_i = 0; p_i < meshdata.primitives.size(); p_i++) {

        const Collada::MeshData::Primitives &p = meshdata.primitives[p_i];

        /* VERTEX SOURCE */
        ERR_FAIL_COND_V(!p.sources.contains("VERTEX"), ERR_INVALID_DATA);

        String vertex_src_id = p.sources.at("VERTEX").source;
        int vertex_ofs = p.sources.at("VERTEX").offset;

        ERR_FAIL_COND_V(!meshdata.vertices.contains(vertex_src_id), ERR_INVALID_DATA);

        ERR_FAIL_COND_V(!meshdata.vertices.at(vertex_src_id).sources.contains("POSITION"), ERR_INVALID_DATA);
        String position_src_id = meshdata.vertices.at(vertex_src_id).sources.at("POSITION");

        ERR_FAIL_COND_V(!meshdata.sources.contains(position_src_id), ERR_INVALID_DATA);

        const Collada::MeshData::Source *vertex_src = &meshdata.sources.at(position_src_id);

        /* NORMAL SOURCE */

        const Collada::MeshData::Source *normal_src = nullptr;

        auto locateSource = [&](StringView name, int &tgt_ofs, const Collada::MeshData::Source *&src) -> Error {
            String source_id;
            if (p.sources.contains_as(name)) {
                source_id = p.sources.at_as(name,{}).source;
                tgt_ofs = p.sources.at_as(name,{}).offset;
            } else if (meshdata.vertices.at(vertex_src_id).sources.contains_as(name)) {
                source_id = meshdata.vertices.at(vertex_src_id).sources.at_as(name,{});
                tgt_ofs = vertex_ofs;
            }

            if (source_id != "") {
                ERR_FAIL_COND_V(!meshdata.sources.contains(source_id), ERR_INVALID_DATA);
                src = &meshdata.sources.at(source_id);
        }
            return Error::OK;
        };
        int normal_ofs = 0;
        Error err = locateSource("NORMAL", normal_ofs, normal_src);
        if (err != OK)
            return err;

        const Collada::MeshData::Source *binormal_src = nullptr;
        int binormal_ofs = 0;

        err = locateSource("TEXBINORMAL", binormal_ofs, binormal_src);
        if (err != OK)
            return err;

        const Collada::MeshData::Source *tangent_src = nullptr;
        int tangent_ofs = 0;

        err = locateSource("TEXTANGENT", tangent_ofs, tangent_src);
        if (err != OK)
            return err;

        const Collada::MeshData::Source *uv_src = nullptr;
        int uv_ofs = 0;

        err = locateSource("TEXCOORD0", uv_ofs, uv_src);
        if (err != OK)
            return err;

        const Collada::MeshData::Source *uv2_src = nullptr;
        int uv2_ofs = 0;

        err = locateSource("TEXCOORD1", uv2_ofs, uv2_src);
        if (err != OK)
            return err;

        const Collada::MeshData::Source *color_src = nullptr;
        int color_ofs = 0;

        err = locateSource("COLOR", color_ofs, color_src);
        if (err != OK)
            return err;

        //find largest source..

        /************************/
        /* ADD WEIGHTS IF EXIST */
        /************************/

        HashMap<int, Vector<Collada::Vertex::Weight> > pre_weights;

        bool has_weights = false;

        if (p_skin_controller) {

            const Collada::SkinControllerData::Source *weight_src = nullptr;
            int weight_ofs = 0;

            if (p_skin_controller->weights.sources.contains("WEIGHT")) {

                String weight_id = p_skin_controller->weights.sources.at("WEIGHT").source;
                weight_ofs = p_skin_controller->weights.sources.at("WEIGHT").offset;
                if (p_skin_controller->sources.contains(weight_id)) {

                    weight_src = &p_skin_controller->sources.at(weight_id);
                }
            }

            int joint_ofs = 0;

            if (p_skin_controller->weights.sources.contains("JOINT")) {

                joint_ofs = p_skin_controller->weights.sources.at("JOINT").offset;
            }

            //should be OK, given this was pre-checked.

            int index_ofs = 0;
            int wstride = p_skin_controller->weights.sources.size();
            for (size_t w_i = 0; w_i < p_skin_controller->weights.sets.size(); w_i++) {

                int amount = p_skin_controller->weights.sets[w_i];

                Vector<Collada::Vertex::Weight> weights;

                for (int a_i = 0; a_i < amount; a_i++) {

                    Collada::Vertex::Weight w;

                    int read_from = index_ofs + a_i * wstride;
                    ERR_FAIL_INDEX_V(
                            read_from + wstride - 1, p_skin_controller->weights.indices.size(), ERR_INVALID_DATA);
                    int weight_index = p_skin_controller->weights.indices[read_from + weight_ofs];
                    ERR_FAIL_INDEX_V(weight_index, weight_src->array.size(), ERR_INVALID_DATA);

                    w.weight = weight_src->array[weight_index];

                    int bone_index = p_skin_controller->weights.indices[read_from + joint_ofs];
                    if (bone_index == -1)
                        continue; //ignore this weight (refers to bind shape)
                    ERR_FAIL_INDEX_V(bone_index, bone_remap.size(), ERR_INVALID_DATA);

                    w.bone_idx = bone_remap[bone_index];

                    weights.emplace_back(w);
                }

                /* FIX WEIGHTS */
                eastl::sort(weights.begin(),weights.end());

                if (weights.size() > 4) {
                    //cap to 4 and make weights add up 1
                    weights.resize(4);
                }

                //make sure weights always add up to 1
                float total = 0;
                for (int i = 0; i < weights.size(); i++)
                    total += weights[i].weight;
                if (total)
                    for (int i = 0; i < weights.size(); i++)
                        weights[i].weight /= total;

                if (weights.empty() || total == 0) { //if nothing, add a weight to bone 0
                    //no weights assigned
                    Collada::Vertex::Weight w;
                    w.bone_idx = 0;
                    w.weight = 1.0;
                    weights.clear();
                    weights.push_back(w);
                }

                pre_weights[w_i] = eastl::move(weights);

                index_ofs += wstride * amount;
            }

            //vertices need to be localized
            has_weights = true;
        }

        Set<Collada::Vertex> vertex_set; //vertex set will be the vertices
        Vector<int> indices_list; //indices will be the indices

        /**************************/
        /* CREATE PRIMITIVE ARRAY */
        /**************************/

        // The way collada uses indices is more optimal, and friendlier with 3D modelling software,
        // because it can index everything, not only vertices (similar to how the WII works).
        // This is, however, more incompatible with standard video cards, so arrays must be converted.
        // Must convert to GL/DX format.

        int _prim_ofs = 0;
        int vertidx = 0;
        for (int p_j = 0; p_j < p.count; p_j++) {

            int amount;
            if (!p.polygons.empty()) {

                ERR_FAIL_INDEX_V(p_j, p.polygons.size(), ERR_INVALID_DATA);
                amount = p.polygons[p_j];
            } else {
                amount = 3; //triangles;
            }

            //COLLADA_PRINT("amount: "+itos(amount));

            int prev2[2] = { 0, 0 };

            for (int j = 0; j < amount; j++) {

                int src = _prim_ofs;
                //_prim_ofs+=p.sources.size()

                ERR_FAIL_INDEX_V(src, p.indices.size(), ERR_INVALID_DATA);

                Collada::Vertex vertex;
                if (!p_optimize)
                    vertex.uid = vertidx++;

                int vertex_index = p.indices[src + vertex_ofs]; //used for index field (later used by controllers)
                int vertex_pos = (vertex_src->stride ? vertex_src->stride : 3) * vertex_index;
                ERR_FAIL_INDEX_V(vertex_pos + 2, vertex_src->array.size(), ERR_INVALID_DATA);
                vertex.vertex = Vector3(vertex_src->array[vertex_pos + 0], vertex_src->array[vertex_pos + 1],
                        vertex_src->array[vertex_pos + 2]);

                if (pre_weights.contains(vertex_index)) {
                    vertex.weights = pre_weights[vertex_index];
                }

                if (normal_src) {

                    int normal_pos = (normal_src->stride ? normal_src->stride : 3) * p.indices[src + normal_ofs];
                    ERR_FAIL_INDEX_V(normal_pos + 2, normal_src->array.size(), ERR_INVALID_DATA);
                    vertex.normal = Vector3(normal_src->array[normal_pos + 0], normal_src->array[normal_pos + 1],
                            normal_src->array[normal_pos + 2]);

                    if (tangent_src && binormal_src) {
                        int binormal_pos =
                                (binormal_src->stride ? binormal_src->stride : 3) * p.indices[src + binormal_ofs];
                        ERR_FAIL_INDEX_V(binormal_pos + 2, binormal_src->array.size(), ERR_INVALID_DATA);
                        Vector3 binormal = Vector3(binormal_src->array[binormal_pos + 0],
                                binormal_src->array[binormal_pos + 1], binormal_src->array[binormal_pos + 2]);

                        int tangent_pos =
                                (tangent_src->stride ? tangent_src->stride : 3) * p.indices[src + tangent_ofs];
                        ERR_FAIL_INDEX_V(tangent_pos + 2, tangent_src->array.size(), ERR_INVALID_DATA);
                        Vector3 tangent = Vector3(tangent_src->array[tangent_pos + 0],
                                tangent_src->array[tangent_pos + 1], tangent_src->array[tangent_pos + 2]);

                        vertex.tangent.normal = tangent;
                        vertex.tangent.d = vertex.normal.cross(tangent).dot(binormal) > 0 ? 1 : -1;
                    }
                }

                if (uv_src) {

                    int uv_pos = (uv_src->stride ? uv_src->stride : 2) * p.indices[src + uv_ofs];
                    ERR_FAIL_INDEX_V(uv_pos + 1, uv_src->array.size(), ERR_INVALID_DATA);
                    vertex.uv = Vector3(uv_src->array[uv_pos + 0], 1.0 - uv_src->array[uv_pos + 1], 0);
                }

                if (uv2_src) {

                    int uv2_pos = (uv2_src->stride ? uv2_src->stride : 2) * p.indices[src + uv2_ofs];
                    ERR_FAIL_INDEX_V(uv2_pos + 1, uv2_src->array.size(), ERR_INVALID_DATA);
                    vertex.uv2 = Vector3(uv2_src->array[uv2_pos + 0], 1.0 - uv2_src->array[uv2_pos + 1], 0);
                }

                if (color_src) {

                    int color_pos = (color_src->stride ? color_src->stride : 3) *
                                    p.indices[src + color_ofs]; // colors are RGB in collada..
                    ERR_FAIL_INDEX_V(color_pos, color_src->array.size(), ERR_INVALID_DATA);
                    vertex.color = Color(color_src->array[color_pos + 0], color_src->array[color_pos + 1],
                            color_src->array[color_pos + 2],
                            (color_src->stride > 3) ? color_src->array[color_pos + 3] : 1.0);
                }

#ifndef NO_UP_AXIS_SWAP
                if (collada.state.up_axis == Vector3::AXIS_Z) {

                    Vector3 bn = vertex.normal.cross(vertex.tangent.normal) * vertex.tangent.d;

                    SWAP(vertex.vertex.z, vertex.vertex.y);
                    vertex.vertex.z = -vertex.vertex.z;
                    SWAP(vertex.normal.z, vertex.normal.y);
                    vertex.normal.z = -vertex.normal.z;
                    SWAP(vertex.tangent.normal.z, vertex.tangent.normal.y);
                    vertex.tangent.normal.z = -vertex.tangent.normal.z;
                    SWAP(bn.z, bn.y);
                    bn.z = -bn.z;

                    vertex.tangent.d = vertex.normal.cross(vertex.tangent.normal).dot(bn) > 0 ? 1 : -1;
                }

#endif

                vertex.fix_unit_scale(collada);
                int index = 0;
                //COLLADA_PRINT("vertex: "+vertex.vertex);

                if (vertex_set.contains(vertex)) {

                    index = vertex_set.find(vertex)->idx;
                } else {

                    index = vertex_set.size();
                    vertex.idx = index;
                    vertex_set.insert(vertex);
                }

                //build triangles if needed
                if (j == 0)
                    prev2[0] = index;

                if (j >= 2) {
                    //insert indices in reverse order (collada uses CCW as frontface)
                    if (local_xform_mirror) {

                        indices_list.push_back(prev2[0]);
                        indices_list.push_back(prev2[1]);
                        indices_list.push_back(index);

                    } else {
                        indices_list.push_back(prev2[0]);
                        indices_list.push_back(index);
                        indices_list.push_back(prev2[1]);
                    }
                }

                prev2[1] = index;
                _prim_ofs += p.vertex_size;
            }
        }

        Vector<Collada::Vertex> vertex_array; //there we go, vertex array

        vertex_array.resize(vertex_set.size());
        for (Set<Collada::Vertex>::iterator F = vertex_set.begin(); F!=vertex_set.end(); ++F) {

            vertex_array[F->idx] = *F;
        }

        if (has_weights) {

            //if skeleton, localize
            Transform local_xform = p_local_xform;
            for (int i = 0; i < vertex_array.size(); i++) {

                vertex_array[i].vertex = local_xform.xform(vertex_array[i].vertex);
                vertex_array[i].normal = local_xform.basis.xform(vertex_array[i].normal).normalized();
                vertex_array[i].tangent.normal = local_xform.basis.xform(vertex_array[i].tangent.normal).normalized();
                if (local_xform_mirror) {
                    //i shouldn't do this? wtf?
                    //vertex_array[i].normal*=-1.0;
                    //vertex_array[i].tangent.normal*=-1.0;
                }
            }
        }

        /*****************/
        /* MAKE SURFACES  */
        /*****************/

        {

            Ref<SpatialMaterial> material;
            {
                if (p_material_map.contains(p.material)) {
                    String target = p_material_map.at(p.material).target;
                    Error err = OK;
                    if (!material_cache.contains(target)) {
                        err = _create_material(target);
                    }
                    if (!err)
                        material = dynamic_ref_cast<SpatialMaterial>(material_cache[target]);

                } else if (!p.material.empty()) {
                    WARN_PRINT("Collada: Unreferenced material in geometry instance: " + p.material);
                }
            }

            Ref<SurfaceTool> surftool(make_ref_counted<SurfaceTool>());

            surftool->begin(Mesh::PRIMITIVE_TRIANGLES);

            for (int k = 0; k < vertex_array.size(); k++) {
                if (normal_src) {
                    surftool->add_normal(vertex_array[k].normal);
                    if (binormal_src && tangent_src) {
                        surftool->add_tangent(vertex_array[k].tangent);
                    }
                }
                if (uv_src) {
                    surftool->add_uv(Vector2(vertex_array[k].uv.x, vertex_array[k].uv.y));
                }
                if (uv2_src) {
                    surftool->add_uv2(Vector2(vertex_array[k].uv2.x, vertex_array[k].uv2.y));
                }
                if (color_src) {
                    surftool->add_color(vertex_array[k].color);
                }

                if (has_weights) {
                    Vector<float> weights;
                    Vector<int> bones;
                    weights.resize(RS::ARRAY_WEIGHTS_SIZE);
                    bones.resize(RS::ARRAY_WEIGHTS_SIZE);
                    //float sum=0.0;
                    for (int l = 0; l < RS::ARRAY_WEIGHTS_SIZE; l++) {
                        if (l < vertex_array[k].weights.size()) {
                            weights[l] = vertex_array[k].weights[l].weight;
                            bones[l] = vertex_array[k].weights[l].bone_idx;
                            //sum += vertex_array[k].weights[l].weight;
                        } else {

                            weights[l] = 0;
                            bones[l] = 0;
                        }
                    }

                    surftool->add_bones(bones);
                    surftool->add_weights(weights);
                }

                surftool->add_vertex(vertex_array[k].vertex);
            }

            for (int E : indices_list) {
                surftool->add_index(E);
            }

            if (!normal_src) {
                //should always be normals
                surftool->generate_normals();
            }

            if ((!binormal_src || !tangent_src) && normal_src && uv_src && force_make_tangents) {

                surftool->generate_tangents();
            }

            ////////////////////////////
            // FINALLY CREATE SUFRACE //
            ////////////////////////////

            SurfaceArrays d = surftool->commit_to_arrays();
            Vector<SurfaceArrays> mr;

            ////////////////////////////
            // THEN THE MORPH TARGETS //
            ////////////////////////////

            for (int mi = 0; mi < p_morph_meshes.size(); mi++) {

                SurfaceArrays a = p_morph_meshes[mi]->surface_get_arrays(surface);
                // add valid weight and bone arrays if they exist, TODO check if they are unique to shape (generally
                // not)

                if (has_weights) {
                    a.m_weights= d.m_weights;
                    a.m_bones= d.m_bones;
                }

                a.m_indices.clear();
                //a.resize(Mesh::ARRAY_MAX); //no need for index
                mr.emplace_back(eastl::move(a));
            }

            p_mesh->add_surface_from_arrays(
                    Mesh::PRIMITIVE_TRIANGLES, eastl::move(d), eastl::move(mr), p_use_compression);

            if (material) {
                if (p_use_mesh_material) {
                    p_mesh->surface_set_material(surface, material);
                }
                p_mesh->surface_set_name(surface, material->get_name());
            }
        }

        /*****************/
        /* FIND MATERIAL */
        /*****************/

        surface++;
    }

    return OK;
}

Error ColladaImport::_create_resources(Collada::Node *p_node, uint32_t p_use_compression) {

    if (p_node->type == Collada::Node::TYPE_GEOMETRY && node_map.contains(p_node->id)) {

        Node3D *node = node_map[p_node->id].node;
        Collada::NodeGeometry *ng = static_cast<Collada::NodeGeometry *>(p_node);

        if (object_cast<Path3D>(node)) {

            Path3D *path = object_cast<Path3D>(node);

            if (curve_cache.contains(ng->source)) {

                path->set_curve(curve_cache[ng->source]);
            } else {

                Ref<Curve3D> c(make_ref_counted<Curve3D>());

                const Collada::CurveData &cd = collada.state.curve_data_map[ng->source];

                ERR_FAIL_COND_V(!cd.control_vertices.contains("POSITION"), ERR_INVALID_DATA);
                ERR_FAIL_COND_V(!cd.control_vertices.contains("IN_TANGENT"), ERR_INVALID_DATA);
                ERR_FAIL_COND_V(!cd.control_vertices.contains("OUT_TANGENT"), ERR_INVALID_DATA);
                ERR_FAIL_COND_V(!cd.control_vertices.contains("INTERPOLATION"), ERR_INVALID_DATA);

                ERR_FAIL_COND_V(!cd.sources.contains(cd.control_vertices.at("POSITION")), ERR_INVALID_DATA);
                const Collada::CurveData::Source &vertices = cd.sources.at(cd.control_vertices.at("POSITION"));
                ERR_FAIL_COND_V(vertices.stride != 3, ERR_INVALID_DATA);

                ERR_FAIL_COND_V(!cd.sources.contains(cd.control_vertices.at("IN_TANGENT")), ERR_INVALID_DATA);
                const Collada::CurveData::Source &in_tangents = cd.sources.at(cd.control_vertices.at("IN_TANGENT"));
                ERR_FAIL_COND_V(in_tangents.stride != 3, ERR_INVALID_DATA);

                ERR_FAIL_COND_V(!cd.sources.contains(cd.control_vertices.at("OUT_TANGENT")), ERR_INVALID_DATA);
                const Collada::CurveData::Source &out_tangents = cd.sources.at(cd.control_vertices.at("OUT_TANGENT"));
                ERR_FAIL_COND_V(out_tangents.stride != 3, ERR_INVALID_DATA);

                ERR_FAIL_COND_V(!cd.sources.contains(cd.control_vertices.at("INTERPOLATION")), ERR_INVALID_DATA);
                const Collada::CurveData::Source &interps = cd.sources.at(cd.control_vertices.at("INTERPOLATION"));
                ERR_FAIL_COND_V(interps.stride != 1, ERR_INVALID_DATA);

                const Collada::CurveData::Source *tilts = nullptr;
                if (cd.control_vertices.contains("TILT") && cd.sources.contains(cd.control_vertices.at("TILT")))
                    tilts = &cd.sources.at(cd.control_vertices.at("TILT"));

                int pc = vertices.array.size() / 3;
                for (int i = 0; i < pc; i++) {

                    Vector3 pos(vertices.array[i * 3 + 0], vertices.array[i * 3 + 1], vertices.array[i * 3 + 2]);
                    Vector3 in(
                            in_tangents.array[i * 3 + 0], in_tangents.array[i * 3 + 1], in_tangents.array[i * 3 + 2]);
                    Vector3 out(out_tangents.array[i * 3 + 0], out_tangents.array[i * 3 + 1],
                            out_tangents.array[i * 3 + 2]);

#ifndef NO_UP_AXIS_SWAP
                    if (collada.state.up_axis == Vector3::AXIS_Z) {

                        SWAP(pos.y, pos.z);
                        pos.z = -pos.z;
                        SWAP(in.y, in.z);
                        in.z = -in.z;
                        SWAP(out.y, out.z);
                        out.z = -out.z;
                    }
#endif
                    pos *= collada.state.unit_scale;
                    in *= collada.state.unit_scale;
                    out *= collada.state.unit_scale;

                    c->add_point(pos, in - pos, out - pos);
                    if (tilts)
                        c->set_point_tilt(i, tilts->array[i]);
                }

                curve_cache[ng->source] = c;
                path->set_curve(c);
            }
        }

        if (object_cast<MeshInstance3D>(node)) {

            Collada::NodeGeometry *ng2 = static_cast<Collada::NodeGeometry *>(p_node);

            MeshInstance3D *mi = object_cast<MeshInstance3D>(node);

            ERR_FAIL_COND_V(!mi, ERR_BUG);

            Collada::SkinControllerData *skin = nullptr;
            Collada::MorphControllerData *morph = nullptr;
            String meshid;
            Transform apply_xform;
            Vector<int> bone_remap;
            Vector<Ref<ArrayMesh> > morphs;

            if (ng2->controller) {

                String ngsource = ng2->source;

                if (collada.state.skin_controller_data_map.contains(ngsource)) {

                    ERR_FAIL_COND_V(!collada.state.skin_controller_data_map.contains(ngsource), ERR_INVALID_DATA);
                    skin = &collada.state.skin_controller_data_map[ngsource];

                    const Vector<String> &skeletons = ng2->skeletons;

                    ERR_FAIL_COND_V(skeletons.empty(), ERR_INVALID_DATA);

                    String skname = skeletons[0];
                    ERR_FAIL_COND_V(!node_map.contains(skname), ERR_INVALID_DATA);
                    NodeMap nmsk = node_map[skname];
                    Skeleton *sk = object_cast<Skeleton>(nmsk.node);
                    ERR_FAIL_COND_V(!sk, ERR_INVALID_DATA);
                    ERR_FAIL_COND_V(!skeleton_bone_map.contains(sk), ERR_INVALID_DATA);
                    HashMap<String, int> &bone_remap_map = skeleton_bone_map[sk];

                    meshid = skin->base;

                    if (collada.state.morph_controller_data_map.contains(meshid)) {
                        //it's a morph!!
                        morph = &collada.state.morph_controller_data_map[meshid];
                        ngsource = meshid;
                        meshid = morph->mesh;
                    } else {
                        ngsource.clear();
                    }

                    if (apply_mesh_xform_to_vertices) {
                        apply_xform = collada.fix_transform(p_node->default_transform);
                        node->set_transform(Transform());
                    } else {
                        apply_xform = Transform();
                    }

                    ERR_FAIL_COND_V(!skin->weights.sources.contains("JOINT"), ERR_INVALID_DATA);

                    String joint_id = skin->weights.sources["JOINT"].source;
                    ERR_FAIL_COND_V(!skin->sources.contains(joint_id), ERR_INVALID_DATA);

                    Collada::SkinControllerData::Source *joint_src = &skin->sources[joint_id];

                    bone_remap.resize(joint_src->sarray.size());

                    for (size_t i = 0; i < bone_remap.size(); i++) {

                        String str = joint_src->sarray[i];
                        ERR_FAIL_COND_V(!bone_remap_map.contains(str), ERR_INVALID_DATA);
                        bone_remap[i] = bone_remap_map[str];
                    }
                }

                if (collada.state.morph_controller_data_map.contains(ngsource)) {

                    //it's a morph!!
                    morph = &collada.state.morph_controller_data_map[ngsource];
                    meshid = morph->mesh;

                    if (morph->targets.contains("MORPH_TARGET")) {
                        String target = morph->targets["MORPH_TARGET"];
                        bool valid = false;
                        if (morph->sources.contains(target)) {
                            valid = true;
                            const Vector<String> &names = morph->sources[target].sarray;
                            for (const String& meshid2 : names) {
                                if (!collada.state.mesh_data_map.contains(meshid2)) {
                                    valid = false;
                                    continue; // TODO: SEGS: shouldn't this just break out of the loop here ?
                                }
                                Ref<ArrayMesh> mesh(make_ref_counted<ArrayMesh>());
                                const Collada::MeshData &meshdata = collada.state.mesh_data_map[meshid2];
                                mesh->set_name(meshdata.name);
                                Error err = _create_mesh_surfaces(false, mesh, ng2->material_map, meshdata, apply_xform,
                                        bone_remap, skin, nullptr, Vector<Ref<ArrayMesh>>(), false);
                                ERR_FAIL_COND_V(err, err);

                                morphs.push_back(mesh);
                            }
                        }

                        if (!valid)
                            morphs.clear();
                        ngsource = "";
                    }
                }

                ERR_FAIL_COND_V_MSG(!ngsource.empty(), ERR_INVALID_DATA,
                        "Controller instance source '" + ngsource + "' is neither skin or morph!");

            } else {
                meshid = ng2->source;
            }

            Ref<ArrayMesh> mesh;
            if (mesh_cache.contains(meshid)) {
                mesh = mesh_cache[meshid];
            } else {
                if (collada.state.mesh_data_map.contains(meshid)) {
                    //bleh, must ignore invalid

                    ERR_FAIL_COND_V(!collada.state.mesh_data_map.contains(meshid), ERR_INVALID_DATA);
                    mesh = make_ref_counted<ArrayMesh>();
                    const Collada::MeshData &meshdata = collada.state.mesh_data_map[meshid];
                    mesh->set_name(meshdata.name);
                    Error err = _create_mesh_surfaces(morphs.empty(), mesh, ng2->material_map, meshdata, apply_xform,
                            bone_remap, skin, morph, morphs, p_use_compression, use_mesh_builtin_materials);
                    ERR_FAIL_COND_V_MSG(err, err, "Cannot create mesh surface.");

                    mesh_cache[meshid] = mesh;
                } else {

                    WARN_PRINT("Collada: Will not import geometry: " + meshid);
                }
            }

            if (mesh) {

                mi->set_mesh(mesh);
                if (!use_mesh_builtin_materials) {
                    const Collada::MeshData &meshdata = collada.state.mesh_data_map[meshid];

                    for (size_t i = 0; i < meshdata.primitives.size(); i++) {

                        String matname = meshdata.primitives[i].material;

                        if (ng2->material_map.contains(matname)) {
                            String target = ng2->material_map[matname].target;

                            Ref<Material> material;
                            if (!material_cache.contains(target)) {
                                Error err = _create_material(target);
                                if (!err)
                                    material = material_cache[target];
                            } else
                                material = material_cache[target];

                            mi->set_surface_material(i, material);
                        } else if (!matname.empty()) {
                            WARN_PRINT("Collada: Unreferenced material in geometry instance: " + matname);
                        }
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < p_node->children.size(); i++) {

        Error err = _create_resources(p_node->children[i], p_use_compression);
        if (err)
            return err;
    }
    return OK;
}

Error ColladaImport::load(StringView p_path, int p_flags, bool p_force_make_tangents, uint32_t p_use_compression) {

    Error err = collada.load(p_path, p_flags);
    ERR_FAIL_COND_V_MSG(err, err, "Cannot load file '" + p_path + "'.");

    force_make_tangents = p_force_make_tangents;
    ERR_FAIL_COND_V(!collada.state.visual_scene_map.contains(collada.state.root_visual_scene), ERR_INVALID_DATA);
    Collada::VisualScene &vs = collada.state.visual_scene_map[collada.state.root_visual_scene];

    scene = memnew(Node3D); // root

    //determine what's going on with the lights
    for (size_t i = 0; i < vs.root_nodes.size(); i++) {

        _pre_process_lights(vs.root_nodes[i]);
    }
    //import scene

    for (size_t i = 0; i < vs.root_nodes.size(); i++) {

        Error err2 = _create_scene_skeletons(vs.root_nodes[i]);
        if (err2 != OK) {
            memdelete(scene);
            ERR_FAIL_COND_V(err2, err2);
        }
    }

    for (size_t i = 0; i < vs.root_nodes.size(); i++) {

        Error err2 = _create_scene(vs.root_nodes[i], scene);
        if (err2 != OK) {
            memdelete(scene);
            ERR_FAIL_COND_V(err2, err2);
        }

        Error err3 = _create_resources(vs.root_nodes[i], p_use_compression);
        if (err3 != OK) {
            memdelete(scene);
            ERR_FAIL_COND_V(err3, err3);
        }
    }

    //optatively, set unit scale in the root
    scene->set_transform(collada.get_root_transform());

    return OK;
}

void ColladaImport::_fix_param_animation_tracks() {

    for (const eastl::pair<const String, Collada::Node *> &E : collada.state.scene_map) {

        Collada::Node *n = E.second;
        switch (n->type) {

            case Collada::Node::TYPE_NODE: {
                // ? do nothing
            } break;
            case Collada::Node::TYPE_JOINT: {

            } break;
            case Collada::Node::TYPE_SKELETON: {

            } break;
            case Collada::Node::TYPE_LIGHT: {

            } break;
            case Collada::Node::TYPE_CAMERA: {

            } break;
            case Collada::Node::TYPE_GEOMETRY: {

                Collada::NodeGeometry *ng = static_cast<Collada::NodeGeometry *>(n);
                // test source(s)
                String source = ng->source;

                while (!source.empty()) {

                    if (collada.state.skin_controller_data_map.contains(source)) {

                        const Collada::SkinControllerData &skin = collada.state.skin_controller_data_map[source];

                        //nothing to animate here i think

                        source = skin.base;
                    } else if (collada.state.morph_controller_data_map.contains(source)) {

                        const Collada::MorphControllerData &morph = collada.state.morph_controller_data_map[source];

                        if (morph.targets.contains("MORPH_WEIGHT") && morph.targets.contains("MORPH_TARGET")) {

                            String weights = morph.targets.at("MORPH_WEIGHT");
                            String targets = morph.targets.at("MORPH_TARGET");
                            //fails here

                            if (morph.sources.contains(targets) && morph.sources.contains(weights)) {
                                const Collada::MorphControllerData::Source &weight_src = morph.sources.at(weights);
                                const Collada::MorphControllerData::Source &target_src = morph.sources.at(targets);

                                ERR_FAIL_COND(weight_src.array.size() != target_src.sarray.size());

                                for (size_t i = 0; i < weight_src.array.size(); i++) {

                                    String track_name = weights + "(" + itos(i) + ")";
                                    String mesh_name = target_src.sarray[i];
                                    if (collada.state.mesh_name_map.contains(mesh_name) &&
                                            collada.state.referenced_tracks.contains(track_name)) {

                                        const Vector<int> &rt = collada.state.referenced_tracks[track_name];

                                        for (size_t rti = 0; rti < rt.size(); rti++) {
                                            Collada::AnimationTrack *at = &collada.state.animation_tracks[rt[rti]];

                                            at->target = E.first;
                                            at->param = "morph/" + collada.state.mesh_name_map[mesh_name];
                                            at->property = true;
                                            //at->param
                                        }
                                    }
                                }
                            }
                        }
                        source = morph.mesh;
                    } else {

                        source = ""; // for now nothing else supported
                    }
                }

            } break;
        }
    }
}

void ColladaImport::create_animations(bool p_make_tracks_in_all_bones, bool p_import_value_tracks) {

    _fix_param_animation_tracks();
    for (size_t i = 0; i < collada.state.animation_clips.size(); i++) {

        for (size_t j = 0; j < collada.state.animation_clips[i].tracks.size(); j++)
            tracks_in_clips.insert(collada.state.animation_clips[i].tracks[j]);
    }

    for (int i = 0; i < collada.state.animation_tracks.size(); i++) {

        const Collada::AnimationTrack &at = collada.state.animation_tracks[i];

        String node;

        if (!node_map.contains(at.target)) {

            if (node_name_map.contains(at.target)) {

                node = node_name_map[at.target];
            } else {
                WARN_PRINT("Collada: Couldn't find node: " + at.target);
                continue;
            }
        } else {
            node = at.target;
        }

        if (at.property) {

            valid_animated_properties.push_back(i);

        } else {

            node_map[node].anim_tracks.push_back(i);
            valid_animated_nodes.insert(node);
        }
    }

    create_animation(-1, p_make_tracks_in_all_bones, p_import_value_tracks);
    for (int i = 0; i < collada.state.animation_clips.size(); i++)
        create_animation(i, p_make_tracks_in_all_bones, p_import_value_tracks);
}

void ColladaImport::create_animation(int p_clip, bool p_make_tracks_in_all_bones, bool p_import_value_tracks) {

    Ref<Animation> animation(make_ref_counted<Animation>());

    if (p_clip == -1) {
        animation->set_name("default");
    } else {
        animation->set_name(collada.state.animation_clips[p_clip].name);
    }

    for (auto iter=node_map.begin(); iter!=node_map.end(); ++iter) {

        if (iter->second.bone < 0)
            continue;
        bones_with_animation[iter->first] = false;
    }
    //store and validate tracks

    if (p_clip == -1) {
        //main anim
    }

    Set<int> track_filter;

    if (p_clip == -1) {

        for (int i = 0; i < collada.state.animation_clips.size(); i++) {

            int tc = collada.state.animation_clips[i].tracks.size();
            for (int j = 0; j < tc; j++) {

                String n = collada.state.animation_clips[i].tracks[j];
                if (collada.state.by_id_tracks.contains(n)) {

                    const Vector<int> &ti = collada.state.by_id_tracks[n];
                    for (int k = 0; k < ti.size(); k++) {
                        track_filter.insert(ti[k]);
                    }
                }
            }
        }
    } else {

        int tc = collada.state.animation_clips[p_clip].tracks.size();
        for (int j = 0; j < tc; j++) {

            String n = collada.state.animation_clips[p_clip].tracks[j];
            if (collada.state.by_id_tracks.contains(n)) {

                const Vector<int> &ti = collada.state.by_id_tracks[n];
                for (int k = 0; k < ti.size(); k++) {
                    track_filter.insert(ti[k]);
                }
            }
        }
    }

    //animation->set_loop(true);
    //create animation tracks

    Vector<float> base_snapshots;

    float f = 0;
    float snapshot_interval = 1.0f / bake_fps; //should be customizable somewhere...

    float anim_length = collada.state.animation_length;
    if (p_clip >= 0 && collada.state.animation_clips[p_clip].end)
        anim_length = collada.state.animation_clips[p_clip].end;

    while (f < anim_length) {

        base_snapshots.push_back(f);

        f += snapshot_interval;

        if (f >= anim_length) {
            base_snapshots.push_back(anim_length);
        }
    }

    animation->set_length(anim_length);

    bool tracks_found = false;

    for (Set<String>::iterator E = valid_animated_nodes.begin(); E!=valid_animated_nodes.end(); ++E) {

        // take snapshots

        if (!collada.state.scene_map.contains(*E)) {

            continue;
        }

        NodeMap &nm = node_map[*E];
        String path(scene->get_path_to(nm.node));

        if (nm.bone >= 0) {
            Skeleton *sk = static_cast<Skeleton *>(nm.node);
            String name = sk->get_bone_name(nm.bone);
            path = path + ":" + name;
        }

        bool found_anim = false;

        Collada::Node *cn = collada.state.scene_map[*E];
        if (cn->ignore_anim) {

            continue;
        }

        animation->add_track(Animation::TYPE_TRANSFORM);
        int track = animation->get_track_count() - 1;
        animation->track_set_path(track, NodePath(path));
        animation->track_set_imported(track, true); //helps merging later

        Vector<float> snapshots = base_snapshots;

        if (nm.anim_tracks.size() == 1) {
            //use snapshot keys from anim track instead, because this was most likely exported baked
            const Collada::AnimationTrack &at = collada.state.animation_tracks[nm.anim_tracks.front()];
            snapshots.clear();
            for (int i = 0; i < at.keys.size(); i++)
                snapshots.push_back(at.keys[i].time);
        }

        for (int i = 0; i < snapshots.size(); i++) {

            for (int ET : nm.anim_tracks) {
                //apply tracks

                if (p_clip == -1) {

                    if (track_filter.contains(ET)) {

                        continue;
                    }
                } else {

                    if (!track_filter.contains(ET))
                        continue;
                }

                found_anim = true;

                const Collada::AnimationTrack &at = collada.state.animation_tracks[ET];

                int xform_idx = -1;
                for (int j = 0; j < cn->xform_list.size(); j++) {

                    if (cn->xform_list[j].id == at.param) {

                        xform_idx = j;
                        break;
                    }
                }

                if (xform_idx == -1) {
                    WARN_PRINT(
                            "Collada: Couldn't find matching node " + at.target + " xform for track " + at.param + ".");
                    continue;
                }

                Vector<float> data = at.get_value_at_time(snapshots[i]);
                ERR_CONTINUE(data.empty());

                Collada::Node::XForm &xf = cn->xform_list[xform_idx];

                if (at.component == "ANGLE") {
                    ERR_CONTINUE(data.size() != 1);
                    ERR_CONTINUE(xf.op != Collada::Node::XForm::OP_ROTATE);
                    ERR_CONTINUE(xf.data.size() < 4);
                    xf.data[3] = data[0];
                } else if (at.component == "X" || at.component == "Y" || at.component == "Z") {
                    int cn2 = at.component[0] - 'X';
                    ERR_CONTINUE(cn2 >= xf.data.size());
                    ERR_CONTINUE(data.size() > 1);
                    xf.data[cn2] = data[0];
                } else if (data.size() == xf.data.size()) {
                    xf.data = data;
                } else {
                    ERR_CONTINUE_MSG(data.size() != xf.data.size(), "Component " + at.component + " has datasize " +
                                                                            itos(data.size()) + ", xfdatasize " +
                                                                            itos(xf.data.size()) + ".");
                }
            }

            Transform xform = cn->compute_transform(collada);
            xform = collada.fix_transform(xform) * cn->post_transform;

            if (nm.bone >= 0) {
                //make bone transform relative to rest (in case of skeleton)
                Skeleton *sk = object_cast<Skeleton>(nm.node);
                if (sk) {

                    xform = sk->get_bone_rest(nm.bone).affine_inverse() * xform;
                } else {

                    ERR_PRINT("Collada: Invalid skeleton");
                }
            }

            Vector3 s = xform.basis.get_scale();
            bool singular_matrix = Math::is_equal_approx(s.x, 0.0f) || Math::is_equal_approx(s.y, 0.0f) ||
                                   Math::is_equal_approx(s.z, 0.0f);
            Quat q = singular_matrix ? Quat() : xform.basis.get_rotation_quat();
            Vector3 l = xform.origin;

            animation->transform_track_insert_key(track, snapshots[i], l, q, s);
        }

        if (nm.bone >= 0) {
            if (found_anim)
                bones_with_animation[*E] = true;
        }

        if (found_anim)
            tracks_found = true;
        else {
            animation->remove_track(track);
        }
    }

    if (p_make_tracks_in_all_bones) {

        //some bones may lack animation, but since we don't store pose as a property, we must add keyframes!
        for (const eastl::pair<const String, bool> &E : bones_with_animation) {

            if (E.second)
                continue;

            NodeMap &nm = node_map[E.first];
            String path(scene->get_path_to(nm.node));
            ERR_CONTINUE(nm.bone < 0);
            Skeleton *sk = static_cast<Skeleton *>(nm.node);
            String name = sk->get_bone_name(nm.bone);
            path = path + ":" + name;

            Collada::Node *cn = collada.state.scene_map[E.first];
            if (cn->ignore_anim) {
                WARN_PRINT("Collada: Ignoring animation on node: " + path);
                continue;
            }

            animation->add_track(Animation::TYPE_TRANSFORM);
            int track = animation->get_track_count() - 1;
            animation->track_set_path(track, NodePath(path));
            animation->track_set_imported(track, true); //helps merging later

            Transform xform = cn->compute_transform(collada);
            xform = collada.fix_transform(xform) * cn->post_transform;

            xform = sk->get_bone_rest(nm.bone).affine_inverse() * xform;

            Vector3 s = xform.basis.get_scale();
            bool singular_matrix = Math::is_equal_approx(s.x, 0.0f) || Math::is_equal_approx(s.y, 0.0f) ||
                                   Math::is_equal_approx(s.z, 0.0f);
            Quat q = singular_matrix ? Quat() : xform.basis.get_rotation_quat();
            Vector3 l = xform.origin;

            animation->transform_track_insert_key(track, 0, l, q, s);

            tracks_found = true;
        }
    }

    if (p_import_value_tracks) {
        for (int i = 0; i < valid_animated_properties.size(); i++) {

            int ti = valid_animated_properties[i];

            if (p_clip == -1) {

                if (track_filter.contains(ti))
                    continue;
            } else {

                if (!track_filter.contains(ti))
                    continue;
            }

            const Collada::AnimationTrack &at = collada.state.animation_tracks[ti];

            // take snapshots
            if (!collada.state.scene_map.contains(at.target))
                continue;

            NodeMap &nm = node_map[at.target];
            String path(scene->get_path_to(nm.node));

            animation->add_track(Animation::TYPE_VALUE);
            int track = animation->get_track_count() - 1;

            path += ":" + at.param;
            animation->track_set_path(track, NodePath(path));
            animation->track_set_imported(track, true); //helps merging later

            for (int j = 0; j < at.keys.size(); j++) {

                float time = at.keys[j].time;
                Variant value;
                const Vector<float> &data(at.keys[j].data);
                if (data.size() == 1) {
                    //push a float
                    value = data[0];

                } else if (data.size() == 16) {
                    //matrix
                    WARN_PRINT("Collada: Value keys for matrices not supported.");
                } else {
                    WARN_PRINT("Collada: Unexpected amount of value keys: " + itos(data.size()));
                }

                animation->track_insert_key(track, time, value);
            }

            tracks_found = true;
        }
    }

    if (tracks_found) {

        animations.push_back(animation);
    }
}

/*********************************************************************************/
/*************************************** SCENE ***********************************/
/*********************************************************************************/

uint32_t EditorSceneImporterCollada::get_import_flags() const {

    return IMPORT_SCENE | IMPORT_ANIMATION;
}
void EditorSceneImporterCollada::get_extensions(Vector<String> &r_extensions) const {

    r_extensions.push_back("dae");
}

Node *EditorSceneImporterCollada::import_scene(StringView p_path, uint32_t p_flags, int p_bake_fps,
        uint32_t p_compress_flags, Vector<String> *r_missing_deps, Error *r_err) {
    ColladaImport state;
    uint32_t flags = Collada::IMPORT_FLAG_SCENE;
    if (p_flags & IMPORT_ANIMATION)
        flags |= Collada::IMPORT_FLAG_ANIMATION;

    state.use_mesh_builtin_materials = !(p_flags & IMPORT_MATERIALS_IN_INSTANCES);
    state.bake_fps = p_bake_fps;

    Error err = state.load(p_path, flags, p_flags & IMPORT_GENERATE_TANGENT_ARRAYS, p_compress_flags);

    ERR_FAIL_COND_V_MSG(err != OK, nullptr, "Cannot load scene from file '" + p_path + "'.");

    if (!state.missing_textures.empty()) {
        /*
        for(int i=0;i<state.missing_textures.size();i++) {
            EditorNode::add_io_error("Texture Not Found: "+state.missing_textures[i]);
        }
        */

        if (r_missing_deps) {
            for (int i = 0; i < state.missing_textures.size(); i++) {
                // EditorNode::add_io_error("Texture Not Found: "+state.missing_textures[i]);
                r_missing_deps->push_back(state.missing_textures[i]);
            }
        }
    }

    if (p_flags & IMPORT_ANIMATION) {
        state.create_animations(
                p_flags & IMPORT_ANIMATION_FORCE_ALL_TRACKS_IN_ALL_CLIPS, p_flags & IMPORT_ANIMATION_KEEP_VALUE_TRACKS);
        AnimationPlayer *ap = memnew(AnimationPlayer);
        for (size_t i = 0; i < state.animations.size(); i++) {
            String name;
            if (state.animations[i]->get_name().empty())
                name = "default";
            else
                name = state.animations[i]->get_name();

            if (p_flags & IMPORT_ANIMATION_DETECT_LOOP) {
                if (StringUtils::begins_with(name, "loop") || StringUtils::ends_with(name, "loop") ||
                        StringUtils::begins_with(name, "cycle") || StringUtils::ends_with(name, "cycle")) {
                    state.animations[i]->set_loop(true);
                }
            }

            ap->add_animation(StringName(name), state.animations[i]);
        }
        state.scene->add_child(ap);
        ap->set_owner(state.scene);
    }

    return state.scene;
}

Ref<Animation> EditorSceneImporterCollada::import_animation(StringView p_path, uint32_t p_flags, int p_bake_fps) {

    ColladaImport state;

    state.use_mesh_builtin_materials = false;

    Error err = state.load(p_path, Collada::IMPORT_FLAG_ANIMATION, p_flags & IMPORT_GENERATE_TANGENT_ARRAYS);
    ERR_FAIL_COND_V_MSG(err != OK, Ref<Animation>(), "Cannot load animation from file '" + p_path + "'.");

    state.create_animations(
            p_flags & IMPORT_ANIMATION_FORCE_ALL_TRACKS_IN_ALL_CLIPS, p_flags & IMPORT_ANIMATION_KEEP_VALUE_TRACKS);
    memdelete(state.scene);
    state.scene = nullptr;

    if (state.animations.empty())
        return Ref<Animation>();
    Ref<Animation> anim = state.animations[0];
    String base = StringUtils::to_lower(PathUtils::get_basename(p_path));
    if (p_flags & IMPORT_ANIMATION_DETECT_LOOP) {

        if (StringUtils::begins_with(base, "loop") || StringUtils::ends_with(base, "loop") ||
                StringUtils::begins_with(base, "cycle") || StringUtils::ends_with(base, "cycle")) {
            anim->set_loop(true);
        }
    }

    return anim;
}

EditorSceneImporterCollada::EditorSceneImporterCollada() {}
