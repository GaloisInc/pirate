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

enum class CDRTypeOf {
    FLOAT_T,
    DOUBLE_T,
    LONG_DOUBLE_T,
    STRUCT_T,
    MODULE_T,
};

class TypeSpec {
public:
    virtual CDRTypeOf typeOf() = 0;
    virtual std::string cType() = 0;
    virtual uint8_t alignment() = 0;
};

// Implementation of the primitive types
// The primitive types are all singletons
class BaseTypeSpec : public TypeSpec {
private:
    CDRTypeOf m_typeOf;
    std::string m_cType;
    uint8_t m_align;
    BaseTypeSpec(CDRTypeOf typeOf, std::string cType, uint8_t align) :
        m_typeOf(typeOf), m_cType(cType), m_align(align) { }
public:
    virtual CDRTypeOf typeOf() override { return m_typeOf; }
    virtual std::string cType() override { return m_cType; }
    virtual uint8_t alignment() override { return m_align; }
    static TypeSpec* floatType();
    static TypeSpec* doubleType();
    static TypeSpec* longDoubleType();
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
};

// Implementation of the struct type
class StructTypeSpec : public TypeSpec {
public:
    std::string identifier;
    std::vector<StructMember*> members;
    StructTypeSpec(std::string identifier) : identifier(identifier), members() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::STRUCT_T; }
    virtual std::string cType() override;
    virtual uint8_t alignment() override { return 0; }
    void addMember(StructMember* member);
};

// Implementation of the module declaration
class ModuleDecl : public TypeSpec {
public:
    std::string identifier;
    std::vector<TypeSpec*> definitions;
    ModuleDecl(std::string identifier) : identifier(identifier), definitions() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::MODULE_T; }
    virtual std::string cType() override;
    virtual uint8_t alignment() override { return 0; }
    void addDefinition(TypeSpec* definition);
};
