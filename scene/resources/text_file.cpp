/*************************************************************************/
/*  text_file.cpp                                                        */
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

#include "text_file.h"

#include "core/class_db.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/file_access.h"
#include "core/pool_vector.h"
#include "core/property_info.h"
#include "core/string_utils.inl"
#include "core/list.h"

IMPL_GDCLASS(TextFile)

bool TextFile::has_text() const {
    return !text.empty();
}



void TextFile::set_text(const String &p_code) {
    text = p_code;
}

void TextFile::reload_from_file() {
    load_text(path);
}

Error TextFile::load_text(StringView p_path) {

    PoolVector<uint8_t> sourcef;
    Error err;
    FileAccess *f = FileAccess::open(p_path, FileAccess::READ, &err);
    if (err) {
        ERR_FAIL_COND_V(err, err);
    }

    uint64_t len = f->get_len();
    sourcef.resize(len + 1);
    PoolVector<uint8_t>::Write w = sourcef.write();
    uint64_t r = f->get_buffer(w.ptr(), len);
    f->close();
    memdelete(f);
    ERR_FAIL_COND_V(r != len, ERR_CANT_OPEN);
    w[len] = 0;

    UIString s = StringUtils::from_utf8((const char *)w.ptr());
    ERR_FAIL_COND_V_MSG(s.isEmpty(), ERR_INVALID_DATA, "Script '" + p_path + "' contains invalid unicode (UTF-8), so it was not loaded. Please ensure that scripts are saved in valid UTF-8 unicode.");
    text = (const char *)w.ptr();
    path = p_path;
    return OK;
}
