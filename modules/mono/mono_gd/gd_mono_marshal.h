/*************************************************************************/
/*  gd_mono_marshal.h                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "core/variant.h"
#include "core/string.h"
#include "core/ustring.h"

#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/transform_2d.h"
#include "core/math/transform.h"
#include "core/math/basis.h"
#include "core/math/quat.h"
#include "core/color.h"
#include "core/io/ip_address.h"
#include "gd_mono.h"
#include "gd_mono_utils.h"

namespace GDMonoMarshal {
namespace InteropLayout {

template<typename T>
constexpr bool layoutMatches();

template<>
constexpr bool layoutMatches<uint8_t>() {
    return true;
}
template<>
constexpr bool layoutMatches<int8_t>() {
    return true;
}

template<>
constexpr bool layoutMatches<uint16_t>() {
    return true;
}
template<>
constexpr bool layoutMatches<int16_t>() {
    return true;
}
template<>
constexpr bool layoutMatches<int32_t>() {
    return true;
}
template<>
constexpr bool layoutMatches<uint32_t>() {
    return true;
}

template<>
constexpr bool layoutMatches<float>() {
    return (sizeof(float) == sizeof(uint32_t));
}
template<>
constexpr bool layoutMatches<double>() {
    return (sizeof(double) == sizeof(uint64_t));
}
template<>
constexpr bool layoutMatches<Vector2>() {
    return (layoutMatches<real_t>() && (sizeof(Vector2) == (sizeof(real_t) * 2)) &&
                           offsetof(Vector2, x) == (sizeof(real_t) * 0) &&
                           offsetof(Vector2, y) == (sizeof(real_t) * 1));
}
template<>
constexpr bool layoutMatches<Rect2>() {
    return (layoutMatches<Vector2>() && (sizeof(Rect2) == (sizeof(Vector2) * 2)) &&
            offsetof(Rect2, position) == (sizeof(Vector2) * 0) &&
            offsetof(Rect2, size) == (sizeof(Vector2) * 1));
}
template<>
constexpr bool layoutMatches<Transform2D>() {
    // No field offset required, it stores an array
    return (layoutMatches<Vector2>() && (sizeof(Transform2D) == (sizeof(Vector2) * 3)));
}
template<>
constexpr bool layoutMatches<Vector3>() {
    return (layoutMatches<real_t>() && (sizeof(Vector3) == (sizeof(real_t) * 3)) &&
            offsetof(Vector3, x) == (sizeof(real_t) * 0) &&
            offsetof(Vector3, y) == (sizeof(real_t) * 1) &&
            offsetof(Vector3, z) == (sizeof(real_t) * 2));
}
template<>
constexpr bool layoutMatches<Basis>() {
    return (layoutMatches<Vector3>() && (sizeof(Basis) == (sizeof(Vector3) * 3))); // No field offset required, it stores an array
}
template<>
constexpr bool layoutMatches<Quat>() {
    return (layoutMatches<real_t>() && (sizeof(Quat) == (sizeof(real_t) * 4)) &&
            offsetof(Quat, x) == (sizeof(real_t) * 0) &&
            offsetof(Quat, y) == (sizeof(real_t) * 1) &&
            offsetof(Quat, z) == (sizeof(real_t) * 2) &&
            offsetof(Quat, w) == (sizeof(real_t) * 3));
}
template<>
constexpr bool layoutMatches<Transform>() {
    return (layoutMatches<Basis>() && layoutMatches<Vector3>() && (sizeof(Transform) == (sizeof(Basis) + sizeof(Vector3))) &&
            offsetof(Transform, basis) == 0 &&
            offsetof(Transform, origin) == sizeof(Basis));
}
template<>
constexpr bool layoutMatches<AABB>() {
    return (layoutMatches<Vector3>() && (sizeof(AABB) == (sizeof(Vector3) * 2)) &&
            offsetof(AABB, position) == (sizeof(Vector3) * 0) &&
            offsetof(AABB, size) == (sizeof(Vector3) * 1));
}

template<>
constexpr bool layoutMatches<Color>() {
    return (layoutMatches<float>() && (sizeof(Color) == (sizeof(float) * 4)) &&
            offsetof(Color, r) == (sizeof(float) * 0) &&
            offsetof(Color, g) == (sizeof(float) * 1) &&
            offsetof(Color, b) == (sizeof(float) * 2) &&
            offsetof(Color, a) == (sizeof(float) * 3));
}

template<>
constexpr bool layoutMatches<Plane>() {
    return (layoutMatches<Vector3>() && layoutMatches<real_t>() && (sizeof(Plane) == (sizeof(Vector3) + sizeof(real_t))) &&
            offsetof(Plane, normal) == 0 &&
            offsetof(Plane, d) == sizeof(Vector3));
}

// In the future we may force this if we want to ref return these structs
#ifdef GD_MONO_FORCE_INTEROP_STRUCT_COPY
/* clang-format off */
GD_STATIC_ASSERT(layoutMatches<Vector2>() && layoutMatches<Rect2>() && layoutMatches<Transform2D>() && layoutMatches<Vector3>() &&
                layoutMatches<Basis>() && layoutMatches<Quat>() && layoutMatches<Transform>() && layoutMatches<AABB>() && layoutMatches<Color>() &&layoutMatches<Plane>());
/* clang-format on */
#endif

} // namespace InteropLayout


template <typename T>
T unbox(MonoObject *p_obj) {
    return *(T *)mono_object_unbox(p_obj);
}

template <typename T>
T *unbox_addr(MonoObject *p_obj) {
    return (T *)mono_object_unbox(p_obj);
}

#define BOX_DOUBLE(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(double), &x)
#define BOX_FLOAT(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(float), &x)
#define BOX_INT64(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(int64_t), &x)
#define BOX_INT32(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(int32_t), &x)
#define BOX_INT16(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(int16_t), &x)
#define BOX_INT8(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(int8_t), &x)
#define BOX_UINT64(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(uint64_t), &x)
#define BOX_UINT32(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(uint32_t), &x)
#define BOX_UINT16(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(uint16_t), &x)
#define BOX_UINT8(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(uint8_t), &x)
#define BOX_BOOLEAN(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(bool), &x)
#define BOX_PTR(x) mono_value_box(mono_domain_get(), CACHED_CLASS_RAW(IntPtr), x)
#define BOX_ENUM(m_enum_class, x) mono_value_box(mono_domain_get(), m_enum_class, &x)

GODOT_EXPORT VariantType managed_to_variant_type(const ManagedType &p_type, bool *r_nil_is_variant = nullptr);

bool try_get_array_element_type(const ManagedType &p_array_type, ManagedType &r_elem_type);


// String

GODOT_EXPORT String mono_to_utf8_string(MonoString *p_mono_string);
GODOT_EXPORT UIString mono_to_utf16_string(MonoString *p_mono_string);

_FORCE_INLINE_ String mono_string_to_godot_not_null(MonoString *p_mono_string) {
    return mono_to_utf8_string(p_mono_string);
}

_FORCE_INLINE_ String mono_string_to_godot(MonoString *p_mono_string) {
    if (p_mono_string == nullptr) {
        return String();
    }

    return mono_string_to_godot_not_null(p_mono_string);
}

_FORCE_INLINE_ MonoString *mono_from_utf8_string(StringView p_string) {
    return mono_string_new_len(mono_domain_get(), p_string.data(),(uint32_t)p_string.size());
}

_FORCE_INLINE_ MonoString *mono_from_utf16_string(const UIString &p_string) {
    return mono_string_from_utf16((mono_unichar2 *)p_string.data());
}

/*_FORCE_INLINE_ MonoString *mono_string_from_godot(const UIString &p_string) {
    return mono_from_utf16_string(p_string);
}*/
_FORCE_INLINE_ MonoString *mono_string_from_godot(StringView p_string) {
    return mono_from_utf8_string(p_string);
}
// Helper to allow auto-conversion from IP_Address to string
_FORCE_INLINE_ MonoString* mono_string_from_godot(IP_Address p_string) {
    return mono_from_utf8_string((String)p_string);
}

// Variant

GODOT_EXPORT size_t variant_get_managed_unboxed_size(const ManagedType& p_type);
GODOT_EXPORT void *variant_to_managed_unboxed(const Variant &p_var, const ManagedType &p_type, void *r_buffer, unsigned int &r_offset);
GODOT_EXPORT MonoObject *variant_to_mono_object(const Variant *p_var, const ManagedType &p_type);

GODOT_EXPORT MonoArray *variant_to_mono_array(const Variant &p_var, GDMonoClass *p_type_class);
GODOT_EXPORT MonoObject *variant_to_mono_object_of_class(const Variant &p_var, GDMonoClass *p_type_class);
GODOT_EXPORT MonoObject *variant_to_mono_object_of_genericinst(const Variant &p_var, GDMonoClass *p_type_class);
GODOT_EXPORT MonoString *variant_to_mono_string(const Variant &p_var);

GODOT_EXPORT MonoObject *variant_to_mono_object(const Variant &p_var);

_FORCE_INLINE_ MonoObject *variant_to_mono_object(const Variant &p_var, const ManagedType &p_type) {
    return variant_to_mono_object(&p_var, p_type);
}

GODOT_EXPORT Variant mono_object_to_variant(MonoObject *p_obj);
GODOT_EXPORT Variant mono_object_to_variant(MonoObject *p_obj, const ManagedType &p_type);
Variant mono_object_to_variant_no_err(MonoObject *p_obj, const ManagedType &p_type);

/// Tries to convert the MonoObject* to Variant and then convert the Variant to String.
/// If the MonoObject* cannot be converted to Variant, then 'ToString()' is called instead.
GODOT_EXPORT String mono_object_to_variant_string(MonoObject *p_obj, MonoException **r_exc);

// System.Collections.Generic

MonoObject *Dictionary_to_system_generic_dict(const Dictionary &p_dict, GDMonoClass *p_class, MonoReflectionType *p_key_reftype, MonoReflectionType *p_value_reftype);
Dictionary system_generic_dict_to_Dictionary(MonoObject *p_obj, GDMonoClass *p_class, MonoReflectionType *p_key_reftype, MonoReflectionType *p_value_reftype);

MonoObject *Array_to_system_generic_list(const Array &p_array, GDMonoClass *p_class, MonoReflectionType *p_elem_reftype);
Array system_generic_list_to_Array(MonoObject *p_obj, GDMonoClass *p_class, MonoReflectionType *p_elem_reftype);

// Array

GODOT_EXPORT MonoArray *container_to_mono_array(const Array &p_array);
GODOT_EXPORT MonoArray *container_to_mono_array(const Array &p_array, GDMonoClass *p_array_type_class);
GODOT_EXPORT Array mono_array_to_Array(MonoArray *p_array);
GODOT_EXPORT Array mono_array_to_Array(MonoArray *p_array, GDMonoClass *p_array_type_class);


// Array conversion operators, specific instantiatons done in cpp file.
template<class T>
MonoArray *container_to_mono_array(const PoolVector<T> &p_array);
template<class T>
PoolVector<T> mono_array_to_pool_vec(MonoArray *);


template<class T>
MonoArray *container_to_mono_array(Span<const T> p_array);
template<class T>
Vector<T> mono_array_to_vector(MonoArray *);

template<class T>
Span<const T> mono_array_as_span(MonoArray *A) {
    static_assert (InteropLayout::layoutMatches<T>(),"Layout must match for fast conversion");
    return {(const T *)mono_array_addr_with_size(A,sizeof(T),0) , mono_array_length(A)};
}

template<class T>
MonoArray *container_to_mono_array(const Vector<T> &p_array) {
    return container_to_mono_array<T>(Span<const T>(p_array.data(),p_array.size()));
}
#pragma pack(push, 1)

struct M_Callable {
    MonoObject *target;
    MonoObject *method_string_name;
    MonoDelegate *delegate;
};

struct M_SignalInfo {
    MonoObject *owner;
    MonoObject *name_string_name;
};

#pragma pack(pop)
// Callable
GODOT_EXPORT Callable managed_to_callable(const M_Callable &p_managed_callable);
GODOT_EXPORT M_Callable callable_to_managed(const Callable &p_callable);

// SignalInfo
GODOT_EXPORT Signal managed_to_signal_info(const M_SignalInfo &p_managed_signal);
GODOT_EXPORT M_SignalInfo signal_info_to_managed(const Signal &p_signal);
// Structures

#pragma pack(push, 1)

struct M_Vector2 {
    real_t x, y;

    static _FORCE_INLINE_ Vector2 convert_to(const M_Vector2 &p_from) {
        return Vector2(p_from.x, p_from.y);
    }

    static _FORCE_INLINE_ M_Vector2 convert_from(const Vector2 &p_from) {
        M_Vector2 ret = { p_from.x, p_from.y };
        return ret;
    }
};

struct M_Rect2 {
    M_Vector2 position;
    M_Vector2 size;

    static _FORCE_INLINE_ Rect2 convert_to(const M_Rect2 &p_from) {
        return Rect2(M_Vector2::convert_to(p_from.position),
                M_Vector2::convert_to(p_from.size));
    }

    static _FORCE_INLINE_ M_Rect2 convert_from(const Rect2 &p_from) {
        M_Rect2 ret = { M_Vector2::convert_from(p_from.position), M_Vector2::convert_from(p_from.size) };
        return ret;
    }
};

struct M_Transform2D {
    M_Vector2 elements[3];

    static _FORCE_INLINE_ Transform2D convert_to(const M_Transform2D &p_from) {
        return Transform2D(p_from.elements[0].x, p_from.elements[0].y,
                p_from.elements[1].x, p_from.elements[1].y,
                p_from.elements[2].x, p_from.elements[2].y);
    }

    static _FORCE_INLINE_ M_Transform2D convert_from(const Transform2D &p_from) {
        M_Transform2D ret = {
            M_Vector2::convert_from(p_from.elements[0]),
            M_Vector2::convert_from(p_from.elements[1]),
            M_Vector2::convert_from(p_from.elements[2])
        };
        return ret;
    }
};

struct M_Vector3 {
    real_t x, y, z;

    static _FORCE_INLINE_ Vector3 convert_to(const M_Vector3 &p_from) {
        return Vector3(p_from.x, p_from.y, p_from.z);
    }

    static _FORCE_INLINE_ M_Vector3 convert_from(const Vector3 &p_from) {
        M_Vector3 ret = { p_from.x, p_from.y, p_from.z };
        return ret;
    }
};

struct M_Basis {
    M_Vector3 elements[3];

    static _FORCE_INLINE_ Basis convert_to(const M_Basis &p_from) {
        return Basis(M_Vector3::convert_to(p_from.elements[0]),
                M_Vector3::convert_to(p_from.elements[1]),
                M_Vector3::convert_to(p_from.elements[2]));
    }

    static _FORCE_INLINE_ M_Basis convert_from(const Basis &p_from) {
        M_Basis ret = {
            M_Vector3::convert_from(p_from.elements[0]),
            M_Vector3::convert_from(p_from.elements[1]),
            M_Vector3::convert_from(p_from.elements[2])
        };
        return ret;
    }
};

struct M_Quat {
    real_t x, y, z, w;

    static _FORCE_INLINE_ Quat convert_to(const M_Quat &p_from) {
        return Quat(p_from.x, p_from.y, p_from.z, p_from.w);
    }

    static _FORCE_INLINE_ M_Quat convert_from(const Quat &p_from) {
        M_Quat ret = { p_from.x, p_from.y, p_from.z, p_from.w };
        return ret;
    }
};

struct M_Transform {
    M_Basis basis;
    M_Vector3 origin;

    static _FORCE_INLINE_ Transform convert_to(const M_Transform &p_from) {
        return Transform(M_Basis::convert_to(p_from.basis), M_Vector3::convert_to(p_from.origin));
    }

    static _FORCE_INLINE_ M_Transform convert_from(const Transform &p_from) {
        M_Transform ret = { M_Basis::convert_from(p_from.basis), M_Vector3::convert_from(p_from.origin) };
        return ret;
    }
};

struct M_AABB {
    M_Vector3 position;
    M_Vector3 size;

    static _FORCE_INLINE_ AABB convert_to(const M_AABB &p_from) {
        return AABB(M_Vector3::convert_to(p_from.position), M_Vector3::convert_to(p_from.size));
    }

    static _FORCE_INLINE_ M_AABB convert_from(const AABB &p_from) {
        M_AABB ret = { M_Vector3::convert_from(p_from.position), M_Vector3::convert_from(p_from.size) };
        return ret;
    }
};

struct M_Color {
    float r, g, b, a;

    static _FORCE_INLINE_ Color convert_to(const M_Color &p_from) {
        return Color(p_from.r, p_from.g, p_from.b, p_from.a);
    }

    static _FORCE_INLINE_ M_Color convert_from(const Color &p_from) {
        M_Color ret = { p_from.r, p_from.g, p_from.b, p_from.a };
        return ret;
    }
};

struct M_Plane {
    M_Vector3 normal;
    real_t d;

    static _FORCE_INLINE_ Plane convert_to(const M_Plane &p_from) {
        return Plane(M_Vector3::convert_to(p_from.normal), p_from.d);
    }

    static _FORCE_INLINE_ M_Plane convert_from(const Plane &p_from) {
        M_Plane ret = { M_Vector3::convert_from(p_from.normal), p_from.d };
        return ret;
    }
};

#pragma pack(pop)

#define DECL_TYPE_MARSHAL_TEMPLATES(m_type)                                             \
    template <int>                                                                      \
    _FORCE_INLINE_ m_type marshalled_in_##m_type##_impl(const M_##m_type *p_from);      \
                                                                                        \
    template <>                                                                         \
    _FORCE_INLINE_ m_type marshalled_in_##m_type##_impl<0>(const M_##m_type *p_from) {  \
        return M_##m_type::convert_to(*p_from);                                         \
    }                                                                                   \
                                                                                        \
    template <>                                                                         \
    _FORCE_INLINE_ m_type marshalled_in_##m_type##_impl<1>(const M_##m_type *p_from) {  \
        return *reinterpret_cast<const m_type *>(p_from);                               \
    }                                                                                   \
                                                                                        \
    _FORCE_INLINE_ m_type marshalled_in_##m_type(const M_##m_type *p_from) {            \
        return marshalled_in_##m_type##_impl<InteropLayout::layoutMatches<m_type>()>(p_from);  \
    }                                                                                   \
                                                                                        \
    template <int>                                                                      \
    _FORCE_INLINE_ M_##m_type marshalled_out_##m_type##_impl(const m_type &p_from);     \
                                                                                        \
    template <>                                                                         \
    _FORCE_INLINE_ M_##m_type marshalled_out_##m_type##_impl<0>(const m_type &p_from) { \
        return M_##m_type::convert_from(p_from);                                        \
    }                                                                                   \
                                                                                        \
    template <>                                                                         \
    _FORCE_INLINE_ M_##m_type marshalled_out_##m_type##_impl<1>(const m_type &p_from) { \
        return *reinterpret_cast<const M_##m_type *>(&p_from);                          \
    }                                                                                   \
                                                                                        \
    _FORCE_INLINE_ M_##m_type marshalled_out_##m_type(const m_type &p_from) {           \
        return marshalled_out_##m_type##_impl<InteropLayout::layoutMatches<m_type>()>(p_from); \
    }

DECL_TYPE_MARSHAL_TEMPLATES(Vector2)
DECL_TYPE_MARSHAL_TEMPLATES(Rect2)
DECL_TYPE_MARSHAL_TEMPLATES(Transform2D)
DECL_TYPE_MARSHAL_TEMPLATES(Vector3)
DECL_TYPE_MARSHAL_TEMPLATES(Basis)
DECL_TYPE_MARSHAL_TEMPLATES(Quat)
DECL_TYPE_MARSHAL_TEMPLATES(Transform)
DECL_TYPE_MARSHAL_TEMPLATES(AABB)
DECL_TYPE_MARSHAL_TEMPLATES(Color)
DECL_TYPE_MARSHAL_TEMPLATES(Plane)

#define MARSHALLED_IN(m_type, m_from_ptr) (GDMonoMarshal::marshalled_in_##m_type(m_from_ptr))
#define MARSHALLED_OUT(m_type, m_from) (GDMonoMarshal::marshalled_out_##m_type(m_from))

} // namespace GDMonoMarshal
