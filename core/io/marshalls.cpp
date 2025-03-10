/*************************************************************************/
/*  marshalls.cpp                                                        */
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

#include "marshalls.h"

#include "core/os/keyboard.h"
#include "core/print_string.h"
#include "core/reference.h"
#include "core/object_db.h"
#include "core/method_bind.h"
#include "core/math/vector2.h"
#include "core/math/transform.h"
#include "core/math/transform_2d.h"
#include "core/math/quat.h"
#include "core/node_path.h"
#include "core/color.h"
#include "core/rid.h"
#include "core/pool_vector.h"
#include "core/dictionary.h"

#include "core/string_utils.inl"

#include <climits>
#include <cstdio>

IMPL_GDCLASS(EncodedObjectAsID)

void EncodedObjectAsID::_bind_methods() {
    SE_BIND_METHOD(EncodedObjectAsID,set_object_id);
    SE_BIND_METHOD(EncodedObjectAsID,get_object_id);

    ADD_PROPERTY(PropertyInfo(VariantType::INT, "object_id"), "set_object_id", "get_object_id");
}

void EncodedObjectAsID::set_object_id(GameEntity p_id) {
    id = p_id;
}

GameEntity EncodedObjectAsID::get_object_id() const {

    return id;
}

#define _S(a) ((int32_t)a)
#define ERR_FAIL_ADD_OF(a, b, err) ERR_FAIL_COND_V(_S(b) < 0 || _S(a) < 0 || _S(a) > INT_MAX - _S(b), err);
#define ERR_FAIL_MUL_OF(a, b, err) ERR_FAIL_COND_V(_S(a) < 0 || _S(b) <= 0 || _S(a) > INT_MAX / _S(b), err);

#define ENCODE_MASK 0xFF
#define ENCODE_FLAG_64 1 << 16
#define ENCODE_FLAG_OBJECT_AS_ID 1 << 16

static Error _decode_string(const uint8_t *&buf, int &len, int *r_len, String &r_string) {
    ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);

    uint32_t strlen = decode_uint32(buf);
    int32_t pad = 0;

    // Handle padding
    if (strlen % 4) {
        pad = 4 - int(strlen % 4);
    }

    buf += 4;
    len -= 4;

    // Ensure buffer is big enough
    ERR_FAIL_ADD_OF(strlen, pad, ERR_FILE_EOF);
    ERR_FAIL_COND_V(strlen > (1<<24), ERR_INVALID_DATA);
    ERR_FAIL_COND_V(strlen + pad > uint32_t(len), ERR_FILE_EOF);

    String str((const char *)buf, strlen);

    r_string = str;

    // Add padding
    strlen += pad;

    // Update buffer pos, left data count, and return size
    buf += strlen;
    len -= strlen;
    if (r_len) {
        (*r_len) += 4 + strlen;
    }

    return OK;
}

Error decode_variant(Variant &r_variant, const uint8_t *p_buffer, int p_len, int *r_len, bool p_allow_objects) {

    const uint8_t *buf = p_buffer;
    int len = p_len;

    ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);

    uint32_t type = decode_uint32(buf);

    ERR_FAIL_COND_V((type & ENCODE_MASK) >= int(VariantType::VARIANT_MAX), ERR_INVALID_DATA);

    buf += 4;
    len -= 4;
    if (r_len)
        *r_len = 4;
    VariantType vt = VariantType(type & ENCODE_MASK);
    switch (vt) {

        case VariantType::NIL: {

            r_variant = Variant();
        } break;
        case VariantType::BOOL: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            bool val = decode_uint32(buf);
            r_variant = val;
            if (r_len)
                (*r_len) += 4;
        } break;
        case VariantType::INT: {

            if (type & ENCODE_FLAG_64) {
                ERR_FAIL_COND_V(len < 8, ERR_INVALID_DATA);
                int64_t val = decode_uint64(buf);
                r_variant = val;
                if (r_len)
                    (*r_len) += 8;

            } else {
                ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
                int32_t val = decode_uint32(buf);
                r_variant = val;
                if (r_len)
                    (*r_len) += 4;
            }

        } break;
        case VariantType::FLOAT: {

            if (type & ENCODE_FLAG_64) {
                ERR_FAIL_COND_V(len < 8, ERR_INVALID_DATA);
                double val = decode_double(buf);
                r_variant = val;
                if (r_len)
                    (*r_len) += 8;
            } else {
                ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
                float val = decode_float(buf);
                r_variant = val;
                if (r_len)
                    (*r_len) += 4;
            }

        } break;
        case VariantType::STRING: {

            String str;
            Error err = _decode_string(buf, len, r_len, str);
            if (err)
                return err;
            r_variant = str;

        } break;

        // math types
        case VariantType::VECTOR2: {

            ERR_FAIL_COND_V(len < 4 * 2, ERR_INVALID_DATA);
            Vector2 val;
            val.x = decode_float(&buf[0]);
            val.y = decode_float(&buf[4]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 2;

        } break; // 5
        case VariantType::RECT2: {

            ERR_FAIL_COND_V(len < 4 * 4, ERR_INVALID_DATA);
            Rect2 val;
            val.position.x = decode_float(&buf[0]);
            val.position.y = decode_float(&buf[4]);
            val.size.x = decode_float(&buf[8]);
            val.size.y = decode_float(&buf[12]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 4;

        } break;
        case VariantType::VECTOR3: {

            ERR_FAIL_COND_V(len < 4 * 3, ERR_INVALID_DATA);
            Vector3 val;
            val.x = decode_float(&buf[0]);
            val.y = decode_float(&buf[4]);
            val.z = decode_float(&buf[8]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 3;

        } break;
        case VariantType::TRANSFORM2D: {

            ERR_FAIL_COND_V(len < 4 * 6, ERR_INVALID_DATA);
            Transform2D val;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 2; j++) {

                    val.elements[i][j] = decode_float(&buf[(i * 2 + j) * 4]);
                }
            }

            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 6;

        } break;
        case VariantType::PLANE: {

            ERR_FAIL_COND_V(len < 4 * 4, ERR_INVALID_DATA);
            Plane val;
            val.normal.x = decode_float(&buf[0]);
            val.normal.y = decode_float(&buf[4]);
            val.normal.z = decode_float(&buf[8]);
            val.d = decode_float(&buf[12]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 4;

        } break;
        case VariantType::QUAT: {

            ERR_FAIL_COND_V(len < 4 * 4, ERR_INVALID_DATA);
            Quat val;
            val.x = decode_float(&buf[0]);
            val.y = decode_float(&buf[4]);
            val.z = decode_float(&buf[8]);
            val.w = decode_float(&buf[12]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 4;

        } break;
        case VariantType::AABB: {

            ERR_FAIL_COND_V(len < 4 * 6, ERR_INVALID_DATA);
            AABB val;
            val.position.x = decode_float(&buf[0]);
            val.position.y = decode_float(&buf[4]);
            val.position.z = decode_float(&buf[8]);
            val.size.x = decode_float(&buf[12]);
            val.size.y = decode_float(&buf[16]);
            val.size.z = decode_float(&buf[20]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 6;

        } break;
        case VariantType::BASIS: {

            ERR_FAIL_COND_V(len < 4 * 9, ERR_INVALID_DATA);
            Basis val;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {

                    val.elements[i][j] = decode_float(&buf[(i * 3 + j) * 4]);
                }
            }

            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 9;

        } break;
        case VariantType::TRANSFORM: {

            ERR_FAIL_COND_V(len < 4 * 12, ERR_INVALID_DATA);
            Transform val;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {

                    val.basis.elements[i][j] = decode_float(&buf[(i * 3 + j) * 4]);
                }
            }
            val.origin[0] = decode_float(&buf[36]);
            val.origin[1] = decode_float(&buf[40]);
            val.origin[2] = decode_float(&buf[44]);

            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 12;

        } break;

        // misc types
        case VariantType::COLOR: {

            ERR_FAIL_COND_V(len < 4 * 4, ERR_INVALID_DATA);
            Color val;
            val.r = decode_float(&buf[0]);
            val.g = decode_float(&buf[4]);
            val.b = decode_float(&buf[8]);
            val.a = decode_float(&buf[12]);
            r_variant = val;

            if (r_len)
                (*r_len) += 4 * 4;

        } break;
        case VariantType::STRING_NAME: {
            String str;
            Error err = _decode_string(buf, len, r_len, str);
            if (err) {
                return err;
            }
            r_variant = StringName(str);

        } break;
        case VariantType::NODE_PATH: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            uint32_t inc_strlen = decode_uint32(buf);

            if (inc_strlen & 0x80000000) {
                //new format
                ERR_FAIL_COND_V(len < 12, ERR_INVALID_DATA);
                Vector<StringName> names;
                Vector<StringName> subnames;

                uint32_t namecount = inc_strlen & 0x7FFFFFFF;
                uint32_t subnamecount = decode_uint32(buf + 4);
                uint32_t flags = decode_uint32(buf + 8);

                len -= 12;
                buf += 12;

                if (flags & 2) // Obsolete format with property separate from subpath
                    subnamecount++;

                uint32_t total = namecount + subnamecount;

                if (r_len)
                    (*r_len) += 12;

                for (uint32_t i = 0; i < total; i++) {

                    String str;
                    Error err = _decode_string(buf, len, r_len, str);
                    if (err)
                        return err;
                    StringName sname(str);
                    if (i < namecount)
                        names.push_back(sname);
                    else
                        subnames.push_back(sname);
                }

                r_variant = NodePath(eastl::move(names), eastl::move(subnames), flags & 1);

            } else {
                //old format, just a string

                ERR_FAIL_V(ERR_INVALID_DATA);
            }

        } break;
        case VariantType::_RID: {

            r_variant = RID();
        } break;
        case VariantType::OBJECT: {

            if (type & ENCODE_FLAG_OBJECT_AS_ID) {
                //this _is_ allowed
                ERR_FAIL_COND_V(len < 8, ERR_INVALID_DATA);
                GameEntity val(GE(decode_uint64(buf)));
                if (r_len)
                    (*r_len) += 8;

                if (val!=entt::null) {
                    r_variant = Variant((Object *)nullptr);
                } else {
                    Ref<EncodedObjectAsID> obj_as_id(make_ref_counted<EncodedObjectAsID>());
                    obj_as_id->set_object_id(val);

                    r_variant = obj_as_id;
                }

            } else {
                ERR_FAIL_COND_V(!p_allow_objects, ERR_UNAUTHORIZED);

                String str;
                Error err = _decode_string(buf, len, r_len, str);
                if (err)
                    return err;

                if (str.empty()) {
                    r_variant = Variant((Object *)nullptr);
                } else {

                    Object *obj = ClassDB::instance(StringName(str));

                    ERR_FAIL_COND_V(!obj, ERR_UNAVAILABLE);
                    ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);

                    int32_t count = decode_uint32(buf);
                    buf += 4;
                    len -= 4;
                    if (r_len) {
                        (*r_len) += 4;
                    }

                    for (int i = 0; i < count; i++) {

                        str.clear();
                        err = _decode_string(buf, len, r_len, str);
                        if (err)
                            return err;

                        Variant value;
                        int used;
                        err = decode_variant(value, buf, len, &used, p_allow_objects);
                        if (err)
                            return err;

                        buf += used;
                        len -= used;
                        if (r_len) {
                            (*r_len) += used;
                        }

                        obj->set(StringName(str), value);
                    }

                    if (object_cast<RefCounted>(obj)) {
                        REF ref = REF(object_cast<RefCounted>(obj));
                        r_variant = ref;
                    } else {
                        r_variant = Variant(obj);
                    }
                }
            }

        } break;
        case VariantType::DICTIONARY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            //  bool shared = count&0x80000000;
            count &= 0x7FFFFFFF;

            buf += 4;
            len -= 4;

            if (r_len) {
                (*r_len) += 4;
            }

            Dictionary d;

            for (int i = 0; i < count; i++) {

                Variant key, value;

                int used;
                Error err = decode_variant(key, buf, len, &used, p_allow_objects);
                ERR_FAIL_COND_V_MSG(err != OK ||(key.get_type() != VariantType::STRING&& key.get_type() != VariantType::STRING_NAME), err, "Error when trying to decode Variant.");

                buf += used;
                len -= used;
                if (r_len) {
                    (*r_len) += used;
                }

                err = decode_variant(value, buf, len, &used, p_allow_objects);
                ERR_FAIL_COND_V_MSG(err != OK, err, "Error when trying to decode Variant.");

                buf += used;
                len -= used;
                if (r_len) {
                    (*r_len) += used;
                }

                d[key.as<StringName>()] = value;
            }

            r_variant = d;

        } break;
        case VariantType::ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            //  bool shared = count&0x80000000;
            count &= 0x7FFFFFFF;

            buf += 4;
            len -= 4;

            if (r_len) {
                (*r_len) += 4;
            }

            Array varr;

            for (int i = 0; i < count; i++) {

                int used = 0;
                Variant v;
                Error err = decode_variant(v, buf, len, &used, p_allow_objects);
                ERR_FAIL_COND_V_MSG(err != OK, err, "Error when trying to decode Variant.");
                buf += used;
                len -= used;
                varr.push_back(v);
                if (r_len) {
                    (*r_len) += used;
                }
            }

            r_variant = varr;

        } break;

        // arrays
        case VariantType::POOL_BYTE_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            buf += 4;
            len -= 4;
            ERR_FAIL_COND_V(count < 0 || count > len, ERR_INVALID_DATA);

            PoolVector<uint8_t> data;

            if (count) {
                data.resize(count);
                PoolVector<uint8_t>::Write w = data.write();
                for (int32_t i = 0; i < count; i++) {

                    w[i] = buf[i];
                }
            }

            r_variant = data;

            if (r_len) {
                if (count % 4)
                    (*r_len) += 4 - count % 4;
                (*r_len) += 4 + count;
            }

        } break;
        case VariantType::POOL_INT_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            uint32_t count = decode_uint32(buf);
            buf += 4;
            len -= 4;
            ERR_FAIL_MUL_OF(count, 4, ERR_INVALID_DATA);
            ERR_FAIL_COND_V(count * 4 > uint32_t(len), ERR_INVALID_DATA);

            PoolVector<int> data;

            if (count) {
                //const int*rbuf=(const int*)buf;
                data.resize(count);
                PoolVector<int>::Write w = data.write();
                for (int32_t i = 0; i < count; i++) {

                    w[i] = decode_uint32(&buf[i * 4]);
                }
            }
            r_variant = Variant(data);
            if (r_len) {
                (*r_len) += 4 + count * sizeof(int);
            }

        } break;
        case VariantType::POOL_FLOAT32_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            buf += 4;
            len -= 4;
            ERR_FAIL_MUL_OF(count, 4, ERR_INVALID_DATA);
            ERR_FAIL_COND_V(count < 0 || count * 4 > len, ERR_INVALID_DATA);

            PoolVector<float> data;

            if (count) {
                //const float*rbuf=(const float*)buf;
                data.resize(count);
                PoolVector<float>::Write w = data.write();
                for (int32_t i = 0; i < count; i++) {

                    w[i] = decode_float(&buf[i * 4]);
                }
            }
            r_variant = data;

            if (r_len) {
                (*r_len) += 4 + count * sizeof(float);
            }

        } break;
        case VariantType::POOL_STRING_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);

            PoolVector<String> strings;
            buf += 4;
            len -= 4;

            if (r_len)
                (*r_len) += 4;
            //printf("string count: %i\n",count);

            for (int32_t i = 0; i < count; i++) {

                String str;
                Error err = _decode_string(buf, len, r_len, str);
                if (err)
                    return err;

                strings.push_back(str);
            }

            r_variant = strings;

        } break;
        case VariantType::POOL_VECTOR2_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            buf += 4;
            len -= 4;

            ERR_FAIL_MUL_OF(count, 4 * 2, ERR_INVALID_DATA);
            ERR_FAIL_COND_V(count < 0 || count * 4 * 2 > len, ERR_INVALID_DATA);
            PoolVector<Vector2> varray;

            if (r_len) {
                (*r_len) += 4;
            }

            if (count) {
                varray.resize(count);
                PoolVector<Vector2>::Write w = varray.write();

                for (int32_t i = 0; i < count; i++) {

                    w[i].x = decode_float(buf + i * 4 * 2 + 4 * 0);
                    w[i].y = decode_float(buf + i * 4 * 2 + 4 * 1);
                }

                int adv = 4 * 2 * count;

                if (r_len)
                    (*r_len) += adv;
            }

            r_variant = Variant(varray);

        } break;
        case VariantType::POOL_VECTOR3_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            buf += 4;
            len -= 4;

            ERR_FAIL_MUL_OF(count, 4 * 3, ERR_INVALID_DATA);
            ERR_FAIL_COND_V(count < 0 || count * 4 * 3 > len, ERR_INVALID_DATA);

            PoolVector<Vector3> varray;

            if (r_len) {
                (*r_len) += 4;
            }

            if (count) {
                varray.resize(count);
                PoolVector<Vector3>::Write w = varray.write();

                for (int32_t i = 0; i < count; i++) {

                    w[i].x = decode_float(buf + i * 4 * 3 + 4 * 0);
                    w[i].y = decode_float(buf + i * 4 * 3 + 4 * 1);
                    w[i].z = decode_float(buf + i * 4 * 3 + 4 * 2);
                }

                int adv = 4 * 3 * count;

                if (r_len)
                    (*r_len) += adv;
            }

            r_variant = varray;

        } break;
        case VariantType::POOL_COLOR_ARRAY: {

            ERR_FAIL_COND_V(len < 4, ERR_INVALID_DATA);
            int32_t count = decode_uint32(buf);
            buf += 4;
            len -= 4;

            ERR_FAIL_MUL_OF(count, 4 * 4, ERR_INVALID_DATA);
            ERR_FAIL_COND_V(count < 0 || count * 4 * 4 > len, ERR_INVALID_DATA);

            PoolVector<Color> carray;

            if (r_len) {
                (*r_len) += 4;
            }

            if (count) {
                carray.resize(count);
                PoolVector<Color>::Write w = carray.write();

                for (int32_t i = 0; i < count; i++) {

                    w[i].r = decode_float(buf + i * 4 * 4 + 4 * 0);
                    w[i].g = decode_float(buf + i * 4 * 4 + 4 * 1);
                    w[i].b = decode_float(buf + i * 4 * 4 + 4 * 2);
                    w[i].a = decode_float(buf + i * 4 * 4 + 4 * 3);
                }

                int adv = 4 * 4 * count;

                if (r_len)
                    (*r_len) += adv;
            }

            r_variant = carray;

        } break;
        default: {
            ERR_FAIL_V(ERR_BUG);
        }
    }

    return OK;
}

static void _encode_string(StringView p_string, uint8_t *&buf, int &r_len) {

    size_t len=p_string.size();
    if (buf) {
        encode_uint32(len, buf);
        buf += 4;
        memcpy(buf, p_string.data(), len);
        buf += len;
    }

    r_len += 4 + len;
    while (r_len % 4) {
        r_len++; //pad
        if (buf) {
            *(buf++) = 0;
        }
    }
}

Error encode_variant(const Variant &p_variant, uint8_t *r_buffer, int &r_len, bool p_full_objects, int p_depth) {
    ERR_FAIL_COND_V_MSG(p_depth > Variant::MAX_RECURSION_DEPTH, ERR_OUT_OF_MEMORY, "Potential inifite recursion detected. Bailing.");

    uint8_t *buf = r_buffer;

    r_len = 0;

    uint32_t flags = 0;

    switch (p_variant.get_type()) {

        case VariantType::INT: {
            int64_t val = p_variant.as<int64_t>();
            if (val > (int64_t)INT_MAX || val < (int64_t)INT_MIN) {
                flags |= ENCODE_FLAG_64;
            }
        } break;
        case VariantType::FLOAT: {

            double d = p_variant.as<float>();
            float f = d;
            if (double(f) != d) {
                flags |= ENCODE_FLAG_64; //always encode real as double
            }
        } break;
        case VariantType::OBJECT: {
            // Test for potential wrong values sent by the debugger when it breaks or freed objects.
            Object *obj = p_variant.as<Object *>();
            if (!obj) {
                // Object is invalid, send a NULL instead.
                if (buf) {
                    encode_uint32((uint32_t)VariantType::NIL, buf);
                }
                r_len += 4;
                return OK;
            }
            if (!p_full_objects) {
                flags |= ENCODE_FLAG_OBJECT_AS_ID;
            }
        } break;
        default: {
        } // nothing to do at this stage
    }

    if (buf) {
        encode_uint32(uint32_t(p_variant.get_type()) | flags, buf);
        buf += 4;
    }
    r_len += 4;

    switch (p_variant.get_type()) {

        case VariantType::NIL: {

            //nothing to do
        } break;
        case VariantType::BOOL: {

            if (buf) {
                encode_uint32(p_variant.as<bool>(), buf);
            }

            r_len += 4;

        } break;
        case VariantType::INT: {

            if (flags & ENCODE_FLAG_64) {
                //64 bits
                if (buf) {
                    encode_uint64(p_variant.as<int64_t>(), buf);
                }

                r_len += 8;
            } else {
                if (buf) {
                    encode_uint32(p_variant.as<int32_t>(), buf);
                }

                r_len += 4;
            }
        } break;
        case VariantType::FLOAT: {

            if (flags & ENCODE_FLAG_64) {
                if (buf) {
                    encode_double(p_variant.as<double>(), buf);
                }

                r_len += 8;

            } else {

                if (buf) {
                    encode_float(p_variant.as<float>(), buf);
                }

                r_len += 4;
            }

        } break;
        case VariantType::NODE_PATH: {

            NodePath np = p_variant.as<NodePath>();
            if (buf) {
                encode_uint32(uint32_t(np.get_name_count()) | 0x80000000, buf); //for compatibility with the old format
                encode_uint32(np.get_subname_count(), buf + 4);
                uint32_t np_flags = 0;
                if (np.is_absolute())
                    np_flags |= 1;

                encode_uint32(np_flags, buf + 8);

                buf += 12;
            }

            r_len += 12;

            int total = np.get_name_count() + np.get_subname_count();

            for (int i = 0; i < total; i++) {

                StringView str;

                if (i < np.get_name_count())
                    str = np.get_name(i);
                else
                    str = np.get_subname(i - np.get_name_count());

                int pad = 0;

                if (str.length() % 4)
                    pad = 4 - str.length() % 4;

                if (buf) {
                    encode_uint32(str.length(), buf);
                    buf += 4;
                    memcpy(buf, str.data(), str.length());
                    buf += pad + str.length();
                }

                r_len += 4 + str.length() + pad;
            }

        } break;
        case VariantType::STRING: {

            _encode_string(p_variant.as<String>(), buf, r_len);

        } break;

        // math types
        case VariantType::VECTOR2: {

            if (buf) {
                Vector2 v2 = p_variant.as<Vector2>();
                encode_float(v2.x, &buf[0]);
                encode_float(v2.y, &buf[4]);
            }

            r_len += 2 * 4;

        } break; // 5
        case VariantType::RECT2: {

            if (buf) {
                Rect2 r2 = p_variant.as<Rect2>();
                encode_float(r2.position.x, &buf[0]);
                encode_float(r2.position.y, &buf[4]);
                encode_float(r2.size.x, &buf[8]);
                encode_float(r2.size.y, &buf[12]);
            }
            r_len += 4 * 4;

        } break;
        case VariantType::VECTOR3: {

            if (buf) {
                Vector3 v3 = p_variant.as<Vector3>();
                encode_float(v3.x, &buf[0]);
                encode_float(v3.y, &buf[4]);
                encode_float(v3.z, &buf[8]);
            }

            r_len += 3 * 4;

        } break;
        case VariantType::TRANSFORM2D: {

            if (buf) {
                Transform2D val = p_variant.as<Transform2D>();
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 2; j++) {

                        memcpy(&buf[(i * 2 + j) * 4], &val.elements[i][j], sizeof(float));
                    }
                }
            }

            r_len += 6 * 4;

        } break;
        case VariantType::PLANE: {

            if (buf) {
                Plane p = p_variant.as<Plane>();
                encode_float(p.normal.x, &buf[0]);
                encode_float(p.normal.y, &buf[4]);
                encode_float(p.normal.z, &buf[8]);
                encode_float(p.d, &buf[12]);
            }

            r_len += 4 * 4;

        } break;
        case VariantType::QUAT: {

            if (buf) {
                Quat q = p_variant.as<Quat>();
                encode_float(q.x, &buf[0]);
                encode_float(q.y, &buf[4]);
                encode_float(q.z, &buf[8]);
                encode_float(q.w, &buf[12]);
            }

            r_len += 4 * 4;

        } break;
        case VariantType::AABB: {

            if (buf) {
                AABB aabb = p_variant.as<AABB>();
                encode_float(aabb.position.x, &buf[0]);
                encode_float(aabb.position.y, &buf[4]);
                encode_float(aabb.position.z, &buf[8]);
                encode_float(aabb.size.x, &buf[12]);
                encode_float(aabb.size.y, &buf[16]);
                encode_float(aabb.size.z, &buf[20]);
            }

            r_len += 6 * 4;

        } break;
        case VariantType::BASIS: {

            if (buf) {
                Basis val = p_variant.as<Basis>();
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {

                        memcpy(&buf[(i * 3 + j) * 4], &val.elements[i][j], sizeof(float));
                    }
                }
            }

            r_len += 9 * 4;

        } break;
        case VariantType::TRANSFORM: {

            if (buf) {
                Transform val = p_variant.as<Transform>();
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {

                        memcpy(&buf[(i * 3 + j) * 4], &val.basis.elements[i][j], sizeof(float));
                    }
                }

                encode_float(val.origin.x, &buf[36]);
                encode_float(val.origin.y, &buf[40]);
                encode_float(val.origin.z, &buf[44]);
            }

            r_len += 12 * 4;

        } break;

        // misc types
        case VariantType::COLOR: {

            if (buf) {
                Color c = p_variant.as<Color>();
                encode_float(c.r, &buf[0]);
                encode_float(c.g, &buf[4]);
                encode_float(c.b, &buf[8]);
                encode_float(c.a, &buf[12]);
            }

            r_len += 4 * 4;

        } break;
        case VariantType::STRING_NAME: {
            _encode_string((StringName)p_variant, buf, r_len);

        } break;
        case VariantType::_RID: {

        } break;
        case VariantType::OBJECT: {

            if (p_full_objects) {

                Object *obj = p_variant.as<Object *>();
                if (!obj) {
                    if (buf) {
                        encode_uint32(0, buf);
                    }
                    r_len += 4;

                } else {
                    _encode_string(StringView(obj->get_class()), buf, r_len);

                    Vector<PropertyInfo> props;
                    obj->get_property_list(&props);

                    int pc = 0;
                    for(PropertyInfo &E : props ) {

                        if (!(E.usage & PROPERTY_USAGE_STORAGE))
                            continue;
                        pc++;
                    }

                    if (buf) {
                        encode_uint32(pc, buf);
                        buf += 4;
                    }

                    r_len += 4;

                    for(PropertyInfo &E : props ) {

                        if (!(E.usage & PROPERTY_USAGE_STORAGE))
                            continue;

                        _encode_string(E.name, buf, r_len);

                        int len;
                        Error err = encode_variant(obj->get(E.name), buf, len, p_full_objects, p_depth + 1);
                        ERR_FAIL_COND_V(err, err);
                        ERR_FAIL_COND_V(len % 4, ERR_BUG);
                        r_len += len;
                        if (buf)
                            buf += len;
                    }
                }
            } else {
                if (buf) {

                    Object *obj = p_variant.as<Object *>();
                    GameEntity id {entt::null};
                    if (obj) {
                        id = obj->get_instance_id();
                    }

                    encode_uint64(entt::to_integral(id), buf);
                }

                r_len += 8;
            }

        } break;
        case VariantType::DICTIONARY: {

            Dictionary d = p_variant.as<Dictionary>();

            if (buf) {
                encode_uint32(uint32_t(d.size()), buf);
                buf += 4;
            }
            r_len += 4;

            auto keys(d.get_key_list());

            for(auto &E : keys ) {
                Variant *v = d.getptr(E);

                int len;
                Error err = encode_variant(v ? E : Variant("[Deleted Object]"), buf, len, p_full_objects, p_depth + 1);
                ERR_FAIL_COND_V(err, err);
                ERR_FAIL_COND_V(len % 4, ERR_BUG);

                r_len += len;
                if (buf) {
                    buf += len;
                }

                err = encode_variant(v ? *v : Variant(), buf, len, p_full_objects, p_depth + 1);
                ERR_FAIL_COND_V(err, err);
                ERR_FAIL_COND_V(len % 4, ERR_BUG);
                r_len += len;
                if (buf) {
                    buf += len;
                }
            }

        } break;
        case VariantType::ARRAY: {

            Array v = p_variant.as<Array>();

            if (buf) {
                encode_uint32(uint32_t(v.size()), buf);
                buf += 4;
            }

            r_len += 4;

            for (int i = 0; i < v.size(); i++) {

                int len;
                Error err = encode_variant(v.get(i), buf, len, p_full_objects, p_depth + 1);
                ERR_FAIL_COND_V(err, err);
                ERR_FAIL_COND_V(len % 4, ERR_BUG);
                r_len += len;
                if (buf)
                    buf += len;
            }

        } break;
        // arrays
        case VariantType::POOL_BYTE_ARRAY: {

            PoolVector<uint8_t> data = p_variant.as<PoolVector<uint8_t>>();
            int datalen = data.size();
            int datasize = sizeof(uint8_t);

            if (buf) {
                encode_uint32(datalen, buf);
                buf += 4;
                PoolVector<uint8_t>::Read r = data.read();
                memcpy(buf, &r[0], datalen * datasize);
                buf += datalen * datasize;
            }

            r_len += 4 + datalen * datasize;
            while (r_len % 4) {
                r_len++;
                if (buf)
                    *(buf++) = 0;
            }

        } break;
        case VariantType::POOL_INT_ARRAY: {

            PoolVector<int> data = p_variant.as<PoolVector<int>>();
            int datalen = data.size();
            int datasize = sizeof(int32_t);

            if (buf) {
                encode_uint32(datalen, buf);
                buf += 4;
                PoolVector<int>::Read r = data.read();
                for (int i = 0; i < datalen; i++)
                    encode_uint32(r[i], &buf[i * datasize]);
            }

            r_len += 4 + datalen * datasize;

        } break;
        case VariantType::POOL_FLOAT32_ARRAY: {

            PoolVector<real_t> data = p_variant.as<PoolVector<real_t>>();
            int datalen = data.size();
            int datasize = sizeof(real_t);

            if (buf) {
                encode_uint32(datalen, buf);
                buf += 4;
                PoolVector<real_t>::Read r = data.read();
                for (int i = 0; i < datalen; i++)
                    encode_float(r[i], &buf[i * datasize]);
            }

            r_len += 4 + datalen * datasize;

        } break;
        case VariantType::POOL_STRING_ARRAY: {

            PoolVector<String> data = p_variant.as<PoolVector<String>>();
            int len = data.size();

            if (buf) {
                encode_uint32(len, buf);
                buf += 4;
            }

            r_len += 4;

            for (int i = 0; i < len; i++) {

                String utf8(data.get(i));

                if (buf) {
                    encode_uint32(utf8.length(), buf);
                    buf += 4;
                    memcpy(buf, utf8.data(), utf8.length());
                    buf += utf8.length();
                }

                r_len += 4 + utf8.length() + 1;
                while (r_len % 4) {
                    r_len++; //pad
                    if (buf)
                        *(buf++) = 0;
                }
            }

        } break;
        case VariantType::POOL_VECTOR2_ARRAY: {

            PoolVector<Vector2> data = p_variant.as<PoolVector<Vector2>>();
            int len = data.size();

            if (buf) {
                encode_uint32(len, buf);
                buf += 4;
            }

            r_len += 4;

            if (buf) {

                for (int i = 0; i < len; i++) {

                    Vector2 v = data.get(i);

                    encode_float(v.x, &buf[0]);
                    encode_float(v.y, &buf[4]);
                    buf += 4 * 2;
                }
            }

            r_len += 4 * 2 * len;

        } break;
        case VariantType::POOL_VECTOR3_ARRAY: {

            PoolVector<Vector3> data = p_variant.as<PoolVector<Vector3>>();
            int len = data.size();

            if (buf) {
                encode_uint32(len, buf);
                buf += 4;
            }

            r_len += 4;

            if (buf) {

                for (int i = 0; i < len; i++) {

                    Vector3 v = data.get(i);

                    encode_float(v.x, &buf[0]);
                    encode_float(v.y, &buf[4]);
                    encode_float(v.z, &buf[8]);
                    buf += 4 * 3;
                }
            }

            r_len += 4 * 3 * len;

        } break;
        case VariantType::POOL_COLOR_ARRAY: {

            PoolVector<Color> data = p_variant.as<PoolVector<Color>>();
            int len = data.size();

            if (buf) {
                encode_uint32(len, buf);
                buf += 4;
            }

            r_len += 4;

            if (buf) {

                for (int i = 0; i < len; i++) {

                    Color c = data.get(i);

                    encode_float(c.r, &buf[0]);
                    encode_float(c.g, &buf[4]);
                    encode_float(c.b, &buf[8]);
                    encode_float(c.a, &buf[12]);
                    buf += 4 * 4;
                }
            }

            r_len += 4 * 4 * len;

        } break;
        default: {
            ERR_FAIL_V(ERR_BUG);
        }
    }

    return OK;
}
