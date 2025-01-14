/*************************************************************************/
/*  test_main.cpp                                                        */
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

#include "core/os/main_loop.h"
#include "core/print_string.h"
#include "core/string.h"

#ifdef DEBUG_ENABLED

#include "test_astar.h"
#include "test_gui.h"
#include "test_math.h"
#include "test_oa_hash_map.h"
#include "test_physics.h"
#include "test_physics_2d.h"
#include "test_render.h"
#include "test_shader_lang.h"
//#include "test_string.h"

const char **tests_get_names() {

    static const char *test_names[] = {
//        "string",
        "math",
        "physics",
        "physics_2d",
        "render",
        "oa_hash_map",
        "gui",
        "shaderlang",
        "gd_tokenizer",
        "gd_parser",
        "gd_compiler",
        "gd_bytecode",
        "ordered_hash_map",
        "astar",
        nullptr
    };

    return test_names;
}

MainLoop *test_main(const String& p_test, const Vector<String> &p_args) {

    if (p_test == "string") {

//        return TestString::test();
    }

    if (p_test == "math") {

        return TestMath::test();
    }

    if (p_test == "physics") {

        return TestPhysics::test();
    }

    if (p_test == "physics_2d") {

        return TestPhysics2D::test();
    }

    if (p_test == "render") {

        return TestRender::test();
    }

    if (p_test == "oa_hash_map") {

        return TestOAHashMap::test();
    }

#ifndef _3D_DISABLED
    if (p_test == "gui") {

        return TestGUI::test();
    }
#endif

    if (p_test == "shaderlang") {

        return TestShaderLang::test();
    }

    if (p_test == "astar") {

        return TestAStar::test();
    }

    print_line("Unknown test: " + p_test);
    return nullptr;
}

#else

const char **tests_get_names() {

    static const char *test_names[] = {
        NULL
    };

    return test_names;
}

MainLoop *test_main(String p_test, const List<String> &p_args) {

    return NULL;
}

#endif
