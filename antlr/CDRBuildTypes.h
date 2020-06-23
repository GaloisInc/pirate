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

#pragma once

#include "antlr4-runtime.h"
#include "IDLBaseVisitor.h"

class CDRBuildTypes : public IDLBaseVisitor {
public:
  virtual antlrcpp::Any aggregateResult(antlrcpp::Any aggregate, const antlrcpp::Any &nextResult) override;
  virtual antlrcpp::Any visitModule(IDLParser::ModuleContext *ctx) override;
  virtual antlrcpp::Any visitStruct_type(IDLParser::Struct_typeContext *ctx) override;
  virtual antlrcpp::Any visitMember(IDLParser::MemberContext *ctx) override;
  virtual antlrcpp::Any visitUnion_type(IDLParser::Union_typeContext *ctx) override;
  virtual antlrcpp::Any visitCase_stmt(IDLParser::Case_stmtContext *ctx) override;
  virtual antlrcpp::Any visitSimple_declarator(IDLParser::Simple_declaratorContext *ctx) override;
  virtual antlrcpp::Any visitArray_declarator(IDLParser::Array_declaratorContext *ctx) override;
  virtual antlrcpp::Any visitFloating_pt_type(IDLParser::Floating_pt_typeContext *ctx) override;
  virtual antlrcpp::Any visitSigned_tiny_int(IDLParser::Signed_tiny_intContext *ctx) override;
  virtual antlrcpp::Any visitSigned_short_int(IDLParser::Signed_short_intContext *ctx) override;
  virtual antlrcpp::Any visitSigned_long_int(IDLParser::Signed_long_intContext *ctx) override;
  virtual antlrcpp::Any visitSigned_longlong_int(IDLParser::Signed_longlong_intContext *ctx) override;
  virtual antlrcpp::Any visitUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext *ctx) override;
  virtual antlrcpp::Any visitUnsigned_short_int(IDLParser::Unsigned_short_intContext *ctx) override;
  virtual antlrcpp::Any visitUnsigned_long_int(IDLParser::Unsigned_long_intContext *ctx) override;
  virtual antlrcpp::Any visitUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext *ctx) override;
  virtual antlrcpp::Any visitChar_type(IDLParser::Char_typeContext *ctx) override;
  virtual antlrcpp::Any visitBoolean_type(IDLParser::Boolean_typeContext *ctx) override;
  virtual antlrcpp::Any visitOctet_type(IDLParser::Octet_typeContext *ctx) override;
};
