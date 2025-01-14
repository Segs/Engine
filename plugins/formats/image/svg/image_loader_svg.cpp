/*************************************************************************/
/*  image_loader_svg.cpp                                                 */
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

#include "image_loader_svg.h"

#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/print_string.h"
#include "core/ustring.h"
#include "core/color.h"
#include "core/string_formatter.h"
#include "core/pool_vector.h"
#include "core/image_data.h"

#include <nanosvg.h>
#include <nanosvgrast.h>

void SVGRasterizer::rasterize(NSVGimage *p_image, float p_tx, float p_ty, float p_scale, unsigned char *p_dst, int p_w, int p_h, int p_stride) {
    nsvgRasterize(rasterizer, p_image, p_tx, p_ty, p_scale, p_dst, p_w, p_h, p_stride);
}

SVGRasterizer::SVGRasterizer() {
    rasterizer = nsvgCreateRasterizer();
}
SVGRasterizer::~SVGRasterizer() {
    nsvgDeleteRasterizer(rasterizer);
}

SVGRasterizer ImageLoaderSVG::rasterizer;

inline void change_nsvg_paint_color(NSVGpaint *p_paint, const uint32_t p_old, const uint32_t p_new) {

    if (p_paint->type == NSVG_PAINT_COLOR) {
        if (p_paint->color << 8 == p_old << 8) {
            p_paint->color = (p_paint->color & 0xFF000000) | (p_new & 0x00FFFFFF);
        }
    }

    if (p_paint->type == NSVG_PAINT_LINEAR_GRADIENT || p_paint->type == NSVG_PAINT_RADIAL_GRADIENT) {
        for (int stop_index = 0; stop_index < p_paint->gradient->nstops; stop_index++) {
            if (p_paint->gradient->stops[stop_index].color << 8 == p_old << 8) {
                p_paint->gradient->stops[stop_index].color = p_new;
            }
        }
    }
}

void ImageLoaderSVG::_convert_colors(NSVGimage *p_svg_image) {

    for (NSVGshape *shape = p_svg_image->shapes; shape != nullptr; shape = shape->next) {

        for (int i = 0; i < replace_colors.old_colors.size(); i++) {
            change_nsvg_paint_color(&(shape->stroke), replace_colors.old_colors[i], replace_colors.new_colors[i]);
            change_nsvg_paint_color(&(shape->fill), replace_colors.old_colors[i], replace_colors.new_colors[i]);
        }
    }
}

void ImageLoaderSVG::set_convert_colors(Vector<eastl::pair<Color,Color>> *p_replace_color) {

    if (p_replace_color) {
        for (const eastl::pair<Color,Color> &entry : *p_replace_color) {
            Color old_color = entry.first;
            Color new_color = entry.second;
            replace_colors.old_colors.push_back(old_color.to_abgr32());
            replace_colors.new_colors.push_back(new_color.to_abgr32());
        }
    } else {
        replace_colors.old_colors.clear();
        replace_colors.new_colors.clear();
    }
}

Error ImageLoaderSVG::_create_image(ImageData &p_image, const PoolVector<uint8_t> *p_data, const LoadParams &params) {
    NSVGimage *svg_image;
    PoolVector<uint8_t>::Read src_r = p_data->read();
    svg_image = nsvgParse((char *)src_r.ptr(), "px", 96);
    if (svg_image == nullptr) {
        ERR_PRINT("SVG Corrupted");
        return ERR_FILE_CORRUPT;
    }
    if (params.p_convert_colors)
        _convert_colors(svg_image);

    float upscale = params.p_will_upsample ? 2.0 : 1.0;

    int w = (int)(svg_image->width * params.p_scale * upscale);
    ERR_FAIL_COND_V_MSG(w > ImageData::MAX_WIDTH, ERR_PARAMETER_RANGE_ERROR,
            FormatVE("Can't create image from SVG with scale %f, the resulting image size exceeds max width.",
                    params.p_scale));

    int h = (int)(svg_image->height * params.p_scale * upscale);
    ERR_FAIL_COND_V_MSG(h > ImageData::MAX_HEIGHT, ERR_PARAMETER_RANGE_ERROR,
            FormatVE("Can't create image from SVG with scale %f, the resulting image size exceeds max height.",
                    params.p_scale));

    p_image.data.resize(w * h * 4);

    PoolVector<uint8_t>::Write dw = p_image.data.write();

    rasterizer.rasterize(svg_image, 0, 0, params.p_scale * upscale, (uint8_t *)dw.ptr(), w, h, w * 4);

    dw.release();
    p_image.width = w;
    p_image.height = h;
    p_image.mipmaps = false;
    p_image.format = ImageData::FORMAT_RGBA8;

    nsvgDelete(svg_image);
    return OK;
}

Error ImageLoaderSVG::load_image(ImageData &p_image, FileAccess *f, LoadParams params) {

    uint64_t size = f->get_len();
    PoolVector<uint8_t> src_image;
    src_image.resize(size + 1);
    PoolVector<uint8_t>::Write src_w = src_image.write();
    f->get_buffer(src_w.ptr(), size);
    src_w.ptr()[size] = '\0';

    return _create_image(p_image, &src_image, params);
}

void ImageLoaderSVG::get_recognized_extensions(Vector<String> &p_extensions) const {

    p_extensions.push_back(("svg"));
}

ImageLoaderSVG::ImageLoaderSVG() {
}

ImageLoaderSVG::ReplaceColors ImageLoaderSVG::replace_colors;
