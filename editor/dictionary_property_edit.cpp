/*************************************************************************/
/*  dictionary_property_edit.cpp                                         */
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

#include "dictionary_property_edit.h"
#include "core/method_bind.h"
#include "core/object_db.h"
#include "core/object_tooling.h"
#include "editor_node.h"
#include "EASTL/sort.h"

IMPL_GDCLASS(DictionaryPropertyEdit)

void DictionaryPropertyEdit::_notif_change() {
    Object_change_notify(this);
}

void DictionaryPropertyEdit::_notif_changev(const String &p_v) {
    Object_change_notify(this,StringName(p_v));
}

void DictionaryPropertyEdit::_set_key(const StringName&p_old_key, const StringName &p_new_key) {

    // TODO: Set key of a dictionary is not allowed yet
}

void DictionaryPropertyEdit::_set_value(const StringName &p_key, const Variant &p_value) {

    Dictionary dict = get_dictionary();
    dict[p_key] = p_value;
    Object *o = object_for_entity(obj);
    if (!o)
        return;

    o->set(property, dict);
}

Dictionary DictionaryPropertyEdit::get_dictionary() const {

    Object *o = object_for_entity(obj);
    if (!o)
        return Dictionary();
    Variant dict = o->get(property);
    if (dict.get_type() != VariantType::DICTIONARY)
        return Dictionary();
    return dict.as<Dictionary>();
}

void DictionaryPropertyEdit::_get_property_list(Vector<PropertyInfo> *p_list) const {

    Dictionary dict = get_dictionary();

    auto keys = dict.get_key_list();
    eastl::sort(keys.begin(), keys.end(), WrapAlphaCompare());

    for (int i = 0; i < keys.size(); i++) {
        String index = itos(i);

        const auto &key = keys[i];
        p_list->emplace_back(VariantType::STRING_NAME, StringName(index + ": key"));

        const Variant &value = dict[key];
        p_list->emplace_back(value.get_type(), StringName(index + ": value"));
    }
}

void DictionaryPropertyEdit::edit(Object *p_obj, const StringName &p_prop) {

    property = p_prop;
    obj = p_obj->get_instance_id();
}

Node *DictionaryPropertyEdit::get_node() {

    Object *o = object_for_entity(obj);
    if (!o)
        return nullptr;

    return object_cast<Node>(o);
}

bool DictionaryPropertyEdit::_dont_undo_redo() {
    return true;
}

void DictionaryPropertyEdit::_bind_methods() {

    SE_BIND_METHOD(DictionaryPropertyEdit,_set_key);
    SE_BIND_METHOD(DictionaryPropertyEdit,_set_value);
    SE_BIND_METHOD(DictionaryPropertyEdit,_notif_change);
    SE_BIND_METHOD(DictionaryPropertyEdit,_notif_changev);
    SE_BIND_METHOD(DictionaryPropertyEdit,_dont_undo_redo);
}

bool DictionaryPropertyEdit::_set(const StringName &p_name, const Variant &p_value) {
    using namespace eastl;
    Dictionary dict = get_dictionary();
    auto keys = dict.get_key_list();
    eastl::sort(keys.begin(),keys.end(),WrapAlphaCompare());

    String pn(p_name);
    auto slash = StringUtils::find(pn,": ");
    if (slash != String::npos && pn.length() > slash) {
        StringView type = StringUtils::substr(pn,slash + 2, pn.length());
        int index = StringUtils::to_int(StringUtils::substr(pn,0, slash));
        if (type == "key"_sv && index < keys.size()) {

            const Variant &key = keys[index];
            UndoRedo *ur = EditorNode::get_undo_redo();

            ur->create_action(TTR("Change Dictionary Key"));
            ur->add_do_method(this, "_set_key", key, p_value);
            ur->add_undo_method(this, "_set_key", p_value, key);
            ur->add_do_method(this, "_notif_changev", p_name);
            ur->add_undo_method(this, "_notif_changev", p_name);
            ur->commit_action();

            return true;
        } else if (type == "value"_sv && index < keys.size()) {
            const auto &key = keys[index];
            if (dict.has(key)) {

                Variant value = dict[key];
                UndoRedo *ur = EditorNode::get_undo_redo();

                ur->create_action(TTR("Change Dictionary Value"));
                ur->add_do_method(this, "_set_value", key, p_value);
                ur->add_undo_method(this, "_set_value", key, value);
                ur->add_do_method(this, "_notif_changev", p_name);
                ur->add_undo_method(this, "_notif_changev", p_name);
                ur->commit_action();

                return true;
            }
        }
    }

    return false;
}

bool DictionaryPropertyEdit::_get(const StringName &p_name, Variant &r_ret) const {
    using namespace eastl;

    Dictionary dict = get_dictionary();
    auto keys = dict.get_key_list();
    eastl::sort(keys.begin(),keys.end(),WrapAlphaCompare());

    String pn(p_name);
    auto slash = StringUtils::find(pn,": ");

    if (slash != String::npos && pn.length() > slash) {

        StringView type = StringUtils::substr(pn,slash + 2, pn.length());
        int index = StringUtils::to_int(StringUtils::substr(pn,0, slash));

        if (type == "key"_sv && index < keys.size()) {
            r_ret = keys[index];
            return true;
        } else if (type == "value"_sv && index < keys.size()) {
            const auto &key = keys[index];
            if (dict.has(key)) {
                r_ret = dict[key];
                return true;
            }
        }
    }

    return false;
}

DictionaryPropertyEdit::DictionaryPropertyEdit() {
    obj = entt::null;
}
