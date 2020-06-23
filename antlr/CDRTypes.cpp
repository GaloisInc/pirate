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

#include "CDRTypes.h"
#include "indent_facet.hpp"

#include <iostream>

TypeSpec* BaseTypeSpec::floatType() {
    static BaseTypeSpec instance(CDRTypeOf::FLOAT_T, "float", CDRBits::B32);
    return &instance;
}

TypeSpec* BaseTypeSpec::doubleType() {
    static BaseTypeSpec instance(CDRTypeOf::DOUBLE_T, "double", CDRBits::B64);
    return &instance;
}

TypeSpec* BaseTypeSpec::longDoubleType() {
    static BaseTypeSpec instance(CDRTypeOf::LONG_DOUBLE_T, "long double", CDRBits::B128);
    return &instance;
}

TypeSpec* BaseTypeSpec::tinyType() {
    static BaseTypeSpec instance(CDRTypeOf::TINY_T, "int8_t", CDRBits::B8);
    return &instance;
}

TypeSpec* BaseTypeSpec::shortType() {
    static BaseTypeSpec instance(CDRTypeOf::SHORT_T, "int16_t", CDRBits::B16);
    return &instance;
}

TypeSpec* BaseTypeSpec::longType() {
    static BaseTypeSpec instance(CDRTypeOf::LONG_T, "int32_t", CDRBits::B32);
    return &instance;
}

TypeSpec* BaseTypeSpec::longLongType() {
    static BaseTypeSpec instance(CDRTypeOf::LONG_LONG_T, "int64_t", CDRBits::B64);
    return &instance;
}

TypeSpec* BaseTypeSpec::unsignedTinyType() {
    static BaseTypeSpec instance(CDRTypeOf::UNSIGNED_TINY_T, "uint8_t", CDRBits::B8);
    return &instance;
}

TypeSpec* BaseTypeSpec::unsignedShortType() {
    static BaseTypeSpec instance(CDRTypeOf::UNSIGNED_SHORT_T, "uint16_t", CDRBits::B16);
    return &instance;
}

TypeSpec* BaseTypeSpec::unsignedLongType() {
    static BaseTypeSpec instance(CDRTypeOf::UNSIGNED_LONG_T, "uint32_t", CDRBits::B32);
    return &instance;
}

TypeSpec* BaseTypeSpec::unsignedLongLongType() {
    static BaseTypeSpec instance(CDRTypeOf::UNSIGNED_LONG_LONG_T, "uint64_t", CDRBits::B64);
    return &instance;
}

TypeSpec* BaseTypeSpec::charType() {
    static BaseTypeSpec instance(CDRTypeOf::CHAR_T, "char", CDRBits::B8);
    return &instance;
}

TypeSpec* BaseTypeSpec::boolType() {
    static BaseTypeSpec instance(CDRTypeOf::BOOL_T, "uint8_t", CDRBits::B8);
    return &instance;
}

TypeSpec* BaseTypeSpec::octetType() {
    static BaseTypeSpec instance(CDRTypeOf::OCTET_T, "uint8_t", CDRBits::B8);
    return &instance;
}


void StructMember::addDeclarator(Declarator* declarator) {
    declarators.push_back(declarator);   
}

void StructTypeSpec::addMember(StructMember* member) {
    members.push_back(member);
}

StructMember::~StructMember() {
    if (!typeSpec->singleton()) {
        delete typeSpec;
    }
    for (Declarator* declarator : declarators) {
        delete declarator;
    }
}

void StructTypeSpec::cTypeDecl(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "struct" << " " << identifier << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            int alignment = bitsAlignment(member->typeSpec->cTypeBits());
            ostream << member->typeSpec->cTypeName() << " ";
            ostream << declarator->identifier;
            // TODO: implement multidimensional arrays
            if (declarator->arrayLength > 0) {
                ostream << "[" << declarator->arrayLength << "]";
            }
            if (alignment > 0) {
                if (declarator->arrayLength > 0) {
                    alignment *= declarator->arrayLength;
                }
                ostream << " " << "__attribute__((aligned(" << alignment << ")))";
            }
            ostream << ";" << std::endl;
        }
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void StructTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << "void" << " ";
    switch (functionType) {
        case CDRFunc::SERIALIZE:
            ostream << "encode";
            break;
        case CDRFunc::DESERIALIZE:
            ostream << "decode";
            break;
    }
    ostream << "_" << identifier << "(";
    ostream << "struct" << " " << identifier << "*" << " " << "input";
    ostream << "," << " " << "struct" << " " << identifier << "*" << " " << "output";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            cDeclareLocalVar(ostream, member->typeSpec, declarator->identifier);
        }
    }
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            cCopyMemoryIn(ostream, member->typeSpec, declarator->identifier, declarator->identifier);
        }
    }
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            cConvertByteOrder(ostream, member->typeSpec, declarator->identifier, functionType);
        }
    }
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            cCopyMemoryOut(ostream, member->typeSpec, declarator->identifier, declarator->identifier);
        }
    }
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

StructTypeSpec::~StructTypeSpec() {
    for (StructMember* member : members) {
        delete member;
    }
}

void UnionMember::addLabel(std::string label) {
    labels.push_back(label);
}

void UnionMember::setHasDefault() {
    hasDefault = true;
}

UnionMember::~UnionMember() {
    if (!typeSpec->singleton()) {
        delete typeSpec;
    }
    delete declarator;
}

void UnionTypeSpec::cTypeDecl(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "struct" << " " << identifier << " " << "{" << std::endl;
    ostream << indent_manip::push;
    int tagAlign = bitsAlignment(switchType->cTypeBits());
    ostream << switchType->cTypeName() << " " << "tag";
    ostream << " " << "__attribute__((aligned(" << tagAlign << ")))";
    ostream << ";" << std::endl;
    ostream << "union" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (UnionMember* member : members) {
        Declarator* declarator = member->declarator;
        int alignment = bitsAlignment(member->typeSpec->cTypeBits());
        ostream << member->typeSpec->cTypeName() << " ";
        ostream << declarator->identifier;
        // TODO: implement multidimensional arrays
        if (declarator->arrayLength > 0) {
            ostream << "[" << declarator->arrayLength << "]";
        }
        if (alignment > 0) {
            if (declarator->arrayLength > 0) {
                alignment *= declarator->arrayLength;
            }
            ostream << " " << "__attribute__((aligned(" << alignment << ")))";
        }
        ostream << ";" << std::endl;
    }
    ostream << indent_manip::pop;
    ostream << "}" << " " << "data" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void UnionTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << "void" << " ";
    switch (functionType) {
        case CDRFunc::SERIALIZE:
            ostream << "encode";
            break;
        case CDRFunc::DESERIALIZE:
            ostream << "decode";
            break;
    }
    ostream << "_" << identifier << "(";
    ostream << "struct" << " " << identifier << "*" << " " << "input";
    ostream << "," << " " << "struct" << " " << identifier << "*" << " " << "output";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    cDeclareLocalVar(ostream, switchType, "tag");
    for (UnionMember* member : members) {
        Declarator* declarator = member->declarator;
        cDeclareLocalVar(ostream, member->typeSpec, "data_" + declarator->identifier);
    }
    cCopyMemoryIn(ostream, switchType, "tag", "tag");
    cConvertByteOrder(ostream, switchType, "tag", functionType);
    cCopyMemoryOut(ostream, switchType, "tag", "tag");
    ostream << "switch" << " " << "(" << "tag" << ")" << " " << "{" << std::endl;
    for (UnionMember* member : members) {
         Declarator* declarator = member->declarator;
        for (std::string label : member->labels) {
            ostream << "case" << " " << label << ":" << std::endl;
        }
        if (member->hasDefault) {
            ostream << "default" << ":" << std::endl;
        }
        ostream << indent_manip::push;
        cCopyMemoryIn(ostream, member->typeSpec, "data_" + declarator->identifier, "data." + declarator->identifier);
        cConvertByteOrder(ostream, member->typeSpec, "data_" + declarator->identifier, functionType);
        cCopyMemoryOut(ostream, member->typeSpec, "data_" + declarator->identifier, "data." + declarator->identifier);
        ostream << "break" << ";" << std::endl;
        ostream << indent_manip::pop;
    }
    ostream << "}" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void UnionTypeSpec::addMember(UnionMember* member) {
    members.push_back(member);
}

UnionTypeSpec::~UnionTypeSpec() {
    if (!switchType->singleton()) {
        delete switchType;
    }
    for (UnionMember* member : members) {
        delete member;
    }
}

void ModuleDecl::addDefinition(TypeSpec* definition) {
    definitions.push_back(definition);
}

void ModuleDecl::cTypeDecl(std::ostream &ostream) {
    // TODO: prefix definition names with module namespace
    for (TypeSpec* definition : definitions) {
        definition->cTypeDecl(ostream);
    }
}

void ModuleDecl::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    // TODO: prefix function names with module namespace
    for (TypeSpec* definition : definitions) {
        ostream << std::endl;
        definition->cDeclareFunctions(ostream, functionType);
    }
}

ModuleDecl::~ModuleDecl() {
    for (TypeSpec* definition : definitions) {
        delete definition;
    }
}

std::string bitsCType(CDRBits cdrBits) {
    switch (cdrBits) {
        case CDRBits::B8:
            return "uint8_t";
        case CDRBits::B16:
            return "uint16_t";
        case CDRBits::B32:
            return "uint32_t";
        case CDRBits::B64:
            return "uint64_t";
        case CDRBits::B128:
            return "uint128_t";
        case CDRBits::UNDEFINED:
        default:
            throw std::runtime_error("unexpected bits type");
    }
}

uint8_t bitsAlignment(CDRBits cdrBits) {
    switch (cdrBits) {
        case CDRBits::B8:
            return 1;
        case CDRBits::B16:
            return 2;
        case CDRBits::B32:
            return 4;
        case CDRBits::B64:
            return 8;
        case CDRBits::B128:
            return 16;
        case CDRBits::UNDEFINED:
            return 0;
        default:
            throw std::runtime_error("unexpected bits type");
    }
}

std::string bitsSerialize(CDRBits cdrBits) {
    switch (cdrBits) {
        case CDRBits::B16:
            return "htobe16";
        case CDRBits::B32:
            return "htobe32";
        case CDRBits::B64:
            return "htobe64";
        case CDRBits::B8:
        case CDRBits::B128:
        case CDRBits::UNDEFINED:
        default:
            throw std::runtime_error("unexpected bits type");
    }
}

std::string bitsDeserialize(CDRBits cdrBits) {
    switch (cdrBits) {
        case CDRBits::B16:
            return "be16toh";
        case CDRBits::B32:
            return "be32toh";
        case CDRBits::B64:
            return "be64toh";
        case CDRBits::B8:
        case CDRBits::B128:
        case CDRBits::UNDEFINED:
        default:
            throw std::runtime_error("unexpected bits type");
    }
}

void cDeclareLocalVar(std::ostream &ostream, TypeSpec* typeSpec, std::string identifier) {
    CDRBits cdrBits = typeSpec->cTypeBits();
    if (cdrBits == CDRBits::UNDEFINED) {
        return;
    }
    // TODO: arrays
    ostream << bitsCType(cdrBits) << " " << identifier << ";" << std::endl;
}

void cCopyMemoryIn(std::ostream &ostream, TypeSpec* typeSpec, std::string local, std::string input) {
    CDRBits cdrBits = typeSpec->cTypeBits();
    if (cdrBits == CDRBits::UNDEFINED) {
        return;
    }
    ostream << "memcpy" << "(";
    ostream << "&" << local << "," << " ";
    ostream << "&" << "input->" << input << "," << " ";
    ostream << "sizeof" << "(" << bitsCType(cdrBits) << ")";
    ostream << ")" << ";" << std::endl;
}

void cConvertByteOrder(std::ostream &ostream, TypeSpec* typeSpec, std::string identifier, CDRFunc functionType) {
    CDRBits cdrBits = typeSpec->cTypeBits();
    if ((cdrBits == CDRBits::UNDEFINED) || (cdrBits == CDRBits::B8)) {
        return;
    }
    ostream << identifier << " " << "=" << " ";
    switch (functionType) {
        case CDRFunc::SERIALIZE:
            ostream << bitsSerialize(cdrBits);
            break;
        case CDRFunc::DESERIALIZE:
            ostream << bitsDeserialize(cdrBits);
            break;
    }
    ostream << "(" << identifier << ")" << ";" << std::endl;
}

void cCopyMemoryOut(std::ostream &ostream, TypeSpec* typeSpec, std::string local, std::string output) {
    CDRBits cdrBits = typeSpec->cTypeBits();
    if (cdrBits == CDRBits::UNDEFINED) {
        return;
    }
    ostream << "memcpy" << "(";
    ostream << "&" << "output->" << output << "," << " ";
    ostream << "&" << local << "," << " ";
    ostream << "sizeof" << "(" << bitsCType(cdrBits) << ")";
    ostream << ")" << ";" << std::endl;
}
