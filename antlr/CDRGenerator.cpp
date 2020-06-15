#include <iostream>

#include "antlr4-runtime.h"
#include "BooleanErrorListener.h"
#include "IDLLexer.h"
#include "IDLParser.h"

using namespace std;
using namespace antlr4;

int main(int argc, const char* argv[]) {
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

    return 0;
}
