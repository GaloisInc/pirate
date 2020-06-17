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

#include <iostream>
#include <sstream>

TypeSpec* BaseTypeSpec::floatType() {
    static BaseTypeSpec instance(CDRTypeOf::FLOAT_T, "float", 4);
    return &instance;
}

TypeSpec* BaseTypeSpec::doubleType() {
    static BaseTypeSpec instance(CDRTypeOf::DOUBLE_T, "double", 8);
    return &instance;
}

TypeSpec* BaseTypeSpec::longDoubleType() {
    static BaseTypeSpec instance(CDRTypeOf::LONG_DOUBLE_T, "long double", 16);
    return &instance;
}

void StructMember::addDeclarator(Declarator* declarator) {
    declarators.push_back(declarator);   
}

void StructTypeSpec::addMember(StructMember* member) {
    members.push_back(member);
}

std::string StructTypeSpec::cType() {
    std::stringstream ostream;
    ostream << "struct" << " " << identifier << " " << "{" << std::endl;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            ostream << member->typeSpec->cType() << " ";
            ostream << declarator->identifier;
            // TODO: implement multidimensional arrays
            if (declarator->arrayLength > 0) {
                ostream << "[" << declarator->arrayLength << "]";
            }
            if (member->typeSpec->alignment() > 0) {
                int alignment = member->typeSpec->alignment();                
                if (declarator->arrayLength > 0) {
                    alignment *= declarator->arrayLength;
                }
                ostream << " " << "__attribute__((aligned(" << alignment << ")))";
            }
            ostream << ";" << std::endl;
        }
    }
    ostream << "}" << ";" << std::endl;
    return ostream.str();
}

void ModuleDecl::addDefinition(TypeSpec* definition) {
    definitions.push_back(definition);
}

std::string ModuleDecl::cType() {
    // TODO: prefix definition names with module namespace
    std::stringstream ostream;
    for (TypeSpec* definition : definitions) {
        ostream << std::endl;
        ostream << definition->cType();
    }
    return ostream.str();
}
