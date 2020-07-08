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

void StructTypeSpec::cCppFunctionBody(std::ostream &ostream, CDRFunc functionType) {
    cDeclareFunctionApply(true, true, [&ostream] (StructMember* member, Declarator* declarator)
        { cDeclareLocalVar(ostream, member->typeSpec, "field_" + declarator->identifier); });
    cDeclareFunctionApply(false, true, [&ostream, functionType] (StructMember* member, Declarator* declarator)
        { cConvertByteOrderArray(ostream, member->typeSpec, declarator, functionType, "field_", ""); });
    cDeclareFunctionApply(true, false, [&ostream] (StructMember* member, Declarator* declarator)
        { cCopyMemoryIn(ostream, member->typeSpec, "field_" + declarator->identifier, declarator->identifier); });
    cDeclareFunctionApply(true, false, [&ostream, functionType] (StructMember* member, Declarator* declarator)
        { cConvertByteOrder(ostream, member->typeSpec, "field_" + declarator->identifier, functionType); });
    cDeclareFunctionApply(true, false, [&ostream] (StructMember* member, Declarator* declarator)
        { cCopyMemoryOut(ostream, member->typeSpec, "field_" + declarator->identifier, declarator->identifier); });
}

void StructTypeSpec::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
    ostream << std::endl;
    cDeclareFunctionName(ostream, functionType, identifier);
    ostream << indent_manip::push;
    cCppFunctionBody(ostream, functionType);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareFunctions(std::ostream &ostream) {
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

void StructTypeSpec::cppDeclareSerializationFunction(std::ostream &ostream) {
    cppDeclareSerializationFunctionName(ostream, "struct " + namespacePrefix + identifier);
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "buf" << "->" << "resize" << "(";
    ostream << "sizeof(" << "struct" << " " << namespacePrefix << identifier << ")";
    ostream << ")" << ";" << std::endl;
    ostream << "struct" << " " << namespacePrefix << identifier << "_wire" << "*";
    ostream << " " << "output" << " " << "=" << " ";
    ostream << "(" << "struct" << " " << namespacePrefix << identifier << "_wire" << "*" << ")" << " ";
    ostream << "buf" << "->" << "data" << "(" << ")" << ";" << std::endl;
    ostream << "const" << " " << "struct" << " " << namespacePrefix << identifier << "*" << " " << "input" << " ";
    ostream << "=" << " " << "&" << "val" << ";" << std::endl;
    cCppFunctionBody(ostream, CDRFunc::SERIALIZE);
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void StructTypeSpec::cppDeclareDeserializationFunction(std::ostream &ostream) {
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
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
    cCppFunctionBody(ostream, CDRFunc::DESERIALIZE);
    ostream << "return" << " " << "retval" << ";" << std::endl;
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