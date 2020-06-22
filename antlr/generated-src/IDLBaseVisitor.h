
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"
#include "IDLVisitor.h"


/**
 * This class provides an empty implementation of IDLVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  IDLBaseVisitor : public IDLVisitor {
public:

  virtual antlrcpp::Any visitSpecification(IDLParser::SpecificationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDefinition(IDLParser::DefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitModule(IDLParser::ModuleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitA_scoped_name(IDLParser::A_scoped_nameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitScoped_name(IDLParser::Scoped_nameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitConst_exp(IDLParser::Const_expContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLiteral(IDLParser::LiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitPositive_int_const(IDLParser::Positive_int_constContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitType_decl(IDLParser::Type_declContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitType_spec(IDLParser::Type_specContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSimple_type_spec(IDLParser::Simple_type_specContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBase_type_spec(IDLParser::Base_type_specContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitConstr_type_spec(IDLParser::Constr_type_specContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDeclarators(IDLParser::DeclaratorsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDeclarator(IDLParser::DeclaratorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSimple_declarator(IDLParser::Simple_declaratorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitComplex_declarator(IDLParser::Complex_declaratorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFloating_pt_type(IDLParser::Floating_pt_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitInteger_type(IDLParser::Integer_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSigned_int(IDLParser::Signed_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSigned_tiny_int(IDLParser::Signed_tiny_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSigned_short_int(IDLParser::Signed_short_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSigned_long_int(IDLParser::Signed_long_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSigned_longlong_int(IDLParser::Signed_longlong_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnsigned_int(IDLParser::Unsigned_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnsigned_short_int(IDLParser::Unsigned_short_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnsigned_long_int(IDLParser::Unsigned_long_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitChar_type(IDLParser::Char_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBoolean_type(IDLParser::Boolean_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitOctet_type(IDLParser::Octet_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStruct_type(IDLParser::Struct_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMember_list(IDLParser::Member_listContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMember(IDLParser::MemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnion_type(IDLParser::Union_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSwitch_type_spec(IDLParser::Switch_type_specContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSwitch_body(IDLParser::Switch_bodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCase_stmt(IDLParser::Case_stmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCase_label(IDLParser::Case_labelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitElement_spec(IDLParser::Element_specContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitEnum_type(IDLParser::Enum_typeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitEnumerator(IDLParser::EnumeratorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitArray_declarator(IDLParser::Array_declaratorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFixed_array_size(IDLParser::Fixed_array_sizeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnnapps(IDLParser::AnnappsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnnotation_appl(IDLParser::Annotation_applContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnnotation_appl_params(IDLParser::Annotation_appl_paramsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnnotation_appl_param(IDLParser::Annotation_appl_paramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitIdentifier(IDLParser::IdentifierContext *ctx) override {
    return visitChildren(ctx);
  }


};

