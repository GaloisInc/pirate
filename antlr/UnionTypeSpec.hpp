/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#pragma once

#include "CDRTypes.hpp"

class UnionMember {
public:
    TypeSpec* typeSpec;
    Declarator* declarator;
    std::vector<std::string> labels;
    bool hasDefault;
    UnionMember(TypeSpec* typeSpec, Declarator *declarator) :
        typeSpec(typeSpec), declarator(declarator), labels(),
        hasDefault(false) { }
    void addLabel(std::string label);
    void setHasDefault();
    ~UnionMember();
};

// Implementation of the union type
class UnionTypeSpec : public TypeSpec {
private:
    void cCppFunctionBody(std::ostream &ostream, CDRFunc functionType, bool packed);
    void cCppTypeDecl(std::ostream &ostream, bool cpp);
    void cppDeclareSerializationFunction(std::ostream &ostream, bool packed);
    void cppDeclareDeserializationFunction(std::ostream &ostream, bool packed);
public:
    std::string namespacePrefix;
    std::string identifier;
    TypeSpec* switchType;
    std::vector<UnionMember*> members;
    UnionTypeSpec(std::string namespacePrefix, std::string identifier, TypeSpec *switchType) :
        namespacePrefix(namespacePrefix), identifier(identifier),
        switchType(switchType), members() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::UNION_T; }
    virtual void cTypeDecl(std::ostream &ostream) override;
    virtual void cTypeDeclWire(std::ostream &ostream, bool packed) override;
    virtual std::string cTypeName() override { return "struct " + identifier; }
    virtual std::string cppNamespacePrefix() override { return namespacePrefix; }
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType, bool packed) override;
    virtual void cDeclareAnnotationValidate(std::ostream& ostream) override;
    virtual void cDeclareAnnotationTransform(std::ostream &ostream) override;
    virtual void cDeclareAsserts(std::ostream &ostream, bool packed) override;
    virtual void cppTypeDecl(std::ostream &ostream) override;
    virtual void cppTypeDeclWire(std::ostream &ostream, bool packed) override { cTypeDeclWire(ostream, packed); }
    virtual void cppDeclareAsserts(std::ostream &ostream, bool packed) override { cDeclareAsserts(ostream, packed); }
    virtual void cppDeclareFunctions(std::ostream &ostream, bool packed) override;
    void addMember(UnionMember* member);
    virtual ~UnionTypeSpec();
};
