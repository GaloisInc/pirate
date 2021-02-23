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
 * Copyright 2020 Galois, All rights reserved.
 */

#include "DFDLGenerator.hpp"
#include "StructTypeSpec.hpp"
#include "UnionTypeSpec.hpp"

#include <ticpp.h>
#include <iostream>

using namespace ticpp;

namespace {

const std::string license_header = "  Licensed to the Apache Software Foundation (ASF) under one or more\n\
  contributor license agreements.  See the NOTICE file distributed with\n\
  this work for additional information regarding copyright ownership.\n\
  The ASF licenses this file to You under the Apache License, Version 2.0\n\
  (the \"License\"); you may not use this file except in compliance with\n\
  the License.  You may obtain a copy of the License at\n\
\n\
      http://www.apache.org/licenses/LICENSE-2.0\n\
\n\
  Unless required by applicable law or agreed to in writing, software\n\
  distributed under the License is distributed on an \"AS IS\" BASIS,\n\
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
  See the License for the specific language governing permissions and\n\
  limitations under the License.";

[[noreturn]] void
not_implemented(char const* err)
{
    std::cerr << err << std::endl;
    exit(1);
}

Element*
add_element(Node *parent, char const* name)
{
    return parent->InsertEndChild(Element(name))->ToElement();
}

void
set_defaults(Element* defaults, bool packed)
{
    if (packed) {
        defaults->SetAttribute("alignment", "1");
    }
    defaults->SetAttribute("alignmentUnits", "bytes");
    defaults->SetAttribute("binaryBooleanFalseRep", "0");
    defaults->SetAttribute("binaryBooleanTrueRep", "1");
    defaults->SetAttribute("binaryFloatRep", "ieee");
    defaults->SetAttribute("binaryNumberCheckPolicy", "lax");
    defaults->SetAttribute("binaryNumberRep", "binary");
    defaults->SetAttribute("bitOrder", "mostSignificantBitFirst");
    defaults->SetAttribute("byteOrder", "bigEndian");
    defaults->SetAttribute("choiceLengthKind", "implicit");
    defaults->SetAttribute("encoding", "utf-8");
    defaults->SetAttribute("encodingErrorPolicy", "replace");
    defaults->SetAttribute("escapeSchemeRef", "");
    defaults->SetAttribute("fillByte", "%NUL;");
    defaults->SetAttribute("floating", "no");
    defaults->SetAttribute("ignoreCase", "no");
    defaults->SetAttribute("initiatedContent", "no");
    defaults->SetAttribute("initiator", "");
    defaults->SetAttribute("leadingSkip", "0");
    defaults->SetAttribute("lengthKind", "implicit");
    defaults->SetAttribute("lengthUnits", "bytes");
    defaults->SetAttribute("occursCountKind", "implicit");
    defaults->SetAttribute("prefixIncludesPrefixLength", "no");
    defaults->SetAttribute("representation", "binary");
    defaults->SetAttribute("separator", "");
    defaults->SetAttribute("separatorPosition", "infix");
    defaults->SetAttribute("sequenceKind", "ordered");
    defaults->SetAttribute("terminator", "");
    defaults->SetAttribute("textBidi", "no");
    defaults->SetAttribute("textPadKind", "none");
    defaults->SetAttribute("trailingSkip", "0");
    defaults->SetAttribute("truncateSpecifiedLengthString", "no");
}

void
add_primitive_type(
    Element* schema,
    char const* name,
    char const* type,
    char const* size,
    bool packed
) {
    auto simpleType = schema->InsertEndChild(Element("xs:simpleType"))->ToElement();
    simpleType->SetAttribute("name", name);
    simpleType->SetAttribute("dfdl:length", size);
    simpleType->SetAttribute("dfdl:lengthKind", "explicit");
    if (!packed) {
        simpleType->SetAttribute("dfdl:alignment", size);
    }
    add_element(simpleType, "xs:restriction")->SetAttribute("base", type);
}

void
add_primitive_types(Element* schema, bool packed)
{
    add_primitive_type(schema, "float", "xs:float", "4", packed);
    add_primitive_type(schema, "double", "xs:double", "8", packed);
    add_primitive_type(schema, "int8", "xs:byte", "1", packed);
    add_primitive_type(schema, "int16", "xs:short", "2", packed);
    add_primitive_type(schema, "int32", "xs:int", "4", packed);
    add_primitive_type(schema, "int64", "xs:long", "8", packed);
    add_primitive_type(schema, "uint8", "xs:unsignedByte", "1", packed);
    add_primitive_type(schema, "uint16", "xs:unsignedShort", "2", packed);
    add_primitive_type(schema, "uint32", "xs:unsignedInt", "4", packed);
    add_primitive_type(schema, "uint64", "xs:unsignedLong", "8", packed);
    add_primitive_type(schema, "boolean", "xs:boolean", "1", packed);
}

int
type_size(TypeSpec* type)
{
    if (auto * typeref = dynamic_cast<TypeReference*>(type)) {
        return type_size(typeref->child);
    }

    switch (type->typeOf()) {
        case CDRTypeOf::ENUM_T: return 4;
        case CDRTypeOf::LONG_DOUBLE_T: not_implemented("type_size of enum");
        case CDRTypeOf::MODULE_T: not_implemented("type_size of module");
        case CDRTypeOf::ERROR_T: not_implemented("type_size of error");
        case CDRTypeOf::FLOAT_T: return 4;
        case CDRTypeOf::DOUBLE_T: return 8;
        case CDRTypeOf::TINY_T: return 1;
        case CDRTypeOf::SHORT_T: return 2;
        case CDRTypeOf::LONG_T: return 4;
        case CDRTypeOf::LONG_LONG_T: return 8;
        case CDRTypeOf::UNSIGNED_TINY_T: return 1;
        case CDRTypeOf::UNSIGNED_SHORT_T: return 2;
        case CDRTypeOf::UNSIGNED_LONG_T: return 4;
        case CDRTypeOf::UNSIGNED_LONG_LONG_T: return 8;
        case CDRTypeOf::CHAR_T: return 1;
        case CDRTypeOf::BOOL_T: return 1;
        case CDRTypeOf::OCTET_T: return 1;
        case CDRTypeOf::UNION_T:
        {
            auto u = static_cast<UnionTypeSpec*>(type);
            int acc = 1;
            for (auto m : u->members) {
                acc = std::max(acc, type_size(m->typeSpec));
            }
            return acc;
        }
        case CDRTypeOf::STRUCT_T:
        {
            auto s = static_cast<StructTypeSpec*>(type);
            int acc = 0;
            for (auto m : s->members) {
                for (auto d : m->declarators) {
                    int sz = type_size(m->typeSpec);
                    for (auto dim : d->dimensions) {
                        sz *= dim;
                    }
                    acc += sz;
                }
            }
            return acc;
        }
        default: not_implemented("type_size of unspecified type");
    }
}

int
type_alignment(TypeSpec* type)
{
    if (auto * typeref = dynamic_cast<TypeReference*>(type)) {
        return type_alignment(typeref->child);
    }

    switch (type->typeOf()) {
        case CDRTypeOf::ENUM_T: return 4;
        case CDRTypeOf::LONG_DOUBLE_T: not_implemented("type_alignment of enum");
        case CDRTypeOf::MODULE_T: not_implemented("type_alignment of module");
        case CDRTypeOf::ERROR_T: not_implemented("type_alignment of error");
        case CDRTypeOf::FLOAT_T: return 4;
        case CDRTypeOf::DOUBLE_T: return 8;
        case CDRTypeOf::TINY_T: return 1;
        case CDRTypeOf::SHORT_T: return 2;
        case CDRTypeOf::LONG_T: return 4;
        case CDRTypeOf::LONG_LONG_T: return 8;
        case CDRTypeOf::UNSIGNED_TINY_T: return 1;
        case CDRTypeOf::UNSIGNED_SHORT_T: return 2;
        case CDRTypeOf::UNSIGNED_LONG_T: return 4;
        case CDRTypeOf::UNSIGNED_LONG_LONG_T: return 8;
        case CDRTypeOf::CHAR_T: return 1;
        case CDRTypeOf::BOOL_T: return 1;
        case CDRTypeOf::OCTET_T: return 1;
        case CDRTypeOf::UNION_T:
        {
            auto u = static_cast<UnionTypeSpec*>(type);
            int acc = 8;
            for (auto m : u->members) {
                acc = std::max(acc, type_alignment(m->typeSpec));
            }
            return acc;
        }
        case CDRTypeOf::STRUCT_T:
        {
            auto s = static_cast<StructTypeSpec*>(type);
            if (s->members.empty()) {
                return 8;
            } else {
                return type_alignment(s->members[0]->typeSpec);
            }
        }
        default: not_implemented("type_alignment of unspecified type");
    }
}

std::string
get_type_name(TypeSpec* typeSpec)
{
    if (auto * p = dynamic_cast<StructTypeSpec*>(typeSpec)) {
        return p->identifier;
    } else if (auto * p = dynamic_cast<UnionTypeSpec*>(typeSpec)) {
        return p->identifier;
    } else if (auto * p = dynamic_cast<TypeReference*>(typeSpec)) {
        return get_type_name(p->child);
    } else if (auto * p = dynamic_cast<EnumTypeSpec*>(typeSpec)) {
        return p->identifier;
    } else if (NULL != dynamic_cast<BaseTypeSpec*>(typeSpec)) {
        not_implemented("get_type_name of base type");
    } else {
        not_implemented("get_type_name of unknown type");
    }
}

void construct_member(Element* e, Declarator* decl, TypeSpec* type, bool packed);

std::string
case_label(TypeSpec* typeSpec, std::string const& label)
{
    while (auto * ref = dynamic_cast<TypeReference*>(typeSpec)) {
        typeSpec = ref->child;
    }

    if (typeSpec->typeOf() == CDRTypeOf::ENUM_T) {
        auto colon = label.find_last_of(':');
        
        // Strip away namespace
        std::string label_
          = std::string::npos == colon
          ? label
          : label.substr(colon+1);

        auto* u = static_cast<EnumTypeSpec*>(typeSpec);
        for (size_t i = 0; i < u->enumerators.size(); i++) {
            if (u->enumerators[i] == label_) {
                return std::to_string(i);
            }
        }
    }
    return label;
}

std::string
construct_type(TypeSpec* typeSpec)
{
    switch (typeSpec->typeOf()) {
        case CDRTypeOf::MODULE_T:
            not_implemented("construct_type of module");
        case CDRTypeOf::STRUCT_T:
            not_implemented("construct_type of struct");
        case CDRTypeOf::UNION_T:
            not_implemented("construct_type of union");
        case CDRTypeOf::ENUM_T:
            not_implemented("construct_type of enum");
        case CDRTypeOf::LONG_DOUBLE_T:
            not_implemented("construct_type of long double");
        case CDRTypeOf::ERROR_T:
            not_implemented("construct_type of error");
        case CDRTypeOf::FLOAT_T:
            return "idl:float";
        case CDRTypeOf::DOUBLE_T:
            return "idl:double";
        case CDRTypeOf::TINY_T:
            return "idl:int8";
        case CDRTypeOf::SHORT_T:
            return "idl:int16";
        case CDRTypeOf::LONG_T:
            return "idl:int32";
        case CDRTypeOf::LONG_LONG_T:
            return "idl:int64";
        case CDRTypeOf::UNSIGNED_TINY_T:
            return "idl:uint8";
        case CDRTypeOf::UNSIGNED_SHORT_T:
            return "idl:uint16";
        case CDRTypeOf::UNSIGNED_LONG_T:
            return "idl:uint32";
        case CDRTypeOf::UNSIGNED_LONG_LONG_T:
            return "idl:uint64";
        case CDRTypeOf::CHAR_T:
            return "idl:int8";
        case CDRTypeOf::BOOL_T:
            return "idl:boolean";
        case CDRTypeOf::OCTET_T:
            return "idl:uint8";
        default:
            not_implemented("construct_type of unknown type");
    }
}

void
finish_inner_type(Element* e, Declarator* decl, TypeSpec* typeSpec, bool packed)
{
    bool nestedType = false;
    if (auto * typeref = dynamic_cast<TypeReference*>(typeSpec)) {
        e->SetAttribute("type", "idl:" + get_type_name(typeref));
        return;
    }
    switch (typeSpec->typeOf()) {
        case CDRTypeOf::STRUCT_T:
            not_implemented("finish_inner_type of struct");
            break;
        case CDRTypeOf::UNION_T:
            not_implemented("finish_inner_type of union");
            break;
        case CDRTypeOf::ENUM_T:
            not_implemented("finish_inner_type of enum");
            break;
        default:
            break;
    }
    if (decl != nullptr) {
        for (auto ann : decl->annotations) {
            if (dynamic_cast<ValueAnnotation*>(ann) == nullptr) {
                nestedType = true;
            }
        }
    }
    if (!nestedType) {
        e->SetAttribute("type", construct_type(typeSpec));
    } else {
        auto st = add_element(e, "xs:simpleType");
        auto restrict = add_element(st, "xs:restriction");
        restrict->SetAttribute("base", construct_type(typeSpec));
        if (decl != nullptr) {
            for (auto ann : decl->annotations) {
                if (auto * rangeAnn = dynamic_cast<RangeAnnotation*>(ann)) {
                    add_element(restrict, "xs:minInclusive")->SetAttribute("value", rangeAnn->min);
                    add_element(restrict, "xs:maxInclusive")->SetAttribute("value", rangeAnn->max);
                }
            }
        }
    }
}


void
declare_top_type(Element* schema, TypeSpec* typeSpec, bool packed)
{
    switch (typeSpec->typeOf()) {
        case CDRTypeOf::STRUCT_T:
        {
            auto s = static_cast<StructTypeSpec const*>(typeSpec);

            auto complex = add_element(schema, "xs:complexType");
            complex->SetAttribute("name", get_type_name(typeSpec));
            auto sequence = add_element(complex, "xs:sequence");

            for (auto m : s->members) {
                for (auto d : m->declarators) {
                    auto member = add_element(sequence, "xs:element");
                    member->SetAttribute("name", d->identifier);

                    construct_member(member, d, m->typeSpec, packed);
                }
            }
            auto elem = add_element(schema, "xs:element");
            elem->SetAttribute("name", s->identifier + "Decl");
            elem->SetAttribute("type", "idl:" + get_type_name(typeSpec));
            break;
        }
        case CDRTypeOf::UNION_T:
        {
            auto u = static_cast<UnionTypeSpec*>(typeSpec);

            auto complex = add_element(schema, "xs:complexType");
            complex->SetAttribute("name", get_type_name(typeSpec));
            auto sequence = add_element(complex, "xs:sequence");

            auto tag = add_element(sequence, "xs:element");
            tag->SetAttribute("name", "tag");
            finish_inner_type(tag, nullptr, u->switchType, packed);

            auto data = add_element(sequence, "xs:element");
            data->SetAttribute("name", "data");
            data->SetAttribute("dfdl:length", type_size(u));
            data->SetAttribute("dfdl:lengthKind", "explicit");
            if (!packed) {
                data->SetAttribute("dfdl:alignment", type_alignment(u));
            }

            auto unionelement = add_element(data, "xs:complexType");
            auto unionchoice = add_element(unionelement, "xs:choice");
            unionchoice->SetAttribute("dfdl:choiceDispatchKey", "{xs:string(../tag)}");

            for (auto m : u->members) {
                if (m->labels.empty()) continue;

                std::string labels;
                for (auto const& l : m->labels) {
                    labels += case_label(u->switchType, l);
                    labels += " ";
                }
                labels.pop_back(); // remove trailing space

                auto member = add_element(unionchoice, "xs:element");
                member->SetAttribute("dfdl:choiceBranchKey", labels);
                member->SetAttribute("name", m->declarator->identifier);

                construct_member(member, m->declarator, m->typeSpec, packed);
            }
            auto elem = add_element(schema, "xs:element");
            elem->SetAttribute("name", u->identifier + "Decl");
            elem->SetAttribute("type", "idl:" + get_type_name(typeSpec));
            break;
        }
        case CDRTypeOf::ENUM_T:
        {
            auto simple = add_element(schema, "xs:simpleType");
            simple->SetAttribute("name", get_type_name(typeSpec));
            auto rest = add_element(simple, "xs:restriction");
            rest->SetAttribute("base", "idl:uint32");
            // should we declare an element for this enum?
            // auto ets = static_cast<EnumTypeSpec*>(typeSpec);
            // auto elem = add_element(schema, "xs:element");
            // elem->SetAttribute("name", ets->identifier + "Decl");
            // elem->SetAttribute("type", "idl:" + get_type_name(typeSpec));
            break;
        }
        default:
            not_implemented("declare_top_type on inner type");
            break;
    }


}

void construct_member(Element* e, Declarator* decl, TypeSpec* type, bool packed)
{
    for (auto dim : decl->dimensions) {
        auto complexType = add_element(e, "xs:complexType");
        auto sequence = add_element(complexType, "xs:sequence");
        e = add_element(sequence, "xs:element");
        e->SetAttribute("name", "item");
        e->SetAttribute("minOccurs", dim);
        e->SetAttribute("maxOccurs", dim);
        e->SetAttribute("dfdl:occursCountKind", "fixed");
    }
    for (auto ann : decl->annotations) {
        if (auto * valueAnn = dynamic_cast<ValueAnnotation*>(ann)) {
            e->SetAttribute("fixed", valueAnn->value);
        }
    }

    finish_inner_type(e, decl, type, packed);
}

}

int generate_dfdl(
    std::ostream &ostream,
    CDRBuildTypes const& buildTypes,
    ModuleDecl const* moduleDecl)
{
    Document doc;
    auto schema = add_element(&doc, "xs:schema");

    schema->SetAttribute("xmlns:xs", "http://www.w3.org/2001/XMLSchema");
    schema->SetAttribute("xmlns:dfdl", "http://www.ogf.org/dfdl/dfdl-1.0/");
    schema->SetAttribute("xmlns:idl", "urn:idl:1.0");
    schema->SetAttribute("targetNamespace", "urn:idl:1.0");

    auto annotation = add_element(schema, "xs:annotation");
    
    auto appinfo = add_element(annotation, "xs:appinfo");
    appinfo->SetAttribute("source", "http://www.ogf.org/dfdl/");

    auto defineFormat = add_element(appinfo, "dfdl:defineFormat");
    defineFormat->SetAttribute("name", "defaults");

    auto defaults = add_element(defineFormat, "dfdl:format");
    set_defaults(defaults, moduleDecl->packed);

    auto format = add_element(appinfo, "dfdl:format");
    format->SetAttribute("ref", "idl:defaults");

    add_primitive_types(schema, moduleDecl->packed);

    for (auto * def : moduleDecl->definitions) {
        declare_top_type(schema, def, moduleDecl->packed);
    }
    ostream << "<!--" << std::endl;
    ostream << license_header << std::endl;
    ostream << "-->" << std::endl;
    ostream << doc;
    return 0;
}
