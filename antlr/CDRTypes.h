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

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


enum class CDRTypeOf {
    FLOAT_T,
    DOUBLE_T,
    LONG_DOUBLE_T,
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
    ENUM_T,
    STRUCT_T,
    UNION_T,
    MODULE_T,
    ERROR_T,
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
    virtual void cTypeDecl(std::ostream &ostream) = 0;
    virtual void cTypeDeclWire(std::ostream &ostream) = 0;
    virtual std::string cTypeName() = 0;
    virtual CDRBits cTypeBits() = 0;
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) = 0;
    virtual void cDeclareAsserts(std::ostream &ostream) { }
    virtual void cDeclareAnnotationValidate(std::ostream &ostream) = 0;
    virtual void cDeclareAnnotationTransform(std::ostream &ostream) = 0;
    virtual bool singleton() { return false; } // workaround for preventing destruction of singletons
    virtual ~TypeSpec() { };
};

class AnnotationSpec {
public:
    int id;
    AnnotationSpec(int id) : id(id) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) = 0;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) = 0;
    virtual ~AnnotationSpec() { };
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
    virtual CDRBits cTypeBits() override { return m_cTypeBits; }
    virtual void cTypeDecl(std::ostream &ostream) override { }
    virtual void cTypeDeclWire(std::ostream &ostream) override { }
    virtual std::string cTypeName() override { return m_cType; }
    virtual void cDeclareFunctions(std::ostream& /*ostream*/, CDRFunc /*functionType*/) override { };
    virtual void cDeclareAnnotationValidate(std::ostream& /*ostream*/) override { };
    virtual void cDeclareAnnotationTransform(std::ostream& /*ostream*/) override { };
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
    static TypeSpec* errorType();
    virtual bool singleton() override { return true; }
};

// Implementation of the enum type
class EnumTypeSpec : public TypeSpec {
public:
    std::string identifier;
    std::vector<std::string> enumerators;
    EnumTypeSpec(std::string identifier) : identifier(identifier), enumerators() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::ENUM_T; }
    virtual void cTypeDecl(std::ostream &ostream) override;
    virtual void cTypeDeclWire(std::ostream &ostream) override { }
    virtual std::string cTypeName() override { return "uint32_t"; }
    virtual CDRBits cTypeBits() override { return CDRBits::B32; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override;
    virtual void cDeclareAnnotationValidate(std::ostream& /*ostream*/) override { };
    virtual void cDeclareAnnotationTransform(std::ostream& /*ostream*/) override { };
    void addEnumerator(std::string enumerator);
    virtual ~EnumTypeSpec() { }
};

class Declarator {
public:
    std::string identifier;
    std::vector<int> dimensions;
    std::vector<AnnotationSpec*> annotations;
    Declarator(std::string identifier) : identifier(identifier), dimensions() { }
    void addDimension(int dimension);
    void addAnnotation(AnnotationSpec* annotation);
    ~Declarator();
};

// TypeReference wraps a reference to another type.
// Acts as weak reference to the type. Destructor does not cleanup.
class TypeReference : public TypeSpec {
private:
    TypeSpec *child;
public:
    TypeReference(TypeSpec *child) : child(child) { }
    virtual CDRTypeOf typeOf() override { return child->typeOf(); };
    virtual void cTypeDecl(std::ostream &ostream) override { }
    virtual void cTypeDeclWire(std::ostream &ostream) override { }
    virtual std::string cTypeName() override { return child->cTypeName(); }
    virtual CDRBits cTypeBits() override { return child->cTypeBits(); }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override { }
    virtual void cDeclareAnnotationValidate(std::ostream& /*ostream*/) override { };
    virtual void cDeclareAnnotationTransform(std::ostream& /*ostream*/) override { };
    virtual ~TypeReference() { child = nullptr; }
};

class StructMember {
public:
    TypeSpec* typeSpec;
    std::vector<Declarator*> declarators;
    StructMember(TypeSpec* typeSpec) : typeSpec(typeSpec), declarators() { }
    void addDeclarator(Declarator* declarator);
    ~StructMember();
};

typedef std::function<void(StructMember* member, Declarator* declarator)> StructFunction;

// Implementation of the struct type
class StructTypeSpec : public TypeSpec {
private:
    void cDeclareFunctionApply(bool scalar, bool array, StructFunction apply);
public:
    std::string identifier;
    std::vector<StructMember*> members;
    StructTypeSpec(std::string identifier) : identifier(identifier), members() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::STRUCT_T; }
    virtual void cTypeDecl(std::ostream &ostream) override;
    virtual void cTypeDeclWire(std::ostream &ostream) override;
    // nested structs must be prefixed by parent names in C++
    virtual std::string cTypeName() override { return "struct " + identifier; }
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override;
    virtual void cDeclareAnnotationValidate(std::ostream &ostream) override;
    virtual void cDeclareAnnotationTransform(std::ostream &ostream) override;
    virtual void cDeclareAsserts(std::ostream &ostream) override;
    void addMember(StructMember* member);
    virtual ~StructTypeSpec();
};

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
public:
    std::string identifier;
    TypeSpec* switchType;
    std::vector<UnionMember*> members;
    UnionTypeSpec(std::string identifier, TypeSpec *switchType) :
        identifier(identifier), switchType(switchType), members() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::UNION_T; }
    virtual void cTypeDecl(std::ostream &ostream) override;
    virtual void cTypeDeclWire(std::ostream &ostream) override;
    // nested structs must be prefixed by parent names in C++
    virtual std::string cTypeName() override { return "struct " + identifier; }
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override;
    virtual void cDeclareAnnotationValidate(std::ostream& ostream) override;
    virtual void cDeclareAnnotationTransform(std::ostream &ostream) override;
    virtual void cDeclareAsserts(std::ostream &ostream) override;
    void addMember(UnionMember* member);
    virtual ~UnionTypeSpec();
};

// Implementation of the module declaration
class ModuleDecl : public TypeSpec {
public:
    std::string identifier;
    std::vector<TypeSpec*> definitions;
    ModuleDecl(std::string identifier) : identifier(identifier), definitions() { }
    virtual CDRTypeOf typeOf() override { return CDRTypeOf::MODULE_T; }
    virtual void cTypeDecl(std::ostream &ostream) override;
    virtual void cTypeDeclWire(std::ostream &ostream) override;
    virtual std::string cTypeName() override { throw std::runtime_error("module has no type name"); }
    virtual CDRBits cTypeBits() override { return CDRBits::UNDEFINED; }
    virtual void cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) override;
    virtual void cDeclareAnnotationValidate(std::ostream& ostream) override;
    virtual void cDeclareAnnotationTransform(std::ostream &ostream) override;
    virtual void cDeclareAsserts(std::ostream &ostream) override;
    void addDefinition(TypeSpec* definition);
    virtual ~ModuleDecl();
};

class MinAnnotation : public AnnotationSpec {
public:
    std::string min;
    MinAnnotation(int id, std::string min) : AnnotationSpec(id), min(min) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~MinAnnotation() { };
};

class MaxAnnotation : public AnnotationSpec {
public:
    std::string max;
    MaxAnnotation(int id, std::string max) : AnnotationSpec(id), max(max) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~MaxAnnotation() { };
};

class RangeAnnotation : public AnnotationSpec {
public:
    std::string min;
    std::string max;
    RangeAnnotation(int id, std::string min, std::string max) : AnnotationSpec(id), min(min), max(max) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~RangeAnnotation() { };
};

class RoundAnnotation : public AnnotationSpec {
public:
    RoundAnnotation(int id) : AnnotationSpec(id) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override { };
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override;
    virtual ~RoundAnnotation() { };
};

class ErrorAnnotation : public AnnotationSpec {
public:
    ErrorAnnotation() : AnnotationSpec(0) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override { };
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~ErrorAnnotation() { };
};

void cDeclareLocalVar(std::ostream &ostream, TypeSpec* typeSpec, std::string identifier);
void cCopyMemoryIn(std::ostream &ostream, TypeSpec* typeSpec, std::string local, std::string input);
void cConvertByteOrder(std::ostream &ostream, TypeSpec* typeSpec, std::string identifier, CDRFunc functionType);
void cCopyMemoryOut(std::ostream &ostream, TypeSpec* typeSpec, std::string local, std::string output);

void cConvertByteOrderArray(std::ostream &ostream, TypeSpec* typeSpec,
    Declarator* declarator, CDRFunc functionType,
    std::string localPrefix, std::string remotePrefix);
