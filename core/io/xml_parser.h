/*************************************************************************/
/*  xml_parser.h                                                         */
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

#include "core/os/file_access.h"
#include "core/reference.h"
#include "core/string.h"
#include "core/vector.h"

/*
  Based on irrXML (see their zlib license). Added mainly for compatibility with their Collada loader.
*/

class GODOT_EXPORT XMLParser : public RefCounted {

    GDCLASS(XMLParser,RefCounted)

public:
    //! Enumeration of all supported source text file formats
    enum SourceFormat {
        SOURCE_ASCII,
        SOURCE_UTF8,
        SOURCE_UTF16_BE,
        SOURCE_UTF16_LE,
        SOURCE_UTF32_BE,
        SOURCE_UTF32_LE
    };

    enum NodeType {
        NODE_NONE,
        NODE_ELEMENT,
        NODE_ELEMENT_END,
        NODE_TEXT,
        NODE_COMMENT,
        NODE_CDATA,
        NODE_UNKNOWN
    };

private:
    char *data = nullptr;
    char *P = nullptr;
    uint64_t length = 0;
    String node_name;
    bool node_empty = false;
    NodeType node_type = NODE_NONE;
    uint64_t node_offset = 0;

    struct Attribute {
        String name;
        String value;
    };

    Vector<Attribute> attributes;

    bool _set_text(char *start, char *end);
    void _parse_closing_xml_element();
    void _ignore_definition();
    bool _parse_cdata();
    void _parse_comment();
    void _parse_opening_xml_element();
    void _parse_current_node();

    static void _bind_methods();

public:
    Error read();
    NodeType get_node_type();
    const String &get_node_name() const;
    const String &get_node_data() const;
    uint64_t get_node_offset() const;
    int get_attribute_count() const;
    const String &get_attribute_name(int p_idx) const;
    const String &get_attribute_value(int p_idx) const;
    const String &get_attribute_value(StringView p_name) const;
    bool has_attribute(StringView p_name) const;
    String get_attribute_value_safe(StringView p_name) const; // do not print error if doesn't exist
    bool is_empty() const;
    int get_current_line() const;

    void skip_section();
    Error seek(uint64_t p_pos);

    Error open(StringView p_path);
    Error open_buffer(const PoolVector<uint8_t> &p_buffer);

    void close();

    XMLParser();
    ~XMLParser() override;
};
