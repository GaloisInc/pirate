#pragma once

#include "antlr4-runtime.h"
#include "IDLBaseVisitor.h"

class CDRDeclareTypes : public IDLBaseVisitor {
public:
  virtual antlrcpp::Any visitStruct_type(IDLParser::Struct_typeContext *ctx) override;
  virtual antlrcpp::Any visitFloating_pt_type(IDLParser::Floating_pt_typeContext *context) override;
  virtual antlrcpp::Any visitMember(IDLParser::MemberContext *ctx) override;
  virtual antlrcpp::Any visitDeclarator(IDLParser::DeclaratorContext *ctx) override;
};
