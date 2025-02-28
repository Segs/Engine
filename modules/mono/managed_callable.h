/*************************************************************************/
/*  managed_callable.h                                                   */
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

#include "core/callable.h"
#include "core/os/mutex.h"
#include "core/self_list.h"

#include "mono_gc_handle.h"
#include "mono_gd/gd_mono_method.h"

#include <mono/metadata/object.h>

class ManagedCallable : public CallableCustom {
    friend class CSharpLanguage;
    MonoGCHandleData delegate_handle;
    GDMonoMethod *delegate_invoke;

#ifdef GD_MONO_HOT_RELOAD
    IntrusiveListNode<ManagedCallable> self_instance {this};
    static IntrusiveList<ManagedCallable> instances;
    static Map<ManagedCallable *, Array> instances_pending_reload;
    static Mutex instances_mutex;
#endif

public:
    uint32_t hash() const override;
    String get_as_text() const override;
    CompareEqualFunc get_compare_equal_func() const override;
    CompareLessFunc get_compare_less_func() const override;
    GameEntity get_object() const override;
    void call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, Callable::CallError &r_call_error) const override;

    _FORCE_INLINE_ MonoDelegate *get_delegate() { return (MonoDelegate *)delegate_handle.get_target(); }

    void set_delegate(MonoDelegate *p_delegate);

    static bool compare_equal(const CallableCustom *p_a, const CallableCustom *p_b);
    static bool compare_less(const CallableCustom *p_a, const CallableCustom *p_b);

    static constexpr CompareEqualFunc compare_equal_func_ptr = &ManagedCallable::compare_equal;
    static constexpr CompareEqualFunc compare_less_func_ptr = &ManagedCallable::compare_less;

    ManagedCallable(MonoDelegate *p_delegate);
    ~ManagedCallable();
};
