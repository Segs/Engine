/*************************************************************************/
/*  stream_peer.h                                                        */
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

#include "core/reference.h"
#include "core/pool_vector.h"

class GODOT_EXPORT StreamPeer : public RefCounted {
    GDCLASS(StreamPeer, RefCounted)
    OBJ_CATEGORY("Networking")

protected:
    static void _bind_methods();
public:
    //bind helpers
    Error _put_data(Span<const uint8_t> p_data);
    Array _put_partial_data(Span<const uint8_t> p_data);

    Array _get_data(int p_bytes);
    Array _get_partial_data(int p_bytes);


public:
    virtual Error put_data(const uint8_t *p_data, int p_bytes) = 0; ///< put a whole chunk of data, blocking until it sent
    virtual Error put_partial_data(const uint8_t *p_data, int p_bytes, int &r_sent) = 0; ///< put as much data as possible, without blocking.

    virtual Error get_data(uint8_t *p_buffer, int p_bytes) = 0; ///< read p_bytes of data, if p_bytes > available, it will block
    virtual Error get_partial_data(uint8_t *p_buffer, int p_bytes, int &r_received) = 0; ///< read as much data as p_bytes into buffer, if less was read, return in r_received

    virtual int get_available_bytes() const = 0;

    void put_8(int8_t p_val);
    void put_u8(uint8_t p_val);
    void put_16(int16_t p_val);
    void put_u16(uint16_t p_val);
    void put_32(int32_t p_val);
    void put_u32(uint32_t p_val);
    void put_64(int64_t p_val);
    void put_u64(uint64_t p_val);
    void put_float(float p_val);
    void put_double(double p_val);
    void put_string(StringView p_string);
    void put_utf8_string(StringView p_string);
    void put_var(const Variant &p_variant, bool p_full_objects = false);

    uint8_t get_u8();
    int8_t get_8();
    uint16_t get_u16();
    int16_t get_16();
    uint32_t get_u32();
    int32_t get_32();
    uint64_t get_u64();
    int64_t get_64();
    float get_float();
    double get_double();
    String get_string(int p_bytes = -1);
    Variant get_var(bool p_allow_objects = false);

    StreamPeer() {}
};

class GODOT_EXPORT StreamPeerBuffer : public StreamPeer {

    GDCLASS(StreamPeerBuffer, StreamPeer)

    Vector<uint8_t> data;
    int pointer;

protected:
    static void _bind_methods();

public:
    Error put_data(const uint8_t *p_data, int p_bytes) override;
    Error put_partial_data(const uint8_t *p_data, int p_bytes, int &r_sent) override;

    Error get_data(uint8_t *p_buffer, int p_bytes) override;
    Error get_partial_data(uint8_t *p_buffer, int p_bytes, int &r_received) override;

    int get_available_bytes() const override;

    void seek(int p_pos);
    int get_size() const;
    int get_position() const;
    void resize(int p_size);

    void set_data_array(Vector<uint8_t> &&p_data);
    const Vector<uint8_t> &get_data_array() const { return data; }

    void clear();

    Ref<StreamPeerBuffer> duplicate() const;

    StreamPeerBuffer() : pointer(0) {}
};
