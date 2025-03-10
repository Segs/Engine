/*************************************************************************/
/*  lipo.h                                                               */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

// Universal / Universal 2 fat binary file creator and extractor.

#ifndef OSX_LIPO_H
#define OSX_LIPO_H

#include "core/os/file_access.h"
#include "core/reference.h"
#include "modules/modules_enabled.gen.h" // For regex.

#include "macho.h"

#ifdef MODULE_REGEX_ENABLED

class LipO : public Reference {
    struct FatArch {
        uint32_t cputype;
        uint32_t cpusubtype;
        uint64_t offset;
        uint64_t size;
        uint32_t align;
    };

    FileAccess *fa = nullptr;
    Vector<FatArch> archs;

    static inline size_t PAD(size_t s, size_t a) {
        return (a - s % a);
    }

public:
    static bool is_lipo(const String &p_path);

    bool create_file(const String &p_output_path, const PoolStringArray &p_files);

    bool open_file(const String &p_path);
    int get_arch_count() const;
    bool extract_arch(int p_index, const String &p_path);

    void close();

    ~LipO();
};

#endif // MODULE_REGEX_ENABLED

#endif // OSX_LIPO_H
