
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"
#include "IDLListener.h"


/**
 * This class provides an empty implementation of IDLListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  IDLBaseListener : public IDLListener {
public:

  virtual void enterSpecification(IDLParser::SpecificationContext * /*ctx*/) override { }
  virtual void exitSpecification(IDLParser::SpecificationContext * /*ctx*/) override { }

  virtual void enterDefinition(IDLParser::DefinitionContext * /*ctx*/) override { }
  virtual void exitDefinition(IDLParser::DefinitionContext * /*ctx*/) override { }

  virtual void enterModule(IDLParser::ModuleContext * /*ctx*/) override { }
  virtual void exitModule(IDLParser::ModuleContext * /*ctx*/) override { }

  virtual void enterA_scoped_name(IDLParser::A_scoped_nameContext * /*ctx*/) override { }
  virtual void exitA_scoped_name(IDLParser::A_scoped_nameContext * /*ctx*/) override { }

  virtual void enterScoped_name(IDLParser::Scoped_nameContext * /*ctx*/) override { }
  virtual void exitScoped_name(IDLParser::Scoped_nameContext * /*ctx*/) override { }

  virtual void enterConst_exp(IDLParser::Const_expContext * /*ctx*/) override { }
  virtual void exitConst_exp(IDLParser::Const_expContext * /*ctx*/) override { }

  virtual void enterLiteral(IDLParser::LiteralContext * /*ctx*/) override { }
  virtual void exitLiteral(IDLParser::LiteralContext * /*ctx*/) override { }

  virtual void enterPositive_int_const(IDLParser::Positive_int_constContext * /*ctx*/) override { }
  virtual void exitPositive_int_const(IDLParser::Positive_int_constContext * /*ctx*/) override { }

  virtual void enterType_decl(IDLParser::Type_declContext * /*ctx*/) override { }
  virtual void exitType_decl(IDLParser::Type_declContext * /*ctx*/) override { }

  virtual void enterType_spec(IDLParser::Type_specContext * /*ctx*/) override { }
  virtual void exitType_spec(IDLParser::Type_specContext * /*ctx*/) override { }

  virtual void enterSimple_type_spec(IDLParser::Simple_type_specContext * /*ctx*/) override { }
  virtual void exitSimple_type_spec(IDLParser::Simple_type_specContext * /*ctx*/) override { }

  virtual void enterBase_type_spec(IDLParser::Base_type_specContext * /*ctx*/) override { }
  virtual void exitBase_type_spec(IDLParser::Base_type_specContext * /*ctx*/) override { }

  virtual void enterConstr_type_spec(IDLParser::Constr_type_specContext * /*ctx*/) override { }
  virtual void exitConstr_type_spec(IDLParser::Constr_type_specContext * /*ctx*/) override { }

  virtual void enterDeclarators(IDLParser::DeclaratorsContext * /*ctx*/) override { }
  virtual void exitDeclarators(IDLParser::DeclaratorsContext * /*ctx*/) override { }

  virtual void enterDeclarator(IDLParser::DeclaratorContext * /*ctx*/) override { }
  virtual void exitDeclarator(IDLParser::DeclaratorContext * /*ctx*/) override { }

  virtual void enterSimple_declarator(IDLParser::Simple_declaratorContext * /*ctx*/) override { }
  virtual void exitSimple_declarator(IDLParser::Simple_declaratorContext * /*ctx*/) override { }

  virtual void enterComplex_declarator(IDLParser::Complex_declaratorContext * /*ctx*/) override { }
  virtual void exitComplex_declarator(IDLParser::Complex_declaratorContext * /*ctx*/) override { }

  virtual void enterFloating_pt_type(IDLParser::Floating_pt_typeContext * /*ctx*/) override { }
  virtual void exitFloating_pt_type(IDLParser::Floating_pt_typeContext * /*ctx*/) override { }

  virtual void enterInteger_type(IDLParser::Integer_typeContext * /*ctx*/) override { }
  virtual void exitInteger_type(IDLParser::Integer_typeContext * /*ctx*/) override { }

  virtual void enterSigned_int(IDLParser::Signed_intContext * /*ctx*/) override { }
  virtual void exitSigned_int(IDLParser::Signed_intContext * /*ctx*/) override { }

  virtual void enterSigned_tiny_int(IDLParser::Signed_tiny_intContext * /*ctx*/) override { }
  virtual void exitSigned_tiny_int(IDLParser::Signed_tiny_intContext * /*ctx*/) override { }

  virtual void enterSigned_short_int(IDLParser::Signed_short_intContext * /*ctx*/) override { }
  virtual void exitSigned_short_int(IDLParser::Signed_short_intContext * /*ctx*/) override { }

  virtual void enterSigned_long_int(IDLParser::Signed_long_intContext * /*ctx*/) override { }
  virtual void exitSigned_long_int(IDLParser::Signed_long_intContext * /*ctx*/) override { }

  virtual void enterSigned_longlong_int(IDLParser::Signed_longlong_intContext * /*ctx*/) override { }
  virtual void exitSigned_longlong_int(IDLParser::Signed_longlong_intContext * /*ctx*/) override { }

  virtual void enterUnsigned_int(IDLParser::Unsigned_intContext * /*ctx*/) override { }
  virtual void exitUnsigned_int(IDLParser::Unsigned_intContext * /*ctx*/) override { }

  virtual void enterUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext * /*ctx*/) override { }
  virtual void exitUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext * /*ctx*/) override { }

  virtual void enterUnsigned_short_int(IDLParser::Unsigned_short_intContext * /*ctx*/) override { }
  virtual void exitUnsigned_short_int(IDLParser::Unsigned_short_intContext * /*ctx*/) override { }

  virtual void enterUnsigned_long_int(IDLParser::Unsigned_long_intContext * /*ctx*/) override { }
  virtual void exitUnsigned_long_int(IDLParser::Unsigned_long_intContext * /*ctx*/) override { }

  virtual void enterUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext * /*ctx*/) override { }
  virtual void exitUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext * /*ctx*/) override { }

  virtual void enterChar_type(IDLParser::Char_typeContext * /*ctx*/) override { }
  virtual void exitChar_type(IDLParser::Char_typeContext * /*ctx*/) override { }

  virtual void enterBoolean_type(IDLParser::Boolean_typeContext * /*ctx*/) override { }
  virtual void exitBoolean_type(IDLParser::Boolean_typeContext * /*ctx*/) override { }

  virtual void enterOctet_type(IDLParser::Octet_typeContext * /*ctx*/) override { }
  virtual void exitOctet_type(IDLParser::Octet_typeContext * /*ctx*/) override { }

  virtual void enterStruct_type(IDLParser::Struct_typeContext * /*ctx*/) override { }
  virtual void exitStruct_type(IDLParser::Struct_typeContext * /*ctx*/) override { }

  virtual void enterMember_list(IDLParser::Member_listContext * /*ctx*/) override { }
  virtual void exitMember_list(IDLParser::Member_listContext * /*ctx*/) override { }

  virtual void enterMember(IDLParser::MemberContext * /*ctx*/) override { }
  virtual void exitMember(IDLParser::MemberContext * /*ctx*/) override { }

  virtual void enterUnion_type(IDLParser::Union_typeContext * /*ctx*/) override { }
  virtual void exitUnion_type(IDLParser::Union_typeContext * /*ctx*/) override { }

  virtual void enterSwitch_type_spec(IDLParser::Switch_type_specContext * /*ctx*/) override { }
  virtual void exitSwitch_type_spec(IDLParser::Switch_type_specContext * /*ctx*/) override { }

  virtual void enterSwitch_body(IDLParser::Switch_bodyContext * /*ctx*/) override { }
  virtual void exitSwitch_body(IDLParser::Switch_bodyContext * /*ctx*/) override { }

  virtual void enterCase_stmt(IDLParser::Case_stmtContext * /*ctx*/) override { }
  virtual void exitCase_stmt(IDLParser::Case_stmtContext * /*ctx*/) override { }

  virtual void enterCase_label(IDLParser::Case_labelContext * /*ctx*/) override { }
  virtual void exitCase_label(IDLParser::Case_labelContext * /*ctx*/) override { }

  virtual void enterElement_spec(IDLParser::Element_specContext * /*ctx*/) override { }
  virtual void exitElement_spec(IDLParser::Element_specContext * /*ctx*/) override { }

  virtual void enterEnum_type(IDLParser::Enum_typeContext * /*ctx*/) override { }
  virtual void exitEnum_type(IDLParser::Enum_typeContext * /*ctx*/) override { }

  virtual void enterEnumerator(IDLParser::EnumeratorContext * /*ctx*/) override { }
  virtual void exitEnumerator(IDLParser::EnumeratorContext * /*ctx*/) override { }

  virtual void enterArray_declarator(IDLParser::Array_declaratorContext * /*ctx*/) override { }
  virtual void exitArray_declarator(IDLParser::Array_declaratorContext * /*ctx*/) override { }

  virtual void enterFixed_array_size(IDLParser::Fixed_array_sizeContext * /*ctx*/) override { }
  virtual void exitFixed_array_size(IDLParser::Fixed_array_sizeContext * /*ctx*/) override { }

  virtual void enterAnnapps(IDLParser::AnnappsContext * /*ctx*/) override { }
  virtual void exitAnnapps(IDLParser::AnnappsContext * /*ctx*/) override { }

  virtual void enterAnnotation_appl(IDLParser::Annotation_applContext * /*ctx*/) override { }
  virtual void exitAnnotation_appl(IDLParser::Annotation_applContext * /*ctx*/) override { }

  virtual void enterAnnotation_appl_params(IDLParser::Annotation_appl_paramsContext * /*ctx*/) override { }
  virtual void exitAnnotation_appl_params(IDLParser::Annotation_appl_paramsContext * /*ctx*/) override { }

  virtual void enterAnnotation_appl_param(IDLParser::Annotation_appl_paramContext * /*ctx*/) override { }
  virtual void exitAnnotation_appl_param(IDLParser::Annotation_appl_paramContext * /*ctx*/) override { }

  virtual void enterIdentifier(IDLParser::IdentifierContext * /*ctx*/) override { }
  virtual void exitIdentifier(IDLParser::IdentifierContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

