#include "antlr4-runtime.h"
#include "CDRDeclareTypes.h"

#include <algorithm>

using namespace std;

antlrcpp::Any CDRDeclareTypes::visitStruct_type(IDLParser::Struct_typeContext *ctx) {
  string name = ctx->identifier()->getText();
  transform(name.begin(), name.end(), name.begin(), ::tolower);
  cout << "struct " << name << " {" << endl;
  visitChildren(ctx);
  cout << "};" << endl;
  return nullptr;
}

antlrcpp::Any CDRDeclareTypes::visitFloating_pt_type(IDLParser::Floating_pt_typeContext *ctx) {
  cout << ctx->getText();
  if (ctx->KW_LONG() && ctx->KW_DOUBLE()) {
    return antlrcpp::Any(16);
  } else if (ctx->KW_DOUBLE()) {
    return antlrcpp::Any(8);
  } else if (ctx->KW_FLOAT()) {
    return antlrcpp::Any(4);
  }
  return nullptr;
}

antlrcpp::Any CDRDeclareTypes::visitMember(IDLParser::MemberContext *ctx) {
  antlrcpp::Any align = this->visit(ctx->type_spec());
  this->visit(ctx->declarators());
  if (align.isNotNull()) {
    // TODO: is this correct for array declarations?
    int alignVal = align;
    cout << " __attribute__((aligned(" << alignVal << ")))";
  }
  cout << ";" << endl;
  return nullptr;
}

antlrcpp::Any CDRDeclareTypes::visitDeclarator(IDLParser::DeclaratorContext *ctx) {
  cout << " " << ctx->getText();
  return nullptr;
}
