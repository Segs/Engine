/*************************************************************************/
/*  cowdata.h                                                            */
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

#include "core/os/memory.h"
#include "core/error_list.h"
#include "core/error_macros.h"
#include "core/safe_refcount.h"

#include "EASTL/type_traits.h"
#include <cstring>

SAFE_NUMERIC_TYPE_PUN_GUARANTEES(uint32_t)

template <class T>
class CowData {

private:
    mutable T *_ptr = nullptr;

    // internal helpers

    _FORCE_INLINE_ SafeNumeric<uint32_t> *_get_refcount() const {

        if (!_ptr) return nullptr;

        return reinterpret_cast<SafeNumeric<uint32_t> *>(_ptr) - 2;
    }
    _FORCE_INLINE_ uint32_t *_get_size() const;
    static size_t _get_alloc_size(size_t p_elements) {
        return next_power_of_2(uint32_t(p_elements * sizeof(T)));
    }

    _FORCE_INLINE_ bool _get_alloc_size_checked(size_t p_elements, size_t *out) const {
#if defined(__GNUC__)
        size_t o;
        size_t p;
        if (__builtin_mul_overflow(p_elements, sizeof(T), &o)) {
            *out = 0;
            return false;
        }
        *out = next_power_of_2(o);
        if (__builtin_add_overflow(o, static_cast<size_t>(32), &p)) {
            return false; // No longer allocated here.
    }
        return true;
#else
        // Speed is more important than correctness here, do the operations unchecked
        // and hope for the best.
        * out = _get_alloc_size(p_elements);
        return true;
#endif
    }


    void _unref(void *p_data);
    void _ref(const CowData *p_from);
    void _ref(const CowData &p_from);
    uint32_t _copy_on_write();

public:
    CowData &operator=(const CowData<T> &p_from) { _ref(p_from); return *this; }

    _FORCE_INLINE_ T *ptrw() {
        _copy_on_write();
        return _ptr;
    }

    const T *ptr() const {
        return _ptr;
    }

    _FORCE_INLINE_ int size() const {
        uint32_t* size = _get_size();
        if (likely(size)) // TODO: mark as likely ?
            return *size;

        return 0;
    }

    void clear() { resize(0); }
    bool empty() const noexcept { return _ptr == nullptr; }

    void set(int p_index, const T &p_elem) {

        CRASH_BAD_INDEX(p_index, size());
        _copy_on_write();
        _ptr[p_index] = p_elem;
    }

    _FORCE_INLINE_ T &get_m(int p_index);

    _FORCE_INLINE_ const T &get(int p_index) const;

    Error resize(int p_size);

    _FORCE_INLINE_ void remove(int p_index);

    Error insert(int p_pos, const T &p_val);

    int find(const T &p_val, int p_from = 0) const;

    constexpr CowData() noexcept = default;
    _FORCE_INLINE_ ~CowData();
    _FORCE_INLINE_ CowData(const CowData<T> &p_from) { _ref(p_from); }
};

extern template class EXPORT_TEMPLATE_DECL CowData<String>;
//GODOT_TEMPLATE_EXT_DECLARE(CowData<char16_t>)
