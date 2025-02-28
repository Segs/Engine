#pragma once

#include "reflection_visitor_support.h"
#include "type_system.h"

struct ConstantInterface;
struct MethodInterface;
struct EnumInterface;
struct TypeInterface;
struct PropertyInterface;
struct SignalInterface;

struct TypeRegistrationPass : public ReflectionVisitorBase {

    bool m_currently_visiting_imported=false;

    static bool covariantSetterGetterTypes(StringView getter, StringView setter);
    static int _determine_enum_prefix(const TS_Enum &p_ienum);
    static void _apply_prefix_to_enum_constants(const TS_Enum &p_ienum, int p_prefix_length);

    explicit TypeRegistrationPass(ProjectContext &ctx) : ReflectionVisitorBase(ctx) {}
    void visitConstant(const ConstantInterface *ci);
    void visitEnum(const EnumInterface *ei);
    void visitMethodInterface(const MethodInterface *fi);
    void visitSignalInterface(const SignalInterface *fi);
    bool processProperty(const PropertyInterface *pi, TS_Type *curr_type, TS_Property *prop,
            const PropertyInterface::TypedEntry &val);
    void visitTypeProperty(const PropertyInterface *pi);

    void registerTypesPass(const TypeInterface *ti);
    void registerTypeDetails(const TS_Type *type);
    void visitModule(const ReflectionData *rd,bool imported=false) override;
    void visitNamespace(const NamespaceInterface &iface) override;
    void finalize() override;
    void visit(const ReflectionData *refl) override;

};
