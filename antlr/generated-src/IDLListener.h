
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"
#include "IDLParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by IDLParser.
 */
class  IDLListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterSpecification(IDLParser::SpecificationContext *ctx) = 0;
  virtual void exitSpecification(IDLParser::SpecificationContext *ctx) = 0;

  virtual void enterDefinition(IDLParser::DefinitionContext *ctx) = 0;
  virtual void exitDefinition(IDLParser::DefinitionContext *ctx) = 0;

  virtual void enterModule(IDLParser::ModuleContext *ctx) = 0;
  virtual void exitModule(IDLParser::ModuleContext *ctx) = 0;

  virtual void enterA_scoped_name(IDLParser::A_scoped_nameContext *ctx) = 0;
  virtual void exitA_scoped_name(IDLParser::A_scoped_nameContext *ctx) = 0;

  virtual void enterScoped_name(IDLParser::Scoped_nameContext *ctx) = 0;
  virtual void exitScoped_name(IDLParser::Scoped_nameContext *ctx) = 0;

  virtual void enterConst_exp(IDLParser::Const_expContext *ctx) = 0;
  virtual void exitConst_exp(IDLParser::Const_expContext *ctx) = 0;

  virtual void enterLiteral(IDLParser::LiteralContext *ctx) = 0;
  virtual void exitLiteral(IDLParser::LiteralContext *ctx) = 0;

  virtual void enterPositive_int_const(IDLParser::Positive_int_constContext *ctx) = 0;
  virtual void exitPositive_int_const(IDLParser::Positive_int_constContext *ctx) = 0;

  virtual void enterType_decl(IDLParser::Type_declContext *ctx) = 0;
  virtual void exitType_decl(IDLParser::Type_declContext *ctx) = 0;

  virtual void enterType_spec(IDLParser::Type_specContext *ctx) = 0;
  virtual void exitType_spec(IDLParser::Type_specContext *ctx) = 0;

  virtual void enterSimple_type_spec(IDLParser::Simple_type_specContext *ctx) = 0;
  virtual void exitSimple_type_spec(IDLParser::Simple_type_specContext *ctx) = 0;

  virtual void enterBase_type_spec(IDLParser::Base_type_specContext *ctx) = 0;
  virtual void exitBase_type_spec(IDLParser::Base_type_specContext *ctx) = 0;

  virtual void enterConstr_type_spec(IDLParser::Constr_type_specContext *ctx) = 0;
  virtual void exitConstr_type_spec(IDLParser::Constr_type_specContext *ctx) = 0;

  virtual void enterDeclarators(IDLParser::DeclaratorsContext *ctx) = 0;
  virtual void exitDeclarators(IDLParser::DeclaratorsContext *ctx) = 0;

  virtual void enterDeclarator(IDLParser::DeclaratorContext *ctx) = 0;
  virtual void exitDeclarator(IDLParser::DeclaratorContext *ctx) = 0;

  virtual void enterSimple_declarator(IDLParser::Simple_declaratorContext *ctx) = 0;
  virtual void exitSimple_declarator(IDLParser::Simple_declaratorContext *ctx) = 0;

  virtual void enterComplex_declarator(IDLParser::Complex_declaratorContext *ctx) = 0;
  virtual void exitComplex_declarator(IDLParser::Complex_declaratorContext *ctx) = 0;

  virtual void enterFloating_pt_type(IDLParser::Floating_pt_typeContext *ctx) = 0;
  virtual void exitFloating_pt_type(IDLParser::Floating_pt_typeContext *ctx) = 0;

  virtual void enterInteger_type(IDLParser::Integer_typeContext *ctx) = 0;
  virtual void exitInteger_type(IDLParser::Integer_typeContext *ctx) = 0;

  virtual void enterSigned_int(IDLParser::Signed_intContext *ctx) = 0;
  virtual void exitSigned_int(IDLParser::Signed_intContext *ctx) = 0;

  virtual void enterSigned_tiny_int(IDLParser::Signed_tiny_intContext *ctx) = 0;
  virtual void exitSigned_tiny_int(IDLParser::Signed_tiny_intContext *ctx) = 0;

  virtual void enterSigned_short_int(IDLParser::Signed_short_intContext *ctx) = 0;
  virtual void exitSigned_short_int(IDLParser::Signed_short_intContext *ctx) = 0;

  virtual void enterSigned_long_int(IDLParser::Signed_long_intContext *ctx) = 0;
  virtual void exitSigned_long_int(IDLParser::Signed_long_intContext *ctx) = 0;

  virtual void enterSigned_longlong_int(IDLParser::Signed_longlong_intContext *ctx) = 0;
  virtual void exitSigned_longlong_int(IDLParser::Signed_longlong_intContext *ctx) = 0;

  virtual void enterUnsigned_int(IDLParser::Unsigned_intContext *ctx) = 0;
  virtual void exitUnsigned_int(IDLParser::Unsigned_intContext *ctx) = 0;

  virtual void enterUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext *ctx) = 0;
  virtual void exitUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext *ctx) = 0;

  virtual void enterUnsigned_short_int(IDLParser::Unsigned_short_intContext *ctx) = 0;
  virtual void exitUnsigned_short_int(IDLParser::Unsigned_short_intContext *ctx) = 0;

  virtual void enterUnsigned_long_int(IDLParser::Unsigned_long_intContext *ctx) = 0;
  virtual void exitUnsigned_long_int(IDLParser::Unsigned_long_intContext *ctx) = 0;

  virtual void enterUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext *ctx) = 0;
  virtual void exitUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext *ctx) = 0;

  virtual void enterChar_type(IDLParser::Char_typeContext *ctx) = 0;
  virtual void exitChar_type(IDLParser::Char_typeContext *ctx) = 0;

  virtual void enterBoolean_type(IDLParser::Boolean_typeContext *ctx) = 0;
  virtual void exitBoolean_type(IDLParser::Boolean_typeContext *ctx) = 0;

  virtual void enterOctet_type(IDLParser::Octet_typeContext *ctx) = 0;
  virtual void exitOctet_type(IDLParser::Octet_typeContext *ctx) = 0;

  virtual void enterStruct_type(IDLParser::Struct_typeContext *ctx) = 0;
  virtual void exitStruct_type(IDLParser::Struct_typeContext *ctx) = 0;

  virtual void enterMember_list(IDLParser::Member_listContext *ctx) = 0;
  virtual void exitMember_list(IDLParser::Member_listContext *ctx) = 0;

  virtual void enterMember(IDLParser::MemberContext *ctx) = 0;
  virtual void exitMember(IDLParser::MemberContext *ctx) = 0;

  virtual void enterUnion_type(IDLParser::Union_typeContext *ctx) = 0;
  virtual void exitUnion_type(IDLParser::Union_typeContext *ctx) = 0;

  virtual void enterSwitch_type_spec(IDLParser::Switch_type_specContext *ctx) = 0;
  virtual void exitSwitch_type_spec(IDLParser::Switch_type_specContext *ctx) = 0;

  virtual void enterSwitch_body(IDLParser::Switch_bodyContext *ctx) = 0;
  virtual void exitSwitch_body(IDLParser::Switch_bodyContext *ctx) = 0;

  virtual void enterCase_stmt(IDLParser::Case_stmtContext *ctx) = 0;
  virtual void exitCase_stmt(IDLParser::Case_stmtContext *ctx) = 0;

  virtual void enterCase_label(IDLParser::Case_labelContext *ctx) = 0;
  virtual void exitCase_label(IDLParser::Case_labelContext *ctx) = 0;

  virtual void enterElement_spec(IDLParser::Element_specContext *ctx) = 0;
  virtual void exitElement_spec(IDLParser::Element_specContext *ctx) = 0;

  virtual void enterEnum_type(IDLParser::Enum_typeContext *ctx) = 0;
  virtual void exitEnum_type(IDLParser::Enum_typeContext *ctx) = 0;

  virtual void enterEnumerator(IDLParser::EnumeratorContext *ctx) = 0;
  virtual void exitEnumerator(IDLParser::EnumeratorContext *ctx) = 0;

  virtual void enterArray_declarator(IDLParser::Array_declaratorContext *ctx) = 0;
  virtual void exitArray_declarator(IDLParser::Array_declaratorContext *ctx) = 0;

  virtual void enterFixed_array_size(IDLParser::Fixed_array_sizeContext *ctx) = 0;
  virtual void exitFixed_array_size(IDLParser::Fixed_array_sizeContext *ctx) = 0;

  virtual void enterAnnapps(IDLParser::AnnappsContext *ctx) = 0;
  virtual void exitAnnapps(IDLParser::AnnappsContext *ctx) = 0;

  virtual void enterAnnotation_appl(IDLParser::Annotation_applContext *ctx) = 0;
  virtual void exitAnnotation_appl(IDLParser::Annotation_applContext *ctx) = 0;

  virtual void enterAnnotation_appl_params(IDLParser::Annotation_appl_paramsContext *ctx) = 0;
  virtual void exitAnnotation_appl_params(IDLParser::Annotation_appl_paramsContext *ctx) = 0;

  virtual void enterAnnotation_appl_param(IDLParser::Annotation_appl_paramContext *ctx) = 0;
  virtual void exitAnnotation_appl_param(IDLParser::Annotation_appl_paramContext *ctx) = 0;

  virtual void enterIdentifier(IDLParser::IdentifierContext *ctx) = 0;
  virtual void exitIdentifier(IDLParser::IdentifierContext *ctx) = 0;


};

