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

#include "StructTypeSpec.hpp"
#include "indent_facet.hpp"

void StructMember::addDeclarator(Declarator* declarator) {
    declarators.push_back(declarator);   
}

StructMember::~StructMember() {
    if (!typeSpec->singleton()) {
        delete typeSpec;
    }
    for (Declarator* declarator : declarators) {
        delete declarator;
    }
}

void StructTypeSpec::addMember(StructMember* member) {
    members.push_back(member);
}

void StructTypeSpec::cTypeDecl(std::ostream &ostream) {
    cCppTypeDecl(ostream, TargetLanguage::C_LANG);
}

void StructTypeSpec::cTypeDeclWire(std::ostream &ostream) {
    cCppTypeDeclWire(ostream, TargetLanguage::C_LANG);
}

void StructTypeSpec::cppTypeDecl(std::ostream &ostream) {
    cCppTypeDecl(ostream, TargetLanguage::CPP_LANG);
}

void StructTypeSpec::cppTypeDeclWire(std::ostream &ostream) {
    cCppTypeDeclWire(ostream, TargetLanguage::CPP_LANG);
}

void StructTypeSpec::cCppTypeDecl(std::ostream &ostream, TargetLanguage languageType) {
    ostream << std::endl;
    if (members.empty() && (languageType == TargetLanguage::C_LANG)) {
        // zero element structs are a gcc/clang extension
        ostream << "__extension__" << " ";
    }
    ostream << "struct" << " " << identifier << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            if (languageType == TargetLanguage::CPP_LANG) {
                ostream << member->typeSpec->cppTypeName();
            } else {
                ostream << member->typeSpec->cTypeName();
            }
            ostream << " ";
            ostream << declarator->identifier;
            for (int dim : declarator->dimensions) {
                ostream << "[" << dim << "]";
            }
            if (!member->typeSpec->container()) {
                int alignment = bitsAlignment(member->typeSpec->cTypeBits());
                ostream << " " << "__attribute__((aligned(" << alignment << ")))";
            }
            ostream << ";" << std::endl;
        }
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void StructTypeSpec::cCppTypeDeclWire(std::ostream &ostream, TargetLanguage languageType) {
    ostream << std::endl;
    if (members.empty() && (languageType == TargetLanguage::C_LANG)) {
        // zero element structs are a gcc/clang extension
        ostream << "__extension__" << " ";
    }
    ostream << "struct" << " " << identifier << "_wire" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            if (member->typeSpec->container()) {
                ostream << member->typeSpec->cTypeName() << "_wire" << " ";
                ostream << declarator->identifier;
                for (int dim : declarator->dimensions) {
                    ostream << "[" << dim << "]";
                }
            } else {
                int alignment = bitsAlignment(member->typeSpec->cTypeBits());
                ostream << "unsigned" << " " << "char" << " ";
                ostream << declarator->identifier;
                for (int dim : declarator->dimensions) {
                    ostream << "[" << dim << "]";
                }
                ostream << "[" << alignment << "]";
                if (!packed) {
                    ostream << " " << "__attribute__((aligned(" << alignment << ")))";
                }
            }
            ostream << ";" << std::endl;
        }
    }
    ostream << indent_manip::pop;
    if (packed) {
        ostream << "}" << " " << "__attribute__((packed))" << " " <<  ";" << std::endl;
    } else {
        ostream << "}" << ";" << std::endl;
    }
}

void StructTypeSpec::cDeclareAsserts(std::ostream &ostream) {
    if (!packed) {
        ostream << "static_assert" << "(";
        ostream << "sizeof" << "(" << "struct" << " " << identifier << ")";
        ostream << " " << "==" << " ";
        ostream << "sizeof" << "(" << "struct" << " " << identifier << "_wire" << ")";
        ostream << "," << " ";
        ostream << "\"" << "size of struct " << identifier << " not equal to wire protocol struct" << "\"";
        ostream << ")" << ";" << std::endl;
    }
}

void StructTypeSpec::cDeclareFunctionApply(bool scalar, bool array, StructFunction apply) {
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            if (member->typeSpec->container()) {
                continue;
            }
            if (((declarator->dimensions.size() == 0) && scalar) ||
                ((declarator->dimensions.size() > 0) && array)) {
                apply(member, declarator);
            }
        }
    }
}

void StructTypeSpec::cCppFunctionBody(std::ostream &ostream, CDRFunc functionType, TargetLanguage languageType) {
    if (members.empty()) {
        ostream << "(" << "void" << ")" << " " << "input" << ";" << std::endl;
        ostream << "(" << "void" << ")" << " " << "output" << ";" << std::endl;
        return;
    }
    cDeclareFunctionApply(true, true, [&ostream] (StructMember* member, Declarator* declarator)
        { cDeclareLocalVar(ostream, member->typeSpec, "field_" + declarator->identifier); });
    // unpacked struct types should fill the bytes of padding with 0's
    if (!packed && (functionType == CDRFunc::SERIALIZE)) {
        ostream << "memset" << "(";
        ostream << "output" << ",";
        ostream << " " << "0" << ",";
        ostream << " " << "sizeof" << "(" << "*" << "output" << ")" << ")" << ";" << std::endl;
    }
    cDeclareFunctionApply(false, true, [&ostream, functionType] (StructMember* member, Declarator* declarator)
        { cConvertByteOrderArray(ostream, member->typeSpec, declarator, functionType, "field_", ""); });
    cDeclareFunctionApply(true, false, [&ostream] (StructMember* member, Declarator* declarator)
        { cCopyMemoryIn(ostream, member->typeSpec, "field_" + declarator->identifier, declarator->identifier); });
    cDeclareFunctionApply(true, false, [&ostream, functionType] (StructMember* member, Declarator* declarator)
        { cConvertByteOrder(ostream, member->typeSpec, "field_" + declarator->identifier, functionType); });
    cDeclareFunctionApply(true, false, [&ostream] (StructMember* member, Declarator* declarator)
        { cCopyMemoryOut(ostream, member->typeSpec, "field_" + declarator->identifier, declarator->identifier); });
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
            cDeclareFunctionNested(ostream, member->typeSpec,
                declarator->identifier, functionType, languageType);
        }
    }
}

void StructTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << std::endl;
    cDeclareFunctionName(ostream, functionType, identifier);
    ostream << indent_manip::push;
    cCppFunctionBody(ostream, functionType, TargetLanguage::C_LANG);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareFunctions(std::ostream &ostream) {
    ostream << std::endl;
    cppDeclareInternalSerializationFunction(ostream);
    ostream << std::endl;
    cppDeclareInternalDeserializationFunction(ostream);
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

void StructTypeSpec::cppDeclareInternalSerializationFunction(std::ostream &ostream) {
    ostream << "inline" << " ";
    cppDeclareInternalSerializationFunctionName(ostream, "struct " + namespacePrefix + identifier);
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    cCppFunctionBody(ostream, CDRFunc::SERIALIZE, TargetLanguage::CPP_LANG);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareSerializationFunction(std::ostream &ostream) {
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
    ostream << "toWireType" << "(" << "input" << "," << " " << "output" << ")" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareInternalDeserializationFunction(std::ostream &ostream) {
    ostream << "inline" << " ";
    cppDeclareInternalDeserializationFunctionName(ostream, "struct " + namespacePrefix + identifier);
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "struct" << " " << namespacePrefix << identifier << " " << "retval" << ";" << std::endl;
    if (members.empty()) {
        ostream << "(" << "void" << ")" << " " << "input" << ";" << std::endl;
    } else {
        ostream << "struct" << " " << namespacePrefix << identifier << "*" << " " << "output" << " ";
        ostream << "=" << " " << "&" << "retval" << ";" << std::endl;
        cCppFunctionBody(ostream, CDRFunc::DESERIALIZE, TargetLanguage::CPP_LANG);
    }
    ostream << "return" << " " << "retval" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareDeserializationFunction(std::ostream &ostream) {
    cppDeclareDeserializationFunctionName(ostream, "struct " + namespacePrefix + identifier);
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "const" << " " << "struct" << " " << namespacePrefix << identifier << "_wire" << "*";
    ostream << " " << "input" << " " << "=" << " ";
    ostream << "(" << "const" << " " << "struct" << " " << namespacePrefix << identifier << "_wire" << "*" << ")";
    ostream << " " << "buf" << "." << "data" << "(" << ")" << ";" << std::endl;
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
    ostream << "return" << " " << "fromWireType" << "(" << "input" << ")"  << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cDeclareAnnotationValidate(std::ostream &ostream) {
    std::string funcname = identifier;
    transform(funcname.begin(), funcname.end(), funcname.begin(), ::tolower);
    ostream << std::endl;
    ostream << "int" << " ";
    ostream << "validate";
    ostream << "_" << funcname << "(";
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
    std::string funcname = identifier;
    transform(funcname.begin(), funcname.end(), funcname.begin(), ::tolower);
    ostream << std::endl;
    ostream << "void" << " ";
    ostream << "transform";
    ostream << "_" << funcname << "(";
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
