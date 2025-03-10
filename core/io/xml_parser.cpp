/*************************************************************************/
/*  xml_parser.cpp                                                       */
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

#include "xml_parser.h"

#include "core/print_string.h"
#include "core/method_bind.h"
#include "core/pool_vector.h"
#include "core/method_enum_caster.h"

//#define DEBUG_XML

IMPL_GDCLASS(XMLParser)

VARIANT_ENUM_CAST(XMLParser::NodeType);

static inline bool _is_white_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

//! sets the state that text was found. Returns true if set should be set
bool XMLParser::_set_text(char *start, char *end) {
    // check if text is more than 2 characters, and if not, check if there is
    // only white space, so that this text won't be reported
    if (end - start < 3) {
        char *p = start;
        for (; p != end; ++p)
            if (!_is_white_space(*p))
                break;

        if (p == end)
            return false;
    }

    // set current text to the parsed text, and replace xml special characters
    String s(start, (int)(end - start));
    node_name = StringUtils::xml_unescape(s);

    // current XML node type is text
    node_type = NODE_TEXT;

    return true;
}

void XMLParser::_parse_closing_xml_element() {
    node_type = NODE_ELEMENT_END;
    node_empty = false;
    attributes.clear();

    ++P;
    const char *pBeginClose = P;

    while (*P && *P != '>')
        ++P;

    node_name = String(pBeginClose, (int)(P - pBeginClose));
#ifdef DEBUG_XML
    print_line("XML CLOSE: " + node_name);
#endif
    if (*P) {
        ++P;
    }
}

void XMLParser::_ignore_definition() {
    node_type = NODE_UNKNOWN;

    char *F = P;
    // move until end marked with '>' reached
    while (*P && *P != '>') {
        ++P;
    }
    node_name = String(F, P - F);
    if (*P) {
        ++P;
    }
}

bool XMLParser::_parse_cdata() {

    if (*(P + 1) != '[')
        return false;

    node_type = NODE_CDATA;

    // skip '<![CDATA['
    int count = 0;
    while (*P && count < 8) {
        ++P;
        ++count;
    }

    if (!*P) {
        node_name.clear();
        return true;
    }

    char *cDataBegin = P;
    char *cDataEnd = nullptr;

    // find end of CDATA
    while (*P && !cDataEnd) {
        if (*P == '>' &&
                (*(P - 1) == ']') &&
                (*(P - 2) == ']')) {
            cDataEnd = P - 2;
        }

        ++P;
    }

    if (!cDataEnd) {
        cDataEnd = P;
    }
    node_name = String(cDataBegin, (int)(cDataEnd - cDataBegin));
#ifdef DEBUG_XML
    print_line("XML CDATA: " + node_name);
#endif

    return true;
}

void XMLParser::_parse_comment() {
    node_type = NODE_COMMENT;
    P += 1;

    char *pEndOfInput = data + length;
    char *pCommentBegin;
    char *pCommentEnd;

    if (P + 1 < pEndOfInput && P[0] == '-' && P[1] == '-') {
        // Comment, use '-->' as end.
        pCommentBegin = P + 2;
        for (pCommentEnd = pCommentBegin; pCommentEnd + 2 < pEndOfInput; pCommentEnd++) {
            if (pCommentEnd[0] == '-' && pCommentEnd[1] == '-' && pCommentEnd[2] == '>') {
                break;
            }
        }
        if (pCommentEnd + 2 < pEndOfInput) {
            P = pCommentEnd + 3;
        } else {
            P = pCommentEnd = pEndOfInput;
        }
    } else {
        // Like document type definition, match angle brackets.
        pCommentBegin = P;

        int count = 1;
        while (*P && count) {
            if (*P == '>') {
                --count;
            } else if (*P == '<') {
                ++count;
            }
            ++P;
        }

        if (count) {
            pCommentEnd = P;
        } else {
            pCommentEnd = P - 1;
        }
    }

    node_name.assign(pCommentBegin, (int)(pCommentEnd - pCommentBegin));
#ifdef DEBUG_XML
    print_line("XML COMMENT: " + node_name);
#endif
}

void XMLParser::_parse_opening_xml_element() {

    node_type = NODE_ELEMENT;
    node_empty = false;
    attributes.clear();

    // find name
    const char *startName = P;

    // find end of element
    while (*P && *P != '>' && !_is_white_space(*P)) {
        ++P;
    }

    const char *endName = P;

    // find attributes
    while (*P && *P != '>') {
        if (_is_white_space(*P))
            ++P;
        else {
            if (*P != '/') {
                // we've got an attribute

                // read the attribute names
                const char *attributeNameBegin = P;

                while (*P && !_is_white_space(*P) && *P != '=')
                    ++P;
                if (!*P) {
                    break;
                }

                const char *attributeNameEnd = P;
                ++P;

                // read the attribute value
                // check for quotes and single quotes, thx to murphy
                while (*P && (*P != '\"') && (*P != '\'')) {
                    ++P;
                }

                if (!*P) { // malformatted xml file
                    break;
                }

                const char attributeQuoteChar = *P;

                ++P;
                const char *attributeValueBegin = P;

                while (*P != attributeQuoteChar && *P) {
                    ++P;
                }

                const char *attributeValueEnd = P;
                if (*P) {
                    ++P;
                }

                Attribute attr;
                attr.name = String(attributeNameBegin, (int)(attributeNameEnd - attributeNameBegin));

                String s(attributeValueBegin, (int)(attributeValueEnd - attributeValueBegin));

                attr.value = StringUtils::xml_unescape(s);
                attributes.push_back(attr);
            } else {
                // tag is closed directly
                ++P;
                node_empty = true;
                break;
            }
        }
    }

    // check if this tag is closing directly
    if (endName > startName && *(endName - 1) == '/') {
        // directly closing tag
        node_empty = true;
        endName--;
    }

    node_name = String(startName, (int)(endName - startName));
#ifdef DEBUG_XML
    print_line("XML OPEN: " + node_name);
#endif

    if (*P) {
        ++P;
    }
}

void XMLParser::_parse_current_node() {

    char *start = P;
    node_offset = P - data;

    // more forward until '<' found
    while (*P != '<' && *P)
        ++P;

    if (P - start > 0) {
        // we found some text, store it
        if (_set_text(start, P))
            return;
    }

    if (!*P) {
        return;
    }

    ++P;

    // based on current token, parse and report next element
    switch (*P) {
        case '/':
            _parse_closing_xml_element();
            break;
        case '?':
            _ignore_definition();
            break;
        case '!':
            if (!_parse_cdata())
                _parse_comment();
            break;
        default:
            _parse_opening_xml_element();
            break;
    }
}

uint64_t XMLParser::get_node_offset() const {

    return node_offset;
}

Error XMLParser::seek(uint64_t p_pos) {

    ERR_FAIL_COND_V(!data, ERR_FILE_EOF);
    ERR_FAIL_COND_V(p_pos >= length, ERR_FILE_EOF);

    P = data + p_pos;

    return read();
}

void XMLParser::_bind_methods() {

    SE_BIND_METHOD(XMLParser,read);
    SE_BIND_METHOD(XMLParser,get_node_type);
    SE_BIND_METHOD(XMLParser,get_node_name);
    SE_BIND_METHOD(XMLParser,get_node_data);
    SE_BIND_METHOD(XMLParser,get_node_offset);
    SE_BIND_METHOD(XMLParser,get_attribute_count);
    SE_BIND_METHOD(XMLParser,get_attribute_name);
    MethodBinder::bind_method(D_METHOD("get_attribute_value", {"idx"}), (const String &(XMLParser::*)(int) const) & XMLParser::get_attribute_value);
    SE_BIND_METHOD(XMLParser,has_attribute);
    MethodBinder::bind_method(D_METHOD("get_named_attribute_value", {"name"}), (const String &(XMLParser::*)(StringView) const) & XMLParser::get_attribute_value);
    MethodBinder::bind_method(D_METHOD("get_named_attribute_value_safe", {"name"}), &XMLParser::get_attribute_value_safe);
    SE_BIND_METHOD(XMLParser,is_empty);
    SE_BIND_METHOD(XMLParser,get_current_line);
    SE_BIND_METHOD(XMLParser,skip_section);
    SE_BIND_METHOD(XMLParser,seek);
    SE_BIND_METHOD(XMLParser,open);
    SE_BIND_METHOD(XMLParser,open_buffer);

    BIND_ENUM_CONSTANT(NODE_NONE);
    BIND_ENUM_CONSTANT(NODE_ELEMENT);
    BIND_ENUM_CONSTANT(NODE_ELEMENT_END);
    BIND_ENUM_CONSTANT(NODE_TEXT);
    BIND_ENUM_CONSTANT(NODE_COMMENT);
    BIND_ENUM_CONSTANT(NODE_CDATA);
    BIND_ENUM_CONSTANT(NODE_UNKNOWN);
}

Error XMLParser::read() {

    // if not end reached, parse the node
    if (P && (P - data) < (int64_t)length - 1 && *P != 0) {
        _parse_current_node();
        return OK;
    }

    return ERR_FILE_EOF;
}

XMLParser::NodeType XMLParser::get_node_type() {

    return node_type;
}
const String &XMLParser::get_node_data() const {

    ERR_FAIL_COND_V(node_type != NODE_TEXT, null_string);
    return node_name;
}

const String &XMLParser::get_node_name() const {
    ERR_FAIL_COND_V(node_type == NODE_TEXT, null_string);
    return node_name;
}
int XMLParser::get_attribute_count() const {

    return attributes.size();
}
const String &XMLParser::get_attribute_name(int p_idx) const {

    ERR_FAIL_INDEX_V(p_idx, attributes.size(), null_string);
    return attributes[p_idx].name;
}
const String &XMLParser::get_attribute_value(int p_idx) const {

    ERR_FAIL_INDEX_V(p_idx, attributes.size(), null_string);
    return attributes[p_idx].value;
}
bool XMLParser::has_attribute(StringView p_name) const {

    for (const Attribute &attr: attributes) {
        if (attr.name == p_name)
            return true;
    }

    return false;
}

const String & XMLParser::get_attribute_value(StringView p_name) const {

    int idx = -1;
    for (int i = 0; i < attributes.size(); i++) {
        if (attributes[i].name == p_name) {
            idx = i;
            break;
        }
    }

    ERR_FAIL_COND_V_MSG(idx < 0, null_string, "Attribute not found: " + String(p_name) + ".");

    return attributes[idx].value;
}

String XMLParser::get_attribute_value_safe(StringView p_name) const {

    int idx = -1;
    for (int i = 0; i < attributes.size(); i++) {
        if (attributes[i].name == p_name) {
            idx = i;
            break;
        }
    }

    if (idx < 0)
        return String();
    return attributes[idx].value;
}
bool XMLParser::is_empty() const {

    return node_empty;
}

Error XMLParser::open_buffer(const PoolVector<uint8_t> &p_buffer) {

    ERR_FAIL_COND_V(p_buffer.empty(), ERR_INVALID_DATA);

    if (data) {
        memdelete_arr(data);
    }

    length = p_buffer.size();
    data = memnew_arr(char, length + 1);
    memcpy(data, p_buffer.read().ptr(), length);
    data[length] = 0;
    P = data;
    return OK;
}

Error XMLParser::open(StringView p_path) {

    Error err;
    FileAccess *file = FileAccess::open(p_path, FileAccess::READ, &err);

    ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot open file '" + String(p_path) + "'.");

    length = file->get_len();
    ERR_FAIL_COND_V(length < 1, ERR_FILE_CORRUPT);

    if (data) {
        memdelete_arr(data);
    }

    data = memnew_arr(char, length + 1);
    file->get_buffer((uint8_t *)data, length);
    data[length] = 0;
    P = data;

    memdelete(file);

    return OK;
}

void XMLParser::skip_section() {

    // skip if this element is empty anyway.
    if (is_empty())
        return;

    // read until we've reached the last element in this section
    int tagcount = 1;

    while (tagcount && read() == OK) {
        if (get_node_type() == XMLParser::NODE_ELEMENT &&
                !is_empty()) {
            ++tagcount;
        } else if (get_node_type() == XMLParser::NODE_ELEMENT_END)
            --tagcount;
    }
}

void XMLParser::close() {

    if (data)
        memdelete_arr(data);
    data = nullptr;
    length = 0;
    P = nullptr;
    node_empty = false;
    node_type = NODE_NONE;
    node_offset = 0;
}

int XMLParser::get_current_line() const {

    return 0;
}

XMLParser::XMLParser() {
}
XMLParser::~XMLParser() {

    if (data)
        memdelete_arr(data);
}
