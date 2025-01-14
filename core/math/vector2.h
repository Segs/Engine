/*************************************************************************/
/*  vector2.h                                                            */
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

#include "core/math/math_funcs.h"
#include "core/error_macros.h"

struct Vector2i;

struct [[nodiscard]] GODOT_EXPORT Vector2 {

    enum Axis {
        AXIS_X,
        AXIS_Y,
    };
    union {
        real_t x;
        real_t width;
    };
    union {
        real_t y;
        real_t height;
    };

    constexpr real_t &operator[](int p_idx) {
        return p_idx ? y : x;
    }
    constexpr real_t operator[](int p_idx) const {
        return p_idx ? y : x;
    }

    void normalize();
    Vector2 normalized() const;
    bool is_normalized() const;

    real_t length() const;
    real_t length_squared() const;

    real_t distance_to(Vector2 p_vector2) const;
    real_t distance_squared_to(Vector2 p_vector2) const;
    real_t angle_to(Vector2 p_vector2) const;
    real_t angle_to_point(Vector2 p_vector2) const;
    _FORCE_INLINE_ Vector2 direction_to(Vector2 p_b) const;

    real_t dot(Vector2 p_other) const;
    real_t cross(Vector2 p_other) const;
    Vector2 posmod(const real_t p_mod) const;
    Vector2 posmodv(Vector2 p_modv) const;
    Vector2 project(Vector2 p_b) const;

    Vector2 plane_project(real_t p_d, Vector2 p_vec) const;

    Vector2 limit_length(real_t p_len = 1.0f) const;

    _FORCE_INLINE_ static Vector2 linear_interpolate(Vector2 p_a, Vector2 p_b, real_t p_t);
    _FORCE_INLINE_ Vector2 linear_interpolate(Vector2 p_b, real_t p_t) const;
    _FORCE_INLINE_ Vector2 slerp(Vector2 p_b, real_t p_t) const;
    Vector2 cubic_interpolate(Vector2 p_b, Vector2 p_pre_a, Vector2 p_post_b, real_t p_t) const;
    Vector2 move_toward(Vector2 p_to, const real_t p_delta) const;

    Vector2 slide(Vector2 p_normal) const;
    Vector2 bounce(Vector2 p_normal) const;
    Vector2 reflect(Vector2 p_normal) const;

    bool is_equal_approx(Vector2 p_v) const {
        return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y);
    }
    Vector2 operator+(Vector2 p_v) const;
    void operator+=(Vector2 p_v);
    Vector2 operator-(Vector2 p_v) const;
    void operator-=(Vector2 p_v);
    constexpr Vector2 operator*(Vector2 p_v1) const;

    Vector2 operator*(real_t rvalue) const;
    void operator*=(real_t rvalue);
    Vector2 &operator*=(Vector2 rvalue) { *this = *this * rvalue; return *this; }

    Vector2 operator/(Vector2 p_v1) const;

    Vector2 operator/(real_t rvalue) const;

    void operator/=(real_t rvalue);
    void operator/=(Vector2 rvalue) { *this = *this / rvalue; }

    Vector2 operator-() const;

    bool operator==(Vector2 p_vec2) const;
    bool operator!=(Vector2 p_vec2) const;

    constexpr bool operator<(Vector2 p_vec2) const { return x == p_vec2.x ? (y < p_vec2.y) : (x < p_vec2.x); }
    constexpr bool operator>(Vector2 p_vec2) const { return x == p_vec2.x ? (y > p_vec2.y) : (x > p_vec2.x); }
    constexpr bool operator<=(Vector2 p_vec2) const { return x == p_vec2.x ? (y <= p_vec2.y) : (x < p_vec2.x); }
    constexpr bool operator>=(Vector2 p_vec2) const { return x == p_vec2.x ? (y >= p_vec2.y) : (x > p_vec2.x); }

    real_t angle() const;

    void set_rotation(real_t p_radians) {

        x = Math::cos(p_radians);
        y = Math::sin(p_radians);
    }

    _FORCE_INLINE_ Vector2 abs() const {
        return Vector2(Math::abs(x), Math::abs(y));
    }

    Vector2 rotated(real_t p_by) const;
    constexpr Vector2 tangent() const {
        return Vector2(y, -x);
    }

    Vector2 sign() const;
    Vector2 floor() const;
    Vector2 ceil() const;
    Vector2 round() const;
    Vector2 snapped(Vector2 p_by) const;
    constexpr real_t aspect() const { return width / height; }

    operator String() const;

    constexpr Vector2(real_t p_x, real_t p_y) noexcept : x(p_x),y(p_y) {
    }
    constexpr Vector2() noexcept : x(0),y(0) {
    }
};
static_assert (std::is_trivially_copyable<Vector2>() );

_FORCE_INLINE_ Vector2 Vector2::plane_project(real_t p_d, Vector2 p_vec) const {

    return p_vec - *this * (dot(p_vec) - p_d);
}

_FORCE_INLINE_ Vector2 operator*(real_t p_scalar, Vector2 p_vec) {

    return p_vec * p_scalar;
}

_FORCE_INLINE_ Vector2 Vector2::operator+(Vector2 p_v) const {

    return Vector2(x + p_v.x, y + p_v.y);
}
_FORCE_INLINE_ void Vector2::operator+=(Vector2 p_v) {

    x += p_v.x;
    y += p_v.y;
}
_FORCE_INLINE_ Vector2 Vector2::operator-(Vector2 p_v) const {

    return Vector2(x - p_v.x, y - p_v.y);
}
_FORCE_INLINE_ void Vector2::operator-=(Vector2 p_v) {

    x -= p_v.x;
    y -= p_v.y;
}

constexpr _FORCE_INLINE_ Vector2 Vector2::operator*(Vector2 p_v1) const {

    return Vector2(x * p_v1.x, y * p_v1.y);
}

_FORCE_INLINE_ Vector2 Vector2::operator*(real_t rvalue) const {

    return Vector2(x * rvalue, y * rvalue);
}
_FORCE_INLINE_ void Vector2::operator*=(real_t rvalue) {

    x *= rvalue;
    y *= rvalue;
}

_FORCE_INLINE_ Vector2 Vector2::operator/(Vector2 p_v1) const {

    return Vector2(x / p_v1.x, y / p_v1.y);
}

_FORCE_INLINE_ Vector2 Vector2::operator/(real_t rvalue) const {

    return Vector2(x / rvalue, y / rvalue);
}

_FORCE_INLINE_ void Vector2::operator/=(real_t rvalue) {

    x /= rvalue;
    y /= rvalue;
}

_FORCE_INLINE_ Vector2 Vector2::operator-() const {

    return Vector2(-x, -y);
}

_FORCE_INLINE_ bool Vector2::operator==(Vector2 p_vec2) const {

    return x == p_vec2.x && y == p_vec2.y;
}
_FORCE_INLINE_ bool Vector2::operator!=(Vector2 p_vec2) const {

    return x != p_vec2.x || y != p_vec2.y;
}

Vector2 Vector2::linear_interpolate(Vector2 p_b, real_t p_t) const {

    Vector2 res = *this;

    res.x += (p_t * (p_b.x - x));
    res.y += (p_t * (p_b.y - y));

    return res;
}

Vector2 Vector2::slerp(Vector2 p_b, real_t p_t) const {
#ifdef MATH_CHECKS
    ERR_FAIL_COND_V_MSG(!is_normalized(), Vector2(), "The start Vector2 must be normalized.");
#endif
    real_t theta = angle_to(p_b);
    return rotated(theta * p_t);
}

Vector2 Vector2::direction_to(Vector2 p_b) const {
    Vector2 ret(p_b.x - x, p_b.y - y);
    ret.normalize();
    return ret;
}

Vector2 Vector2::linear_interpolate(Vector2 p_a, Vector2 p_b, real_t p_t) {

    Vector2 res = p_a;

    res.x += (p_t * (p_b.x - p_a.x));
    res.y += (p_t * (p_b.y - p_a.y));

    return res;
}

using Size2 = Vector2;
using Point2 = Vector2;

/* INTEGER STUFF */

struct GODOT_EXPORT [[nodiscard]] Vector2i {
    enum Axis {
        AXIS_X,
        AXIS_Y,
    };

    union {
        int x;
        int width;
    };
    union {
        int y;
        int height;
    };

    constexpr int &operator[](int p_idx) { return p_idx ? y : x; }

    constexpr int operator[](int p_idx) const { return p_idx ? y : x; }

    Vector2i operator+(Vector2i p_v) const { return Vector2i(x + p_v.x, y + p_v.y); }

    void operator+=(Vector2i p_v);
    Vector2i operator-(Vector2i p_v) const;
    void operator-=(Vector2i p_v);
    constexpr Vector2i operator*(Vector2i p_v1) const { return Vector2i(x * p_v1.x, y * p_v1.y); }

    Vector2i operator*(int rvalue) const { return Vector2i(x * rvalue, y * rvalue); }

    void operator*=(int rvalue) {
        x *= rvalue;
        y *= rvalue;
    }

    constexpr Vector2i operator/(Vector2i p_v1) const { return Vector2i(x / p_v1.x, y / p_v1.y); };
    constexpr Vector2i operator/(int rvalue) const { return Vector2i(x / rvalue, y / rvalue); }

    Vector2i &operator/=(int rvalue) {
        x /= rvalue;
        y /= rvalue;
        return *this;
    }
    Vector2i operator-() const;
    bool operator<(Vector2i p_vec2) const { return (x == p_vec2.x) ? (y < p_vec2.y) : (x < p_vec2.x); }
    bool operator>(Vector2i p_vec2) const { return (x == p_vec2.x) ? (y > p_vec2.y) : (x > p_vec2.x); }

    bool operator<=(Vector2i p_vec2) const { return x == p_vec2.x ? (y <= p_vec2.y) : (x < p_vec2.x); }
    bool operator>=(Vector2i p_vec2) const { return x == p_vec2.x ? (y >= p_vec2.y) : (x > p_vec2.x); }

    bool operator==(Vector2i p_vec2) const;
    bool operator!=(Vector2i p_vec2) const;

    constexpr real_t aspect() const { return width / (real_t)height; }
    constexpr Vector2i sign() const { return Vector2i(SGN(x), SGN(y)); }
    constexpr Vector2i abs() const { return Vector2i(ABS(x), ABS(y)); }

    operator String() const;

    constexpr operator Vector2() const { return Vector2(float(x), float(y)); }
    constexpr Vector2i(Vector2 p_vec2) : x((int)p_vec2.x), y((int)p_vec2.y) {}
    constexpr Vector2i(int p_x, int p_y) : x(p_x), y(p_y) {}
    constexpr Vector2i() : x(0), y(0) {}
};

using Size2i = Vector2i;
using Point2i = Vector2i;
