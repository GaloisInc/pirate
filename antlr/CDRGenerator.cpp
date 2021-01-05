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

#include <iostream>

#include "antlr4-runtime.h"
#include "IDLLexer.h"
#include "IDLParser.h"
#include "BooleanErrorListener.hpp"
#include "CDRBuildTypes.hpp"
#include "CDRGenerator.hpp"
#include "CDRModuleCounter.hpp"
#include "ModuleDecl.hpp"
#include "DFDLGenerator.hpp"

using namespace antlr4;

static int generate_c(std::ostream &ostream, CDRBuildTypes &buildTypes, bool packed, ModuleDecl *moduleDecl) {
    ostream << "#include <assert.h>" << std::endl;
    ostream << "#include <endian.h>" << std::endl;
    if (buildTypes.hasTransformAnnotations()) {
        ostream << "#include <fenv.h>"   << std::endl;
        ostream << "#include <math.h>"   << std::endl;
    }
    ostream << "#include <stdint.h>" << std::endl;
    ostream << "#include <string.h>" << std::endl;
    ostream << std::endl;
    moduleDecl->cTypeDecl(ostream);
    moduleDecl->cTypeDeclWire(ostream, packed);
    moduleDecl->cDeclareAsserts(ostream, packed);
    moduleDecl->cDeclareFunctions(ostream, CDRFunc::SERIALIZE, packed);
    moduleDecl->cDeclareFunctions(ostream, CDRFunc::DESERIALIZE, packed);
    if (buildTypes.hasValidateAnnotations()) {
        moduleDecl->cDeclareAnnotationValidate(ostream);
    }
    if (buildTypes.hasTransformAnnotations()) {
        moduleDecl->cDeclareAnnotationTransform(ostream);
    }
    return 0;
}

static int generate_cpp(std::ostream &ostream, CDRBuildTypes &buildTypes, bool packed, ModuleDecl *moduleDecl) {
    std::string guardname = "_" + moduleDecl->identifier + "_IDL_CODEGEN_H";
    transform(guardname.begin(), guardname.end(), guardname.begin(), ::toupper);
    ostream << "#ifndef" << " " << guardname << std::endl;
    ostream << "#define" << " " << guardname << std::endl;
    ostream << std::endl;
    ostream << "#include <cassert>"   << std::endl;
    ostream << "#include <cstdint>"   << std::endl;
    ostream << "#include <cstring>"   << std::endl;
    ostream << "#include <stdexcept>" << std::endl;
    ostream << "#include <vector>"    << std::endl;
    ostream << std::endl;
    ostream << "#include <endian.h>"  << std::endl;
    moduleDecl->cppDeclareHeader(ostream);
    moduleDecl->cppTypeDecl(ostream);
    moduleDecl->cppTypeDeclWire(ostream, packed);
    moduleDecl->cppDeclareAsserts(ostream, packed);
    moduleDecl->cppDeclareFooter(ostream);
    cppPirateNamespaceHeader(ostream);
    moduleDecl->cppDeclareFunctions(ostream, packed);
    cppPirateNamespaceFooter(ostream);
    ostream << std::endl;
    ostream << "#endif" << " " << "//" << " " << guardname << std::endl;
    return 0;
}

int parse(std::istream &istream, std::ostream &ostream, std::ostream &estream, target_t target, bool packed) {
    CDRModuleCounter moduleCounter;
    antlr4::tree::ParseTreeWalker moduleWalker;
    CDRBuildTypes buildTypes;
    ANTLRInputStream input(istream);
    IDLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    IDLParser parser(&tokens);
    BooleanErrorListener errorListener;
    int rv;

    lexer.addErrorListener(&errorListener);
    parser.addErrorListener(&errorListener);
    IDLParser::SpecificationContext* specification = parser.specification();

    if (errorListener.hasError()) {
        return 1;
    }

    if (specification->definition().size() != 1) {
        estream << "Expected top-level module definition" << std::endl;
        return 1;
    }

    IDLParser::DefinitionContext* topLevelDef = specification->definition()[0];

    if (topLevelDef->module() == NULL) {
        estream << "Expected top-level module definition" << std::endl;
        return 1;
    }

    moduleWalker.walk(&moduleCounter, topLevelDef);
    if (moduleCounter.getCounter() > 1) {
        estream << "Nested modules are not-yet-implemented" << std::endl;
        return 1;
    }

    TypeSpec* topLevelSpec = buildTypes.visit(topLevelDef);
    if (buildTypes.getErrors().size() > 0) {
        for (std::string errorMsg : buildTypes.getErrors()) {
            estream << errorMsg << std::endl;
        }
        return 1;
    }
    ModuleDecl *moduleDecl = dynamic_cast<ModuleDecl*>(topLevelSpec);

    switch (target) {
        case TargetLanguage::C_LANG:
            rv = generate_c(ostream, buildTypes, packed, moduleDecl);
            break;
        case TargetLanguage::CPP_LANG:
            rv = generate_cpp(ostream, buildTypes, packed, moduleDecl);
            break;
        case TargetLanguage::DFDL_LANG:
            rv = generate_dfdl(ostream, buildTypes, packed, moduleDecl);
            break;
        default:
            estream << "unknown target language " << target << std::endl;
            rv = 1;
            break;
    }

    delete topLevelSpec;
    return rv;
}

std::string target_as_string(TargetLanguage target) {
    switch (target) {
        case C_LANG:
            return "c";
        case CPP_LANG:
            return "cpp";
        case DFDL_LANG:
            return "dfdl";
        case UNKNOWN:
        default:
            return "unknown";
    }
}
