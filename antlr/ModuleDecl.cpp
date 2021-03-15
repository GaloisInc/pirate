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

#include "ModuleDecl.hpp"
#include "indent_facet.hpp"

void ModuleDecl::addDefinition(TypeSpec* definition) {
    definitions.push_back(definition);
}

void ModuleDecl::cTypeDecl(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cTypeDecl(ostream);
    }
}

void ModuleDecl::cppTypeDecl(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cppTypeDecl(ostream);
    }
}

void ModuleDecl::cTypeDeclWire(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cTypeDeclWire(ostream);
    }
}

void ModuleDecl::cppTypeDeclWire(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cppTypeDeclWire(ostream);
    }
}

void ModuleDecl::cDeclareFunctions(std::ostream &ostream, CDRFunc functionType) {
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

void ModuleDecl::cppDeclareFunctions(std::ostream &ostream) {
    for (TypeSpec* definition : definitions) {
        definition->cppDeclareFunctions(ostream);
    }
}

void ModuleDecl::cppDeclareHeader(std::ostream &ostream) {
    ostream << std::endl;
    ostream << "namespace" << " " << identifier << " " << "{" << std::endl;
    ostream << indent_manip::push;
}

void ModuleDecl::cppDeclareFooter(std::ostream &ostream) {
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

ModuleDecl::~ModuleDecl() {
    for (TypeSpec* definition : definitions) {
        delete definition;
    }
}
