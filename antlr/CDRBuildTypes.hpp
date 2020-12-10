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

#include <map>

#include "antlr4-runtime.h"
#include "IDLBaseVisitor.h"
#include "CDRTypes.hpp"
#include "Annotations.hpp"

class CDRBuildTypes : public IDLBaseVisitor {
private:
  antlr4::tree::ParseTreeProperty<std::string> namespacePrefix;
  std::map<std::string, TypeSpec*> typeDeclarations;
  std::set<std::string> errors;
  int annotationIds;
  bool hasValidate;
  bool hasTransform;
  ValueAnnotation* buildValueAnnotation(IDLParser::Annotation_appl_paramsContext *params);
  MinAnnotation*   buildMinAnnotation(IDLParser::Annotation_appl_paramsContext *params);
  MaxAnnotation*   buildMaxAnnotation(IDLParser::Annotation_appl_paramsContext *params);
  RangeAnnotation* buildRangeAnnotation(IDLParser::Annotation_appl_paramsContext *params);
  RoundAnnotation* buildRoundAnnotation(IDLParser::Annotation_appl_paramsContext *params);
public:
  CDRBuildTypes() : namespacePrefix(), typeDeclarations(), errors(),
    annotationIds(0), hasValidate(false), hasTransform(false) { }
  std::set<std::string> getErrors() { return errors; }
  bool hasValidateAnnotations() { return hasValidate; }
  bool hasTransformAnnotations() { return hasTransform; }
  virtual antlrcpp::Any aggregateResult(antlrcpp::Any aggregate, const antlrcpp::Any &nextResult) override;
  virtual antlrcpp::Any visitModule(IDLParser::ModuleContext *ctx) override;
  virtual antlrcpp::Any visitDefinition(IDLParser::DefinitionContext *ctx) override;
  virtual antlrcpp::Any visitAnnotation_appl(IDLParser::Annotation_applContext *ctx) override;
  virtual antlrcpp::Any visitSimple_type_spec(IDLParser::Simple_type_specContext *ctx) override;
  virtual antlrcpp::Any visitEnum_type(IDLParser::Enum_typeContext *ctx) override;
  virtual antlrcpp::Any visitType_decl(IDLParser::Type_declContext *ctx) override;
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
