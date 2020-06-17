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
  virtual antlrcpp::Any visitSimple_declarator(IDLParser::Simple_declaratorContext *ctx) override;
  virtual antlrcpp::Any visitArray_declarator(IDLParser::Array_declaratorContext *ctx) override;
  virtual antlrcpp::Any visitFloating_pt_type(IDLParser::Floating_pt_typeContext *context) override;
};
