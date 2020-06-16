#include <iostream>

#include "antlr4-runtime.h"
#include "BooleanErrorListener.h"
#include "IDLLexer.h"
#include "IDLParser.h"
#include "CDRDeclareTypes.h"
#include "CDRModuleCounter.h"

using namespace std;
using namespace antlr4;

int main(int argc, const char* argv[]) {
    CDRModuleCounter moduleCounter;
    antlr4::tree::ParseTreeWalker moduleWalker;
    CDRDeclareTypes declareTypes;
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

    declareTypes.visit(topLevelDef);

    return 0;
}
