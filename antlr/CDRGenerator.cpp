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

using namespace std;
using namespace antlr4;

int main(int argc, const char* argv[]) {
    CDRModuleCounter moduleCounter;
    antlr4::tree::ParseTreeWalker moduleWalker;
    CDRBuildTypes buildTypes;
    ANTLRInputStream input(cin);
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
        cerr << "Expected top-level module definition" << endl;
        return 1;
    }

    IDLParser::DefinitionContext* topLevelDef = specification->definition()[0];

    if (topLevelDef->module() == NULL) {
        cerr << "Expected top-level module definition" << endl;
        return 1;
    }

    moduleWalker.walk(&moduleCounter, topLevelDef);
    if (moduleCounter.getCounter() > 1) {
        cerr << "Nested modules are not-yet-implemented" << endl;
        return 1;
    }

    TypeSpec* topLevelSpec = buildTypes.visit(topLevelDef);
    ModuleDecl *moduleDecl = dynamic_cast<ModuleDecl*>(topLevelSpec);

    cout << moduleDecl->cType() << endl;

    return 0;
}
