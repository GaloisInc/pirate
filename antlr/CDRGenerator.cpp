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
#include "BooleanErrorListener.h"
#include "IDLLexer.h"
#include "IDLParser.h"
#include "CDRBuildTypes.h"
#include "CDRModuleCounter.h"
#include "CDRTypes.h"

using namespace antlr4;

int parse(std::istream &istream, std::ostream &ostream, std::ostream &estream) {
    CDRModuleCounter moduleCounter;
    antlr4::tree::ParseTreeWalker moduleWalker;
    CDRBuildTypes buildTypes;
    ANTLRInputStream input(istream);
    IDLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    IDLParser parser(&tokens);
    BooleanErrorListener errorListener;

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
    moduleDecl->cTypeDeclWire(ostream);
    moduleDecl->cDeclareAsserts(ostream);
    moduleDecl->cDeclareFunctions(ostream, CDRFunc::SERIALIZE);
    moduleDecl->cDeclareFunctions(ostream, CDRFunc::DESERIALIZE);
    if (buildTypes.hasValidateAnnotations()) {
        moduleDecl->cDeclareAnnotationValidate(ostream);
    }
    if (buildTypes.hasTransformAnnotations()) {
        moduleDecl->cDeclareAnnotationTransform(ostream);
    }

    delete topLevelSpec;
    return 0;
}
