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

#include "CDRTypes.hpp"
#include "indent_facet.hpp"

#include <algorithm>
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

TypeSpec* BaseTypeSpec::errorType() {
    static BaseTypeSpec instance(CDRTypeOf::ERROR_T, "", CDRBits::UNDEFINED);
    return &instance;
}

void EnumTypeSpec::addEnumerator(std::string enumerator) {
    enumerators.push_back(enumerator);
}

void EnumTypeSpec::cTypeDecl(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "enum" << " " << identifier << " " << "{" << std::endl;
    ostream << indent_manip::push;
    size_t len = enumerators.size();
    for (size_t i = 0; i < len; i++) {
        ostream << enumerators[i];
        if (i < len - 1) {
            ostream << ",";
        }
        ostream << std::endl;
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void EnumTypeSpec::cppTypeDecl(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "enum" << " " << "class" << " " << identifier << " ";
    ostream << ":" << " " << "uint32_t" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    size_t len = enumerators.size();
    for (size_t i = 0; i < len; i++) {
        ostream << enumerators[i];
        if (i < len - 1) {
            ostream << ",";
        }
        ostream << std::endl;
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void EnumTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    std::string funcname = identifier;
    transform(funcname.begin(), funcname.end(), funcname.begin(), ::tolower);
    ostream << std::endl;
    ostream << "uint32_t" << " ";
    switch (functionType) {
        case CDRFunc::SERIALIZE:
            ostream << "encode";
            break;
        case CDRFunc::DESERIALIZE:
            ostream << "decode";
            break;
    }
    ostream << "_" << funcname << "(";
    ostream << "uint32_t" << " " << "value";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    cConvertByteOrder(ostream, this, "value", functionType);
    ostream << "return" << " " << "value" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void Declarator::addDimension(int dimension) {
    dimensions.push_back(dimension);
}

void Declarator::addAnnotation(AnnotationSpec* annotation) {
    annotations.push_back(annotation);
}

Declarator::~Declarator() {
    for (AnnotationSpec* annotation : annotations) {
        delete annotation;
    }
}

void cDeclareFunctionName(std::ostream &ostream, CDRFunc functionType, std::string identifier) {
    switch (functionType) {
        case CDRFunc::SERIALIZE:
            ostream << "void" << " ";
            ostream << "encode";
            ostream << "_" << identifier << "(";
            ostream << "struct" << " " << identifier << "*" << " " << "input";
            ostream << "," << " " << "struct" << " " << identifier << "_wire" << "*" << " " << "output";
            ostream << ")" << " " << "{" << std::endl;
            break;
        case CDRFunc::DESERIALIZE:
            ostream << "void" << " ";
            ostream << "decode";
            ostream << "_" << identifier << "(";
            ostream << "struct" << " " << identifier << "_wire" << "*" << " " << "input";
            ostream << "," << " " << "struct" << " " << identifier << "*" << " " << "output";
            ostream << ")" << " " << "{" << std::endl;
            break;
    }
}

void cppDeclareSerializationFunctionName(std::ostream &ostream, std::string typeName) {
    ostream << "static" << " " << "void" << " " << "toBuffer";
    ostream << "(" << typeName << " " << "const" << "&" << " " << "val";
    ostream << "," << " " << "std" << "::" << "vector" << "<" << "char" << ">" << "&";
    ostream << " " << "buf" << ")";
}

void cppDeclareDeserializationFunctionName(std::ostream &ostream, std::string typeName) {
    ostream << "static" << " " << typeName << " " << "fromBuffer";
    ostream << "(" << "std" << "::" << "vector" << "<" << "char" << ">" << " " << "const" << "&";
    ostream << " " << "buf" << ")";
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

void cConvertByteOrderArray(std::ostream &ostream, TypeSpec* typeSpec,
    Declarator* declarator, CDRFunc functionType,
    std::string localPrefix, std::string remotePrefix) {

    std::vector<int> dimensions = declarator->dimensions;
    std::string identifier = declarator->identifier;
    int len = dimensions.size();
    std::stringstream dest;
    for(int i = 0; i < len; i++) {
        std::string idx = identifier + "_" + std::to_string(i);
        int dim = dimensions[i];
        ostream << "for" << " " << "(";
        ostream << "size_t" << " " << idx << " " << "=" << " " << "0" << ";";
        ostream << " " << idx << " " << "<" << " " << dim << ";";
        ostream << " " << idx << "++" << ")" << " " << "{" << std::endl;
        ostream << indent_manip::push;
    }
    dest << identifier;
    for(int i = 0; i < len; i++) {
        std::string idx = identifier + "_" + std::to_string(i);
        dest << "[" << idx << "]";
    }
    cCopyMemoryIn(ostream, typeSpec, localPrefix + identifier, remotePrefix + dest.str());
    cConvertByteOrder(ostream, typeSpec, localPrefix + identifier, functionType);
    cCopyMemoryOut(ostream, typeSpec, localPrefix + identifier, remotePrefix + dest.str());
    for(int i = 0; i < len; i++) {
        ostream << indent_manip::pop;
        ostream << "}" << std::endl;
    }
}

void cppPirateNamespaceHeader(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "namespace" << " " << "pirate" << " " << "{" << std::endl;
    ostream << "#ifndef" << " " << "_PIRATE_SERIALIZATION_H" << std::endl;
    ostream << "#define" << " " << "_PIRATE_SERIALIZATION_H" << std::endl;
    ostream << indent_manip::push;
    ostream << "template" << " " << "<" << "typename" << " " << "T" << ">" << std::endl;
    ostream << "struct" << " " << "Serialization" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    cppDeclareSerializationFunctionName(ostream, "T");
    ostream << ";" << std::endl;
    cppDeclareDeserializationFunctionName(ostream, "T");
    ostream << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "#endif" << " " << "//" << " " << "_PIRATE_SERIALIZATION_H" << std::endl;
    ostream << indent_manip::push;
}

void cppPirateNamespaceFooter(std::ostream &ostream) {
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}
