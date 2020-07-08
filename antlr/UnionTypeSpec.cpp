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

#include "UnionTypeSpec.hpp"
#include "indent_facet.hpp"

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
    cDeclareFunctionName(ostream, functionType, identifier);
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
