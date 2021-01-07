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
    cCppTypeDecl(ostream, false);
}

void StructTypeSpec::cppTypeDecl(std::ostream &ostream) {
    cCppTypeDecl(ostream, true);
}

void StructTypeSpec::cCppTypeDecl(std::ostream &ostream, bool cpp) {
    ostream << std::endl;
    ostream << "struct" << " " << identifier << " " << "{" << std::endl;
    ostream << indent_manip::push;
    for (StructMember* member : members) {
        for (Declarator* declarator : member->declarators) {
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
    }
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void StructTypeSpec::cTypeDeclWire(std::ostream &ostream, bool packed) {
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

void StructTypeSpec::cDeclareAsserts(std::ostream &ostream, bool packed) {
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
            if (((declarator->dimensions.size() == 0) && scalar) ||
                ((declarator->dimensions.size() > 0) && array)) {
                apply(member, declarator);
            }
        }
    }
}

void StructTypeSpec::cCppFunctionBody(std::ostream &ostream, CDRFunc functionType, bool packed) {
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
}

void StructTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType, bool packed) {
    ostream << std::endl;
    cDeclareFunctionName(ostream, functionType, identifier);
    ostream << indent_manip::push;
    cCppFunctionBody(ostream, functionType, packed);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareFunctions(std::ostream &ostream, bool packed) {
    ostream << std::endl;
    ostream << "template" << "<" << ">" << std::endl;
    ostream << "struct" << " " << "Serialization";
    ostream << "<" << "struct" << " " << namespacePrefix << identifier << ">" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    cppDeclareSerializationFunction(ostream, packed);
    ostream << std::endl;
    cppDeclareDeserializationFunction(ostream, packed);
    ostream << indent_manip::pop;
    ostream << "}" << ";" << std::endl;
}

void StructTypeSpec::cppDeclareSerializationFunction(std::ostream &ostream, bool packed) {
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
    cCppFunctionBody(ostream, CDRFunc::SERIALIZE, packed);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareDeserializationFunction(std::ostream &ostream, bool packed) {
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
    cCppFunctionBody(ostream, CDRFunc::DESERIALIZE, packed);
    ostream << "return" << " " << "retval" << ";" << std::endl;
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
