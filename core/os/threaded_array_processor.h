/*************************************************************************/
/*  threaded_array_processor.h                                           */
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

#include "core/os/mutex.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/os/thread_safe.h"
#include "core/safe_refcount.h"

template <class C, class U>
struct ThreadArrayProcessData {
    uint32_t elements;
    SafeNumeric<uint32_t> index;
    C *instance;
    U userdata;
    void (C::*method)(uint32_t, U);

    void process(uint32_t p_index) {
        (instance->*method)(p_index, userdata);
    }
};

template <class T>
void process_array_thread(void *ud) {

    T &data = *(T *)ud;
    while (true) {
        uint32_t index = data.index.increment();
        if (index >= data.elements)
            break;
        data.process(index);
    }
}

// p_num_threads is the number of logical CPU cores to use (0 = use all logical CPU cores available).
// Negative values subtract from the total number of logical CPU cores available.
template <class C, class M, class U>
void thread_process_array(uint32_t p_elements, C *p_instance, M p_method, U p_userdata, int p_num_threads = 0) {

    ThreadArrayProcessData<C, U> data;
    data.method = p_method;
    data.instance = p_instance;
    data.userdata = p_userdata;
    data.index.set(0);
    data.elements = p_elements;
    data.process(0); //process first, let threads increment for next

    int thread_count;
    if (p_num_threads <= 0) {
        thread_count = eastl::max(1, OS::get_singleton()->get_processor_count() + p_num_threads);
    } else {
        thread_count = p_num_threads;
    }

    FixedVector<Thread ,16,true> threads; // allow for more than 16 processors, but will heap alloc in that case

    threads.resize(thread_count);

    for (int i = 0; i < threads.size(); i++) {
        threads.emplace_back().start(process_array_thread<ThreadArrayProcessData<C, U> >, &data);
    }

    for (int i = 0; i < threads.size(); i++) {
        threads[i].wait_to_finish();
    }
}
