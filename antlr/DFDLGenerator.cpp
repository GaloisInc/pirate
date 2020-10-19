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
set_defaults(Element* defaults)
{
    defaults->SetAttribute("alignment", "1");
    defaults->SetAttribute("alignmentUnits", "bits");
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
    defaults->SetAttribute("floating", "no");
    defaults->SetAttribute("ignoreCase", "no");
    defaults->SetAttribute("initiatedContent", "no");
    defaults->SetAttribute("initiator", "");
    defaults->SetAttribute("leadingSkip", "0");
    defaults->SetAttribute("lengthKind", "implicit");
    defaults->SetAttribute("lengthUnits", "bits");
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
    char const* size
) {
    auto simpleType = schema->InsertEndChild(Element("xs:simpleType"))->ToElement();
    simpleType->SetAttribute("name", name);
    simpleType->SetAttribute("dfdl:length", size);
    simpleType->SetAttribute("dfdl:lengthKind", "explicit");
    simpleType->SetAttribute("dfdl:alignment", size);
    add_element(simpleType, "xs:restriction")->SetAttribute("base", type);
}

void
add_primitive_types(Element* schema)
{
    add_primitive_type(schema, "float", "xs:float", "32");
    add_primitive_type(schema, "double", "xs:double", "64");
    add_primitive_type(schema, "int8", "xs:integer", "8");
    add_primitive_type(schema, "int16", "xs:integer", "16");
    add_primitive_type(schema, "int32", "xs:integer", "32");
    add_primitive_type(schema, "int64", "xs:integer", "64");
    add_primitive_type(schema, "uint8", "xs:nonNegativeInteger", "8");
    add_primitive_type(schema, "uint16", "xs:nonNegativeInteger", "16");
    add_primitive_type(schema, "uint32", "xs:nonNegativeInteger", "32");
    add_primitive_type(schema, "uint64", "xs:nonNegativeInteger", "64");
    add_primitive_type(schema, "boolean", "xs:boolean", "8");
}

int
type_size(TypeSpec* type)
{
    if (auto * typeref = dynamic_cast<TypeReference*>(type)) {
        return type_size(typeref->child);
    }

    switch (type->typeOf()) {
        case CDRTypeOf::ENUM_T: not_implemented("type_size of enum");
        case CDRTypeOf::LONG_DOUBLE_T: not_implemented("type_size of enum");
        case CDRTypeOf::MODULE_T: not_implemented("type_size of module");
        case CDRTypeOf::ERROR_T: not_implemented("type_size of error");
        case CDRTypeOf::FLOAT_T: return 32;
        case CDRTypeOf::DOUBLE_T: return 64;
        case CDRTypeOf::TINY_T: return 8;
        case CDRTypeOf::SHORT_T: return 16;
        case CDRTypeOf::LONG_T: return 32;
        case CDRTypeOf::LONG_LONG_T: return 64;
        case CDRTypeOf::UNSIGNED_TINY_T: return 8;
        case CDRTypeOf::UNSIGNED_SHORT_T: return 16;
        case CDRTypeOf::UNSIGNED_LONG_T: return 32;
        case CDRTypeOf::UNSIGNED_LONG_LONG_T: return 64;
        case CDRTypeOf::CHAR_T: return 8;
        case CDRTypeOf::BOOL_T: return 8;
        case CDRTypeOf::OCTET_T: return 8;
        case CDRTypeOf::UNION_T:
        {
            auto u = static_cast<UnionTypeSpec*>(type);
            int acc = 8;
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
                acc += type_size(m->typeSpec) * m->declarators.size();
            }
            return acc;
        }
    }
}

int
type_alignment(TypeSpec* type)
{
    if (auto * typeref = dynamic_cast<TypeReference*>(type)) {
        return type_alignment(typeref->child);
    }

    switch (type->typeOf()) {
        case CDRTypeOf::ENUM_T: not_implemented("type_alignment of enum");
        case CDRTypeOf::LONG_DOUBLE_T: not_implemented("type_alignment of enum");
        case CDRTypeOf::MODULE_T: not_implemented("type_alignment of module");
        case CDRTypeOf::ERROR_T: not_implemented("type_alignment of error");
        case CDRTypeOf::FLOAT_T: return 32;
        case CDRTypeOf::DOUBLE_T: return 64;
        case CDRTypeOf::TINY_T: return 8;
        case CDRTypeOf::SHORT_T: return 16;
        case CDRTypeOf::LONG_T: return 32;
        case CDRTypeOf::LONG_LONG_T: return 64;
        case CDRTypeOf::UNSIGNED_TINY_T: return 8;
        case CDRTypeOf::UNSIGNED_SHORT_T: return 16;
        case CDRTypeOf::UNSIGNED_LONG_T: return 32;
        case CDRTypeOf::UNSIGNED_LONG_LONG_T: return 64;
        case CDRTypeOf::CHAR_T: return 8;
        case CDRTypeOf::BOOL_T: return 8;
        case CDRTypeOf::OCTET_T: return 8;
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
    }
}

std::string
get_type_name(TypeSpec* typeSpec)
{
    if (auto * p = dynamic_cast<StructTypeSpec*>(typeSpec)) {
        return p->identifier;
    } else if (auto * p = dynamic_cast<UnionTypeSpec*>(typeSpec)) {
        return p->identifier;
    } else {
        not_implemented("get_type_name of non union/struct");
    }
}

void construct_array(Element* e, std::vector<int> const& dimensions, TypeSpec* type);

void
finish_type(Element* e, TypeSpec* typeSpec)
{
    if (auto * typeref = dynamic_cast<TypeReference*>(typeSpec)) {
        std::string ref = "idl:";
        ref += get_type_name(typeref->child);
        e->SetAttribute("ref", ref);
        e->RemoveAttribute("name");
        return;
    }

    switch (typeSpec->typeOf()) {
        case CDRTypeOf::MODULE_T:
            not_implemented("finish_type of module");
        case CDRTypeOf::ENUM_T:
            not_implemented("finish_type of enum");
        case CDRTypeOf::LONG_DOUBLE_T:
            not_implemented("finish_type of long double");
        case CDRTypeOf::ERROR_T:
            not_implemented("finish_type of error");
        case CDRTypeOf::FLOAT_T:
            e->SetAttribute("type", "idl:float");
            return;
        case CDRTypeOf::DOUBLE_T:
            e->SetAttribute("type", "idl:double");
            return;
        case CDRTypeOf::TINY_T:
            e->SetAttribute("type", "idl:int8");
            return;
        case CDRTypeOf::SHORT_T:
            e->SetAttribute("type", "idl:int16");
            return;
        case CDRTypeOf::LONG_T:
            e->SetAttribute("type", "idl:int32");
            return;
        case CDRTypeOf::LONG_LONG_T:
            e->SetAttribute("type", "idl:int64");
            return;
        case CDRTypeOf::UNSIGNED_TINY_T:
            e->SetAttribute("type", "idl:uint8");
            return;
        case CDRTypeOf::UNSIGNED_SHORT_T:
            e->SetAttribute("type", "idl:uint16");
            return;
        case CDRTypeOf::UNSIGNED_LONG_T:
            e->SetAttribute("type", "idl:uint32");
            return;
        case CDRTypeOf::UNSIGNED_LONG_LONG_T: 
            e->SetAttribute("type", "dl:uint64");
            return;
        case CDRTypeOf::CHAR_T:
            e->SetAttribute("type", "idl:int8");
            return;
        case CDRTypeOf::BOOL_T:
            e->SetAttribute("type", "idl:boolean");
            return;
        case CDRTypeOf::OCTET_T:
            e->SetAttribute("type", "idl:uint8");
            return;
        case CDRTypeOf::STRUCT_T:
        {
            auto s = static_cast<StructTypeSpec const*>(typeSpec);

            auto complex = add_element(e, "xs:complexType");
            auto sequence = add_element(complex, "xs:sequence");

            for (auto m : s->members) {
                for (auto d : m->declarators) {
                    auto member = add_element(sequence, "xs:element");
                    member->SetAttribute("name", d->identifier);

                    construct_array(member, d->dimensions, m->typeSpec);
                }
            }
            return;
        }
        case CDRTypeOf::UNION_T:
        {
            auto u = static_cast<UnionTypeSpec*>(typeSpec);

            auto complex = add_element(e, "xs:complexType");
            auto sequence = add_element(complex, "xs:sequence");

            auto tag = add_element(sequence, "xs:element");
            tag->SetAttribute("name", "tag");
            finish_type(tag, u->switchType);

            auto data = add_element(sequence, "xs:element");
            data->SetAttribute("name", "data");
            data->SetAttribute("dfdl:length", type_size(u));
            data->SetAttribute("dfdl:lengthKind", "explicit");
            data->SetAttribute("dfdl:alignment", type_alignment(u));

            auto unionelement = add_element(data, "xs:complexType");
            auto unionchoice = add_element(unionelement, "xs:choice");
            unionchoice->SetAttribute("dfdl:choiceDispatchKey", "{xs:string(../tag)}");

            for (auto m : u->members) {
                auto member = add_element(unionchoice, "xs:element");
                member->SetAttribute("dfdl:choiceBranchKey", m->labels[0]);
                member->SetAttribute("name", m->declarator->identifier);

                construct_array(member, m->declarator->dimensions, m->typeSpec);
            }
            return;
        }
    }
}

void construct_array(Element* e, std::vector<int> const& dimensions, TypeSpec* type)
{
    for (auto d : dimensions) {
        auto complexType = add_element(e, "xs:complexType");
        auto sequence = add_element(complexType, "xs:sequence");
        e = add_element(sequence, "xs:element");
        e->SetAttribute("name", "item");
        e->SetAttribute("minOccurs", d);
        e->SetAttribute("maxOccurs", d);
        e->SetAttribute("dfdl:occursCountKind", "fixed");
    }

    finish_type(e, type);
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
    set_defaults(defaults);

    auto format = add_element(appinfo, "dfdl:format");
    format->SetAttribute("ref", "idl:defaults");

    add_primitive_types(schema);


    for (auto * def : moduleDecl->definitions) {
        auto name = get_type_name(def);
        auto element = add_element(schema, "xs:element");
        element->SetAttribute("name", get_type_name(def));
        finish_type(element, def);
    }

    ostream << doc;
    return 0;
}
