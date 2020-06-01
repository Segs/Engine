/* http://www.segs.dev/
 * Copyright (c) 2006 - 2020 SEGS Team (see AUTHORS.md)
 * This software is licensed under the terms of the 3-clause BSD License.
 * See LICENSE.md for details.
*/

/**
 * CSharp binding generator creates the following hierarchy:
 *  Arguments:  godot.json TARGET_DIR
 *  Project name is built from json filename
 *  TARGET_DIR/MonoBindings/godot
 *      cpp_gen/
 *          CMakeLists.txt
 *          godot_editor_cs_bindings.gen.cpp
 *          godot_client_cs_bindings.gen.cpp
 *          godot_server_cs_bindings.gen.cpp
 *      cs_gen/
 *          Namespace_1/
 *              Namespace_2/
 *                  Class_1a.cs
 *              Class_1.cs
 *          Godot_Editor.csproj
 *          Godot_Client.csproj
 *          Godot_Server.csproj
 *  TARGET_DIR/project.sln will be updated.
 * Note: it will overwrite existing files !
 * By default, the produced plugin files are located under
 * PROJECT_SOURCE_DIR/bin/plugins
 * and compiled cs assemblies under PROJECT_SOURCE_DIR/bin/CSharp
 */

#include "bindings_generator.h"

#include "sln_support.h"
#include "cmake_support.h"

#include "core/string_builder.h"


#include "core/script_language.h"
//#include "core/string_formatter.h"
//#include "core/string_utils.h"
//#include "core/string_utils.inl"
#include "core/reflection_support/reflection_data.h"
#include "EASTL/vector_set.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStringBuilder>
#include <QUuid>
#include <cstdio>

static const QUuid g_generator_project_namespace("527d3b9b-e33e-485b-a8ea-baddfbdf7f68");

#if defined(DEBUG_METHODS_ENABLED) && defined(TOOLS_ENABLED)

void _err_print_error(const char* p_function, const char* p_file, int p_line, StringView p_error, StringView p_message, ErrorHandlerType p_type) {

    qWarning() << QLatin1String(p_error.data(), p_error.size());
    qWarning() << QLatin1String(p_message.data(), p_message.size());
}
int Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
    #ifdef _MSC_VER
        return _vsnprintf(pDestination, n, pFormat, arguments);
    #else
        return vsnprintf(pDestination, n, pFormat, arguments);
    #endif
}

static bool allUpperCase(StringView s) {
    for(char c : s) {
        if(eastl::CharToUpper(c)!=c)
            return false;
    }
    return true;
}


/*
#include "core/engine.h"
#include "core/global_constants.h"
#include "core/method_info.h"
#include "core/method_bind.h"
#include "core/io/compression.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/os/os.h"
#include "core/register_core_types.h"
#include "core/math/transform.h"
//#include "glue/cs_glue_version.gen.h"
#include "modules/register_module_types.h"
#include "plugins/plugin_registry_interface.h"
#include "scene/register_scene_types.h"

#include "../godotsharp_defs.h"
//#include "../mono_gd/gd_mono_marshal.h"
#include "../utils/path_utils.h"
#include "../utils/string_utils.h"
#include "main/main.h"
#include "csharp_project.h"
*/
#endif


static String snake_to_pascal_case(StringView p_identifier, bool p_input_is_upper = false) {

    String ret;
    Vector<StringView> parts;
    String::split_ref(parts,p_identifier, "_", true);

    for (size_t i = 0; i < parts.size(); i++) {
        String part(parts[i]);

        if (part.length()) {
            part[0] = eastl::CharToUpper(part[0]);
            if (p_input_is_upper) {
                for (size_t j = 1; j < part.length(); j++)
                    part[j] = eastl::CharToLower(part[j]);
            }
            ret += part;
        }
        else {
            if (i == 0 || i == (parts.size() - 1)) {
                // Preserve underscores at the beginning and end
                ret += "_";
            }
            else {
                // Preserve contiguous underscores
                if (parts[i - 1].length()) {
                    ret += "__";
                }
                else {
                    ret += "_";
                }
            }
        }
    }

    return ret;
}
static String snake_to_camel_case(StringView p_identifier, bool p_input_is_upper = false) {

    String ret;
    Vector<StringView> parts;
    String::split_ref(parts,p_identifier,'_',true);

    for (size_t i = 0; i < parts.size(); i++) {
        String part(parts[i]);

        if (part.length()) {
            if (i != 0) {
                part[0] = eastl::CharToUpper(part[0]);
            }
            if (p_input_is_upper) {
                for (size_t j = i != 0 ? 1 : 0; j < part.length(); j++)
                    part[j] = eastl::CharToLower(part[j]);
            }
            ret += part;
        }
        else {
            if (i == 0 || i == (parts.size() - 1)) {
                // Preserve underscores at the beginning and end
                ret += "_";
            }
            else {
                // Preserve contiguous underscores
                if (parts[i - 1].length()) {
                    ret += "__";
                }
                else {
                    ret += "_";
                }
            }
        }
    }

    return ret;
}


enum class CSAccessLevel {
    Public,
    Internal,
    Protected,
    Private
};

struct CSConstant {
    static HashMap<String,CSConstant *> constants;
    const ConstantInterface *m_rd_data;
    const TypeInterface *const_type;
    String xml_doc;
    String cs_name;
    String value;
    CSAccessLevel access_level = CSAccessLevel::Public;
    static String convert_name(StringView cpp_ns_name) {
        if (allUpperCase(cpp_ns_name))
            return snake_to_pascal_case(cpp_ns_name, true);
        return String(cpp_ns_name);
    }
    static CSConstant *get_instance_for(const String &access_path,const ConstantInterface *src) {
        auto iter = constants.find(access_path+src->name);
        if(iter!=constants.end())
            return iter->second;
        auto res = new CSConstant;
        res->m_rd_data = src;
        res->cs_name = convert_name(src->name);
        char buf[32]={0};
        snprintf(buf,31,"%d",src->value);
        res->value = buf;
        constants.emplace(access_path+src->name,res);
        //assert(false);
        return res;
    }
};
HashMap<String,CSConstant *> CSConstant::constants;

struct CSEnum {
    static HashMap<String,CSEnum *> enums;
    const EnumInterface *m_rd_data;
    Vector<const CSConstant *> m_entries;
    String xml_doc;
    String cs_name;

    static String convert_name(StringView cpp_ns_name) {
        return String(cpp_ns_name);
    }
    void add_constant(const String &access_path, const ConstantInterface *ci) {

        bool already_have_it=eastl::find_if(m_entries.begin(),m_entries.end(),
                                            [ci](const CSConstant *a) {
                          return a->m_rd_data==ci;
                      })!=m_entries.end();
        assert(!already_have_it);

        CSConstant *to_add = CSConstant::get_instance_for(access_path+"::"+cs_name,ci);
        m_entries.emplace_back(to_add);
    }
    static CSEnum *get_instance_for(const String &access_path,const EnumInterface *src) {
        auto iter = enums.find(access_path+src->cname);
        if(iter!=enums.end())
            return iter->second;
        auto res = new CSEnum;
        res->m_rd_data = src;
        res->cs_name = convert_name(src->cname);
        enums.emplace(access_path+src->cname,res);
        //assert(false);
        return res;
    }
};
HashMap<String,CSEnum *> CSEnum::enums;

struct CSType {
    String xml_doc;
    String cs_name;

    const TypeInterface* source_type;
    Vector<CSConstant *> m_class_constants;
    void add_constant(const String &access_path, const ConstantInterface *ci) {

        bool already_have_it=eastl::find_if(m_class_constants.begin(),m_class_constants.end(),
                                            [ci](const CSConstant *a) {
                          return a->m_rd_data==ci;
                      })!=m_class_constants.end();
        assert(!already_have_it);

        CSConstant *to_add = CSConstant::get_instance_for(access_path+"::"+cs_name,ci);
        m_class_constants.emplace_back(to_add);
    }
};

struct CSNamespace {
    static HashMap<String,CSNamespace *> namespaces;
    String cs_name;
    CSType m_globals;
    const NamespaceInterface *m_rd_data;

    Vector<CSEnum> m_enums;
    Vector<CSType> m_types;
    Vector<CSNamespace *> m_child_namespaces;

    static String convert_ns_name(StringView cpp_ns_name) {
        return String(cpp_ns_name);
    }
    static CSNamespace *get_instance_for(const String &access_path,const NamespaceInterface *src) {
        auto iter = namespaces.find(access_path+src->namespace_name);
        if(iter!=namespaces.end())
            return iter->second;

        auto res=new CSNamespace();
        res->m_rd_data = src;
        res->cs_name = convert_ns_name(src->namespace_name);
        namespaces[access_path+src->namespace_name] = res;
        return res;
    }

};
HashMap<String,CSNamespace *> CSNamespace::namespaces;


#if 0

#include "core/doc_support/doc_data.h"
#include "core/typesystem_decls.h"
#include "core/string_builder.h"

#include "EASTL/sort.h"
#include "EASTL/unordered_set.h"

#define CS_INDENT "    " // 4 whitespaces

#define INDENT1 CS_INDENT
#define INDENT2 INDENT1 INDENT1
#define INDENT3 INDENT2 INDENT1
#define INDENT4 INDENT3 INDENT1
#define INDENT5 INDENT4 INDENT1

#define MEMBER_BEGIN "\n" INDENT2

#define OPEN_BLOCK "{\n"
#define CLOSE_BLOCK "}\n"

#define OPEN_BLOCK_L2 INDENT2 OPEN_BLOCK INDENT3
#define OPEN_BLOCK_L3 INDENT3 OPEN_BLOCK INDENT4
#define OPEN_BLOCK_L4 INDENT4 OPEN_BLOCK INDENT5
#define CLOSE_BLOCK_L2 INDENT2 CLOSE_BLOCK
#define CLOSE_BLOCK_L3 INDENT3 CLOSE_BLOCK
#define CLOSE_BLOCK_L4 INDENT4 CLOSE_BLOCK

#define CS_FIELD_MEMORYOWN "memoryOwn"
#define CS_PARAM_INSTANCE "ptr"
#define CS_SMETHOD_GETINSTANCE "GetPtr"
#define CS_METHOD_CALL "Call"

#define GLUE_HEADER_FILE "modules/mono/glue/glue_header.h"
#define ICALL_PREFIX "godot_icall_"
#define SINGLETON_ICALL_SUFFIX "_get_singleton"
#define ICALL_GET_METHODBIND ICALL_PREFIX "Object_ClassDB_get_method"

#define C_LOCAL_RET "ret"
#define C_LOCAL_VARARG_RET "vararg_ret"
#define C_LOCAL_PTRCALL_ARGS "call_args"
#define C_MACRO_OBJECT_CONSTRUCT "GODOTSHARP_INSTANCE_OBJECT"

#define C_NS_MONOUTILS "GDMonoUtils"
#define C_NS_MONOINTERNALS "GDMonoInternals"
#define C_METHOD_TIE_MANAGED_TO_UNMANAGED C_NS_MONOINTERNALS "::tie_managed_to_unmanaged"
#define C_METHOD_UNMANAGED_GET_MANAGED C_NS_MONOUTILS "::unmanaged_get_managed"

#define C_NS_MONOMARSHAL "GDMonoMarshal"
#define C_METHOD_MANAGED_TO_VARIANT C_NS_MONOMARSHAL "::mono_object_to_variant"
#define C_METHOD_MANAGED_FROM_VARIANT C_NS_MONOMARSHAL "::variant_to_mono_object"
#define C_METHOD_MONOSTR_TO_GODOT C_NS_MONOMARSHAL "::mono_string_to_godot"
#define C_METHOD_MONOSTR_FROM_GODOT C_NS_MONOMARSHAL "::mono_string_from_godot"
#define C_METHOD_MONOARRAY_TO(m_type) C_NS_MONOMARSHAL "::mono_array_to_" #m_type
#define C_METHOD_MONOARRAY_TO_NC(m_type) C_NS_MONOMARSHAL "::mono_array_to_NC_" #m_type
#define C_METHOD_MONOARRAY_FROM(m_type) C_NS_MONOMARSHAL "::" #m_type "_to_mono_array"
#define C_METHOD_MONOARRAY_FROM_NC(m_type) C_NS_MONOMARSHAL "::" #m_type "_NC_to_mono_array"

#define BINDINGS_GENERATOR_VERSION UINT32_C(11)


static StringName _get_int_type_name_from_meta(GodotTypeInfo::Metadata p_meta);
static StringName _get_string_type_name_from_meta(GodotTypeInfo::Metadata p_meta);
static Error _save_file(StringView p_path, const StringBuilder& p_content);
DocData *g_doc_data;
static String fix_doc_description(StringView p_bbcode) {

    // This seems to be the correct way to do this. It's the same EditorHelp does.

    return String(StringUtils::strip_edges(StringUtils::dedent(p_bbcode).replaced("\t", "")
            .replaced("\r", "")));
}

struct NameCache {
    StringName type_void;
    StringName type_Array;
    StringName type_Dictionary;
    StringName type_Variant;
    StringName type_VarArg;
    StringName type_Object;
    StringName type_Reference;
    StringName type_RID;
    StringName type_String;
    StringName type_at_GlobalScope;
    StringName enum_Error;

    StringName type_sbyte;
    StringName type_short;
    StringName type_int;
    StringName type_long;
    StringName type_byte;
    StringName type_ushort;
    StringName type_uint;
    StringName type_ulong;
    StringName type_float;
    StringName type_double;

    NameCache() {
        type_void = StaticCString("void");
        type_Array = StaticCString("Array");
        type_Dictionary = StaticCString("Dictionary");
        type_Variant = StaticCString("Variant");
        type_VarArg = StaticCString("VarArg");
        type_Object = StaticCString("Object");
        type_Reference = StaticCString("RefCounted");
        type_RID = StaticCString("RID");
        type_String = StaticCString("String");
        type_at_GlobalScope = StaticCString("@GlobalScope");
        enum_Error = StaticCString("Error");

        type_sbyte = StaticCString("sbyte");
        type_short = StaticCString("short");
        type_int = StaticCString("int");
        type_long = StaticCString("long");
        type_byte = StaticCString("byte");
        type_ushort = StaticCString("ushort");
        type_uint = StaticCString("uint");
        type_ulong = StaticCString("ulong");
        type_float = StaticCString("float");
        type_double = StaticCString("double");
    }

private:
    NameCache(const NameCache &);
    NameCache &operator=(const NameCache &);
};

NameCache *name_cache;

ReflectionData rd;

static inline String get_unique_sig(const TypeInterface &p_type) {
    if (p_type.is_reference)
        return "Ref";
    else if (p_type.is_object_type)
        return "Obj";
    else if (p_type.is_enum)
        return "int";

    return p_type.name;
}


String BindingsGenerator::bbcode_to_xml(StringView p_bbcode, const TypeInterface *p_itype,DocData *doc) {

    // Based on the version in EditorHelp
    using namespace eastl;

    if (p_bbcode.empty())
        return String();

    String bbcode(p_bbcode);

    StringBuilder xml_output;

    xml_output.append("<para>");

    List<String> tag_stack;
    bool code_tag = false;

    size_t pos = 0;
    while (pos < bbcode.length()) {
        auto brk_pos = bbcode.find('[', pos);

        if (brk_pos == String::npos)
            brk_pos = bbcode.length();

        if (brk_pos > pos) {
            StringView text = StringUtils::substr(bbcode,pos, brk_pos - pos);
            if (code_tag || tag_stack.size() > 0) {
                xml_output.append(StringUtils::xml_escape(text));
            } else {
                Vector<StringView> lines = StringUtils::split(text,'\n');
                for (size_t i = 0; i < lines.size(); i++) {
                    if (i != 0)
                        xml_output.append("<para>");

                    xml_output.append(StringUtils::xml_escape(lines[i]));

                    if (i != lines.size() - 1)
                        xml_output.append("</para>\n");
                }
            }
        }

        if (brk_pos == bbcode.length())
            break; // nothing else to add

        size_t brk_end = bbcode.find("]", brk_pos + 1);

        if (brk_end == String::npos) {
            StringView text = StringUtils::substr(bbcode,brk_pos, bbcode.length() - brk_pos);
            if (code_tag || tag_stack.size() > 0) {
                xml_output.append(StringUtils::xml_escape(text));
            } else {
                Vector<StringView> lines = StringUtils::split(text,'\n');
                for (size_t i = 0; i < lines.size(); i++) {
                    if (i != 0)
                        xml_output.append("<para>");

                    xml_output.append(StringUtils::xml_escape(lines[i]));

                    if (i != lines.size() - 1)
                        xml_output.append("</para>\n");
                }
            }

            break;
        }

        StringView tag = StringUtils::substr(bbcode,brk_pos + 1, brk_end - brk_pos - 1);

        if (tag.starts_with('/')) {
            bool tag_ok = tag_stack.size() && tag_stack.front() == tag.substr(1, tag.length());

            if (!tag_ok) {
                xml_output.append("[");
                pos = brk_pos + 1;
                continue;
            }

            tag_stack.pop_front();
            pos = brk_end + 1;
            code_tag = false;

            if (tag == "/url"_sv) {
                xml_output.append("</a>");
            } else if (tag == "/code"_sv) {
                xml_output.append("</c>");
            } else if (tag == "/codeblock"_sv) {
                xml_output.append("</code>");
            }
        } else if (code_tag) {
            xml_output.append("[");
            pos = brk_pos + 1;
        } else if (tag.starts_with("method ") || tag.starts_with("member ") || tag.starts_with("signal ") || tag.starts_with("enum ") || tag.starts_with("constant ")) {
            StringView link_target = tag.substr(tag.find(" ") + 1, tag.length());
            StringView link_tag = tag.substr(0, tag.find(" "));

            Vector<StringView> link_target_parts = StringUtils::split(link_target,".");

            if (link_target_parts.size() <= 0 || link_target_parts.size() > 2) {
                ERR_PRINT("Invalid reference format: '" + tag + "'.");

                xml_output.append("<c>");
                xml_output.append(tag);
                xml_output.append("</c>");

                pos = brk_end + 1;
                continue;
            }

            const TypeInterface *target_itype;
            StringName target_cname;

            if (link_target_parts.size() == 2) {
                target_itype = rd._get_type_or_null(TypeReference { StringName(link_target_parts[0]) });
                if (!target_itype) {
                    target_itype = rd._get_type_or_null(TypeReference {"_" + link_target_parts[0]});
                }
                target_cname = StringName(link_target_parts[1]);
            } else {
                target_itype = p_itype;
                target_cname = StringName(link_target_parts[0]);
            }

            if (link_tag == "method"_sv) {
                if (!target_itype || !target_itype->is_object_type) {
                    if (OS::get_singleton()->is_stdout_verbose()) {
                        if (target_itype) {
                            OS::get_singleton()->print(FormatVE("Cannot resolve method reference for non-Godot.Object type in documentation: %.*s\n", link_target.size(),link_target.data()));
                        } else {
                            OS::get_singleton()->print(FormatVE("Cannot resolve type from method reference in documentation: %.*s\n", link_target.size(),link_target.data()));
                        }
                    }

                    // TODO Map what we can
                    xml_output.append("<c>");
                    xml_output.append(link_target);
                    xml_output.append("</c>");
                } else {
                    const MethodInterface *target_imethod = target_itype->find_method_by_name(target_cname);

                    if (target_imethod) {
                        xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                        xml_output.append(target_itype->proxy_name);
                        xml_output.append(".");
                        xml_output.append(target_imethod->proxy_name);
                        xml_output.append("\"/>");
                    }
                }
            } else if (link_tag == "member"_sv) {
                if (!target_itype || !target_itype->is_object_type) {
                    if (OS::get_singleton()->is_stdout_verbose()) {
                        if (target_itype) {
                            OS::get_singleton()->print(FormatVE("Cannot resolve member reference for non-Godot.Object type in documentation: %.*s\n", link_target.size(),link_target.data()));
                        } else {
                            OS::get_singleton()->print(FormatVE("Cannot resolve type from member reference in documentation: %.*s\n", link_target.size(),link_target.data()));
                        }
                    }

                    // TODO Map what we can
                    xml_output.append("<c>");
                    xml_output.append(link_target);
                    xml_output.append("</c>");
                } else {
                    const PropertyInterface *target_iprop = target_itype->find_property_by_name(target_cname);

                    if (target_iprop) {
                        xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                        xml_output.append(target_itype->proxy_name);
                        xml_output.append(".");
                        xml_output.append(target_iprop->proxy_name);
                        xml_output.append("\"/>");
                    }
                }
            } else if (link_tag == "signal"_sv) {
                // We do not declare signals in any way in C#, so there is nothing to reference
                xml_output.append("<c>");
                xml_output.append(link_target);
                xml_output.append("</c>");
            } else if (link_tag == "enum"_sv) {
                StringName search_cname = !target_itype ? target_cname :
                                                          StringName(target_itype->name + "." + (String)target_cname);

                auto enum_match = rd.enum_types.find(search_cname);

                if (rd.enum_types.end()==enum_match && search_cname != target_cname) {
                    enum_match = rd.enum_types.find(target_cname);
                }
                if (rd.enum_types.end() == enum_match) // try the fixed name -> "Enum"
                    enum_match = rd.enum_types.find(search_cname+"Enum");

                if (rd.enum_types.end()!=enum_match) {
                    const TypeInterface &target_enum_itype = enum_match->second;

                    xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                    xml_output.append(target_enum_itype.proxy_name); // Includes nesting class if any
                    xml_output.append("\"/>");
                } else {
                    ERR_PRINT("Cannot resolve enum reference in documentation: '" + link_target + "'.");

                    xml_output.append("<c>");
                    xml_output.append(link_target);
                    xml_output.append("</c>");
                }
            } else if (link_tag == "const"_sv) {
                if (!target_itype || !target_itype->is_object_type) {
                    if (OS::get_singleton()->is_stdout_verbose()) {
                        if (target_itype) {
                            OS::get_singleton()->print(FormatVE("Cannot resolve constant reference for non-Godot.Object type in documentation: %.*s\n", link_target.size(),link_target.data()));
                        } else {
                            OS::get_singleton()->print(FormatVE("Cannot resolve type from constant reference in documentation: %.*s\n", link_target.size(),link_target.data()));
                        }
                    }

                    // TODO Map what we can
                    xml_output.append("<c>");
                    xml_output.append(link_target);
                    xml_output.append("</c>");
                } else if (!target_itype && target_cname == name_cache->type_at_GlobalScope) {
                    // Try to find as a global constant
                    const ConstantInterface *target_iconst = rd.find_constant_by_name(target_cname, rd.global_constants);

                    if (target_iconst) {
                        // Found global constant
                        xml_output.append("<see cref=\"" BINDINGS_NAMESPACE "." BINDINGS_GLOBAL_SCOPE_CLASS ".");
                        xml_output.append(target_iconst->proxy_name);
                        xml_output.append("\"/>");
                    } else {
                        // Try to find as global enum constant
                        const EnumInterface *target_ienum = nullptr;

                        for (const EnumInterface &E : rd.global_enums) {
                            target_ienum = &E;
                            target_iconst = rd.find_constant_by_name(target_cname, target_ienum->constants);
                            if (target_iconst)
                                break;
                        }

                        if (target_iconst) {
                            xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                            xml_output.append(target_ienum->cname);
                            xml_output.append(".");
                            xml_output.append(target_iconst->proxy_name);
                            xml_output.append("\"/>");
                        } else {
                            ERR_PRINT("Cannot resolve global constant reference in documentation: '" + link_target + "'.");

                            xml_output.append("<c>");
                            xml_output.append(link_target);
                            xml_output.append("</c>");
                        }
                    }
                } else {
                    // Try to find the constant in the current class
                    const ConstantInterface *target_iconst = rd.find_constant_by_name(target_cname, target_itype->constants);

                    if (target_iconst) {
                        // Found constant in current class
                        xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                        xml_output.append(target_itype->proxy_name);
                        xml_output.append(".");
                        xml_output.append(target_iconst->proxy_name);
                        xml_output.append("\"/>");
                    } else {
                        // Try to find as enum constant in the current class
                        const EnumInterface *target_ienum = nullptr;

                        for (const EnumInterface &E : target_itype->enums) {
                            target_ienum = &E;
                            target_iconst = rd.find_constant_by_name(target_cname, target_ienum->constants);
                            if (target_iconst)
                                break;
                        }

                        if (target_iconst) {
                            xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                            xml_output.append(target_itype->proxy_name);
                            xml_output.append(".");
                            xml_output.append(target_ienum->cname);
                            xml_output.append(".");
                            xml_output.append(target_iconst->proxy_name);
                            xml_output.append("\"/>");
                        } else {
                            ERR_PRINT("Cannot resolve constant reference in documentation: '" + link_target + "'.");

                            xml_output.append("<c>");
                            xml_output.append(link_target);
                            xml_output.append("</c>");
                        }
                    }
                }
            }

            pos = brk_end + 1;
        } else if (doc->class_list.contains(StringName(tag))) {
            if (tag == "Array"_sv || tag == "Dictionary"_sv) {
                xml_output.append("<see cref=\"" BINDINGS_NAMESPACE_COLLECTIONS ".");
                xml_output.append(tag);
                xml_output.append("\"/>");
            } else if (tag == "bool"_sv || tag == "int"_sv) {
                xml_output.append("<see cref=\"");
                xml_output.append(tag);
                xml_output.append("\"/>");
            } else if (tag == "float"_sv) {
                xml_output.append("<see cref=\""
#ifdef REAL_T_IS_DOUBLE
                                  "double"
#else
                                  "float"
#endif
                                  "\"/>");
            } else if (tag == "Variant"_sv) {
                // We use System.Object for Variant, so there is no Variant type in C#
                xml_output.append("<c>Variant</c>");
            } else if (tag == "String"_sv) {
                xml_output.append("<see cref=\"string\"/>");
            } else if (tag == "Nil"_sv) {
                xml_output.append("<see langword=\"null\"/>");
            } else if (tag.starts_with('@')) {
                // @GlobalScope, @GDScript, etc
                xml_output.append("<c>");
                xml_output.append(tag);
                xml_output.append("</c>");
            } else if (tag == "PoolByteArray"_sv) {
                xml_output.append("<see cref=\"byte\"/>");
            } else if (tag == "PoolIntArray"_sv) {
                xml_output.append("<see cref=\"int\"/>");
            } else if (tag == "PoolRealArray"_sv) {
#ifdef REAL_T_IS_DOUBLE
                xml_output.append("<see cref=\"double\"/>");
#else
                xml_output.append("<see cref=\"float\"/>");
#endif
            } else if (tag == "PoolStringArray"_sv) {
                xml_output.append("<see cref=\"string\"/>");
            } else if (tag == "PoolVector2Array"_sv) {
                xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".Vector2\"/>");
            } else if (tag == "PoolVector3Array"_sv) {
                xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".Vector3\"/>");
            } else if (tag == "PoolColorArray"_sv) {
                xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".Color\"/>");
            } else {
                const TypeInterface *target_itype = rd._get_type_or_null({StringName(tag)});

                if (!target_itype) {
                    target_itype = rd._get_type_or_null({"_" + tag});
                }

                if (target_itype) {
                    xml_output.append("<see cref=\"" BINDINGS_NAMESPACE ".");
                    xml_output.append(target_itype->proxy_name);
                    xml_output.append("\"/>");
                } else {
                    ERR_PRINT("Cannot resolve type reference in documentation: '" + tag + "'.");

                    xml_output.append("<c>");
                    xml_output.append(tag);
                    xml_output.append("</c>");
                }
            }

            pos = brk_end + 1;
        } else if (tag == "b"_sv) {
            // bold is not supported in xml comments
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "i"_sv) {
            // italics is not supported in xml comments
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "code"_sv) {
            xml_output.append("<c>");

            code_tag = true;
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "codeblock"_sv) {
            xml_output.append("<code>");

            code_tag = true;
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "center"_sv) {
            // center is alignment not supported in xml comments
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "br"_sv) {
            xml_output.append("\n"); // FIXME: Should use <para> instead. Luckily this tag isn't used for now.
            pos = brk_end + 1;
        } else if (tag == "u"_sv) {
            // underline is not supported in xml comments
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "s"_sv) {
            // strikethrough is not supported in xml comments
            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag == "url"_sv) {
            size_t end = bbcode.find("[", brk_end);
            if (end == String::npos)
                end = bbcode.length();
            StringView url = StringUtils::substr(bbcode,brk_end + 1, end - brk_end - 1);
            xml_output.append("<a href=\"");
            xml_output.append(url);
            xml_output.append("\">");
            xml_output.append(url);

            pos = brk_end + 1;
            tag_stack.push_front(String(tag));
        } else if (tag.starts_with("url=")) {
            StringView url = tag.substr(4, tag.length());
            xml_output.append("<a href=\"");
            xml_output.append(url);
            xml_output.append("\">");

            pos = brk_end + 1;
            tag_stack.push_front("url");
        } else if (tag == "img"_sv) {
            auto end = bbcode.find("[", brk_end);
            if (end == String::npos)
                end = bbcode.length();
            StringView image(StringUtils::substr(bbcode,brk_end + 1, end - brk_end - 1));

            // Not supported. Just append the bbcode.
            xml_output.append("[img]");
            xml_output.append(image);
            xml_output.append("[/img]");

            pos = end;
            tag_stack.push_front(String(tag));
        } else if (tag.starts_with("color=")) {
            // Not supported.
            pos = brk_end + 1;
            tag_stack.push_front("color");
        } else if (tag.starts_with("font=")) {
            // Not supported.
            pos = brk_end + 1;
            tag_stack.push_front("font");
        } else {
            xml_output.append("["); // ignore
            pos = brk_pos + 1;
        }
    }

    xml_output.append("</para>");

    return xml_output.as_string();
}

int BindingsGenerator::_determine_enum_prefix(const EnumInterface &p_ienum) {

    CRASH_COND(p_ienum.constants.empty());

    const ConstantInterface &front_iconstant = p_ienum.constants.front();
    auto front_parts = front_iconstant.name.split('_', /* p_allow_empty: */ true);
    size_t candidate_len = front_parts.size() - 1;

    if (candidate_len == 0)
        return 0;

    for (const ConstantInterface &iconstant : p_ienum.constants) {

        auto parts = iconstant.name.split('_', /* p_allow_empty: */ true);

        size_t i;
        for (i = 0; i < candidate_len && i < parts.size(); i++) {
            if (front_parts[i] != parts[i]) {
                // HARDCODED: Some Flag enums have the prefix 'FLAG_' for everything except 'FLAGS_DEFAULT' (same for 'METHOD_FLAG_' and'METHOD_FLAGS_DEFAULT').
                bool hardcoded_exc = (i == candidate_len - 1 && ((front_parts[i] == "FLAGS" && parts[i] == "FLAG") || (front_parts[i] == "FLAG" && parts[i] == "FLAGS")));
                if (!hardcoded_exc)
                    break;
            }
        }
        candidate_len = i;

        if (candidate_len == 0)
            return 0;
    }

    return candidate_len;
}

void BindingsGenerator::_apply_prefix_to_enum_constants(EnumInterface &p_ienum, int p_prefix_length) {

    if (p_prefix_length <= 0)
        return;

    for (ConstantInterface &curr_const : p_ienum.constants) {
        int curr_prefix_length = p_prefix_length;

        String constant_name = curr_const.name;

        auto parts = constant_name.split('_', /* p_allow_empty: */ true);

        if (parts.size() <= curr_prefix_length)
            continue;

        if (parts[curr_prefix_length][0] >= '0' && parts[curr_prefix_length][0] <= '9') {
            // The name of enum constants may begin with a numeric digit when strip from the enum prefix,
            // so we make the prefix for this constant one word shorter in those cases.
            for (curr_prefix_length = curr_prefix_length - 1; curr_prefix_length > 0; curr_prefix_length--) {
                if (parts[curr_prefix_length][0] < '0' || parts[curr_prefix_length][0] > '9')
                    break;
            }
        }

        constant_name = "";
        for (int i = curr_prefix_length; i < parts.size(); i++) {
            if (i > curr_prefix_length)
                constant_name += "_";
            constant_name += parts[i];
        }

        curr_const.proxy_name = snake_to_pascal_case(constant_name, true);
    }
}
static void hash_combine(uint32_t &p_hash, const uint32_t &p_with_hash) {
    p_hash ^= p_with_hash + 0x9e3779b9 + (p_hash << 6) + (p_hash >> 2);
}
#endif
void BindingsGenerator::_generate_method_icalls(const TypeInterface& p_itype) {
    for (const MethodInterface& imethod : p_itype.methods) {

        if (imethod.is_virtual)
            continue;
#if 0

        FixedVector<StringName,16,true> unique_parts;
        String method_signature(p_itype.cname);
        method_signature+="_"+imethod.cname+"_";
        const TypeInterface *return_type = rd._get_type_or_placeholder(imethod.return_type);

        String im_sig;
        String im_unique_sig = String(imethod.return_type.cname) + ",IntPtr,IntPtr";

        im_sig += "IntPtr " CS_PARAM_INSTANCE;
        // Get arguments information
        int i = 0;
        for (const ArgumentInterface &F : imethod.arguments) {
            const TypeInterface *arg_type = rd._get_type_or_placeholder(F.type);

            im_sig += ", ";
            im_sig += arg_type->im_type_in;
            im_sig += " arg";
            im_sig += itos(i + 1);

            im_unique_sig += ",";
            im_unique_sig += get_unique_sig(*arg_type)+arg_type->cname;
            unique_parts.push_back(F.type.cname);

            i++;
        }
        method_signature = method_signature.replaced(".","_");
        uint32_t arg_hash= StringUtils::hash(return_type->cname);
        for(const StringName &s : unique_parts) {
            hash_combine(arg_hash, StringUtils::hash(s));
        }
        im_unique_sig = method_signature+StringUtils::num_int64(arg_hash,16);
        method_signature+= StringUtils::num_int64(arg_hash, 16);
        String im_type_out = return_type->im_type_out;

        if (return_type->ret_as_byref_arg) {
            // Doesn't affect the unique signature
            im_type_out = "void";

            im_sig += ", ";
            im_sig += return_type->im_type_out;
            im_sig += " argRet";

            i++;
        }

        // godot_icall_{argc}_{icallcount}
        String icall_method = ICALL_PREFIX;
        icall_method += method_signature;
        if(p_itype.cname=="Object" && imethod.cname=="free")
            continue;

        InternalCall im_icall = InternalCall(p_itype.api_type, icall_method, im_type_out, im_sig, im_unique_sig);

        auto iter_match = method_icalls.find(im_icall.unique_sig);

        if (iter_match != method_icalls.end()) {
            if (p_itype.api_type != APIType::Editor)
                iter_match->second.editor_only = false;
            method_icalls_map.emplace(&imethod, &iter_match->second);
        }
        else {
            auto loc = method_icalls.emplace(im_icall.unique_sig, im_icall);
            method_icalls_map.emplace(&imethod, &loc.first->second);
        }
#endif
    }
}
static void _generate_global_constants(StringBuilder &p_output,const String &ns_name,DocData *doc) {

    // Constants (in partial GD class)

    p_output.append("\n#pragma warning disable CS1591 // Disable warning: "
                    "'Missing XML comment for publicly visible type or member'\n");

    p_output.append("namespace " +ns_name+ "\n {\n");
    p_output.indent();
    p_output.append_indented("public static partial class Constants\n");
    p_output.append_indented("{\n");

#if 0
    for (const ConstantInterface &iconstant : rd.global_constants) {
        p_output.indent();
        auto const_doc = rd.constant_doc("@GlobalScope","",iconstant.name);
        if (const_doc && const_doc->description.size()) {
            String xml_summary = bbcode_to_xml(fix_doc_description(const_doc->description), nullptr,doc);
            auto summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

            if (summary_lines.size()) {
                p_output.append_indented("/// <summary>\n");

                for (size_t i = 0; i < summary_lines.size(); i++) {
                    p_output.append_indented("/// ");
                    p_output.append(summary_lines[i]);
                    p_output.append("\n");
                }

                p_output.append_indented("/// </summary>\n");
            }
        }

        p_output.append_indented("public const int ");
        p_output.append(iconstant.proxy_name);
        p_output.append(" = ");
        p_output.append(itos(iconstant.value));
        p_output.append(";");
        p_output.dedent();
    }

    if (!rd.global_constants.empty())
        p_output.append("\n");

    p_output.append(INDENT1 CLOSE_BLOCK); // end of GD class

    // Enums

    for (const EnumInterface &ienum : rd.global_enums) {

        CRASH_COND(ienum.constants.empty());

        StringView enum_proxy_name(ienum.cname);

        bool enum_in_static_class = false;

        if (enum_proxy_name.contains(".")) {
            enum_in_static_class = true;
            StringView enum_class_name = StringUtils::get_slice(enum_proxy_name,'.', 0);
            enum_proxy_name = StringUtils::get_slice(enum_proxy_name,'.', 1);

            CRASH_COND(enum_class_name != StringView("Variant")); // Hard-coded...

            _log("Declaring global enum '%.*s' inside static class '%.*s'\n", int(enum_proxy_name.size()),enum_proxy_name.data(),
                 int(enum_class_name.size()),enum_class_name.data());

            p_output.append("\n" INDENT1 "public static partial class ");
            p_output.append(enum_class_name);
            p_output.append("\n" INDENT1 OPEN_BLOCK);
        }

        p_output.append("\n" INDENT1 "public enum ");
        p_output.append(enum_proxy_name);
        p_output.append("\n" INDENT1 OPEN_BLOCK);

        for (const ConstantInterface &iconstant : ienum.constants) {
            auto const_doc = rd.constant_doc("@GlobalScope",String(ienum.cname),iconstant.name);

            if (const_doc && const_doc->description.size()) {
                String xml_summary = bbcode_to_xml(fix_doc_description(const_doc->description), nullptr,doc);
                Vector<String> summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

                if (summary_lines.size()) {
                    p_output.append(INDENT2 "/// <summary>\n");

                    for (int i = 0; i < summary_lines.size(); i++) {
                        p_output.append(INDENT2 "/// ");
                        p_output.append(summary_lines[i]);
                        p_output.append("\n");
                    }

                    p_output.append(INDENT2 "/// </summary>\n");
                }
            }

            p_output.append(INDENT2);
            p_output.append(iconstant.proxy_name);
            p_output.append(" = ");
            p_output.append(itos(iconstant.value));
            p_output.append(&iconstant != &ienum.constants.back() ? ",\n" : "\n");
        }

        p_output.append(INDENT1 CLOSE_BLOCK);

        if (enum_in_static_class)
            p_output.append(INDENT1 CLOSE_BLOCK);
    }

#endif
    p_output.append("} // end of namespace");

    p_output.append("\n#pragma warning restore CS1591\n");
}
#if 0
Error BindingsGenerator::generate_cs_core_project(StringView p_proj_dir,GeneratorContext &ctx,DocData *doc) {

    ERR_FAIL_COND_V(!initialized, ERR_UNCONFIGURED);

    DirAccessRef da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
    ERR_FAIL_COND_V(!da, ERR_CANT_CREATE);

    if (!DirAccess::exists(p_proj_dir)) {
        Error err = da->make_dir_recursive(p_proj_dir);
        ERR_FAIL_COND_V_MSG(err != OK, ERR_CANT_CREATE, "Cannot create directory '" + p_proj_dir + "'.");
    }

    da->change_dir(p_proj_dir);
    da->make_dir("Generated");
    da->make_dir("Generated/GodotObjects");

    String base_gen_dir = path::join(p_proj_dir, "Generated");
    String godot_objects_gen_dir = path::join(base_gen_dir, "GodotObjects");

    Vector<String> compile_items;

    // Generate source file for global scope constants and enums
    {
        StringBuilder constants_source;
        _generate_global_constants(constants_source,doc);
        String output_file = path::join(base_gen_dir, ctx.m_globals_class + "_constants.cs");
        Error save_err = _save_file(output_file, constants_source);
        if (save_err != OK)
            return save_err;

        compile_items.emplace_back(output_file);
    }

    for (const StringName & E : rd.obj_type_insert_order) {
        const TypeInterface &itype = rd.obj_types[E];

        if (itype.api_type == ClassDB::API_EDITOR)
            continue;

        String output_file = path::join(godot_objects_gen_dir, itype.proxy_name + ".cs");
        Error err = _generate_cs_type(itype, output_file,ctx);

        if (err == ERR_SKIP)
            continue;

        if (err != OK)
            return err;

        compile_items.emplace_back(output_file);
    }

    // Generate sources from compressed files

    StringBuilder cs_icalls_content;

    cs_icalls_content.append("using System;\n"
                             "using System.Runtime.CompilerServices;\n"
                             "\n");
    cs_icalls_content.append("namespace " + ctx.m_cs_namespace + "\n {\n");
    cs_icalls_content.indent();
    cs_icalls_content.append_indented("internal static class " + ctx.m_native_calls_class + "\n    {\n");
    cs_icalls_content.indent();

    cs_icalls_content.append_indented("internal static ulong godot_api_hash = ");
    cs_icalls_content.append(StringUtils::num_uint64(ctx.api_hash) + ";\n");
    cs_icalls_content.append_indented("internal static uint bindings_version = ");
    cs_icalls_content.append(StringUtils::num_uint64(BINDINGS_GENERATOR_VERSION) + ";\n");
    cs_icalls_content.append_indented("internal static uint cs_glue_version = ");
    cs_icalls_content.append(StringUtils::num_uint64(ctx.cs_side_hash) + ";\n");
    cs_icalls_content.append("\n");

#define ADD_INTERNAL_CALL(m_icall)                                                               \
    if (!m_icall.editor_only) {                                                                  \
        cs_icalls_content.append_indented("[MethodImpl(MethodImplOptions.InternalCall)]\n"); \
        cs_icalls_content.append_indented("internal static extern ");                             \
        cs_icalls_content.append(m_icall.im_type_out + " ");                                     \
        cs_icalls_content.append(m_icall.name + "(");                                            \
        cs_icalls_content.append(m_icall.im_sig + ");\n");                                       \
    }

    for (const InternalCall &E : ctx.custom_icalls)
        ADD_INTERNAL_CALL(E)
    auto keys=method_icalls.keys();
    eastl::sort(keys.begin(),keys.end());
    for (const auto &E : keys)
        ADD_INTERNAL_CALL(method_icalls[E])

#undef ADD_INTERNAL_CALL
    cs_icalls_content.dedent();
    cs_icalls_content.append_indented("}\n" "}\n");

    String internal_methods_file = path::join(base_gen_dir, ctx.m_native_calls_class + ".cs");

    Error err = _save_file(internal_methods_file, cs_icalls_content);
    if (err != OK)
        return err;

    compile_items.emplace_back(internal_methods_file);

    StringBuilder includes_props_content;
    includes_props_content.append("<Project>\n"
                                  "  <ItemGroup>\n");

    for (size_t i = 0; i < compile_items.size(); i++) {
        String include = path::relative_to(compile_items[i], p_proj_dir).replaced("/", "\\");
        includes_props_content.append("    <Compile Include=\"" + include + "\" />\n");
    }

    includes_props_content.append("  </ItemGroup>\n"
                                  "</Project>\n");

    String includes_props_file = path::join(base_gen_dir, "GeneratedIncludes.props");

    err = _save_file(includes_props_file, includes_props_content);
    if (err != OK)
        return err;

    return OK;
}

Error BindingsGenerator::generate_cs_editor_project(const String &p_proj_dir, GeneratorContext &ctx) {

    ERR_FAIL_COND_V(!initialized, ERR_UNCONFIGURED);

    DirAccessRef da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
    ERR_FAIL_COND_V(!da, ERR_CANT_CREATE);

    if (!DirAccess::exists(p_proj_dir)) {
        Error err = da->make_dir_recursive(p_proj_dir);
        ERR_FAIL_COND_V_MSG(err != OK, ERR_CANT_CREATE, "Cannot create directory '" + p_proj_dir + "'.");
    }

    da->change_dir(p_proj_dir);
    da->make_dir("Generated");
    da->make_dir("Generated/GodotObjects");

    String base_gen_dir = path::join(p_proj_dir, "Generated");
    String godot_objects_gen_dir = path::join(base_gen_dir, "GodotObjects");

    Vector<String> compile_items;

    for (const StringName& E : rd.obj_type_insert_order) {
        const TypeInterface& itype = rd.obj_types[E];

        if (itype.api_type != ClassDB::API_EDITOR)
            continue;

        String output_file = path::join(godot_objects_gen_dir, itype.proxy_name + ".cs");
        Error err = _generate_cs_type(itype, output_file,ctx);

        if (err == ERR_SKIP)
            continue;

        if (err != OK)
            return err;

        compile_items.emplace_back(output_file);
    }

    // Generate sources from compressed files

    StringBuilder cs_icalls_content;

    cs_icalls_content.append("using System;\n"
                             "using System.Runtime.CompilerServices;\n"
                             "\n");
    cs_icalls_content.append("namespace " + ctx.m_cs_namespace + "\n{\n");
    cs_icalls_content.indent();
    cs_icalls_content.append_indented("internal static class " + ctx.m_native_calls_class + "\n    {\n");
    cs_icalls_content.indent();
    cs_icalls_content.append_indented("internal static ulong godot_api_hash = ");
    cs_icalls_content.append(StringUtils::num_uint64(ctx.api_hash) + ";\n");
    cs_icalls_content.append_indented("internal static uint bindings_version = ");
    cs_icalls_content.append(StringUtils::num_uint64(BINDINGS_GENERATOR_VERSION) + ";\n");
    cs_icalls_content.append_indented("internal static uint cs_glue_version = ");
    cs_icalls_content.append(StringUtils::num_uint64(ctx.cs_side_hash) + ";\n");
    cs_icalls_content.append("\n");

#define ADD_INTERNAL_CALL(m_icall)                                                          \
    if (m_icall.editor_only) {                                                              \
        cs_icalls_content.append_indented("[MethodImpl(MethodImplOptions.InternalCall)]\n"); \
        cs_icalls_content.append_indented("internal static extern ");                       \
        cs_icalls_content.append(m_icall.im_type_out + " ");                                \
        cs_icalls_content.append(m_icall.name + "(");                                       \
        cs_icalls_content.append(m_icall.im_sig + ");\n");                                  \
    }

    for (const InternalCall &E : ctx.custom_icalls)
        ADD_INTERNAL_CALL(E)

    auto keys=method_icalls.keys();
    eastl::sort(keys.begin(),keys.end());
    for (const auto &E : keys)
        ADD_INTERNAL_CALL(method_icalls[E])

#undef ADD_INTERNAL_CALL
    cs_icalls_content.dedent();
    cs_icalls_content.append_indented("}\n" "}\n");

    String internal_methods_file = path::join(base_gen_dir, ctx.m_native_calls_class + ".cs");

    Error err = _save_file(internal_methods_file, cs_icalls_content);
    if (err != OK)
        return err;

    compile_items.emplace_back(internal_methods_file);

    StringBuilder includes_props_content;
    includes_props_content.append("<Project>\n"
                                  "  <ItemGroup>\n");

    for (size_t i = 0; i < compile_items.size(); i++) {
        String include = path::relative_to(compile_items[i], p_proj_dir).replaced("/", "\\");
        includes_props_content.append("    <Compile Include=\"" + include + "\" />\n");
    }

    includes_props_content.append("  </ItemGroup>\n"
                                  "</Project>\n");

    String includes_props_file = path::join(base_gen_dir, "GeneratedIncludes.props");

    err = _save_file(includes_props_file, includes_props_content);
    if (err != OK)
        return err;

    return OK;
}

Error BindingsGenerator::generate_cs_api(StringView p_output_dir, GeneratorContext &ctx,DocData *doc) {

    ERR_FAIL_COND_V(!initialized, ERR_UNCONFIGURED);

    String output_dir = path::abspath(path::realpath(p_output_dir));

    DirAccessRef da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
    ERR_FAIL_COND_V(!da, ERR_CANT_CREATE);

    if (!DirAccess::exists(output_dir)) {
        Error err = da->make_dir_recursive(output_dir);
        ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);
    }

    Error proj_err;

    // Generate GodotSharp source files

    String core_proj_dir = PathUtils::plus_file(output_dir,ctx.m_assembly_name);

    proj_err = generate_cs_core_project(core_proj_dir,ctx,doc);
    if (proj_err != OK) {
        ERR_PRINT("Generation of the Core API C# project failed.");
        return proj_err;
    }

    assert(false);
//    // Generate GodotSharpEditor source files

//    String editor_proj_dir = PathUtils::plus_file(output_dir,EDITOR_API_ASSEMBLY_NAME);

//    proj_err = generate_cs_editor_project(editor_proj_dir,editor_generator);
//    if (proj_err != OK) {
//        ERR_PRINT("Generation of the Editor API C# project failed.");
//        return proj_err;
//    }

//    _log("The Godot API sources were successfully generated\n");

    return OK;
}

Error BindingsGenerator::generate_cs_type_docs(const TypeInterface &itype, const DocData::ClassDoc *class_doc, StringBuilder &output)
{
    if (!class_doc)
        return OK;
    // Add constants

    for (const ConstantInterface &iconstant : itype.constants) {
        auto const_doc = rd.constant_doc(itype.proxy_name,"",iconstant.name);
        if (const_doc && const_doc->description.size()) {
            String xml_summary = bbcode_to_xml(fix_doc_description(const_doc->description), &itype,rd.doc);
            Vector<String> summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

            if (summary_lines.size()) {
                output.append(MEMBER_BEGIN "/// <summary>\n");

                for (int i = 0; i < summary_lines.size(); i++) {
                    output.append(INDENT2 "/// ");
                    output.append(summary_lines[i]);
                    output.append("\n");
                }

                output.append(INDENT2 "/// </summary>");
            }
        }

        output.append(MEMBER_BEGIN "public const int ");
        output.append(iconstant.proxy_name);
        output.append(" = ");
        output.append(itos(iconstant.value));
        output.append(";");
    }

    if (itype.constants.size())
        output.append("\n");

    // Add enums

    for (const EnumInterface &ienum : itype.enums) {

        ERR_FAIL_COND_V(ienum.constants.empty(), ERR_BUG);

        output.append(MEMBER_BEGIN "public enum ");
        output.append(ienum.cname);
        output.append(MEMBER_BEGIN OPEN_BLOCK);

        for (const ConstantInterface &iconstant : ienum.constants) {

            auto const_doc = rd.constant_doc(itype.proxy_name,String(ienum.cname),iconstant.name);

            if (const_doc && const_doc->description.size()) {
                String xml_summary = bbcode_to_xml(fix_doc_description(const_doc->description), &itype,rd.doc);
                Vector<String> summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

                if (summary_lines.size()) {
                    output.append(INDENT3 "/// <summary>\n");

                    for (size_t i = 0; i < summary_lines.size(); i++) {
                        output.append(INDENT3 "/// ");
                        output.append(summary_lines[i]);
                        output.append("\n");
                    }

                    output.append(INDENT3 "/// </summary>\n");
                }
            }

            output.append(INDENT3);
            output.append(iconstant.proxy_name);
            output.append(" = ");
            output.append(itos(iconstant.value));
            output.append(&iconstant != &ienum.constants.back() ? ",\n" : "\n");
        }

        output.append(INDENT2 CLOSE_BLOCK);
    }

    // Add properties

    for (const PropertyInterface &iprop : itype.properties) {
        Error prop_err = _generate_cs_property(itype, iprop, output);
        ERR_FAIL_COND_V_MSG(prop_err != OK, prop_err,
                String("Failed to generate property '") + iprop.cname + "' for class '" + itype.name + "'.");
    }
    return OK;
}
void BindingsGenerator::generate_cs_type_doc_summary(const TypeInterface &itype, const DocData::ClassDoc *class_doc, StringBuilder &output)
{
    if (class_doc && !class_doc->description.empty()) {
        String xml_summary = bbcode_to_xml(fix_doc_description(class_doc->description), &itype,rd.doc);
        Vector<String> summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

        if (summary_lines.size()) {
            output.append(INDENT1 "/// <summary>\n");

            for (size_t i = 0; i < summary_lines.size(); i++) {
                output.append(INDENT1 "/// ");
                output.append(summary_lines[i]);
                output.append("\n");
            }

            output.append(INDENT1 "/// </summary>\n");
        }
    }
}
// FIXME: There are some members that hide other inherited members.
// - In the case of both members being the same kind, the new one must be declared
// explicitly as 'new' to avoid the warning (and we must print a message about it).
// - In the case of both members being of a different kind, then the new one must
// be renamed to avoid the name collision (and we must print a warning about it).
// - Csc warning e.g.:
// ObjectType/LineEdit.cs(140,38): warning CS0108: 'LineEdit.FocusMode' hides inherited member 'Control.FocusMode'. Use the new keyword if hiding was intended.


Error BindingsGenerator::_generate_cs_type(const TypeInterface &itype, StringView p_output_file, GeneratorContext &ctx) {

    CRASH_COND(!itype.is_object_type);

    bool is_derived_type = itype.base_name != StringName();

    if (!is_derived_type && !itype.is_namespace) {
        // Some Godot.Object assertions
        CRASH_COND(itype.cname != name_cache->type_Object);
        CRASH_COND(!itype.is_instantiable);
        CRASH_COND(itype.api_type != ClassDB::API_CORE);
        CRASH_COND(itype.is_reference);
        CRASH_COND(itype.is_singleton);
    }
    //itype.api_type == ClassDB::API_EDITOR ? editor_custom_icalls : core_custom_icalls;
    List<InternalCall> &custom_icalls = ctx.custom_icalls;

    _log("Generating %s.cs...\n", itype.proxy_name.asCString());

    String ctor_method(ICALL_PREFIX + itype.proxy_name + "_Ctor"); // Used only for derived types

    StringBuilder output;

    output.append("using System;\n"); // IntPtr
    output.append("using System.Diagnostics;\n"); // DebuggerBrowsable

    output.append("\n"
                  "#pragma warning disable CS1591 // Disable warning: "
                  "'Missing XML comment for publicly visible type or member'\n"
                  "#pragma warning disable CS1573 // Disable warning: "
                  "'Parameter has no matching param tag in the XML comment'\n");

    output.append("\nnamespace " BINDINGS_NAMESPACE "\n" OPEN_BLOCK);

    auto iter = rd.doc->class_list.find(itype.proxy_name);
    const DocData::ClassDoc *class_doc = iter!=rd.doc->class_list.end() ? &iter->second : nullptr;

    generate_cs_type_doc_summary(itype, class_doc, output);

    output.append(INDENT1 "public ");
    if (itype.is_singleton) {
        output.append("static partial class ");
    } else {
        if(itype.is_namespace)
            output.append("static class ");
        else
            output.append(itype.is_instantiable ? "partial class " : "abstract partial class ");
    }
    output.append(itype.proxy_name);

    if (itype.is_singleton || itype.is_namespace) {
        output.append("\n");
    } else if (is_derived_type) {
        if (rd.obj_types.contains(itype.base_name)) {
            output.append(" : ");
            output.append(rd.obj_types[itype.base_name].proxy_name);
            output.append("\n");
        } else {
            ERR_PRINT(String("Base type '") + itype.base_name + "' does not exist, for class '" + itype.name + "'.");
            return ERR_INVALID_DATA;
        }
    }

    output.append(INDENT1 "{");

    Error res=generate_cs_type_docs(itype, class_doc, output);
    if(res!=OK)
        return res;

    // TODO: BINDINGS_NATIVE_NAME_FIELD should be StringName, once we support it in C#

    if (itype.is_singleton) {
        // Add the type name and the singleton pointer as static fields

        output.append(MEMBER_BEGIN "private static Godot.Object singleton;\n");
        output.append(MEMBER_BEGIN "public static Godot.Object Singleton\n" INDENT2 "{\n" INDENT3
                                   "get\n" INDENT3 "{\n" INDENT4 "if (singleton == null)\n" INDENT5
                                   "singleton = Engine.GetNamedSingleton(typeof(");
        output.append(itype.proxy_name);
        output.append(").Name);\n" INDENT4 "return singleton;\n" INDENT3 "}\n" INDENT2 "}\n");

        output.append(MEMBER_BEGIN "private const string " BINDINGS_NATIVE_NAME_FIELD " = \"");
        output.append(itype.name);
        output.append("\";\n");

        output.append(INDENT2 "internal static IntPtr " BINDINGS_PTR_FIELD " = ");
        output.append(itype.api_type == ClassDB::API_EDITOR ? BINDINGS_CLASS_NATIVECALLS_EDITOR : BINDINGS_CLASS_NATIVECALLS);
        output.append("." ICALL_PREFIX);
        output.append(itype.name);
        output.append(SINGLETON_ICALL_SUFFIX "();\n");
    } else if (is_derived_type) {
        // Add member fields

        output.append(MEMBER_BEGIN "private const string " BINDINGS_NATIVE_NAME_FIELD " = \"");
        output.append(itype.name);
        output.append("\";\n");

        // Add default constructor
        if (itype.is_instantiable) {
            output.append(MEMBER_BEGIN "public ");
            output.append(itype.proxy_name);
            output.append("() : this(");
            output.append(itype.memory_own ? "true" : "false");

            // The default constructor may also be called by the engine when instancing existing native objects
            // The engine will initialize the pointer field of the managed side before calling the constructor
            // This is why we only allocate a new native object from the constructor if the pointer field is not set
            output.append(")\n" OPEN_BLOCK_L2 "if (" BINDINGS_PTR_FIELD " == IntPtr.Zero)\n" INDENT4 BINDINGS_PTR_FIELD " = ");
            output.append(itype.api_type == ClassDB::API_EDITOR ? BINDINGS_CLASS_NATIVECALLS_EDITOR : BINDINGS_CLASS_NATIVECALLS);
            output.append("." + ctor_method);
            output.append("(this);\n" CLOSE_BLOCK_L2);
        } else {
            // Hide the constructor
            output.append(MEMBER_BEGIN "internal ");
            output.append(itype.proxy_name);
            output.append("() {}\n");
        }

        // Add.. em.. trick constructor. Sort of.
        output.append(MEMBER_BEGIN "internal ");
        output.append(itype.proxy_name);
        output.append("(bool " CS_FIELD_MEMORYOWN ") : base(" CS_FIELD_MEMORYOWN ") {}\n");
    }

    int method_bind_count = 0;
    for (const MethodInterface &imethod : itype.methods) {

        Error method_err = _generate_cs_method(itype, imethod, method_bind_count, output);
        ERR_FAIL_COND_V_MSG(method_err != OK, method_err,
                "Failed to generate method '" + imethod.name + "' for class '" + itype.name + "'.");
    }

    if (itype.is_singleton) {
        InternalCall singleton_icall = InternalCall(itype.api_type, ICALL_PREFIX + itype.name + SINGLETON_ICALL_SUFFIX, "IntPtr");

        if (!has_named_icall(singleton_icall.name, custom_icalls))
            custom_icalls.push_back(singleton_icall);
    }

    if (is_derived_type && itype.is_instantiable) {
        InternalCall ctor_icall = InternalCall(itype.api_type, ctor_method, "IntPtr", String(itype.proxy_name) + " obj");

        if (!has_named_icall(ctor_icall.name, custom_icalls))
            custom_icalls.push_back(ctor_icall);
    }

    output.append(INDENT1 CLOSE_BLOCK /* class */
                    CLOSE_BLOCK /* namespace */);

    output.append("\n"
                  "#pragma warning restore CS1591\n"
                  "#pragma warning restore CS1573\n");

    return _save_file(p_output_file, output);
}
static bool covariantSetterGetterTypes(StringView getter,StringView setter) {
    using namespace eastl;
    if(getter==setter)
        return true;
    bool getter_stringy_type = (getter=="String"_sv) || (getter == "StringName"_sv) || (getter == "StringView"_sv);
    bool setter_stringy_type = (setter == "String"_sv) || (setter == "StringName"_sv) || (setter == "StringView"_sv);
    return getter_stringy_type== setter_stringy_type;
}
Error BindingsGenerator::_generate_cs_property(const TypeInterface &p_itype, const PropertyInterface &p_iprop, StringBuilder &p_output) {

    const MethodInterface *setter = p_itype.find_method_by_name(p_iprop.setter);

    // Search it in base types too
    const TypeInterface *current_type = &p_itype;
    while (!setter && current_type->base_name != StringName()) {
        auto base_match = rd.obj_types.find(current_type->base_name);
        ERR_FAIL_COND_V_MSG(rd.obj_types.end()==base_match, ERR_BUG,
                "Type not found '" + current_type->base_name + "'. Inherited by '" + current_type->name + "'.");
        current_type = &base_match->second;
        setter = current_type->find_method_by_name(p_iprop.setter);
    }

    const MethodInterface *getter = p_itype.find_method_by_name(p_iprop.getter);

    // Search it in base types too
    current_type = &p_itype;
    while (!getter && current_type->base_name != StringName()) {
        auto base_match = rd.obj_types.find(current_type->base_name);
        ERR_FAIL_COND_V_MSG(rd.obj_types.end() == base_match, ERR_BUG,
                "Type not found '" + current_type->base_name + "'. Inherited by '" + current_type->name + "'.");
        current_type = &base_match->second;
        getter = current_type->find_method_by_name(p_iprop.getter);
    }

    ERR_FAIL_COND_V(!setter && !getter, ERR_BUG);

    if (setter) {
        int setter_argc = p_iprop.index != -1 ? 2 : 1;
        ERR_FAIL_COND_V(setter->arguments.size() != setter_argc, ERR_BUG);
    }

    if (getter) {
        int getter_argc = p_iprop.index != -1 ? 1 : 0;
        ERR_FAIL_COND_V(getter->arguments.size() != getter_argc, ERR_BUG);
    }

    if (getter && setter) {
        if (unlikely(!covariantSetterGetterTypes(getter->return_type.cname , setter->arguments.back().type.cname))) {
            _err_print_error(FUNCTION_STR, __FILE__, __LINE__,
                    "Condition ' getter->return_type.cname != setter->arguments.back().type.cname "
                    " ' is true. returned: ERR_BUG");
            return ERR_BUG;
        }
    }

    const TypeReference &proptype_name = getter ? getter->return_type : setter->arguments.back().type;

    const TypeInterface *prop_itype = rd._get_type_or_null(proptype_name);
    ERR_FAIL_NULL_V(prop_itype, ERR_BUG); // Property type not found
    auto prop_doc = rd.doc_lookup_helpers[p_itype.proxy_name].properties.at(String(p_iprop.cname),nullptr);
    if (prop_doc && prop_doc->description.size()) {
        String xml_summary = bbcode_to_xml(fix_doc_description(prop_doc->description), &p_itype,rd.doc);
        Vector<String> summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

        if (!summary_lines.empty()) {
            p_output.append(MEMBER_BEGIN "/// <summary>\n");

            for (int i = 0; i < summary_lines.size(); i++) {
                p_output.append(INDENT2 "/// ");
                p_output.append(summary_lines[i]);
                p_output.append("\n");
            }

            p_output.append(INDENT2 "/// </summary>");
        }
    }

    p_output.append(MEMBER_BEGIN "public ");

    if (p_itype.is_singleton)
        p_output.append("static ");

    p_output.append(prop_itype->cs_type);
    p_output.append(" ");
    p_output.append(p_iprop.proxy_name);
    p_output.append("\n" INDENT2 OPEN_BLOCK);

    if (getter) {
        p_output.append(INDENT3 "get\n"

                                // TODO Remove this once we make accessor methods private/internal (they will no longer be marked as obsolete after that)
                                "#pragma warning disable CS0618 // Disable warning about obsolete method\n"

                OPEN_BLOCK_L3);

        p_output.append("return ");
        p_output.append(getter->proxy_name + "(");
        if (p_iprop.index != -1) {
            const ArgumentInterface &idx_arg = getter->arguments.front();
            if (idx_arg.type.cname != name_cache->type_int) {
                // Assume the index parameter is an enum
                const TypeInterface *idx_arg_type = rd._get_type_or_null(idx_arg.type);
                CRASH_COND(idx_arg_type == nullptr);
                p_output.append("(" + idx_arg_type->proxy_name + ")" + itos(p_iprop.index));
            } else {
                p_output.append(itos(p_iprop.index));
            }
        }
        p_output.append(");\n"

                CLOSE_BLOCK_L3

                        // TODO Remove this once we make accessor methods private/internal (they will no longer be marked as obsolete after that)
                        "#pragma warning restore CS0618\n");
    }

    if (setter) {
        p_output.append(INDENT3 "set\n"

                                // TODO Remove this once we make accessor methods private/internal (they will no longer be marked as obsolete after that)
                                "#pragma warning disable CS0618 // Disable warning about obsolete method\n"

                OPEN_BLOCK_L3);

        p_output.append(setter->proxy_name + "(");
        if (p_iprop.index != -1) {
            const ArgumentInterface &idx_arg = setter->arguments.front();
            if (idx_arg.type.cname != name_cache->type_int) {
                // Assume the index parameter is an enum
                const TypeInterface *idx_arg_type = rd._get_type_or_null(idx_arg.type);
                CRASH_COND(idx_arg_type == NULL);
                p_output.append("(" + idx_arg_type->proxy_name + ")" + itos(p_iprop.index) + ", ");
            } else {
                p_output.append(itos(p_iprop.index) + ", ");
            }
        }
        p_output.append("value);\n"

                CLOSE_BLOCK_L3

                        // TODO Remove this once we make accessor methods private/internal (they will no longer be marked as obsolete after that)
                        "#pragma warning restore CS0618\n");
    }

    p_output.append(CLOSE_BLOCK_L2);

    return OK;
}

Error BindingsGenerator::_generate_cs_method(const TypeInterface &p_itype, const MethodInterface &p_imethod,
        int &p_method_bind_count, StringBuilder &p_output) {

    const TypeInterface *return_type = rd._get_type_or_placeholder(p_imethod.return_type);

    String arguments_sig;
    String cs_in_statements;

    String icall_params;
    icall_params += sformat(p_itype.cs_in, "this");

    StringBuilder default_args_doc;

    // Retrieve information from the arguments
    for (const ArgumentInterface &iarg : p_imethod.arguments) {

        const TypeInterface *arg_type = rd._get_type_or_placeholder(iarg.type);

        // Add the current arguments to the signature
        // If the argument has a default value which is not a constant, we will make it Nullable
        {
            if (&iarg != &p_imethod.arguments.front())
                arguments_sig += ", ";

            if (iarg.def_param_mode == ArgumentInterface::NULLABLE_VAL)
                arguments_sig += "Nullable<";

            arguments_sig += arg_type->cs_type;

            if (iarg.def_param_mode == ArgumentInterface::NULLABLE_VAL)
                arguments_sig += "> ";
            else
                arguments_sig += " ";

            arguments_sig += iarg.name;

            if (!iarg.default_argument.empty()) {
                if (iarg.def_param_mode != ArgumentInterface::CONSTANT)
                    arguments_sig += " = null";
                else
                    arguments_sig += " = " + sformat(iarg.default_argument, arg_type->cs_type);
            }
        }

        icall_params += ", ";

        if (iarg.default_argument.size() && iarg.def_param_mode != ArgumentInterface::CONSTANT) {
            // The default value of an argument must be constant. Otherwise we make it Nullable and do the following:
            // Type arg_in = arg.HasValue ? arg.Value : <non-const default value>;
            String arg_in = iarg.name;
            arg_in += "_in";

            cs_in_statements += arg_type->cs_type;
            cs_in_statements += " ";
            cs_in_statements += arg_in;
            cs_in_statements += " = ";
            cs_in_statements += iarg.name;

            if (iarg.def_param_mode == ArgumentInterface::NULLABLE_VAL)
                cs_in_statements += ".HasValue ? ";
            else
                cs_in_statements += " != null ? ";

            cs_in_statements += iarg.name;

            if (iarg.def_param_mode == ArgumentInterface::NULLABLE_VAL)
                cs_in_statements += ".Value : ";
            else
                cs_in_statements += " : ";

            String def_arg = sformat(iarg.default_argument, arg_type->cs_type);

            cs_in_statements += def_arg;
            cs_in_statements += ";\n" INDENT3;

            icall_params += arg_type->cs_in.empty() ? arg_in : sformat(arg_type->cs_in, arg_in);

            // Apparently the name attribute must not include the @
            String param_tag_name = iarg.name.starts_with("@") ? iarg.name.substr(1, iarg.name.length()) : iarg.name;

            default_args_doc.append(INDENT2 "/// <param name=\"" + param_tag_name + "\">If the parameter is null, then the default value is " + def_arg + "</param>\n");
        } else {
            icall_params += arg_type->cs_in.empty() ? iarg.name : sformat(arg_type->cs_in, iarg.name);
        }
    }

    // Generate method
    {
        auto method_doc = rd.doc_lookup_helpers[p_itype.proxy_name].methods.at(String(p_imethod.cname));
        if (method_doc && method_doc->description.size()) {
            String xml_summary = bbcode_to_xml(fix_doc_description(method_doc->description), &p_itype,rd.doc);
            Vector<String> summary_lines = xml_summary.length() ? xml_summary.split('\n') : Vector<String>();

            if (summary_lines.size() || default_args_doc.get_string_length()) {
                p_output.append(MEMBER_BEGIN "/// <summary>\n");

                for (int i = 0; i < summary_lines.size(); i++) {
                    p_output.append(INDENT2 "/// ");
                    p_output.append(summary_lines[i]);
                    p_output.append("\n");
                }

                p_output.append(default_args_doc.as_string());
                p_output.append(INDENT2 "/// </summary>");
            }
        }

        if (!p_imethod.is_internal) {
            p_output.append(MEMBER_BEGIN "[GodotMethod(\"");
            p_output.append(p_imethod.name);
            p_output.append("\")]");
        }

        if (p_imethod.is_deprecated) {
            if (p_imethod.deprecation_message.empty()) {
                WARN_PRINT("An empty deprecation message is discouraged. Method: '" + p_imethod.proxy_name + "'.");
            }

            p_output.append(MEMBER_BEGIN "[Obsolete(\"");
            p_output.append(p_imethod.deprecation_message);
            p_output.append("\")]");
        }

        p_output.append(MEMBER_BEGIN);
        p_output.append(p_imethod.is_internal ? "internal " : "public ");

        if (p_itype.is_singleton) {
            p_output.append("static ");
        } else if (p_imethod.is_virtual) {
            p_output.append("virtual ");
        }

        p_output.append(return_type->cs_type + " ");
        p_output.append(p_imethod.proxy_name + "(");
        p_output.append(arguments_sig + ")\n" OPEN_BLOCK_L2);

        if (p_imethod.is_virtual) {
            // Godot virtual method must be overridden, therefore we return a default value by default.

            if (return_type->cname == name_cache->type_void) {
                p_output.append("return;\n" CLOSE_BLOCK_L2);
            } else {
                p_output.append("return default(");
                p_output.append(return_type->cs_type);
                p_output.append(");\n" CLOSE_BLOCK_L2);
            }

            return OK; // Won't increment method bind count
        }

        if (p_imethod.requires_object_call) {
            // Fallback to Godot's object.Call(string, params)

            p_output.append(CS_METHOD_CALL "(\"");
            p_output.append(p_imethod.name);
            p_output.append("\"");

            for (const ArgumentInterface &F : p_imethod.arguments) {
                p_output.append(", ");
                p_output.append(F.name);
            }

            p_output.append(");\n" CLOSE_BLOCK_L2);

            return OK; // Won't increment method bind count
        }

        const Map<const MethodInterface *, const InternalCall *>::iterator match = method_icalls_map.find(&p_imethod);
        ERR_FAIL_COND_V(match==method_icalls_map.end(), ERR_BUG);

        const InternalCall *im_icall = match->second;

        String im_call = im_icall->editor_only ? BINDINGS_CLASS_NATIVECALLS_EDITOR : BINDINGS_CLASS_NATIVECALLS;
        im_call += ".";
        im_call += im_icall->name;

        if (!p_imethod.arguments.empty())
            p_output.append(cs_in_statements);

        if (return_type->cname == name_cache->type_void) {
            p_output.append(im_call + "(" + icall_params + ");\n");
        } else if (return_type->cs_out.empty()) {
            p_output.append("return " + im_call + "(" + icall_params + ");\n");
        } else {
            p_output.append(sformat(return_type->cs_out, im_call, icall_params, return_type->cs_type, return_type->im_type_out));
            p_output.append("\n");
        }

        p_output.append(CLOSE_BLOCK_L2);
    }

    p_method_bind_count++;

    return OK;
}

Error BindingsGenerator::generate_glue(StringView p_output_dir,GeneratorContext &ctx) {

    ERR_FAIL_COND_V(!initialized, ERR_UNCONFIGURED);

    bool dir_exists = DirAccess::exists(p_output_dir);
    ERR_FAIL_COND_V_MSG(!dir_exists, ERR_FILE_BAD_PATH, "The output directory does not exist.");

    StringBuilder output;

    output.append("/* THIS FILE IS GENERATED DO NOT EDIT */\n");
    output.append("#include \"" GLUE_HEADER_FILE "\"\n");
    output.append("#include \"core/method_bind.h\"\n");
    output.append("#include \"core/pool_vector.h\"\n");
    output.append("\n#ifdef MONO_GLUE_ENABLED\n");

    eastl::unordered_set<String> used;
    for (const StringName& E : rd.obj_type_insert_order) {
        const TypeInterface& itype = rd.obj_types[E];
        if(used.contains(ClassDB::classes[itype.cname].usage_header))
            continue;
        used.insert(ClassDB::classes[itype.cname].usage_header);
        output.append("#include \""+ClassDB::classes[itype.cname].usage_header+"\"\n");

    }

    output.append(R"RAW(
struct AutoRef {
    Object *self;
    AutoRef(Object *s) : self(s) {}
    template<class T>
    operator Ref<T>() {
        return Ref<T>((T*)self);
    }
    operator RefPtr() {
        return Ref<RefCounted>((RefCounted*)self).get_ref_ptr();
    }
 };
struct ArrConverter {
    Array &a;
    constexpr ArrConverter(Array &v):a(v) {}
    constexpr ArrConverter(Array *v):a(*v) {}
    operator Array() const { return a; }
    template<class T>
    operator Vector<T>() const {
        Vector<T> res;
        res.reserve(a.size());
        for (const Variant& v : a.vals()) {
            res.emplace_back(v.as<T>());
        }
        return res;
    }
    template<class T>
    operator PoolVector<T>() const {
        PoolVector<T> res;
        for (const Variant& v : a.vals()) {
            res.push_back(v.as<T>());
        }
        return res;
    }
};
Array *ToArray(Array && v) {
    return memnew(Array(eastl::move(v)));
}
template<class T>
Array *ToArray(Vector<T> && v) {
    Array * res = memnew(Array());
    for(const T &val : v) {
        res->emplace_back(Variant::from(val));
    }
    return res;
}
template<>
Array* ToArray(Vector<SurfaceArrays>&& v) {
    Array* res = memnew(Array());
    for (const auto& val : v) {
        res->emplace_back(Array(val));
    }
    return res;
}

template<class T>
Array *ToArray(PoolVector<T> && v) {
    Array * res = memnew(Array());
    for(size_t idx=0,fin=v.size();idx<fin; ++idx) {
        res->emplace_back(Variant::from(v[idx]));
    }
    return res;
}
Array* ToArray(Frustum&& v) {
    Array* res = memnew(Array());
    for (const auto& val : v) {
        res->emplace_back(Variant::from(val));
    }
    return res;
}

Array* ToArray(SurfaceArrays&& v) {
    return memnew(Array(v));
}
    )RAW");
    generated_icall_funcs.clear();

    for (const StringName& E : rd.obj_type_insert_order) {
        const TypeInterface& itype = rd.obj_types[E];
        if(itype.is_namespace)
            continue;

        bool is_derived_type = itype.base_name != StringName();

        if (!is_derived_type) {
            // Some Object assertions
            CRASH_COND(itype.cname != name_cache->type_Object);
            CRASH_COND(!itype.is_instantiable);
            CRASH_COND(itype.api_type != ClassDB::API_CORE);
            CRASH_COND(itype.is_reference);
            CRASH_COND(itype.is_singleton);
        }

        List<InternalCall> &custom_icalls = ctx.custom_icalls;

        OS::get_singleton()->print(FormatVE("Generating %s...\n", itype.name.c_str()));

        String ctor_method(ICALL_PREFIX + itype.proxy_name + "_Ctor"); // Used only for derived types

        for (const MethodInterface &imethod : itype.methods) {

            Error method_err = _generate_glue_method(itype, imethod, output);
            ERR_FAIL_COND_V_MSG(method_err != OK, method_err,
                    "Failed to generate method '" + imethod.name + "' for class '" + itype.name + "'.");
        }

        if (itype.is_singleton) {
            String singleton_icall_name = ICALL_PREFIX + itype.name + SINGLETON_ICALL_SUFFIX;
            InternalCall singleton_icall = InternalCall(itype.api_type, singleton_icall_name, "IntPtr");

            if (!has_named_icall(singleton_icall.name, custom_icalls))
                custom_icalls.push_back(singleton_icall);

            output.append("Object* ");
            output.append(singleton_icall_name);
            output.append("() " OPEN_BLOCK "\treturn Engine::get_singleton()->get_named_singleton(\"");
            output.append(itype.proxy_name);
            output.append("\");\n" CLOSE_BLOCK "\n");
        }

        if (is_derived_type && itype.is_instantiable) {
            InternalCall ctor_icall = InternalCall(itype.api_type, ctor_method, "IntPtr", String(itype.proxy_name) + " obj");

            if (!has_named_icall(ctor_icall.name, custom_icalls))
                custom_icalls.push_back(ctor_icall);

            output.append("Object* ");
            output.append(ctor_method);
            output.append("(MonoObject* obj) " OPEN_BLOCK
                          "\t" C_MACRO_OBJECT_CONSTRUCT "(instance, \"");
            output.append(itype.name);
            output.append("\")\n"
                          "\t" C_METHOD_TIE_MANAGED_TO_UNMANAGED "(obj, instance);\n"
                          "\treturn instance;\n" CLOSE_BLOCK "\n");
        }
    }

    output.append("namespace GodotSharpBindings\n" OPEN_BLOCK "\n");

    output.append("uint64_t get_api_hash() { return ");
    output.append(StringUtils::num_uint64(ctx.api_hash) + "U; }\n");

//    output.append("#ifdef TOOLS_ENABLED\n"
//                  "uint64_t get_editor_api_hash() { return ");
//    output.append(StringUtils::num_uint64(GDMono::get_singleton()->get_api_editor_hash()) + "U; }\n");
//    output.append("#endif // TOOLS_ENABLED\n");

    output.append("uint32_t get_bindings_version() { return ");
    output.append(StringUtils::num_uint64(BINDINGS_GENERATOR_VERSION) + "; }\n");

    output.append("uint32_t get_cs_glue_version() { return ");
    output.append(StringUtils::num_uint64(ctx.cs_side_hash) + "; }\n");
    output.append("namespace {\n // anonymous namespace\n");
    output.append("struct FuncReg { const char *name; const void *ptr; };\n");
    output.append("static const FuncReg functions[]={\n");
#define ADD_INTERNAL_CALL_REGISTRATION(m_icall)                                                              \
    {                                                                                                        \
        output.append("\t{");                                                          \
        output.append("\"" BINDINGS_NAMESPACE ".");                                                          \
        output.append(m_icall.editor_only ? BINDINGS_CLASS_NATIVECALLS_EDITOR : BINDINGS_CLASS_NATIVECALLS); \
        output.append(String("::")+m_icall.name+"\", (void*)"+m_icall.name+"},\n");\
    }

    bool tools_sequence = false;
    for (const InternalCall &E : ctx.custom_icalls) {

        if (tools_sequence) {
            if (!E.editor_only) {
                tools_sequence = false;
                output.append("#endif\n");
            }
        }
        else {
            if (E.editor_only) {
                output.append("#ifdef TOOLS_ENABLED\n");
                tools_sequence = true;
            }
        }
        ADD_INTERNAL_CALL_REGISTRATION(E)
    }
    if (tools_sequence) {
        tools_sequence = false;
        output.append("#endif\n");
    }
    output.append("#ifdef TOOLS_ENABLED\n");
    for (const InternalCall &E : ctx.custom_icalls)
        ADD_INTERNAL_CALL_REGISTRATION(E)
    output.append("#endif // TOOLS_ENABLED\n");

    auto keys=method_icalls.keys();
    eastl::sort(keys.begin(),keys.end());
    for (const auto &E : keys) {
        const auto & entry(method_icalls[E]);

        if (tools_sequence) {
            if (!entry.editor_only) {
                tools_sequence = false;
                output.append("#endif\n");
            }
        }
        else {
            if (entry.editor_only) {
                output.append("#ifdef TOOLS_ENABLED\n");
                tools_sequence = true;
            }
        }

        ADD_INTERNAL_CALL_REGISTRATION(entry)
    }

    if (tools_sequence) {
        tools_sequence = false;
        output.append("#endif\n");
    }
    output.append("};\n} // end of anonymous namespace\n");
#undef ADD_INTERNAL_CALL_REGISTRATION

    output.append(R"(
void register_generated_icalls() {
    godot_register_glue_header_icalls();
    for(const auto & f : functions)
        mono_add_internal_call(f.name, (void*)f.ptr);
}
    )");

    output.append("\n} // namespace GodotSharpBindings\n");

    output.append("\n#endif // MONO_GLUE_ENABLED\n");

    Error save_err = _save_file(path::join(p_output_dir, "mono_glue.gen.cpp"), output);
    if (save_err != OK)
        return save_err;

    OS::get_singleton()->print("Mono glue generated successfully\n");

    return OK;
}

uint32_t BindingsGenerator::get_version() {
    return BINDINGS_GENERATOR_VERSION;
}

static Error _save_file(StringView p_path, const StringBuilder &p_content) {

    FileAccessRef file = FileAccess::open(p_path, FileAccess::WRITE);

    ERR_FAIL_COND_V_MSG(!file, ERR_FILE_CANT_WRITE, "Cannot open file: '" + p_path + "'.");

    file->store_string(p_content.as_string());
    file->close();

    return OK;
}
static StringView replace_method_name(StringView from) {
    StringView res = from;
    static const HashMap<StringView, StringView> s_entries = {
        { "_get_slide_collision", "get_slide_collision" },
        { "_set_import_path", "set_import_path" },
        { "add_do_method", "_add_do_method" },
        { "add_property_info", "_add_property_info_bind" },
        { "add_surface_from_arrays", "_add_surface_from_arrays" },
        { "add_undo_method", "_add_undo_method" },
        { "body_test_motion", "_body_test_motion" },
        { "call_recursive", "_call_recursive_bind" },
        { "class_get_category", "get_category" },
        { "class_get_integer_constant", "get_integer_constant" },
        { "class_get_integer_constant_list", "get_integer_constant_list" },
        { "class_get_method_list", "get_method_list" },
        { "class_get_property", "get_property" },
        { "class_get_property_list", "get_property_list" },
        { "class_get_signal", "get_signal" },
        { "class_get_signal_list", "get_signal_list" },
        { "class_has_integer_constant", "has_integer_constant" },
        { "class_has_method", "has_method" },
        { "class_has_signal", "has_signal" },
        { "class_set_property", "set_property" },
        { "copy_from", "copy_internals_from" },
        { "create_from_data", "_create_from_data" },
        { "get_action_list", "_get_action_list" },
        { "get_connection_list", "_get_connection_list" },
        { "get_groups", "_get_groups" },
        { "get_item_area_rect","_get_item_rect"},
        { "get_item_shapes", "_get_item_shapes" },
        { "get_local_addresses", "_get_local_addresses" },
        { "get_local_interfaces", "_get_local_interfaces" },
        { "get_named_attribute_value", "get_attribute_value" },
        { "get_named_attribute_value_safe", "get_attribute_value_safe" },
        { "get_next_selected","_get_next_selected"},
        { "get_node_and_resource", "_get_node_and_resource" },
        { "get_node_connections", "_get_node_connections" },
        { "get_range_config","_get_range_config"},
        { "get_response_headers", "_get_response_headers" },
        { "get_shape_owners", "_get_shape_owners" },
        { "get_slide_collision", "_get_slide_collision" },
        { "get_tiles_ids","_get_tiles_ids"},
        { "get_transformable_selected_nodes", "_get_transformable_selected_nodes" },
        { "make_mesh_previews", "_make_mesh_previews" },
        { "move_and_collide", "_move" },
        { "move_local_x", "move_x" },
        { "move_local_y", "move_y" },
        { "new", "_new" },
        { "open_encrypted_with_pass", "open_encrypted_pass" },
        { "queue_free", "queue_delete" },
        { "rpc", "_rpc_bind" },
        { "rpc_id", "_rpc_id_bind" },
        { "rpc_unreliable", "_rpc_unreliable_bind" },
        { "rpc_unreliable_id", "_rpc_unreliable_id_bind" },
        { "set_item_shapes", "_set_item_shapes" },
        { "set_navigation", "set_navigation_node" },
        { "set_target", "_set_target" },
        { "set_variable_info","_set_variable_info"},
        { "surface_get_blend_shape_arrays", "_surface_get_blend_shape_arrays" },
        { "take_over_path", "set_path" },
        {"_get_gizmo_extents","get_gizmo_extents"},
        {"_set_gizmo_extents","set_gizmo_extents"},
        {"add_user_signal","_add_user_signal"},
        {"call","_call_bind"},
        {"call_deferred","_call_deferred_bind"},
        {"call_group_flags","_call_group_flags"},
        {"cast_motion","_cast_motion"},
        {"collide_shape","_collide_shape"},
        {"emit_signal","_emit_signal"},
        {"force_draw","draw"},
        {"force_sync","sync"},
        {"get_bound_child_nodes_to_bone","_get_bound_child_nodes_to_bone"},
        {"get_breakpoints","get_breakpoints_array"},
        {"get_color_list","_get_color_list"},
        {"get_constant_list","_get_constant_list"},
        {"get_current_script","_get_current_script"},
        {"get_default_font","get_default_theme_font"},
        {"get_expand_margin","get_expand_margin_size"},
        {"get_font_list","_get_font_list"},
        {"get_icon_list","_get_icon_list"},
        {"get_incoming_connections","_get_incoming_connections"},
        {"get_indexed","_get_indexed_bind"},
        {"get_message_list","_get_message_list"},
        {"get_meta_list","_get_meta_list_bind"},
        {"get_method_list","_get_method_list_bind"},
        {"get_open_scripts","_get_open_scripts"},
        {"get_packet","_get_packet"},
        {"get_packet_error","_get_packet_error"},
        {"get_packet_ip","_get_packet_ip"},
        {"get_partial_data","_get_partial_data"},
        {"get_property_list","_get_property_list_bind"},
        {"get_property_default_value","_get_property_default_value"},
        {"get_resource_list","_get_resource_list"},
        {"get_rest_info","_get_rest_info"},
        //{"get_scancode_with_modifiers","get_keycode_with_modifiers"},
        {"get_script_method_list","_get_script_method_list"},
        {"get_script_signal_list","_get_script_signal_list"},
        {"get_script_property_list","_get_script_property_list"},
        {"get_signal_connection_list","_get_signal_connection_list"},
        {"get_script_constant_map","_get_script_constant_map"},
        {"get_signal_list","_get_signal_list"},
        {"get_stylebox_list","_get_stylebox_list"},
        {"get_type_list","_get_type_list"},
        {"has_user_signal","_has_user_signal"},
        {"instances_cull_convex","_instances_cull_convex_bind"},
        {"intersect_point","_intersect_point"},
        {"intersect_point_on_canvas","_intersect_point_on_canvas"},
        {"intersect_ray","_intersect_ray"},
        {"intersect_shape","_intersect_shape"},
        {"is_hide_on_state_item_selection","is_hide_on_multistate_item_selection"},
        {"listen","_listen"},
        {"load_resource_pack","_load_resource_pack"},
        {"mesh_add_surface_from_arrays","_mesh_add_surface_from_arrays"},
        {"newline","add_newline"},
        {"physical_bones_start_simulation","physical_bones_start_simulation_on"},
        {"put_data","_put_data"},
        {"put_packet","_put_packet"},
        {"put_partial_data","_put_partial_data"},
        {"set_dest_address","_set_dest_address"},
        {"set_expand_margin","set_expand_margin_size"},
        {"set_expand_margin_all","set_expand_margin_size_all"},
        {"set_expand_margin_individual","set_expand_margin_size_individual"},
        {"set_hide_on_state_item_selection","set_hide_on_multistate_item_selection"},
        {"set_indexed","_set_indexed_bind"},
        {"shader_get_param_list","_shader_get_param_list_bind"},
        {"share","_share"},
        {"test_motion","_test_motion"},
        {"texture_debug_usage","_texture_debug_usage_bind"},
        {"tile_set_shapes","_tile_set_shapes"},

        {"call_group","_call_group"},
        {"get_nodes_in_group","_get_nodes_in_group"},
        {"tile_get_shapes","_tile_get_shapes"},
        {"_set_editor_description","set_editor_description"},
        { "_get_editor_description","get_editor_description" },
    };
    auto iter = s_entries.find(from);
    if (iter != s_entries.end()) return iter->second;
    return res;
}

Error BindingsGenerator::_generate_glue_method(
        const TypeInterface &p_itype, const MethodInterface &p_imethod, StringBuilder &p_output) {

    if (p_imethod.is_virtual)
        return OK; // Ignore

    if (p_itype.cname == name_cache->type_Object && p_imethod.name == "free")
        return OK;

    bool ret_void = p_imethod.return_type.cname == name_cache->type_void;

    const TypeInterface *return_type = rd._get_type_or_placeholder(p_imethod.return_type);
    String argc_str = itos(p_imethod.arguments.size());
    StringView no_star=StringView(p_itype.c_type_in).substr(0,p_itype.c_type_in.size()-1);
    String class_type(p_itype.c_type_in.ends_with('*') ? no_star : p_itype.c_type_in);
    String c_func_sig = p_itype.c_type_in + " " CS_PARAM_INSTANCE;
    String c_in_statements;
    String c_args_var_content;
    // Get arguments information

    int i = 0;
    for (const ArgumentInterface &iarg : p_imethod.arguments) {
        const TypeInterface *arg_type = rd._get_type_or_placeholder(iarg.type);
        String c_param_name = "arg" + itos(i + 1);
        if (p_imethod.is_vararg) {
            if (i < p_imethod.arguments.size() - 1) {
                c_in_statements += sformat(!arg_type->c_in.empty() ? arg_type->c_in : TypeInterface::DEFAULT_VARARG_C_IN, "Variant", c_param_name);
                c_in_statements += "\t" C_LOCAL_PTRCALL_ARGS "[";
                c_in_statements += itos(i);
                c_in_statements += sformat("] =&%s_in;\n", c_param_name);
            }
        } else {
            if (i > 0)
                c_args_var_content += ", ";
            if (!arg_type->c_in.empty()) {
                c_in_statements += sformat(arg_type->c_in, arg_type->c_type, c_param_name);
            }

            if(arg_type->is_reference)
                c_args_var_content += FormatVE("AutoRef(%s)",c_param_name.c_str());
            else if(arg_type->is_enum) {
                // add enum cast
                StringView enum_name(arg_type->name);
                if(enum_name.ends_with("Enum"))
                    enum_name = enum_name.substr(0, enum_name.size()-4);
                String cast_as(enum_name);
                c_args_var_content += "(" +cast_as.replaced(".","::")+")";
                c_args_var_content += sformat(arg_type->c_arg_in, c_param_name);
            }
            else if(!arg_type->c_in.empty()) // Provided de-marshalling code was used.
            {
                if (iarg.type.pass_by==TypePassBy::Move) // but type is passed by move
                    c_args_var_content += "eastl::move("+sformat(arg_type->c_arg_in, c_param_name)+")";
                else
                    c_args_var_content += sformat(arg_type->c_arg_in, c_param_name);
            }
            else {
                switch(iarg.type.pass_by) {
                case TypePassBy::Value:
                    if(arg_type->c_type_in.ends_with('*') && arg_type->cname!=StringView("Array")) // input as pointer, deref, unless Array which gets handled by ArrConverter
                        c_args_var_content.push_back('*');
                    c_args_var_content.append(sformat(arg_type->c_arg_in, c_param_name));
                    break;
                case TypePassBy::Reference:
                    if(arg_type->cname != StringView("Array"))
                        c_args_var_content.push_back('*');
                    c_args_var_content += sformat(arg_type->c_arg_in, c_param_name);
                    break;
                case TypePassBy::Move:
                    c_args_var_content += "eastl::move(*" + sformat(arg_type->c_arg_in, c_param_name) +")";
                    break;
                case TypePassBy::Pointer:
                    c_args_var_content += "("+String(arg_type->cname)+"*)";
                    c_args_var_content += sformat(arg_type->c_arg_in, c_param_name);
                    break;
                default:
                    c_args_var_content += sformat(arg_type->c_arg_in, c_param_name);
                }

            }
        }

        c_func_sig += ", ";
        c_func_sig += arg_type->c_type_in;
        //special case for NodePath

        c_func_sig += " ";
        c_func_sig += c_param_name;

        i++;
    }

    //TODO: generate code that checks that p_itype.cname.asCString() is a class inheriting from class_type

    if (return_type->ret_as_byref_arg) {
        c_func_sig += ", ";
        c_func_sig += return_type->c_type_in;
        c_func_sig += " ";
        c_func_sig += "arg_ret";

        i++;
    }

    Map<const MethodInterface *, const InternalCall *>::const_iterator match = method_icalls_map.find(&p_imethod);
    ERR_FAIL_COND_V(match==method_icalls_map.end(), ERR_BUG);

    const InternalCall *im_icall = match->second;
    String icall_method = im_icall->name;
    if (generated_icall_funcs.contains(im_icall))
        return OK;

    generated_icall_funcs.push_back(im_icall);

    if (im_icall->editor_only)
        p_output.append("#ifdef TOOLS_ENABLED\n");

    // Generate icall function

    p_output.append((ret_void || return_type->ret_as_byref_arg) ? "void " : return_type->c_type_out + " ");
    p_output.append(icall_method);
    p_output.append("(");
    p_output.append(c_func_sig);
    p_output.append(") " OPEN_BLOCK);

    if (!ret_void) {
        String ptrcall_return_type;
        String initialization;

        if (p_imethod.is_vararg && return_type->cname != name_cache->type_Variant) {
            // VarArg methods always return Variant, but there are some cases in which MethodInfo provides
            // a specific return type. We trust this information is valid. We need a temporary local to keep
            // the Variant alive until the method returns. Otherwise, if the returned Variant holds a RefPtr,
            // it could be deleted too early. This is the case with GDScript.new() which returns OBJECT.
            // Alternatively, we could just return Variant, but that would result in a worse API.
            p_output.append("\tVariant " C_LOCAL_VARARG_RET ";\n");
        }

        String fail_ret = return_type->c_type_out.ends_with("*") && !return_type->ret_as_byref_arg ? "NULL" : return_type->c_type_out + "()";

        if (return_type->ret_as_byref_arg) {
            p_output.append("\tif (" CS_PARAM_INSTANCE " == nullptr) { *arg_ret = ");
            p_output.append(fail_ret);
            p_output.append("; ERR_FAIL_MSG(\"Parameter ' arg_ret ' is null.\"); }\n");
        } else {
            p_output.append("\tERR_FAIL_NULL_V(" CS_PARAM_INSTANCE ", ");
            p_output.append(fail_ret);
            p_output.append(");\n");
        }
    } else {
        p_output.append("\tERR_FAIL_NULL(" CS_PARAM_INSTANCE ");\n");
    }

    if (!p_imethod.arguments.empty()) {
        if (p_imethod.is_vararg) {
            String vararg_arg = "arg" + argc_str;
            String real_argc_str = itos(p_imethod.arguments.size() - 1); // Arguments count without vararg

            p_output.append("\tint vararg_length = mono_array_length(");
            p_output.append(vararg_arg);
            p_output.append(");\n\tint total_length = ");
            p_output.append(real_argc_str);
            p_output.append(" + vararg_length;\n"
                            "\tArgumentsVector<Variant> varargs(vararg_length);\n"
                            "\tArgumentsVector<const Variant *> " C_LOCAL_PTRCALL_ARGS "(total_length);\n");
            p_output.append(c_in_statements);
            p_output.append("\tfor (int i = 0; i < vararg_length; i++) " OPEN_BLOCK
                            "\t\tMonoObject* elem = mono_array_get(");
            p_output.append(vararg_arg);
            p_output.append(", MonoObject*, i);\n"
                            "\t\tvarargs[i]= GDMonoMarshal::mono_object_to_variant(elem);\n"
                            "\t\t" C_LOCAL_PTRCALL_ARGS "[");
            p_output.append(real_argc_str);
            p_output.append(" + i] = &varargs[i];\n\t" CLOSE_BLOCK);
        } else {
            p_output.append(c_in_statements);
        }
    }

    StringView method_to_call(replace_method_name(p_imethod.cname));
    if(p_itype.cname=="Node") {
        if(method_to_call== "get_children")
           method_to_call = "_get_children";
    }
    else if(p_itype.cname=="PacketPeer") {
        if(method_to_call== "get_var")
            method_to_call = "_bnd_get_var";
    }
    else if(p_itype.cname=="TextEdit") {
        if(method_to_call== "search")
            method_to_call = "_search_bind";
    }
    else if(p_itype.cname=="StreamPeer") {
        if(method_to_call== "get_data")
            method_to_call = "_get_data";
    }
    else if(p_itype.cname=="ScriptEditor") {
        if(method_to_call== "goto_line")
            method_to_call = "_goto_script_line2";
    }
    else if(p_itype.cname=="WebSocketServer") {
        //sigh, udp and tcp servers `_listen` but WebSocketServer `listen`s
        if(method_to_call== "_listen")
            method_to_call = "listen";
    }
    else if(p_itype.cname=="Tree") {
        if(method_to_call== "create_item")
            method_to_call = "_create_item";

    }
    else if(p_itype.cname=="StreamPeerTCP") {
        if(method_to_call== "connect_to_host")
            method_to_call = "_connect";

    }

    if (p_imethod.is_vararg) {
        p_output.append("\tCallable::CallError vcall_error;\n\t");

        if (!ret_void) {
            // See the comment on the C_LOCAL_VARARG_RET declaration
            if (return_type->cname != name_cache->type_Variant) {
                p_output.append(C_LOCAL_VARARG_RET " = ");
            } else {
                p_output.append("auto " C_LOCAL_RET " = ");
            }
        }
        p_output.append(FormatVE("static_cast<%s *>(" CS_PARAM_INSTANCE ")->%.*s(", p_itype.cname.asCString(), method_to_call.length(),method_to_call.data()));
        p_output.append(!p_imethod.arguments.empty() ? C_LOCAL_PTRCALL_ARGS ".data()" : "nullptr");
        p_output.append(", total_length, vcall_error);\n");

        // See the comment on the C_LOCAL_VARARG_RET declaration
        if (!ret_void) {
            if (return_type->cname != name_cache->type_Variant) {
                p_output.append("\tauto " C_LOCAL_RET " = " C_LOCAL_VARARG_RET ";\n");
            }
        }
    } else {
        p_output.append("\t");
        if(!ret_void)
            p_output.append("auto " C_LOCAL_RET " = ");
        p_output.append(FormatVE("static_cast<%s *>(" CS_PARAM_INSTANCE ")->%s(", p_itype.cname.asCString(),method_to_call.data()));
        p_output.append(p_imethod.arguments.empty() ? "" : c_args_var_content);
        p_output.append(");\n");
    }

    if (!ret_void) {
        if (return_type->c_out.empty()) {
            p_output.append("\treturn " C_LOCAL_RET ";\n");
        } else if (return_type->ret_as_byref_arg) {
            p_output.append(sformat(return_type->c_out, return_type->c_type_out, C_LOCAL_RET, return_type->name, "arg_ret"));
        } else {
            p_output.append(sformat(return_type->c_out, return_type->c_type_out, C_LOCAL_RET, return_type->name));
        }
    }

    p_output.append(CLOSE_BLOCK "\n");

    if (im_icall->editor_only)
        p_output.append("#endif // TOOLS_ENABLED\n");

    return OK;
}



StringName _get_int_type_name_from_meta(GodotTypeInfo::Metadata p_meta) {

    switch (p_meta) {
        case GodotTypeInfo::METADATA_INT_IS_INT8:
            return "sbyte";
        case GodotTypeInfo::METADATA_INT_IS_INT16:
            return "short";
        case GodotTypeInfo::METADATA_INT_IS_INT32:
            return "int";
        case GodotTypeInfo::METADATA_INT_IS_INT64:
            return "long";
        case GodotTypeInfo::METADATA_INT_IS_UINT8:
            return "byte";
        case GodotTypeInfo::METADATA_INT_IS_UINT16:
            return "ushort";
        case GodotTypeInfo::METADATA_INT_IS_UINT32:
            return "uint";
        case GodotTypeInfo::METADATA_INT_IS_UINT64:
            return "ulong";
        default:
            // Assume INT32
            return "int";
    }
}
static StringName _get_string_type_name_from_meta(GodotTypeInfo::Metadata p_meta) {

    switch (p_meta) {
    case GodotTypeInfo::METADATA_STRING_NAME:
        return "StringName";
    case GodotTypeInfo::METADATA_STRING_VIEW:
        return "StringView";
    default:
        // Assume default String type
        return StringName("String");
    }
}
static StringName _get_variant_type_name_from_meta(VariantType tp,GodotTypeInfo::Metadata p_meta) {
    if(GodotTypeInfo::METADATA_NON_COW_CONTAINER==p_meta) {
        switch(tp) {

            case VariantType::POOL_BYTE_ARRAY:
                return StringName("VecByte");

            case VariantType::POOL_INT_ARRAY:
                return StringName("VecInt");
            case VariantType::POOL_REAL_ARRAY:
                return StringName("VecFloat");
            case VariantType::POOL_STRING_ARRAY:
                return StringName("VecString");
            case VariantType::POOL_VECTOR2_ARRAY:
                return StringName("VecVector2");
            case VariantType::POOL_VECTOR3_ARRAY:
                return StringName("VecVector3");

            case VariantType::POOL_COLOR_ARRAY:
                return StringName("VecColor");
            default: ;
        }
    }
    return Variant::interned_type_name(tp);
}
static StringName _get_float_type_name_from_meta(GodotTypeInfo::Metadata p_meta) {

    switch (p_meta) {
        case GodotTypeInfo::METADATA_REAL_IS_FLOAT:
            return "float";
        case GodotTypeInfo::METADATA_REAL_IS_DOUBLE:
            return "double";
        default:
            // Assume real_t (float or double depending of REAL_T_IS_DOUBLE)
#ifdef REAL_T_IS_DOUBLE
            return "double";
#else
            return "float";
#endif
    }
}

bool BindingsGenerator::_populate_object_type_interfaces() {

    rd.obj_types.clear();
    rd.obj_type_insert_order.clear();

    Vector<StringName> class_list;
    ClassDB::get_class_list(&class_list);
    eastl::sort(class_list.begin(),class_list.end(),WrapAlphaCompare());

    while (!class_list.empty()) {
        StringName type_cname = class_list.front();
        if(type_cname=="@") {
            class_list.pop_front();
            continue;
        }
        ClassDB::APIType api_type = ClassDB::get_api_type(type_cname);

        if (api_type == ClassDB::API_NONE) {
            class_list.pop_front();
            continue;
        }

        if (!ClassDB::is_class_exposed(type_cname)) {
            _log("Ignoring type '%s' because it's not exposed\n", String(type_cname).c_str());
            class_list.pop_front();
            continue;
        }

        if (!ClassDB::is_class_enabled(type_cname)) {
            _log("Ignoring type '%s' because it's not enabled\n", String(type_cname).c_str());
            class_list.pop_front();
            continue;
        }

        auto class_iter = ClassDB::classes.find(type_cname);

        TypeInterface itype = TypeInterface::create_object_type(type_cname, api_type);

        itype.base_name = ClassDB::get_parent_class(type_cname);
        itype.is_singleton = Engine::get_singleton()->has_singleton(itype.proxy_name);
        itype.is_instantiable = class_iter->second.creation_func && !itype.is_singleton;
        itype.is_reference = ClassDB::is_parent_class(type_cname, name_cache->type_Reference);
        itype.memory_own = itype.is_reference;
        itype.is_namespace = class_iter->second.is_namespace;

        itype.c_out = "\treturn ";
        itype.c_out += C_METHOD_UNMANAGED_GET_MANAGED;
        itype.c_out += itype.is_reference ? "((Object *)%1.get());\n" : "((Object *)%1);\n";

        itype.cs_in = itype.is_singleton ? BINDINGS_PTR_FIELD : "Object." CS_SMETHOD_GETINSTANCE "(%0)";

        itype.c_type = "Object";
        itype.c_type_in = "Object *";
        itype.c_type_out = "MonoObject*";
        itype.cs_type = itype.proxy_name;
        itype.im_type_in = "IntPtr";
        itype.im_type_out = itype.proxy_name;

        // Populate properties
        Vector<PropertyInfo> property_list;
        ClassDB::get_property_list(type_cname, &property_list, true);

        Map<StringName, StringName> accessor_methods;

        for (const PropertyInfo &property : property_list) {

            if (property.usage & PROPERTY_USAGE_GROUP || property.usage & PROPERTY_USAGE_CATEGORY)
                continue;

            PropertyInterface iprop;
            iprop.cname = property.name;
            iprop.setter = ClassDB::get_property_setter(type_cname, iprop.cname);
            iprop.getter = ClassDB::get_property_getter(type_cname, iprop.cname);

            if (iprop.setter != StringName())
                accessor_methods[iprop.setter] = iprop.cname;
            if (iprop.getter != StringName())
                accessor_methods[iprop.getter] = iprop.cname;

            bool valid = false;
            iprop.index = ClassDB::get_property_index(type_cname, iprop.cname, &valid);
            ERR_FAIL_COND_V(!valid, false);

            iprop.proxy_name = escape_csharp_keyword(snake_to_pascal_case(iprop.cname));

            // Prevent the property and its enclosing type from sharing the same name
            if (iprop.proxy_name == itype.proxy_name) {
                _log("Name of property '%s' is ambiguous with the name of its enclosing class '%s'. Renaming property to '%s_'\n",
                        iprop.proxy_name.c_str(), itype.proxy_name.asCString(), iprop.proxy_name.c_str());

                iprop.proxy_name += "_";
            }

            iprop.proxy_name = iprop.proxy_name.replaced("/", "__"); // Some members have a slash...

            itype.properties.push_back(iprop);
        }

        // Populate methods

        Vector<MethodInfo> virtual_method_list;
        ClassDB::get_virtual_methods(type_cname, &virtual_method_list, true);

        Vector<MethodInfo> method_list;
        ClassDB::get_method_list(type_cname, &method_list, true);
        eastl::sort(method_list.begin(),method_list.end());
        for (const MethodInfo &method_info : method_list) {
            int argc = method_info.arguments.size();

            if (method_info.name.empty())
                continue;

            auto cname = method_info.name;

            if (blacklisted_methods.contains(itype.cname) && blacklisted_methods[itype.cname].contains(cname))
                continue;

            MethodInterface imethod { String(method_info.name) ,cname };

            if (method_info.flags & METHOD_FLAG_VIRTUAL)
                imethod.is_virtual = true;

            PropertyInfo return_info = method_info.return_val;

            MethodBind *m = imethod.is_virtual ? nullptr : ClassDB::get_method(type_cname, method_info.name);

            const Span<const GodotTypeInfo::Metadata> arg_meta(m? m->get_arguments_meta(): Span<const GodotTypeInfo::Metadata>());
            const Span<const TypePassBy> arg_pass(m? m->get_arguments_passing() : Span<const TypePassBy>());
            imethod.is_vararg = m && m->is_vararg();

            if (!m && !imethod.is_virtual) {
                ERR_FAIL_COND_V_MSG(!virtual_method_list.find(method_info), false,
                        "Missing MethodBind for non-virtual method: '" + itype.name + "." + imethod.name + "'.");

                // A virtual method without the virtual flag. This is a special case.

                // There is no method bind, so let's fallback to Godot's object.Call(string, params)
                imethod.requires_object_call = true;

                // The method Object.free is registered as a virtual method, but without the virtual flag.
                // This is because this method is not supposed to be overridden, but called.
                // We assume the return type is void.
                imethod.return_type.cname = name_cache->type_void;

                // Actually, more methods like this may be added in the future,
                // which could actually will return something different.
                // Let's put this to notify us if that ever happens.
                if (itype.cname != name_cache->type_Object || imethod.name != "free") {
                    WARN_PRINT("Notification: New unexpected virtual non-overridable method found."
                                " We only expected Object.free, but found '" +
                                itype.name + "." + imethod.name + "'.");
                }
            } else if (return_info.type == VariantType::INT && return_info.usage & PROPERTY_USAGE_CLASS_IS_ENUM) {
                imethod.return_type.cname = return_info.class_name;
                imethod.return_type.is_enum = true;
            } else if (!return_info.class_name.empty()) {
                imethod.return_type.cname = return_info.class_name;
                if (!imethod.is_virtual && ClassDB::is_parent_class(return_info.class_name, name_cache->type_Reference) && return_info.hint != PropertyHint::ResourceType) {
                    /* clang-format off */
                    ERR_PRINT("Return type is reference but hint is not 'PropertyHint::ResourceType'."
                            " Are you returning a reference type by pointer? Method: '" + itype.name + "." + imethod.name + "'.");
                    /* clang-format on */
                    ERR_FAIL_V(false);
                }
            } else if (return_info.hint == PropertyHint::ResourceType) {
                imethod.return_type.cname = StringName(return_info.hint_string);
            } else if (return_info.type == VariantType::NIL && return_info.usage & PROPERTY_USAGE_NIL_IS_VARIANT) {
                imethod.return_type.cname = name_cache->type_Variant;
            } else if (return_info.type == VariantType::NIL) {
                imethod.return_type.cname = name_cache->type_void;
            } else {
                if (return_info.type == VariantType::INT) {
                    imethod.return_type.cname = _get_int_type_name_from_meta(arg_meta.size()>0 ? arg_meta[0] : GodotTypeInfo::METADATA_NONE);
                } else if (return_info.type == VariantType::FLOAT) {
                    imethod.return_type.cname = _get_float_type_name_from_meta(arg_meta.size() > 0 ? arg_meta[0] : GodotTypeInfo::METADATA_NONE);
                } else {
                    imethod.return_type.cname = Variant::interned_type_name(return_info.type);
                }
            }

            for (int i = 0; i < argc; i++) {
                const PropertyInfo &arginfo = method_info.arguments[i];

                StringName orig_arg_name = arginfo.name;

                ArgumentInterface iarg;
                iarg.name = orig_arg_name;

                if (arginfo.type == VariantType::INT && arginfo.usage & PROPERTY_USAGE_CLASS_IS_ENUM) {
                    iarg.type.cname = arginfo.class_name;
                    iarg.type.is_enum = true;
                    iarg.type.pass_by = TypePassBy::Value;
                } else if (!arginfo.class_name.empty()) {
                    iarg.type.cname = arginfo.class_name;
                    iarg.type.pass_by = arg_pass.size() > (i + 1) ? arg_pass[i + 1] : TypePassBy::Reference;
                } else if (arginfo.hint == PropertyHint::ResourceType) {
                    iarg.type.cname = StringName(arginfo.hint_string);
                    iarg.type.pass_by = TypePassBy::Reference;
                } else if (arginfo.type == VariantType::NIL) {
                    iarg.type.cname = name_cache->type_Variant;
                    iarg.type.pass_by = arg_pass.size() > (i + 1) ? arg_pass[i + 1] : TypePassBy::Value;
                } else {
                    if (arginfo.type == VariantType::INT) {
                        iarg.type.cname = _get_int_type_name_from_meta(arg_meta.size() > (i+1) ? arg_meta[i+1] : GodotTypeInfo::METADATA_NONE);
                    } else if (arginfo.type == VariantType::FLOAT) {
                        iarg.type.cname = _get_float_type_name_from_meta(arg_meta.size() > (i + 1) ? arg_meta[i + 1] : GodotTypeInfo::METADATA_NONE);
                    } else if (arginfo.type == VariantType::STRING) {
                        iarg.type.cname = _get_string_type_name_from_meta(arg_meta.size() > (i + 1) ? arg_meta[i + 1] : GodotTypeInfo::METADATA_NONE);
                    } else {

                        iarg.type.cname = _get_variant_type_name_from_meta(arginfo.type, arg_meta.size() > (i + 1) ? arg_meta[i + 1] : GodotTypeInfo::METADATA_NONE);
                    }
                    iarg.type.pass_by = arg_pass.size() > (i + 1) ? arg_pass[i + 1] : TypePassBy::Value;
                }
                if(iarg.type.cname=="Object" && iarg.type.pass_by==TypePassBy::Value) {
                    // Fixup for virtual methods, since passing Object by value makes no sense.
                    iarg.type.pass_by = TypePassBy::Pointer;
                }
                iarg.name = escape_csharp_keyword(snake_to_camel_case(iarg.name));

                if (m && m->has_default_argument(i)) {
                    bool defval_ok = rd._arg_default_value_from_variant(m->get_default_argument(i), iarg);
                    ERR_FAIL_COND_V_MSG(!defval_ok, false,
                            "Cannot determine default value for argument '" + orig_arg_name + "' of method '" + itype.name + "." + imethod.name + "'.");
                }

                imethod.add_argument(iarg);
            }

            if (imethod.is_vararg) {
                ArgumentInterface ivararg;
                ivararg.type.cname = name_cache->type_VarArg;
                ivararg.name = "@args";
                imethod.add_argument(ivararg);
            }

            imethod.proxy_name = escape_csharp_keyword(snake_to_pascal_case(imethod.name));

            // Prevent the method and its enclosing type from sharing the same name
            if (imethod.proxy_name == itype.proxy_name) {
                _log("Name of method '%s' is ambiguous with the name of its enclosing class '%s'. Renaming method to '%s_'\n",
                        imethod.proxy_name.c_str(), itype.proxy_name.asCString(), imethod.proxy_name.c_str());

                imethod.proxy_name += "_";
            }

            Map<StringName, StringName>::iterator accessor = accessor_methods.find(imethod.cname);
            if (accessor!=accessor_methods.end()) {
                const PropertyInterface *accessor_property = itype.find_property_by_name(accessor->second);

                // We only deprecate an accessor method if it's in the same class as the property. It's easier this way, but also
                // we don't know if an accessor method in a different class could have other purposes, so better leave those untouched.
                imethod.is_deprecated = true;
                imethod.deprecation_message = imethod.proxy_name + " is deprecated. Use the " + accessor_property->proxy_name + " property instead.";
            }

            if (!imethod.is_virtual && imethod.name[0] == '_') {
                for (const PropertyInterface &iprop : itype.properties) {

                    if (iprop.setter == imethod.name || iprop.getter == imethod.name) {
                        imethod.is_internal = true;
                        itype.methods.push_back(imethod);
                        break;
                    }
                }
            } else {
                itype.methods.push_back(imethod);
            }
        }

        // Populate enums and constants

        List<String> constants;
        ClassDB::get_integer_constant_list(type_cname, &constants, true);

        const HashMap<StringName, List<StringName> > &enum_map = class_iter->second.enum_map;
        for(const auto &F: enum_map) {
            auto parts = StringUtils::split(F.first,"::");
            if(parts.size()>1 && itype.name==parts[0]) {
                parts.pop_front(); // Skip leading type name, this will be fixed below
            }
            StringName enum_proxy_cname(parts.front());
            String enum_proxy_name(enum_proxy_cname);
            if (itype.find_property_by_proxy_name(enum_proxy_name)) {
                // We have several conflicts between enums and PascalCase properties,
                // so we append 'Enum' to the enum name in those cases.
                enum_proxy_name += "Enum";
                enum_proxy_cname = StringName(enum_proxy_name);
            }
            EnumInterface ienum(enum_proxy_cname);
            const List<StringName> &enum_constants = F.second;
            for (const StringName &constant_cname : enum_constants) {
                String constant_name(constant_cname);
                auto value = class_iter->second.constant_map.find(constant_cname);
                ERR_FAIL_COND_V(value==class_iter->second.constant_map.end(), false);
                constants.remove(constant_name);

                ConstantInterface iconstant(constant_name, snake_to_pascal_case(constant_name, true), value->second);

                ienum.constants.push_back(iconstant);
            }

            int prefix_length = _determine_enum_prefix(ienum);

            _apply_prefix_to_enum_constants(ienum, prefix_length);

            itype.enums.push_back(ienum);

            TypeInterface enum_itype;
            enum_itype.is_enum = true;
            enum_itype.name = itype.name + "." + String(enum_proxy_cname);
            enum_itype.cname = StringName(enum_itype.name);
            enum_itype.proxy_name = itype.proxy_name + "." + enum_proxy_name;
            TypeInterface::postsetup_enum_type(enum_itype);
            rd.enum_types.emplace(enum_itype.cname, enum_itype);
        }

        for (const String &constant_name : constants) {
            auto value = class_iter->second.constant_map.find(StringName(constant_name));
            ERR_FAIL_COND_V(value==class_iter->second.constant_map.end(), false);

            ConstantInterface iconstant(constant_name, snake_to_pascal_case(constant_name, true), value->second);

            itype.constants.push_back(iconstant);
        }

        auto insert_res = rd.obj_types.emplace(itype.cname, itype);
       if(insert_res.second) //was inserted, record it in order container
            rd.obj_type_insert_order.emplace_back(itype.cname);

        class_list.pop_front();
    }

    return true;
}

void BindingsGenerator::_populate_builtin_type_interfaces() {

    rd.builtin_types.clear();

    TypeInterface itype;

#define INSERT_STRUCT_TYPE(m_type)                                     \
    {                                                                  \
        itype = TypeInterface::create_value_type(StringName(#m_type));     \
        itype.c_in = "\t%0 %1_in = MARSHALLED_IN(" #m_type ", %1);\n"; \
        itype.c_out = "\t*%3 = MARSHALLED_OUT(" #m_type ", %1);\n";    \
        itype.c_arg_in = "%s_in";                                      \
        itype.c_type_in = "GDMonoMarshal::M_" #m_type "*";             \
        itype.c_type_out = "GDMonoMarshal::M_" #m_type;                \
        itype.cs_in = "ref %s";                                        \
        /* in cs_out, im_type_out (%3) includes the 'out ' part */     \
        itype.cs_out = "%0(%1, %3 argRet); return (%2)argRet;";        \
        itype.im_type_out = "out " + itype.cs_type;                    \
        itype.ret_as_byref_arg = true;                                 \
        rd.builtin_types.emplace(itype.cname, itype);                      \
    }

    INSERT_STRUCT_TYPE(Vector2)
    INSERT_STRUCT_TYPE(Rect2)
    INSERT_STRUCT_TYPE(Transform2D)
    INSERT_STRUCT_TYPE(Vector3)
    INSERT_STRUCT_TYPE(Basis)
    INSERT_STRUCT_TYPE(Quat)
    INSERT_STRUCT_TYPE(Transform)
    INSERT_STRUCT_TYPE(AABB)
    INSERT_STRUCT_TYPE(Color)
    INSERT_STRUCT_TYPE(Plane)

#undef INSERT_STRUCT_TYPE

    // bool
    itype = TypeInterface::create_value_type(StringName("bool"));
    {
        // MonoBoolean <---> bool
        itype.c_in = "\t%0 %1_in = static_cast<%0>(%1);\n";
        itype.c_out = "\treturn static_cast<%0>(%1);\n";
        itype.c_type = "bool";
        itype.c_type_in = "MonoBoolean";
        itype.c_type_out = itype.c_type_in;
        itype.c_arg_in = "%s_in";
    }
    itype.im_type_in = itype.name;
    itype.im_type_out = itype.name;
    rd.builtin_types.emplace(itype.cname, itype);

    // Integer types
    {
        // C interface for 'uint32_t' is the same as that of enums. Remember to apply
        // any of the changes done here to 'TypeInterface::postsetup_enum_type' as well.
#define INSERT_INT_TYPE(m_name, m_c_type_in_out, m_c_type)        \
    {                                                             \
        itype = TypeInterface::create_value_type(StringName(m_name)); \
        {                                                         \
            itype.c_in = "\t%0 %1_in = static_cast<%0>(%1);\n";                \
            itype.c_out = "\treturn static_cast<%0>(%1);\n";      \
            itype.c_type = #m_c_type;                             \
            itype.c_arg_in = "%s_in";                             \
        }                                                         \
        itype.c_type_in = #m_c_type_in_out;                       \
        itype.c_type_out = itype.c_type_in;                       \
        itype.im_type_in = itype.name;                            \
        itype.im_type_out = itype.name;                           \
        rd.builtin_types.emplace(itype.cname, itype);                 \
    }

        INSERT_INT_TYPE("sbyte", int8_t, int8_t)
        INSERT_INT_TYPE("short", int16_t, int16_t)
        INSERT_INT_TYPE("int", int32_t, int32_t)
        INSERT_INT_TYPE("byte", uint8_t, uint8_t)
        INSERT_INT_TYPE("ushort", uint16_t, uint16_t)
        INSERT_INT_TYPE("uint", uint32_t, uint32_t)

        itype = TypeInterface::create_value_type(StringName("long"));
        {
            itype.c_out = "\treturn static_cast<%0>(%1);\n";
            itype.c_in = "\t%0 %1_in = static_cast<%0>(*%1);\n";
            itype.c_out = "\t*%3 = static_cast<%0>(%1);\n";
            itype.c_type = "int64_t";
            itype.c_arg_in = "%s_in";
        }
        itype.c_type_in = "int64_t*";
        itype.c_type_out = "int64_t";
        itype.im_type_in = "ref " + itype.name;
        itype.im_type_out = "out " + itype.name;
        itype.cs_in = "ref %0";
        /* in cs_out, im_type_out (%3) includes the 'out ' part */
        itype.cs_out = "%0(%1, %3 argRet); return (%2)argRet;";
        itype.ret_as_byref_arg = true;
        rd.builtin_types.emplace(itype.cname, itype);

        itype = TypeInterface::create_value_type(StringName("ulong"));
        {
            itype.c_in = "\t%0 %1_in = static_cast<%0>(*%1);\n";
            itype.c_out = "\t*%3 = (%0)%1;\n";
            itype.c_type = "int64_t";
            itype.c_arg_in = "%s_in";
        }
        itype.c_type_in = "uint64_t*";
        itype.c_type_out = "uint64_t";
        itype.im_type_in = "ref " + itype.name;
        itype.im_type_out = "out " + itype.name;
        itype.cs_in = "ref %0";
        /* in cs_out, im_type_out (%3) includes the 'out ' part */
        itype.cs_out = "%0(%1, %3 argRet); return (%2)argRet;";
        itype.ret_as_byref_arg = true;
        rd.builtin_types.emplace(itype.cname, itype);
    }

    // Floating point types
    {
        // float
        itype = TypeInterface();
        itype.name = "float";
        itype.cname = StringName(itype.name);
        itype.proxy_name = "float";
        {
            itype.c_in = "\t%0 %1_in = static_cast<%0>(*%1);\n";
            itype.c_out = "\t*%3 = (%0)%1;\n";
            itype.c_type = "float";
            itype.c_type_in = "float*";
            itype.c_type_out = "float";
            itype.c_arg_in = "%s_in";
        }
        itype.cs_type = itype.proxy_name;
        itype.im_type_in = "ref " + itype.proxy_name;
        itype.im_type_out = "out " + itype.proxy_name;
        itype.cs_in = "ref %0";
        /* in cs_out, im_type_out (%3) includes the 'out ' part */
        itype.cs_out = "%0(%1, %3 argRet); return (%2)argRet;";
        itype.ret_as_byref_arg = true;
        rd.builtin_types.emplace(itype.cname, itype);

        // double
        itype = TypeInterface();
        itype.name = "double";
        itype.cname = StringName(itype.name);
        itype.proxy_name = "double";
        {
            itype.c_in = "\t%0 %1_in = static_cast<%0>(*%1);\n";
            itype.c_out = "\t*%3 = (%0)%1;\n";
            itype.c_type = "double";
            itype.c_type_in = "double*";
            itype.c_type_out = "double";
            itype.c_arg_in = "%s_in";
        }
        itype.cs_type = itype.proxy_name;
        itype.im_type_in = "ref " + itype.proxy_name;
        itype.im_type_out = "out " + itype.proxy_name;
        itype.cs_in = "ref %0";
        /* in cs_out, im_type_out (%3) includes the 'out ' part */
        itype.cs_out = "%0(%1, %3 argRet); return (%2)argRet;";
        itype.ret_as_byref_arg = true;
        rd.builtin_types.emplace(itype.cname, itype);
    }

    // String
    itype = TypeInterface();
    itype.name = "String";
    itype.cname = StringName(itype.name);
    itype.proxy_name = "string";
    itype.c_in = "\t%0 %1_in = " C_METHOD_MONOSTR_TO_GODOT "(%1);\n";
    itype.c_out = "\treturn " C_METHOD_MONOSTR_FROM_GODOT "(%1);\n";
    itype.c_arg_in = "%s_in";
    itype.c_type = itype.name;
    itype.c_type_in = "MonoString*";
    itype.c_type_out = "MonoString*";
    itype.cs_type = itype.proxy_name;
    itype.im_type_in = itype.proxy_name;
    itype.im_type_out = itype.proxy_name;
    rd.builtin_types.emplace(itype.cname, itype);

    // StringView
    itype = TypeInterface();
    itype.name = "String";
    itype.cname = "StringView";
    itype.proxy_name = "string";
    // Use tmp string to allocate the string contents on stack, reducing allocations slightly.
    itype.c_in = "\tTmpString<512> %1_in(" C_METHOD_MONOSTR_TO_GODOT "(%1));\n";
    itype.c_out = "\treturn " C_METHOD_MONOSTR_FROM_GODOT "(%1);\n";
    itype.c_arg_in = "%s_in";
    itype.c_type = "StringView";
    itype.c_type_in = "MonoString*";
    itype.c_type_out = "MonoString*";
    itype.cs_type = itype.proxy_name;
    itype.im_type_in = itype.proxy_name;
    itype.im_type_out = itype.proxy_name;
    rd.builtin_types.emplace(itype.cname, itype);
    // StringName
    itype = TypeInterface();
    itype.name = "String";
    itype.cname = "StringName";
    itype.proxy_name = "string";
    itype.c_in = "\tStringName %1_in(" C_METHOD_MONOSTR_TO_GODOT "(%1));\n";
    itype.c_out = "\treturn " C_METHOD_MONOSTR_FROM_GODOT "(%1);\n";
    itype.c_arg_in = "%s_in";
    itype.c_type = "StringName";
    itype.c_type_in = "MonoString*";
    itype.c_type_out = "MonoString*";
    itype.cs_type = itype.proxy_name;
    itype.im_type_in = itype.proxy_name;
    itype.im_type_out = itype.proxy_name;
    rd.builtin_types.emplace(itype.cname, itype);

    // NodePath
    itype = TypeInterface();
    itype.name = "NodePath";
    itype.cname = StringName(itype.name);
    itype.proxy_name = "NodePath";
    itype.c_out = "\treturn memnew(NodePath(%1));\n";
    itype.c_type = itype.name;
    itype.c_type_in = itype.c_type + "*";
    itype.c_type_out = itype.c_type + "*";
    itype.cs_type = itype.proxy_name;
    itype.cs_in = "NodePath." CS_SMETHOD_GETINSTANCE "(%0)";
    itype.cs_out = "return new %2(%0(%1));";
    itype.im_type_in = "IntPtr";
    itype.im_type_out = "IntPtr";
    rd.builtin_types.emplace(itype.cname, itype);

    // RID
    itype = TypeInterface();
    itype.name = "RID";
    itype.cname = StringName(itype.name);
    itype.proxy_name = "RID";
    itype.c_out = "\treturn memnew(RID(%1));\n";
    itype.c_type = StringName(itype.name);
    itype.c_type_in = itype.c_type + "*";
    itype.c_type_out = itype.c_type + "*";
    itype.cs_type = itype.proxy_name;
    itype.cs_in = "RID." CS_SMETHOD_GETINSTANCE "(%0)";
    itype.cs_out = "return new %2(%0(%1));";
    itype.im_type_in = "IntPtr";
    itype.im_type_out = "IntPtr";
    rd.builtin_types.emplace(itype.cname, itype);

    // Variant
    itype = TypeInterface();
    itype.name = "Variant";
    itype.cname = StringName(itype.name);
    itype.proxy_name = "object";
    itype.c_in = "\t%0 %1_in = " C_METHOD_MANAGED_TO_VARIANT "(%1);\n";
    itype.c_out = "\treturn " C_METHOD_MANAGED_FROM_VARIANT "(%1);\n";
    itype.c_arg_in = "%s_in";
    itype.c_type = itype.name;
    itype.c_type_in = "MonoObject*";
    itype.c_type_out = "MonoObject*";
    itype.cs_type = itype.proxy_name;
    itype.im_type_in = "object";
    itype.im_type_out = itype.proxy_name;
    rd.builtin_types.emplace(itype.cname, itype);

    // VarArg (fictitious type to represent variable arguments)
    itype = TypeInterface();
    itype.name = "VarArg";
    itype.cname = StringName(itype.name);
    itype.proxy_name = "object[]";
    itype.c_in = "\t%0 %1_in = " C_METHOD_MONOARRAY_TO(Array) "(%1);\n";
    itype.c_arg_in = "%s_in";
    itype.c_type = "Array";
    itype.c_type_in = "MonoArray*";
    itype.cs_type = "params object[]";
    itype.im_type_in = "object[]";
    rd.builtin_types.emplace(itype.cname, itype);

#define INSERT_ARRAY_FULL(m_name, m_type, m_proxy_t)                          \
    {                                                                         \
        itype = TypeInterface();                                              \
        itype.name = #m_name;                                                 \
        itype.cname = StringName(itype.name);                                 \
        itype.proxy_name = #m_proxy_t "[]";                                   \
        itype.c_in = "\t%0 %1_in = " C_METHOD_MONOARRAY_TO(m_type) "(%1);\n"; \
        itype.c_out = "\treturn " C_METHOD_MONOARRAY_FROM(m_type) "(%1);\n";  \
        itype.c_arg_in = "%s_in";                                             \
        itype.c_type = #m_type;                                               \
        itype.c_type_in = "MonoArray*";                                       \
        itype.c_type_out = "MonoArray*";                                      \
        itype.cs_type = itype.proxy_name;                                     \
        itype.im_type_in = itype.proxy_name;                                  \
        itype.im_type_out = itype.proxy_name;                                 \
        rd.builtin_types.emplace(StringName(itype.name), itype);                 \
    }

#define INSERT_ARRAY_NC_FULL(m_name, m_type, m_proxy_t)                          \
    {                                                                         \
        itype = TypeInterface();                                              \
        itype.name = #m_name;                                                 \
        itype.cname = StringName(itype.name);                                 \
        itype.proxy_name = #m_proxy_t "[]";                                   \
        itype.c_in = "\tauto %1_in = " C_METHOD_MONOARRAY_TO_NC(m_type) "(%1);\n"; \
        itype.c_out = "\treturn " C_METHOD_MONOARRAY_FROM_NC(m_type) "(%1);\n";  \
        itype.c_arg_in = "%s_in";                                             \
        itype.c_type = #m_type;                                               \
        itype.c_type_in = "MonoArray*";                                       \
        itype.c_type_out = "MonoArray*";                                      \
        itype.cs_type = itype.proxy_name;                                     \
        itype.im_type_in = itype.proxy_name;                                  \
        itype.im_type_out = itype.proxy_name;                                 \
        rd.builtin_types.emplace(StringName(itype.name), itype);                 \
    }
#define INSERT_ARRAY_TPL_FULL(m_name, m_type, m_proxy_t)                      \
    {                                                                         \
        itype = TypeInterface();                                              \
        itype.name = #m_name;                                                 \
        itype.cname = StringName(itype.name);                                 \
        itype.proxy_name = #m_proxy_t "[]";                                   \
        itype.c_in = "\tauto %1_in = " C_METHOD_MONOARRAY_TO_NC(m_type) "(%1);\n"; \
        itype.c_out = "\treturn " C_METHOD_MONOARRAY_FROM_NC(m_type) "(%1);\n";  \
        itype.c_arg_in = "%s_in";                                             \
        itype.c_type = #m_type;                                               \
        itype.c_type_in = "MonoArray*";                                       \
        itype.c_type_out = "MonoArray*";                                      \
        itype.cs_type = itype.proxy_name;                                     \
        itype.im_type_in = itype.proxy_name;                                  \
        itype.im_type_out = itype.proxy_name;                                 \
        rd.builtin_types.emplace(StringName(itype.name), itype);                 \
    }
#define INSERT_ARRAY(m_type, m_proxy_t) INSERT_ARRAY_FULL(m_type, m_type, m_proxy_t)

    INSERT_ARRAY(PoolIntArray, int)
    INSERT_ARRAY_NC_FULL(VecInt,VecInt, int)
    INSERT_ARRAY_NC_FULL(VecByte, VecByte, byte)
    INSERT_ARRAY_NC_FULL(VecFloat,VecFloat, float)
    INSERT_ARRAY_NC_FULL(VecString, VecString, string)
    INSERT_ARRAY_NC_FULL(VecVector2, VecVector2, Vector2)
    INSERT_ARRAY_NC_FULL(VecVector3, VecVector3, Vector3)
    INSERT_ARRAY_NC_FULL(VecColor, VecColor, Color)

    INSERT_ARRAY_FULL(PoolByteArray, PoolByteArray, byte)


#ifdef REAL_T_IS_DOUBLE
    INSERT_ARRAY(PoolRealArray, double)
#else
    INSERT_ARRAY(PoolRealArray, float)
#endif

    INSERT_ARRAY(PoolStringArray, string)

    INSERT_ARRAY(PoolColorArray, Color)
    INSERT_ARRAY(PoolVector2Array, Vector2)
    INSERT_ARRAY(PoolVector3Array, Vector3)

#undef INSERT_ARRAY

    // Array
    itype = TypeInterface();
    itype.name = "Array";
    itype.cname = StringName(itype.name);
    itype.proxy_name = StringName(itype.name);
    itype.c_out = "\treturn ToArray(eastl::move(%1));\n";
    itype.c_type = itype.name;
    itype.c_type_in = itype.c_type + "*";
    itype.c_type_out = itype.c_type + "*";
    itype.c_arg_in = "ArrConverter(%0)";
    itype.cs_type = BINDINGS_NAMESPACE_COLLECTIONS "." + itype.proxy_name;
    itype.cs_in = "%0." CS_SMETHOD_GETINSTANCE "()";
    itype.cs_out = "return new " + itype.cs_type + "(%0(%1));";
    itype.im_type_in = "IntPtr";
    itype.im_type_out = "IntPtr";
    rd.builtin_types.emplace(itype.cname, itype);

    // Dictionary
    itype = TypeInterface();
    itype.name = "Dictionary";
    itype.cname = StringName(itype.name);
    itype.proxy_name = StringName(itype.name);
    itype.c_out = "\treturn memnew(Dictionary(%1));\n";
    itype.c_type = itype.name;
    itype.c_type_in = itype.c_type + "*";
    itype.c_type_out = itype.c_type + "*";
    itype.cs_type = BINDINGS_NAMESPACE_COLLECTIONS "." + itype.proxy_name;
    itype.cs_in = "%0." CS_SMETHOD_GETINSTANCE "()";
    itype.cs_out = "return new " + itype.cs_type + "(%0(%1));";
    itype.im_type_in = "IntPtr";
    itype.im_type_out = "IntPtr";
    rd.builtin_types.emplace(itype.cname, itype);

    // void (fictitious type to represent the return type of methods that do not return anything)
    itype = TypeInterface();
    itype.name = "void";
    itype.cname = StringName(itype.name);
    itype.proxy_name = StringName(itype.name);
    itype.c_type = itype.name;
    itype.c_type_in = itype.c_type;
    itype.c_type_out = itype.c_type;
    itype.cs_type = itype.proxy_name;
    itype.im_type_in = itype.proxy_name;
    itype.im_type_out = itype.proxy_name;
    rd.builtin_types.emplace(itype.cname, itype);
}


void BindingsGenerator::_initialize_blacklisted_methods() {

    blacklisted_methods["Object"].push_back("to_string"); // there is already ToString
    blacklisted_methods["Object"].push_back("_to_string"); // override ToString instead
    blacklisted_methods["Object"].push_back("_init"); // never called in C# (TODO: implement it)
}

void BindingsGenerator::_log(const char *p_format, ...) {

    if (log_print_enabled) {
        va_list list;

        va_start(list, p_format);
        OS::get_singleton()->print(str_format(p_format, list).c_str());
        va_end(list);
    }
}

void BindingsGenerator::_initialize(DocData *docs) {

    initialized = false;

    rd.doc =docs;
    rd.enum_types.clear();
    rd.build_doc_lookup_helper();

    _initialize_blacklisted_methods();

    bool obj_type_ok = _populate_object_type_interfaces();
    ERR_FAIL_COND_MSG(!obj_type_ok, "Failed to generate object type interfaces");

    _populate_builtin_type_interfaces();

    _populate_global_constants();

    // Generate internal calls (after populating type interfaces and global constants)

    //core_custom_icalls.clear();
    //editor_custom_icalls.clear();


    for (const StringName& E : rd.obj_type_insert_order) {
        const TypeInterface& itype = rd.obj_types[E];
        _generate_method_icalls(itype);
    }

    initialized = true;
}

void BindingsGenerator::handle_cmdline_args(const Vector<String> &p_cmdline_args) {

    const int NUM_OPTIONS = 2;
    const String generate_all_glue_option = "--generate-mono-glue";

    String glue_dir_path;
    String cs_dir_path;
    String cpp_dir_path;

    int options_left = NUM_OPTIONS;

    for(auto elem=p_cmdline_args.begin(),fin=p_cmdline_args.end(); elem!=fin; ) {
        if(!options_left)
            break;
        if (*elem == generate_all_glue_option) {
            auto path_elem = ++elem;

            if (path_elem!=fin) {
                glue_dir_path = *path_elem;
                ++elem;
            } else {
                ERR_PRINT(generate_all_glue_option + ": No output directory specified (expected path to '{GODOT_ROOT}/modules/mono/glue').");
            }

            --options_left;
        }
        else
            ++elem;
    }

    if (glue_dir_path.empty() && cs_dir_path.empty() && cpp_dir_path.empty())
        return;
    BindingsGenerator bindings_generator;
    bindings_generator.set_log_print_enabled(true);

    if (!bindings_generator.initialized) {
        ERR_PRINT("Failed to initialize the bindings generator");
        ::exit(0);
    }
    bool generate_core;
    bool generate_editor;

    if (!glue_dir_path.empty()) {
        GeneratorContext core_generator;
        core_generator.api_hash = ClassDB::get_api_hash(ClassDB::API_CORE);
        core_generator.cs_side_hash = 1; //CS_GLUE_VERSION;
        //core_generator.custom_icalls = core_custom_icalls;
        core_generator.m_cs_namespace = BINDINGS_NAMESPACE;
        core_generator.m_globals_class = BINDINGS_GLOBAL_SCOPE_CLASS;
        core_generator.m_native_calls_class = BINDINGS_CLASS_NATIVECALLS;
        core_generator.m_assembly_name = CORE_API_ASSEMBLY_NAME;


        GeneratorContext editor_generator;
        editor_generator.api_hash = ClassDB::get_api_hash(ClassDB::API_EDITOR);
        editor_generator.cs_side_hash = 1; //CS_GLUE_VERSION;
        //core_generator.custom_icalls = core_custom_icalls;

        editor_generator.m_cs_namespace = BINDINGS_NAMESPACE;
        editor_generator.m_globals_class = BINDINGS_GLOBAL_SCOPE_CLASS;
        editor_generator.m_native_calls_class = BINDINGS_CLASS_NATIVECALLS_EDITOR;
        editor_generator.m_assembly_name = EDITOR_API_ASSEMBLY_NAME;


        if (bindings_generator.generate_glue(glue_dir_path,core_generator) != OK) {
            ERR_PRINT(generate_all_glue_option + ": Failed to generate the C++ glue.");
        }
        if (bindings_generator.generate_glue(glue_dir_path,editor_generator) != OK) {
            ERR_PRINT(generate_all_glue_option + ": Failed to generate the C++ glue.");
        }
        assert(false);

//        if (bindings_generator.generate_cs_api(PathUtils::plus_file(glue_dir_path,API_SOLUTION_NAME)) != OK) {
//            ERR_PRINT(generate_all_glue_option + ": Failed to generate the C# API.");
//        }
    }

    // Exit once done
    unload_plugins();
    unregister_scene_types();
    unregister_module_types();
    unregister_core_types();
    ::exit(0);
}

#endif
bool is_csharp_keyword(StringView p_name) {
    using namespace eastl;
    static vector_set<StringView, eastl::less<StringView>, EASTLAllocatorType, eastl::fixed_vector<StringView, 79, false>>
        keywords;
    static bool initialized = false;
    if (!initialized) {
        constexpr const char* kwords[] = {
            "abstract" ,"as" ,"base" ,"bool" ,
            "break" ,"byte" ,"case" ,"catch" ,
            "char" ,"checked" ,"class" ,"const" ,
            "continue" ,"decimal" ,"default" ,"delegate" ,
            "do" ,"double" ,"else" ,"enum" ,
            "event" ,"explicit" ,"extern" ,"false" ,
            "finally" ,"fixed" ,"float" ,"for" ,
            "forech" ,"goto" ,"if" ,"implicit" ,
            "in" ,"int" ,"interface" ,"internal" ,
            "is" ,"lock" ,"long" ,"namespace" ,
            "new" ,"null" ,"object" ,"operator" ,
            "out" ,"override" ,"params" ,"private" ,
            "protected" ,"public" ,"readonly" ,"ref" ,
            "return" ,"sbyte" ,"sealed" ,"short" ,
            "sizeof" ,"stackalloc" ,"static" ,"string" ,
            "struct" ,"switch" ,"this" ,"throw" ,
            "true" ,"try" ,"typeof" ,"uint" ,"ulong" ,
            "unchecked" ,"unsafe" ,"ushort" ,"using" ,
            "virtual" ,"volatile" ,"void" ,"while"
        };
        for (const char* c : kwords)
            keywords.emplace(c);
        initialized = true;
    }
    // Reserved keywords
    return keywords.contains(p_name);
}

String escape_csharp_keyword(StringView p_name) {
    return is_csharp_keyword(p_name) ? String("@") + p_name : String(p_name);
}

// ENUM FIELD NAME CONVERSION snake_to_pascal_case(constant_name, true),

struct CSTypeMapper : BindingTypeMapper {

    String mapIntTypeName(IntTypes);
    String mapFloatTypeName(FloatTypes);
    String mapClassName(StringView class_name, StringView namespace_name = {}) {

    }
    String mapPropertyName(StringView src_name, StringView class_name = {}, StringView namespace_name = {}) {
        String conv_name = escape_csharp_keyword(snake_to_pascal_case(src_name));
        String mapped_class_name = mapClassName(class_name,namespace_name);
        // Prevent the property and its enclosing type from sharing the same name
        if (conv_name == mapped_class_name) {
            qWarning("Name of property '%s' is ambiguous with the name of its enclosing class '%s'. Renaming property to '%s_'\n",
                conv_name.c_str(), mapped_class_name.c_str(), String(src_name).c_str());

            conv_name += "_";
        }
        return conv_name;
    }
    String mapArgumentName(StringView src_name) {
        return escape_csharp_keyword(snake_to_camel_case(src_name));
    }
    bool shouldSkipMethod(StringView method_name, StringView class_name = {}, StringView namespace_name = {}) {

    }
    String mapMethodName(StringView method_name, StringView class_name = {}, StringView namespace_name = {}) {
        String proxy_name = escape_csharp_keyword(snake_to_pascal_case(method_name));
        String mapped_class_name = mapClassName(class_name, namespace_name);

        // Prevent the method and its enclosing type from sharing the same name
        if ((!class_name.empty() && proxy_name == mapped_class_name) || (!namespace_name.empty() && proxy_name==namespace_name)) {
            qWarning("Name of method '%s' is ambiguous with the name of its enclosing class '%s'. Renaming method to '%s_'\n",
                proxy_name.c_str(), mapped_class_name.c_str(), String(method_name).c_str());

            proxy_name += "_";
        }
        return proxy_name;
    }
};


struct FileProducer {
    Map<String,String> target_files;
    bool add_to_file(String fname,String contents) {
        target_files[fname] += contents;
        return true;
    }
};
struct CppProducer : FileProducer {
    QDir working_dir;
    CppProject cpp_editor_producer;
    CppProject cpp_client_producer;
    CppProject cpp_server_producer;
    QDir m_target_dir;
    String m_project_name;
    //TARGET_DIR/MonoBindings/
    CppProducer()  {
    }
    void setup(const QDir& target_dir, const String& project_name) {
        m_target_dir.setPath(target_dir.path() + "/cpp_gen");
        m_project_name = project_name;
        cpp_editor_producer.setup(project_name, "editor");
        cpp_client_producer.setup(project_name, "client");
        cpp_server_producer.setup(project_name, "server");
    }
    bool create_build_files() {
        if(!m_target_dir.mkpath(".")) {

            qCritical() << "Cannot create: " << m_target_dir.path();
            return false;
        }

        QFile target_cmake(m_target_dir.filePath("CMakeLists.txt"));
        if(target_cmake.exists()) {
            qDebug() << "CMakeLists.txt already exists in" << m_target_dir.path() << " overwriting it";
        }
        if(!target_cmake.open(QFile::WriteOnly)) {
            qCritical() << "Cannot write: " << m_target_dir.filePath("CMakeLists.txt");
            return false;
        }
        target_cmake.write(cpp_editor_producer.generate_cmake_contents().c_str());
        target_cmake.write(cpp_client_producer.generate_cmake_contents().c_str());
        target_cmake.write(cpp_server_producer.generate_cmake_contents().c_str());
        return true;

    }
};
struct CSProducer : FileProducer {
    QFile m_current_target_file;
    QDir m_target_dir;
    String m_project_name;
    Vector<String> generated_filenames;

    CSProducer() {

    }
    void setup(const QDir& target_dir, const String& project_name) {
        m_target_dir = target_dir;
        m_project_name = project_name;
    }
    /* re-create csproj files, and add them to the SLN*/
    bool create_build_files() {
        QUuid editor_uuid = QUuid::createUuidV5(g_generator_project_namespace,QByteArray((m_project_name + "editor").c_str()));
        QUuid client_uuid = QUuid::createUuidV5(g_generator_project_namespace,QByteArray((m_project_name + "client").c_str()));
        QUuid server_uuid = QUuid::createUuidV5(g_generator_project_namespace,QByteArray((m_project_name + "server").c_str()));

        QByteArray original_contents;
        QByteArray new_contents;
        QString sln_path = m_target_dir.filePath("project.sln");
        QString new_sln_path = m_target_dir.filePath("new_project.sln");
        if(QFile::exists(sln_path)) {
            QFile sln(m_target_dir.filePath("project.sln"));
            if (!sln.open(QFile::ReadOnly)) {
                qCritical() << "Failed to read from " << m_target_dir.filePath("project.sln");
                return false;
            }
            original_contents = sln.readAll();
        }
        if(!original_contents.isEmpty())
            new_contents = original_contents;

        SLNTransformer transform;

        transform.parse(new_contents);
        transform.add_project_guid(editor_uuid, m_project_name, m_project_name + "_editor.csproj");
        transform.add_project_guid(client_uuid, m_project_name, m_project_name + "_client.csproj");
        transform.add_project_guid(server_uuid, m_project_name, m_project_name + "_server.csproj");
        new_contents = transform.generate().c_str();

        QFile new_sln_file(new_sln_path);
        if(!new_sln_file.open(QFile::WriteOnly)) {
            return false;
        }
        new_sln_file.write(new_contents);
        return true;
    }
    bool generate_constant_files() {
        assert(false);
        // Generate source file for global scope constants and enums
//        for(const auto &ns : CSNamespace::namespaces) {
//            StringBuilder constants_source;
//            _generate_global_constants(constants_source,ns.second,doc);
//            QString output_file =QDir(m_target_dir).filePath((ns.second->cs_name + "_constants.cs").c_str());
//            auto save_err = _save_file(output_file, constants_source);
//            if (save_err != OK)
//                return false;

//            generated_filenames.emplace_back(qPrintable(output_file));

//        }
    }
};

struct CSReflectionVisitor {

    CppProducer cpp_producer;
    CSProducer cs_producer;
    Vector<CSNamespace *> m_namespace_stack;
    Vector<CSType *> m_type_stack;
    const ReflectionData &m_reflection_data;
    CSEnum *m_current_enum = nullptr;
    QDir m_current_directory;

    CSReflectionVisitor(const ReflectionData &rd, const QString &target_dir, const String &project_name) :
        m_reflection_data(rd),
        m_current_directory(target_dir + "/MonoBindings") {
        cpp_producer.setup(m_current_directory,project_name);
        cs_producer.setup(m_current_directory, project_name);

    }
    String current_access_path() const {
        StringBuilder res;
        for(const auto ns : m_namespace_stack) {
            res += ns->cs_name;
            res += "::";
        }
        for(const auto ts : m_type_stack) {
            res += ts->cs_name;
            res += "::";
        }
        if(m_current_enum) {
            res += m_current_enum->cs_name;
            res += "::";
        }
        return res;
    }
    void visit_constant(const ConstantInterface *ci) {
        // A few cases:
        // In namespace, create Constants class, add entry for the constant

        // In class add entry for the constant
        // In enum add entry for the constant
        if(m_current_enum) {
            m_current_enum->add_constant(current_access_path(),ci);
            cs_producer.add_to_file("_GlobalConstants.cs",ci->name);
        } else if(m_type_stack.empty()) {
            assert(!m_namespace_stack.empty());
            m_namespace_stack.back()->m_globals.add_constant(current_access_path(),ci);
        } else {
            m_type_stack.back()->add_constant(current_access_path(),ci);
        }
    }
    void visitEnum(const EnumInterface *ei) {
        CSEnum *en = CSEnum::get_instance_for(current_access_path(),ei);
        m_current_enum = en;
        for(const ConstantInterface & ci : ei->constants)
        {
            visit_constant(&ci);
        }
        m_current_enum = nullptr;
    }
    /*
     EnumInterface ienum(StringName(String(enum_name).replaced("::", ".")));
                auto enum_match = rd.global_enums.find(ienum);
                if (enum_match != rd.global_enums.end()) {
                    enum_match->constants.push_back(iconstant);
                }
                else {
                    ienum.constants.push_back(iconstant);
                    rd.global_enums.push_back(ienum);
                }

     */
    void visitFunction(const MethodInterface *mi) {
       assert(false);
    }
    void visitNamespace(const NamespaceInterface *iface) {
        m_namespace_stack.push_back(CSNamespace::get_instance_for(current_access_path(),iface));
        for (const ConstantInterface& ci : iface->global_constants) {
            visit_constant(&ci);
        }
        for (const EnumInterface& ci : iface->global_enums) {
            visitEnum(&ci);
        }
        for (const auto& ci : iface->obj_types) {
            visitType(&ci.second);
        }

        leaveNamespace();
    }
    void leaveNamespace() {
        m_current_directory.cdUp();
        m_namespace_stack.pop_back();
    }

    void visitType(const TypeInterface *) {
      //  assert(false);
    }
    void visitTypeProperty(const PropertyInterface *) {
      //  assert(false);
    }
    void visitTypeMethod(const PropertyInterface *) {
      //  assert(false);
    }

    void finalize() {
        cs_producer.generate_constant_files();
        cpp_producer.create_build_files();
        cs_producer.create_build_files();
    }
};

bool processReflectionData(const ReflectionData &rd,const QString &target_dir) {
    QFileInfo fi(target_dir);
    if((fi.exists() && !fi.isDir()) || (fi.exists() && fi.isDir() && !fi.isWritable()) ) {
        qCritical() << "Provided target path is not a writeable directory!"<<target_dir;
        return false;
    }
    QDir current_dir = QDir::current();
    QString aa=current_dir.absolutePath();
    if(!current_dir.mkpath(target_dir))
        return false;

    CSReflectionVisitor cs_builder(rd, target_dir,"Godot");
    for (const NamespaceInterface& iface : rd.namespaces) {
        cs_builder.visitNamespace(&iface);
    }
    cs_builder.finalize();
    return true;
}

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(qApp->arguments().size()<2) {
        qCritical() << "Binding generator takes 2 arguments, a source_reflection_data.json and target path.";
        return -1;
    }
    ReflectionData rd;
    if(!rd.load_from_file(qPrintable(qApp->arguments()[1]))) {
        qCritical() << "Binding generator failed to load source reflection data:"<< qApp->arguments()[1];
        return -1;

    }
    processReflectionData(rd,qApp->arguments()[2]);

    return 0;
}