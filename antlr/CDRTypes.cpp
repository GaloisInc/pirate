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
    size_t len = enumerators.size();
    for (size_t i = 0; i < len; i++) {
        ostream << enumerators[i];
        if (i < len - 1) {
            ostream << ",";
        }
        ostream << std::endl;
    }
    ostream << indent_manip::push;
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void EnumTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
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
    ostream << "_" << identifier << "(";
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
            for (int dim : declarator->dimensions) {
                ostream << "[" << dim << "]";
            }
            if (alignment > 0) {
                ostream << " " << "__attribute__((aligned(" << alignment << ")))";
            }
            ostream << ";" << std::endl;
        }
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void StructTypeSpec::cTypeDeclWire(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "struct" << " " << identifier << "_wire" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            int alignment = bitsAlignment(member->typeSpec->cTypeBits());
            if (alignment == 0) {
                ostream << member->typeSpec->cTypeName() << " ";
                ostream << declarator->identifier;
                for (int dim : declarator->dimensions) {
                    ostream << "[" << dim << "]";
                }
            } else {
                ostream << "unsigned" << " " << "char" << " ";
                ostream << declarator->identifier;
                for (int dim : declarator->dimensions) {
                    ostream << "[" << dim << "]";
                }
                ostream << "[" << alignment << "]";
                ostream << " " << "__attribute__((aligned(" << alignment << ")))";
            }
            ostream << ";" << std::endl;
        }
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void StructTypeSpec::cDeclareAsserts(std::ostream &ostream) {
    ostream << "static_assert" << "(";
    ostream << "sizeof" << "(" << "struct" << " " << identifier << ")";
    ostream << " " << "==" << " ";
    ostream << "sizeof" << "(" << "struct" << " " << identifier << "_wire" << ")";
    ostream << "," << " ";
    ostream << "\"" << "size of struct " << identifier << " not equal to wire protocol struct" << "\"";
    ostream << ")" << ";" << std::endl;
}

void StructTypeSpec::cDeclareFunctionApply(bool scalar, bool array, StructFunction apply) {
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            if (((declarator->dimensions.size() == 0) && scalar) ||
                ((declarator->dimensions.size() > 0) && array)) {
                apply(member, declarator);
            }
        }
    }
}

void declareFunctionName(std::ostream &ostream, CDRFunc functionType, std::string identifier) {
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

void StructTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << std::endl;
    declareFunctionName(ostream, functionType, identifier);
    ostream << indent_manip::push;
    cDeclareFunctionApply(true, true, [&ostream] (StructMember* member, Declarator* declarator)
        { cDeclareLocalVar(ostream, member->typeSpec, declarator->identifier); });
    cDeclareFunctionApply(false, true, [&ostream, functionType] (StructMember* member, Declarator* declarator)
        { cConvertByteOrderArray(ostream, member->typeSpec, declarator, functionType, "", ""); });
    cDeclareFunctionApply(true, false, [&ostream] (StructMember* member, Declarator* declarator)
        { cCopyMemoryIn(ostream, member->typeSpec, declarator->identifier, declarator->identifier); });
    cDeclareFunctionApply(true, false, [&ostream, functionType] (StructMember* member, Declarator* declarator)
        { cConvertByteOrder(ostream, member->typeSpec, declarator->identifier, functionType); });
    cDeclareFunctionApply(true, false, [&ostream] (StructMember* member, Declarator* declarator)
        { cCopyMemoryOut(ostream, member->typeSpec, declarator->identifier, declarator->identifier); });
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cDeclareAnnotationValidate(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "int" << " ";
    ostream << "validate";
    ostream << "_" << identifier << "(";
    ostream << "const" << " " << "struct" << " " << identifier << "*" << " " << "input";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            for (AnnotationSpec* annotation : declarator->annotations) {
                annotation->cDeclareConstraint(ostream, "input->" + declarator->identifier);
            }
        }
    }
    ostream << "return" << " " << "0" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cDeclareAnnotationTransform(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "void" << " ";
    ostream << "transform";
    ostream << "_" << identifier << "(";
    ostream << "struct" << " " << identifier << "*" << " " << "input";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            for (AnnotationSpec* annotation : declarator->annotations) {
                annotation->cDeclareTransform(ostream, member->typeSpec, "input->" + declarator->identifier);
            }
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
        for (int dim : declarator->dimensions) {
            ostream << "[" << dim << "]";
        }
        if (alignment > 0) {
            ostream << " " << "__attribute__((aligned(" << alignment << ")))";
        }
        ostream << ";" << std::endl;
    }
    ostream << indent_manip::pop;
    ostream << "}" << " " << "data" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void UnionTypeSpec::cTypeDeclWire(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "struct" << " " << identifier << "_wire" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    int tagAlign = bitsAlignment(switchType->cTypeBits());
    ostream << "unsigned" << " " << "char" << " " << "tag";
    ostream << "[" << tagAlign << "]";
    ostream << ";" << std::endl;
    ostream << "union" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (UnionMember* member : members) {
        Declarator* declarator = member->declarator;
        int alignment = bitsAlignment(member->typeSpec->cTypeBits());
        if (alignment == 0) {
            ostream << member->typeSpec->cTypeName() << " ";
            ostream << declarator->identifier;
            for (int dim : declarator->dimensions) {
                ostream << "[" << dim << "]";
            }
        } else {
            ostream << "unsigned" << " " << "char" << " ";
            ostream << declarator->identifier;
            for (int dim : declarator->dimensions) {
                ostream << "[" << dim << "]";
            }
            ostream << "[" << alignment << "]";
            ostream << " " << "__attribute__((aligned(" << alignment << ")))";
        }
        ostream << ";" << std::endl;
    }
    ostream << indent_manip::pop;
    ostream << "}" << " " << "data" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void UnionTypeSpec::cDeclareAsserts(std::ostream &ostream) {
    ostream << "static_assert" << "(";
    ostream << "sizeof" << "(" << "struct" << " " << identifier << ")";
    ostream << " " << "==" << " ";
    ostream << "sizeof" << "(" << "struct" << " " << identifier << "_wire" << ")";
    ostream << "," << " " << "\"" << "size of " << identifier << " not equal to wire protocol size" << "\"" << std::endl;
    ostream << ")" << ";" << std::endl;
}

void UnionTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << std::endl;
    declareFunctionName(ostream, functionType, identifier);
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
        if (declarator->dimensions.size() == 0) {
            std::string local = "data_" + declarator->identifier;
            std::string remote = "data." + declarator->identifier;
            cCopyMemoryIn(ostream, member->typeSpec, local, remote);
            cConvertByteOrder(ostream, member->typeSpec, local, functionType);
            cCopyMemoryOut(ostream, member->typeSpec, local, remote);
        } else {
            cConvertByteOrderArray(ostream, member->typeSpec, declarator, functionType, "data_", "data.");
        }
        ostream << "break" << ";" << std::endl;
        ostream << indent_manip::pop;
    }
    ostream << "}" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void UnionTypeSpec::cDeclareAnnotationValidate(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "int" << " ";
    ostream << "validate";
    ostream << "_" << identifier << "(";
    ostream << "const" << " " << "struct" << " " << identifier << "*" << " " << "input";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "switch" << " " << "(" << "input->tag" << ")" << " " << "{" << std::endl;
    for (UnionMember* member : members) {
        Declarator* declarator = member->declarator;
        for (std::string label : member->labels) {
            ostream << "case" << " " << label << ":" << std::endl;
        }
        if (member->hasDefault) {
            ostream << "default" << ":" << std::endl;
        }
        ostream << indent_manip::push;
        for (AnnotationSpec* annotation : declarator->annotations) {
            annotation->cDeclareConstraint(ostream, "input->data." + declarator->identifier);
        }
        ostream << "break" << ";" << std::endl;
        ostream << indent_manip::pop;
    }
    ostream << "}" << std::endl;
    ostream << "return" << " " << "0" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void UnionTypeSpec::cDeclareAnnotationTransform(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "void" << " ";
    ostream << "transform";
    ostream << "_" << identifier << "(";
    ostream << "struct" << " " << identifier << "*" << " " << "input";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "switch" << " " << "(" << "input->tag" << ")" << " " << "{" << std::endl;
    for (UnionMember* member : members) {
        ostream << indent_manip::push;
        Declarator* declarator = member->declarator;
        for (std::string label : member->labels) {
            ostream << "case" << " " << label << ":" << std::endl;
        }
        if (member->hasDefault) {
            ostream << "default" << ":" << std::endl;
        }
        ostream << "{" << std::endl;
        ostream << indent_manip::push;
        for (AnnotationSpec* annotation : declarator->annotations) {
            annotation->cDeclareTransform(ostream, member->typeSpec, "input->data." + declarator->identifier);
        }
        ostream << "break" << ";" << std::endl;
        ostream << indent_manip::pop;
        ostream << "}" << std::endl;
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

void ModuleDecl::cTypeDeclWire(std::ostream &ostream) {
    // TODO: prefix definition names with module namespace
    for (TypeSpec* definition : definitions) {
        definition->cTypeDeclWire(ostream);
    }
}

void ModuleDecl::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    // TODO: prefix function names with module namespace
    for (TypeSpec* definition : definitions) {
        definition->cDeclareFunctions(ostream, functionType);
    }
}

void ModuleDecl::cDeclareAnnotationValidate(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cDeclareAnnotationValidate(ostream);
    }
}

void ModuleDecl::cDeclareAnnotationTransform(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cDeclareAnnotationTransform(ostream);
    }
}

void ModuleDecl::cDeclareAsserts(std::ostream &ostream) {
    ostream << std::endl;
    for (TypeSpec* definition : definitions) {
        definition->cDeclareAsserts(ostream);
    }
}

ModuleDecl::~ModuleDecl() {
    for (TypeSpec* definition : definitions) {
        delete definition;
    }
}

void MinAnnotation::cDeclareConstraint(std::ostream &ostream, std::string identifier) {
    ostream << "if" << " " << "(" << identifier << " " << "<" << " " << min << ")";
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "return" << " " << "-1" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void MaxAnnotation::cDeclareConstraint(std::ostream &ostream, std::string identifier) {
    ostream << "if" << " " << "(" << identifier << " " << ">" << " " << max << ")";
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "return" << " " << "-1" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void RangeAnnotation::cDeclareConstraint(std::ostream &ostream, std::string identifier) {
    ostream << "if" << " " << "(" << "(" << identifier << " " << "<" << " " << min << ")";
    ostream << " " << "||" << " " << "(" << identifier << " " << ">" << " " << max << ")";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "return" << " " << "-1" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void RoundAnnotation::cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) {
    CDRTypeOf typeOf = typeSpec->typeOf();
    std::string roundFunc = "";
    switch (typeOf) {
        case CDRTypeOf::FLOAT_T:
            roundFunc = "nearbyintf";
            break;
        case CDRTypeOf::DOUBLE_T:
            roundFunc = "nearbyint";
            break;
        default:
            break;
    }
    if (roundFunc != "") {
        ostream << "int" << " " << "rmode" << " " << "=" << " ";
        ostream << "fegetround" << "(" << ")" << ";" << std::endl;
        ostream << "fesetround" << "(" << "FE_TONEAREST" << ")" << ";" << std::endl;
        ostream << identifier << " " << "=" << " ";
        ostream << roundFunc << "(" << identifier << ")" << ";" << std::endl;
        ostream << "fesetround" << "(" << "rmode" << ")" << ";" << std::endl;
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
