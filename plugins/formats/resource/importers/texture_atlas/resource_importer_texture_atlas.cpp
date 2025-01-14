/*************************************************************************/
/*  resource_importer_texture_atlas.cpp                                  */
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

#include "resource_importer_texture_atlas.h"

#include "editor_atlas_packer.h"

#include "core/io/image_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry.h"
#include "core/os/file_access.h"
#include "core/resource/resource_manager.h"
#include "scene/resources/mesh.h"
#include "scene/resources/texture.h"

#include <QResource>
struct PackData {
    Rect2 region;
    bool is_cropped;
    bool is_mesh;
    Vector<int> chart_pieces; // one for region, many for mesh
    Vector<Vector<Vector2>> chart_vertices; // for mesh
    Ref<Image> image;
};


const char *ResourceImporterTextureAtlas::get_importer_name() const {

    return "texture_atlas";
}

const char *ResourceImporterTextureAtlas::get_visible_name() const {

    return "TextureAtlas";
}
void ResourceImporterTextureAtlas::get_recognized_extensions(Vector<String> &p_extensions) const {

    ImageLoader::get_recognized_extensions(p_extensions);
}

StringName ResourceImporterTextureAtlas::get_save_extension() const {
    return "res";
}

StringName ResourceImporterTextureAtlas::get_resource_type() const {

    return "Texture";
}

bool ResourceImporterTextureAtlas::get_option_visibility(const StringName &/*p_option*/, const HashMap<StringName, Variant> &/*p_options*/) const {

    return true;
}

int ResourceImporterTextureAtlas::get_preset_count() const {
    return 0;
}
StringName ResourceImporterTextureAtlas::get_preset_name(int p_idx) const {

    return StringName();
}

void ResourceImporterTextureAtlas::get_import_options(Vector<ResourceImporterInterface::ImportOption> *r_options, int /*p_preset*/) const {

    r_options->push_back(ImportOption(PropertyInfo(VariantType::STRING, "atlas_file", PropertyHint::SaveFile, "*.png"), ""));
    r_options->push_back(ImportOption(PropertyInfo(VariantType::INT, "import_mode", PropertyHint::Enum, "Region,Mesh2D"), 0));
    r_options->push_back(ImportOption(PropertyInfo(VariantType::BOOL, "crop_to_region"), false));
    r_options->push_back(ImportOption(PropertyInfo(VariantType::BOOL, "trim_alpha_border_from_region"), true));
}

StringName ResourceImporterTextureAtlas::get_option_group_file() const {
    return "atlas_file";
}

Error ResourceImporterTextureAtlas::import(StringView /*p_source_file*/, StringView p_save_path, const HashMap<StringName, Variant> & /*p_options*/, Vector<String> & /*r_missing_deps*/,
                                            Vector<String> * /*r_platform_variants*/, Vector<String> * /*r_gen_files*/, Variant * /*r_metadata*/) {

    /* If this happens, it's because the atlas_file field was not filled, so just import a broken texture */

    //use an xpm because it's size independent, the editor images are vector and size dependent
    //it's a simple hack
    QResource res(":/texture_atlas/TextureAtlasImportFailed.png");
    QByteArray uncompr(res.uncompressedData());
    Ref<Image> broken(make_ref_counted<Image>((const uint8_t *)uncompr.data(),(int)uncompr.size()));
    Ref<ImageTexture> broken_texture(make_ref_counted<ImageTexture>());

    broken_texture->create_from_image(broken);

    String target_file = String(p_save_path) + ".tex";

    gResourceManager().save(target_file, RES(broken_texture));

    return OK;
}

static void _plot_triangle(Vector2 *vertices, const Vector2 &p_offset, bool p_transposed, Ref<Image> p_image, const Ref<Image> &p_src_image) {

    int width = p_image->get_width();
    int height = p_image->get_height();
    int src_width = p_src_image->get_width();
    int src_height = p_src_image->get_height();

    int x[3];
    int y[3];

    for (int j = 0; j < 3; j++) {

        x[j] = vertices[j].x;
        y[j] = vertices[j].y;
    }

    // sort the points vertically
    if (y[1] > y[2]) {
        SWAP(x[1], x[2]);
        SWAP(y[1], y[2]);
    }
    if (y[0] > y[1]) {
        SWAP(x[0], x[1]);
        SWAP(y[0], y[1]);
    }
    if (y[1] > y[2]) {
        SWAP(x[1], x[2]);
        SWAP(y[1], y[2]);
    }

    double dx_far = double(x[2] - x[0]) / (y[2] - y[0] + 1);
    double dx_upper = double(x[1] - x[0]) / (y[1] - y[0] + 1);
    double dx_low = double(x[2] - x[1]) / (y[2] - y[1] + 1);
    double xf = x[0];
    double xt = x[0] + dx_upper; // if y[0] == y[1], special case
    int max_y = MIN(y[2], height - p_offset.y - 1);
    for (int yi = y[0]; yi < max_y; yi++) {
        if (yi >= 0) {
            for (int xi = (xf > 0 ? int(xf) : 0); xi < (xt <= src_width ? xt : src_width); xi++) {

                int px = xi, py = yi;
                int sx = px, sy = py;
                sx = CLAMP(sx, 0, src_width-1);
                sy = CLAMP(sy, 0, src_height-1);
                Color color = p_src_image->get_pixel(sx, sy);
                if (p_transposed) {
                    SWAP(px, py);
                }
                px += p_offset.x;
                py += p_offset.y;

                //may have been cropped, so don't blit what is not visible?
                if (px < 0 || px >= width) {
                    continue;
                }
                if (py < 0 || py >= height) {
                    continue;
                }
                p_image->set_pixel(px, py, color);
            }

            for (int xi = (xf < src_width ? int(xf) : src_width - 1); xi >= (xt > 0 ? xt : 0); xi--) {
                int px = xi, py = yi;
                int sx = px, sy = py;
                sx = CLAMP(sx, 0, src_width-1);
                sy = CLAMP(sy, 0, src_height-1);
                Color color = p_src_image->get_pixel(sx, sy);
                if (p_transposed) {
                    SWAP(px, py);
                }
                px += p_offset.x;
                py += p_offset.y;

                //may have been cropped, so don't blit what is not visible?
                if (px < 0 || px >= width) {
                    continue;
                }
                if (py < 0 || py >= height) {
                    continue;
                }
                p_image->set_pixel(px, py, color);
            }
        }
        xf += dx_far;
        if (yi < y[1])
            xt += dx_upper;
        else
            xt += dx_low;
    }
}

Error ResourceImporterTextureAtlas::import_group_file(StringView p_group_file, const Map<String, HashMap<StringName, Variant> > &p_source_file_options, const Map<String, String> &p_base_paths) {

    ERR_FAIL_COND_V(p_source_file_options.empty(), ERR_BUG); //should never happen

    Vector<EditorAtlasPacker::Chart> charts;
    Vector<PackData> pack_data_files;

    pack_data_files.resize(p_source_file_options.size());

    int idx = -1;
    for (const eastl::pair<const String, HashMap<StringName, Variant> > &E : p_source_file_options) {
        ++idx;
        PackData &pack_data = pack_data_files[idx];
        const String &source(E.first);
        const HashMap<StringName, Variant> &options(E.second);

        Ref<Image> image(make_ref_counted<Image>());

        Error err = ImageLoader::load_image(source, image);
        ERR_CONTINUE(err != OK);

        pack_data.image = image;
        pack_data.is_cropped = options.at("crop_to_region").as<bool>();

        ImportMode mode = options.at("import_mode").as<ImportMode>();
        bool trim_alpha_border_from_region = options.at("trim_alpha_border_from_region").as<bool>();

        if (mode == IMPORT_MODE_REGION) {

            pack_data.is_mesh = false;

            EditorAtlasPacker::Chart chart;

            //clip a region from the image
            Rect2 used_rect = Rect2(Vector2(), image->get_size());
            if (trim_alpha_border_from_region) {
                // Clip a region from the image.
                used_rect = image->get_used_rect();
            }
            pack_data.region = used_rect;

            chart.vertices.push_back(used_rect.position);
            chart.vertices.push_back(used_rect.position + Vector2(used_rect.size.x, 0));
            chart.vertices.push_back(used_rect.position + Vector2(used_rect.size.x, used_rect.size.y));
            chart.vertices.push_back(used_rect.position + Vector2(0, used_rect.size.y));
            EditorAtlasPacker::Chart::Face f;
            f.vertex[0] = 0;
            f.vertex[1] = 1;
            f.vertex[2] = 2;
            chart.faces.push_back(f);
            f.vertex[0] = 0;
            f.vertex[1] = 2;
            f.vertex[2] = 3;
            chart.faces.push_back(f);
            chart.can_transpose = false;
            pack_data.chart_vertices.push_back(chart.vertices);
            pack_data.chart_pieces.push_back(charts.size());
            charts.push_back(chart);

        } else {
            pack_data.is_mesh = true;

            Ref<BitMap> bit_map(make_ref_counted<BitMap>());

            bit_map->create_from_image_alpha(image);
            Vector<Vector<Vector2> > polygons(bit_map->clip_opaque_to_polygons(Rect2(0, 0, image->get_width(), image->get_height())));

            for (auto & polygon : polygons) {

                EditorAtlasPacker::Chart chart;
                chart.vertices = polygon;
                chart.can_transpose = true;

                Vector<int> poly = Geometry::triangulate_polygon(polygon);
                for (size_t i = 0; i < poly.size(); i += 3) {

                    EditorAtlasPacker::Chart::Face f;
                    f.vertex[0] = poly[i + 0];
                    f.vertex[1] = poly[i + 1];
                    f.vertex[2] = poly[i + 2];
                    chart.faces.push_back(f);
                }

                pack_data.chart_pieces.push_back(charts.size());
                charts.emplace_back(eastl::move(chart));

                pack_data.chart_vertices.emplace_back(eastl::move(polygon));
            }
        }
    }

    //pack the charts
    int atlas_width, atlas_height;
    EditorAtlasPacker::chart_pack(charts, atlas_width, atlas_height);

    //blit the atlas
    Ref<Image> new_atlas(make_ref_counted<Image>());

    new_atlas->create(atlas_width, atlas_height, false, ImageData::FORMAT_RGBA8);

    new_atlas->lock();

    for (PackData &pack_data : pack_data_files) {

        pack_data.image->lock();
        for (int j = 0; j < pack_data.chart_pieces.size(); j++) {
            const EditorAtlasPacker::Chart &chart = charts[pack_data.chart_pieces[j]];
            for (int k = 0; k < chart.faces.size(); k++) {
                Vector2 positions[3];
                for (int l = 0; l < 3; l++) {
                    int vertex_idx = chart.faces[k].vertex[l];
                    positions[l] = chart.vertices[vertex_idx];
                }

                _plot_triangle(positions, chart.final_offset, chart.transposed, new_atlas, pack_data.image);
            }
        }
        pack_data.image->unlock();
    }
    new_atlas->unlock();

    //save the atlas

    new_atlas->save_png(p_group_file);

    //update cache if existing, else create
    Ref<Texture> cache;
    if (ResourceCache::has(p_group_file)) {
        Resource *resptr = ResourceCache::get(p_group_file);
        cache = dynamic_ref_cast<Texture>(RES(resptr));
    } else {
        Ref<ImageTexture> res_cache(make_ref_counted<ImageTexture>());

        res_cache->create_from_image(new_atlas);
        res_cache->set_path(p_group_file);
        cache = res_cache;
    }

    //save the images
    idx = -1;
    for (const eastl::pair<const String, HashMap<StringName, Variant> > &E : p_source_file_options) {
        ++idx;
        PackData &pack_data = pack_data_files[idx];

        Ref<Texture> texture;

        if (!pack_data.is_mesh) {
            Vector2 offset = charts[pack_data.chart_pieces[0]].vertices[0] + charts[pack_data.chart_pieces[0]].final_offset;

            //region
            Ref<AtlasTexture> atlas_texture(make_ref_counted<AtlasTexture>());

            atlas_texture->set_atlas(cache);
            atlas_texture->set_region(Rect2(offset, pack_data.region.size));
            if (!pack_data.is_cropped) {
                atlas_texture->set_margin(Rect2(pack_data.region.position, pack_data.image->get_size() - pack_data.region.size));
            }

            texture = atlas_texture;
        } else {
            Ref<ArrayMesh> mesh(make_ref_counted<ArrayMesh>());

            for (size_t i = 0; i < pack_data.chart_pieces.size(); i++) {
                const EditorAtlasPacker::Chart &chart = charts[pack_data.chart_pieces[i]];
                Vector<Vector2> vertices;
                Vector<int> indices;
                Vector<Vector2> uvs;
                size_t vc = chart.vertices.size();
                size_t fc = chart.faces.size();
                vertices.resize(vc);
                uvs.resize(vc);
                indices.resize(fc * 3);

                {
                    for (size_t j = 0; j < vc; j++) {
                        vertices[j] = chart.vertices[j];
                        Vector2 uv = chart.vertices[j];
                        if (chart.transposed) {
                            SWAP(uv.x, uv.y);
                        }
                        uv += chart.final_offset;
                        uv /= new_atlas->get_size(); //normalize uv to 0-1 range
                        uvs[j] = uv;
                    }

                    for (size_t j = 0; j < fc; j++) {
                        indices[j * 3 + 0] = chart.faces[j].vertex[0];
                        indices[j * 3 + 1] = chart.faces[j].vertex[1];
                        indices[j * 3 + 2] = chart.faces[j].vertex[2];
                    }
                }

                SurfaceArrays arrays(eastl::move(vertices));
                arrays.m_uv_1 = eastl::move(uvs);
                arrays.m_indices = eastl::move(indices);

                mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, eastl::move(arrays));
            }

            Ref<MeshTexture> mesh_texture(make_ref_counted<MeshTexture>());

            mesh_texture->set_base_texture(cache);
            mesh_texture->set_image_size(pack_data.image->get_size());
            mesh_texture->set_mesh(Ref<Mesh>(mesh));

            texture = mesh_texture;
            //mesh
        }

        String save_path(p_base_paths.at(E.first) + ".res");
        gResourceManager().save(save_path, Ref<Resource>(texture));
    }

    return OK;
}

ResourceImporterTextureAtlas::ResourceImporterTextureAtlas() {
    Q_INIT_RESOURCE(texture_atlas);
}
