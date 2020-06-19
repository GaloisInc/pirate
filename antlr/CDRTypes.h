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

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

enum class CDRTypeOf {
    FLOAT_T,
    DOUBLE_T,
    LONG_DOUBLE_T,
    STRUCT_T,
    MODULE_T,
    TINY_T,
    SHORT_T,
    LONG_T,
    LONG_LONG_T,
    UNSIGNED_TINY_T,
    UNSIGNED_SHORT_T,
    UNSIGNED_LONG_T,
    UNSIGNED_LONG_LONG_T,
    CHAR_T,
    BOOL_T,
    OCTET_T,
};

enum class CDRFunc {
    SERIALIZE,
    DESERIALIZE,
};

enum class CDRBits {
    UNDEFINED,
    B8,
    B16,
    B32,
    B64,
    B128,
};

std::string bitsCType(CDRBits cdrBits);
uint8_t bitsAlignment(CDRBits cdrBits);
std::string bitsSerialize(CDRBits cdrBits);
std::string bitsDeserialize(CDRBits cdrBits);

class TypeSpec {
public:
    virtual CDRTypeOf typeOf() = 0;
    virtual void cTypeStream(std::ostream &ostream) = 0;
    virtual std::string cTypeString() {
        std::stringstream ostream;
        cTypeStream(ostream);
        return ostream.str();
    }
    virtual CDRBits cTypeBits() = 0;
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) = 0;
    virtual bool singleton() { return false; } // workaround for preventing destruction of singletons
    virtual ~TypeSpec() { };
};

// Implementation of the primitive types
// The primitive types are all singletons
class BaseTypeSpec : public TypeSpec {
private:
    CDRTypeOf m_typeOf;
    std::string m_cType;
    CDRBits m_cTypeBits;
    BaseTypeSpec(CDRTypeOf typeOf, std::string cType, CDRBits cTypeBits) :
        m_typeOf(typeOf), m_cType(cType), m_cTypeBits(cTypeBits) { }
public:
    virtual CDRTypeOf typeOf() override { return m_typeOf; }
    virtual std::string cTypeString() override { return m_cType; }
    virtual CDRBits cTypeBits() override { return m_cTypeBits; }
    virtual void cTypeStream(std::ostream &ostream) override { ostream << m_cType; }
    virtual void cDeclareFunctions(std::ostream& /*ostream*/, CDRFunc /*functionType*/) override { };
    static TypeSpec* floatType();
    static TypeSpec* doubleType();
    static TypeSpec* longDoubleType();
    static TypeSpec* tinyType();
    static TypeSpec* shortType();
    static TypeSpec* longType();
    static TypeSpec* longLongType();
    static TypeSpec* unsignedTinyType();
    static TypeSpec* unsignedShortType();
    static TypeSpec* unsignedLongType();
    static TypeSpec* unsignedLongLongType();
    static TypeSpec* charType();
    static TypeSpec* boolType();
    static TypeSpec* octetType();
    virtual bool singleton() { return true; }
};

class Declarator {
public:
    std::string identifier;
    int arrayLength;
    Declarator(std::string identifier) : identifier(identifier), arrayLength(0) { }
    Declarator(std::string identifier, int arrayLength) : identifier(identifier), arrayLength(arrayLength) { }    
};

class StructMember {
public:
    TypeSpec* typeSpec;
    std::vector<Declarator*> declarators;
    StructMember(TypeSpec* typeSpec) : typeSpec(typeSpec), declarators() { }
    void addDeclarator(Declarator* declarator);
    ~StructMember();
};

// Implementation of the struct type
class StructTypeSpec : public TypeSpec {
private:
    void cConvertByteOrder(std::ostream &ostream, TypeSpec* typeSpec, Declarator *declarator, CDRFunc functionType);
public:
    std::string identifier;
    std::vector<StructMember*> members;
    StructTypeSpec(std::string identifier) : identifier(identifier), members() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::STRUCT_T; }
    virtual void cTypeStream(std::ostream &ostream) override;
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override;
    void addMember(StructMember* member);
    virtual ~StructTypeSpec();
};

// Implementation of the module declaration
class ModuleDecl : public TypeSpec {
public:
    std::string identifier;
    std::vector<TypeSpec*> definitions;
    ModuleDecl(std::string identifier) : identifier(identifier), definitions() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::MODULE_T; }
    virtual void cTypeStream(std::ostream &ostream) override;
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override;
    void addDefinition(TypeSpec* definition);
    virtual ~ModuleDecl();
};
