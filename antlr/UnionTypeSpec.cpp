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
    cCppTypeDecl(ostream, false);
}

void UnionTypeSpec::cppTypeDecl(std::ostream &ostream) {
    cCppTypeDecl(ostream, true);
}

void UnionTypeSpec::cCppTypeDecl(std::ostream &ostream, bool cpp) {
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
        if (cpp) {
            ostream << member->typeSpec->cppTypeName();
        } else {
            ostream << member->typeSpec->cTypeName();
        }
        ostream << " ";
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

void UnionTypeSpec::cCppFunctionBody(std::ostream &ostream, CDRFunc functionType) {
    cDeclareLocalVar(ostream, switchType, "tag");
    for (UnionMember* member : members) {
        Declarator* declarator = member->declarator;
        cDeclareLocalVar(ostream, member->typeSpec, "data_" + declarator->identifier);
    }
    cCopyMemoryIn(ostream, switchType, "tag", "tag");
    cConvertByteOrder(ostream, switchType, "tag", functionType);
    cCopyMemoryOut(ostream, switchType, "tag", "tag");
    if (functionType == CDRFunc::SERIALIZE) {
        ostream << "switch" << " " << "(" << "input" << "->" << "tag" << ")" << " " << "{" << std::endl;
    } else {
        ostream << "switch" << " " << "(" << "output" << "->" << "tag" << ")" << " " << "{" << std::endl;
    }
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
}

void UnionTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << std::endl;
    cDeclareFunctionName(ostream, functionType, identifier);
    ostream << indent_manip::push;
    cCppFunctionBody(ostream, functionType);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void UnionTypeSpec::cDeclareAnnotationValidate(std::ostream &ostream) {
    std::string funcname = identifier;
    transform(funcname.begin(), funcname.end(), funcname.begin(), ::tolower);
    ostream << std::endl;
    ostream << "int" << " ";
    ostream << "validate";
    ostream << "_" << funcname << "(";
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
    std::string funcname = identifier;
    transform(funcname.begin(), funcname.end(), funcname.begin(), ::tolower);
    ostream << std::endl;
    ostream << "void" << " ";
    ostream << "transform";
    ostream << "_" << funcname << "(";
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

void UnionTypeSpec::cppDeclareFunctions(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "template" << "<" << ">" << std::endl;
    ostream << "struct" << " " << "Serialization";
    ostream << "<" << "struct" << " " << namespacePrefix << identifier << ">" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    cppDeclareSerializationFunction(ostream);
    ostream << std::endl;
    cppDeclareDeserializationFunction(ostream);
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void UnionTypeSpec::cppDeclareSerializationFunction(std::ostream &ostream) {
    cppDeclareSerializationFunctionName(ostream, "struct " + namespacePrefix + identifier);
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "buf" << "." << "resize" << "(";
    ostream << "sizeof(" << "struct" << " " << namespacePrefix << identifier << ")";
    ostream << ")" << ";" << std::endl;
    ostream << "struct" << " " << namespacePrefix << identifier << "_wire" << "*";
    ostream << " " << "output" << " " << "=" << " ";
    ostream << "(" << "struct" << " " << namespacePrefix << identifier << "_wire" << "*" << ")" << " ";
    ostream << "buf" << "." << "data" << "(" << ")" << ";" << std::endl;
    ostream << "const" << " " << "struct" << " " << namespacePrefix << identifier << "*" << " " << "input" << " ";
    ostream << "=" << " " << "&" << "val" << ";" << std::endl;
    cCppFunctionBody(ostream, CDRFunc::SERIALIZE);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void UnionTypeSpec::cppDeclareDeserializationFunction(std::ostream &ostream) {
    cppDeclareDeserializationFunctionName(ostream, "struct " + namespacePrefix + identifier);
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "struct" << " " << namespacePrefix << identifier << " " << "retval" << ";" << std::endl;
    ostream << "const" << " " << "struct" << " " << namespacePrefix << identifier << "_wire" << "*";
    ostream << " " << "input" << " " << "=" << " ";
    ostream << "(" << "const" << " " << "struct" << " " << namespacePrefix << identifier << "_wire" << "*" << ")";
    ostream << " " << "buf" << "." << "data" << "(" << ")" << ";" << std::endl;
    ostream << "struct" << " " << namespacePrefix << identifier << "*" << " " << "output" << " ";
    ostream << "=" << " " << "&" << "retval" << ";" << std::endl;
    ostream << "if" << " " << "(" << "buf" << "." << "size" << "(" << ")" << " " << "!=" << " ";
    ostream << "sizeof(" << "struct" << " " << namespacePrefix << identifier << ")";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "static" << " " << "const" << " " << "std" << "::" << "string" << " ";
    ostream << "error_msg" << " " << "=" << std::endl;
    ostream << indent_manip::push;
    ostream << "std" << "::" << "string" << "(" << "\"";
    ostream << "pirate::Serialization::fromBuffer() for ";
    ostream << namespacePrefix << identifier;
    ostream << " type did not receive a buffer of size " << "\"";
    ostream << ")" << " " << "+" << std::endl;
    ostream << "std" << "::" << "to_string" << "(" << "sizeof" << "(";
    ostream << "struct" << " " << namespacePrefix << identifier << ")" << ")" <<";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "throw" << " " << "std" << "::" << "length_error" << "(";
    ostream << "error_msg" << ")" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
    cCppFunctionBody(ostream, CDRFunc::DESERIALIZE);
    ostream << "return" << " " << "retval" << ";" << std::endl;
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
