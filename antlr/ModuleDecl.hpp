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

// Implementation of the module declaration
class ModuleDecl : public TypeSpec {
public:
    std::string namespacePrefix;
    std::string identifier;
    std::vector<TypeSpec*> definitions;
    ModuleDecl(std::string namespacePrefix, std::string identifier) :
        namespacePrefix(namespacePrefix), identifier(identifier), definitions() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::MODULE_T; }
    virtual void cTypeDecl(std::ostream &ostream) override;
    virtual void cTypeDeclWire(std::ostream &ostream, bool packed) override;
    virtual std::string cTypeName() override { throw std::runtime_error("module has no type name"); }
    virtual std::string cppNamespacePrefix() override { return namespacePrefix; }
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType, bool packed) override;
    virtual void cDeclareAnnotationValidate(std::ostream& ostream) override;
    virtual void cDeclareAnnotationTransform(std::ostream &ostream) override;
    virtual void cDeclareAsserts(std::ostream &ostream, bool packed) override;
    virtual void cppDeclareHeader(std::ostream &ostream) override;
    virtual void cppTypeDecl(std::ostream &ostream) override;
    virtual void cppTypeDeclWire(std::ostream &ostream, bool packed) override { cTypeDeclWire(ostream, packed); }
    virtual void cppDeclareAsserts(std::ostream &ostream, bool packed) override { cDeclareAsserts(ostream, packed); }
    virtual void cppDeclareFunctions(std::ostream &ostream, bool packed) override;
    virtual void cppDeclareFooter(std::ostream &ostream) override;
    void addDefinition(TypeSpec* definition);
    virtual ~ModuleDecl();
};
