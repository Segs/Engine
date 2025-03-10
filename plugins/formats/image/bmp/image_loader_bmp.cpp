/*************************************************************************/
/*  image_loader_bmp.cpp                                                 */
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

#include "image_loader_bmp.h"
#include "core/image_data.h"
#include "core/os/file_access.h"
#include "core/ustring.h"

Error ImageLoaderBMP::convert_to_image(ImageData &p_image, const uint8_t *p_buffer, const uint8_t *p_color_buffer,
        uint32_t color_table_size, const bmp_header_s &p_header) {

    Error err = OK;

    if (p_buffer == nullptr)
        err = FAILED;

    if (err == OK) {
        size_t index = 0;
        size_t width = (size_t)p_header.bmp_info_header.bmp_width;
        size_t height = (size_t)p_header.bmp_info_header.bmp_height;
        size_t bits_per_pixel = (size_t)p_header.bmp_info_header.bmp_bit_count;

        // Check whether we can load it

        if (bits_per_pixel == 1) {
            // Requires bit unpacking...
            PLUG_FAIL_COND_V(width % 8 != 0, ERR_UNAVAILABLE)
            PLUG_FAIL_COND_V(height % 8 != 0, ERR_UNAVAILABLE)

        } else if (bits_per_pixel == 4) {
            // Requires bit unpacking...
            PLUG_FAIL_COND_V(width % 2 != 0, ERR_UNAVAILABLE)
            PLUG_FAIL_COND_V(height % 2 != 0, ERR_UNAVAILABLE)

        } else if (bits_per_pixel == 16) {

            PLUG_FAIL_V(ERR_UNAVAILABLE)
        }
        // Image data (might be indexed)
        int data_len = width * height; // indexed

        if (bits_per_pixel > 8) {
            // assume full color
            data_len *= 4;
        }
        PLUG_FAIL_COND_V(data_len == 0, ERR_BUG)
        err = p_image.data.resize(data_len);

        PoolVector<uint8_t>::Write data_w = p_image.data.write();
        uint8_t *write_buffer = data_w.ptr();

        const uint32_t width_bytes = width * bits_per_pixel / 8;
        const uint32_t line_width = (width_bytes + 3) & ~3;

        // The actual data traversal is determined by
        // the data width in case of 8/4/1 bit images
        const uint32_t w = bits_per_pixel >= 24 ? width : width_bytes;
        const uint8_t *line = p_buffer + (line_width * (height - 1));
        const uint8_t *end_buffer =
                p_buffer + p_header.bmp_file_header.bmp_file_size - p_header.bmp_file_header.bmp_file_offset;

        for (size_t i = 0; i < height; i++) {
            const uint8_t *line_ptr = line;

            for (unsigned int j = 0; j < w; j++) {
                ERR_FAIL_COND_V(line_ptr >= end_buffer, ERR_FILE_CORRUPT);
                switch (bits_per_pixel) {
                    case 1: {
                        uint8_t color_index = *line_ptr;

                        write_buffer[index + 0] = (color_index >> 7) & 1;
                        write_buffer[index + 1] = (color_index >> 6) & 1;
                        write_buffer[index + 2] = (color_index >> 5) & 1;
                        write_buffer[index + 3] = (color_index >> 4) & 1;
                        write_buffer[index + 4] = (color_index >> 3) & 1;
                        write_buffer[index + 5] = (color_index >> 2) & 1;
                        write_buffer[index + 6] = (color_index >> 1) & 1;
                        write_buffer[index + 7] = (color_index >> 0) & 1;

                        index += 8;
                        line_ptr += 1;
                    } break;
                    case 4: {
                        uint8_t color_index = *line_ptr;

                        write_buffer[index + 0] = (color_index >> 4) & 0x0f;
                        write_buffer[index + 1] = color_index & 0x0f;

                        index += 2;
                        line_ptr += 1;
                    } break;
                    case 8: {
                        uint8_t color_index = *line_ptr;

                        write_buffer[index] = color_index;

                        index += 1;
                        line_ptr += 1;
                    } break;
                    case 24: {
                        write_buffer[index + 2] = line_ptr[0];
                        write_buffer[index + 1] = line_ptr[1];
                        write_buffer[index + 0] = line_ptr[2];
                        write_buffer[index + 3] = 0xff;

                        index += 4;
                        line_ptr += 3;
                    } break;
                    case 32: {
                        write_buffer[index + 2] = line_ptr[0];
                        write_buffer[index + 1] = line_ptr[1];
                        write_buffer[index + 0] = line_ptr[2];
                        write_buffer[index + 3] = line_ptr[3];

                        index += 4;
                        line_ptr += 4;
                    } break;
                }
            }
            line -= line_width;
        }

        if (p_color_buffer == nullptr || color_table_size == 0) { // regular pixels
            p_image.width = width;
            p_image.height = height;
            p_image.mipmaps = false;
            p_image.format = ImageData::FORMAT_RGBA8;
        } else { // data is in indexed format, extend it

            // Palette data
            PoolVector<uint8_t> palette_data;
            palette_data.resize(color_table_size * 4);

            PoolVector<uint8_t>::Write palette_data_w = palette_data.write();
            uint8_t *pal = palette_data_w.ptr();

            const uint8_t *cb = p_color_buffer;

            for (unsigned int i = 0; i < color_table_size; ++i) {
                pal[i * 4 + 0] = cb[2];
                pal[i * 4 + 1] = cb[1];
                pal[i * 4 + 2] = cb[0];
                pal[i * 4 + 3] = 0xff;

                cb += 4;
            }
            // Extend palette to image
            PoolVector<uint8_t> extended_data;
            extended_data.resize(p_image.data.size() * 4);

            PoolVector<uint8_t>::Write ex_w = extended_data.write();
            uint8_t *dest = ex_w.ptr();

            const int num_pixels = width * height;

            for (int i = 0; i < num_pixels; i++) {
                dest[0] = pal[write_buffer[i] * 4 + 0];
                dest[1] = pal[write_buffer[i] * 4 + 1];
                dest[2] = pal[write_buffer[i] * 4 + 2];
                dest[3] = pal[write_buffer[i] * 4 + 3];

                dest += 4;
            }
            p_image.width = width;
            p_image.height = height;
            p_image.mipmaps = false;
            p_image.format = ImageData::FORMAT_RGBA8;
            p_image.data = eastl::move(extended_data);
        }
    }
    return err;
}

Error ImageLoaderBMP::load_image(ImageData &p_image, FileAccess *f, LoadParams params) {

    bmp_header_s bmp_header;
    Error err = ERR_INVALID_DATA;

    // A valid bmp file should always at least have a
    // file header and a minimal info header
    if (f->get_len() > BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_MIN_SIZE) {
        // File Header
        bmp_header.bmp_file_header.bmp_signature = f->get_16();
        if (bmp_header.bmp_file_header.bmp_signature == BITMAP_SIGNATURE) {
            bmp_header.bmp_file_header.bmp_file_size = f->get_32();
            bmp_header.bmp_file_header.bmp_file_padding = f->get_32();
            bmp_header.bmp_file_header.bmp_file_offset = f->get_32();

            // Info Header
            bmp_header.bmp_info_header.bmp_header_size = f->get_32();
            PLUG_FAIL_COND_V(bmp_header.bmp_info_header.bmp_header_size < BITMAP_INFO_HEADER_MIN_SIZE, ERR_FILE_CORRUPT)

            bmp_header.bmp_info_header.bmp_width = f->get_32();
            bmp_header.bmp_info_header.bmp_height = f->get_32();

            bmp_header.bmp_info_header.bmp_planes = f->get_16();
            PLUG_FAIL_COND_V(bmp_header.bmp_info_header.bmp_planes != 1, ERR_FILE_CORRUPT)

            bmp_header.bmp_info_header.bmp_bit_count = f->get_16();
            bmp_header.bmp_info_header.bmp_compression = f->get_32();
            bmp_header.bmp_info_header.bmp_size_image = f->get_32();
            bmp_header.bmp_info_header.bmp_pixels_per_meter_x = f->get_32();
            bmp_header.bmp_info_header.bmp_pixels_per_meter_y = f->get_32();
            bmp_header.bmp_info_header.bmp_colors_used = f->get_32();
            bmp_header.bmp_info_header.bmp_important_colors = f->get_32();

            switch (bmp_header.bmp_info_header.bmp_compression) {
                case BI_RLE8:
                case BI_RLE4:
                case BI_CMYKRLE8:
                case BI_CMYKRLE4: {
                    // Stop parsing
                    String bmp_path = f->get_path();
                    f->close();
                    PLUG_FAIL_V_MSG(ERR_UNAVAILABLE, "Compressed BMP files are not supported: " + bmp_path + ".")
                } break;
            }
            // Don't rely on sizeof(bmp_file_header) as structure padding
            // adds 2 bytes offset leading to misaligned color table reading
            uint32_t ct_offset = BITMAP_FILE_HEADER_SIZE + bmp_header.bmp_info_header.bmp_header_size;
            f->seek(ct_offset);

            uint32_t color_table_size = 0;

            // bmp_colors_used may report 0 despite having a color table
            // for 4 and 1 bit images, so don't rely on this information
            if (bmp_header.bmp_info_header.bmp_bit_count <= 8) {
                // Support 256 colors max
                color_table_size = 1 << bmp_header.bmp_info_header.bmp_bit_count;
                PLUG_FAIL_COND_V(color_table_size == 0, ERR_BUG)
            }

            PoolVector<uint8_t> bmp_color_table;
            // Color table is usually 4 bytes per color -> [B][G][R][0]
            bmp_color_table.resize(color_table_size * 4);
            PoolVector<uint8_t>::Write bmp_color_table_w = bmp_color_table.write();
            f->get_buffer(bmp_color_table_w.ptr(), color_table_size * 4);

            f->seek(bmp_header.bmp_file_header.bmp_file_offset);

            uint32_t bmp_buffer_size =
                    (bmp_header.bmp_file_header.bmp_file_size - bmp_header.bmp_file_header.bmp_file_offset);

            PoolVector<uint8_t> bmp_buffer;
            err = bmp_buffer.resize(bmp_buffer_size);
            if (err == OK) {
                PoolVector<uint8_t>::Write bmp_buffer_w = bmp_buffer.write();
                f->get_buffer(bmp_buffer_w.ptr(), bmp_buffer_size);

                PoolVector<uint8_t>::Read bmp_buffer_r = bmp_buffer.read();
                PoolVector<uint8_t>::Read bmp_color_table_r = bmp_color_table.read();
                err = convert_to_image(
                        p_image, bmp_buffer_r.ptr(), bmp_color_table_r.ptr(), color_table_size, bmp_header);
            }
            f->close();
        }
    }
    return err;
}

void ImageLoaderBMP::get_recognized_extensions(Vector<String> &p_extensions) const {

    p_extensions.push_back("bmp");
}

ImageLoaderBMP::ImageLoaderBMP() {}
