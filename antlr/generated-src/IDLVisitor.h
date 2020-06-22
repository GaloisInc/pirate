
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"
#include "IDLParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by IDLParser.
 */
class  IDLVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by IDLParser.
   */
    virtual antlrcpp::Any visitSpecification(IDLParser::SpecificationContext *context) = 0;

    virtual antlrcpp::Any visitDefinition(IDLParser::DefinitionContext *context) = 0;

    virtual antlrcpp::Any visitModule(IDLParser::ModuleContext *context) = 0;

    virtual antlrcpp::Any visitA_scoped_name(IDLParser::A_scoped_nameContext *context) = 0;

    virtual antlrcpp::Any visitScoped_name(IDLParser::Scoped_nameContext *context) = 0;

    virtual antlrcpp::Any visitConst_exp(IDLParser::Const_expContext *context) = 0;

    virtual antlrcpp::Any visitLiteral(IDLParser::LiteralContext *context) = 0;

    virtual antlrcpp::Any visitPositive_int_const(IDLParser::Positive_int_constContext *context) = 0;

    virtual antlrcpp::Any visitType_decl(IDLParser::Type_declContext *context) = 0;

    virtual antlrcpp::Any visitType_spec(IDLParser::Type_specContext *context) = 0;

    virtual antlrcpp::Any visitSimple_type_spec(IDLParser::Simple_type_specContext *context) = 0;

    virtual antlrcpp::Any visitBase_type_spec(IDLParser::Base_type_specContext *context) = 0;

    virtual antlrcpp::Any visitConstr_type_spec(IDLParser::Constr_type_specContext *context) = 0;

    virtual antlrcpp::Any visitDeclarators(IDLParser::DeclaratorsContext *context) = 0;

    virtual antlrcpp::Any visitDeclarator(IDLParser::DeclaratorContext *context) = 0;

    virtual antlrcpp::Any visitSimple_declarator(IDLParser::Simple_declaratorContext *context) = 0;

    virtual antlrcpp::Any visitComplex_declarator(IDLParser::Complex_declaratorContext *context) = 0;

    virtual antlrcpp::Any visitFloating_pt_type(IDLParser::Floating_pt_typeContext *context) = 0;

    virtual antlrcpp::Any visitInteger_type(IDLParser::Integer_typeContext *context) = 0;

    virtual antlrcpp::Any visitSigned_int(IDLParser::Signed_intContext *context) = 0;

    virtual antlrcpp::Any visitSigned_tiny_int(IDLParser::Signed_tiny_intContext *context) = 0;

    virtual antlrcpp::Any visitSigned_short_int(IDLParser::Signed_short_intContext *context) = 0;

    virtual antlrcpp::Any visitSigned_long_int(IDLParser::Signed_long_intContext *context) = 0;

    virtual antlrcpp::Any visitSigned_longlong_int(IDLParser::Signed_longlong_intContext *context) = 0;

    virtual antlrcpp::Any visitUnsigned_int(IDLParser::Unsigned_intContext *context) = 0;

    virtual antlrcpp::Any visitUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext *context) = 0;

    virtual antlrcpp::Any visitUnsigned_short_int(IDLParser::Unsigned_short_intContext *context) = 0;

    virtual antlrcpp::Any visitUnsigned_long_int(IDLParser::Unsigned_long_intContext *context) = 0;

    virtual antlrcpp::Any visitUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext *context) = 0;

    virtual antlrcpp::Any visitChar_type(IDLParser::Char_typeContext *context) = 0;

    virtual antlrcpp::Any visitBoolean_type(IDLParser::Boolean_typeContext *context) = 0;

    virtual antlrcpp::Any visitOctet_type(IDLParser::Octet_typeContext *context) = 0;

    virtual antlrcpp::Any visitStruct_type(IDLParser::Struct_typeContext *context) = 0;

    virtual antlrcpp::Any visitMember_list(IDLParser::Member_listContext *context) = 0;

    virtual antlrcpp::Any visitMember(IDLParser::MemberContext *context) = 0;

    virtual antlrcpp::Any visitUnion_type(IDLParser::Union_typeContext *context) = 0;

    virtual antlrcpp::Any visitSwitch_type_spec(IDLParser::Switch_type_specContext *context) = 0;

    virtual antlrcpp::Any visitSwitch_body(IDLParser::Switch_bodyContext *context) = 0;

    virtual antlrcpp::Any visitCase_stmt(IDLParser::Case_stmtContext *context) = 0;

    virtual antlrcpp::Any visitCase_label(IDLParser::Case_labelContext *context) = 0;

    virtual antlrcpp::Any visitElement_spec(IDLParser::Element_specContext *context) = 0;

    virtual antlrcpp::Any visitEnum_type(IDLParser::Enum_typeContext *context) = 0;

    virtual antlrcpp::Any visitEnumerator(IDLParser::EnumeratorContext *context) = 0;

    virtual antlrcpp::Any visitArray_declarator(IDLParser::Array_declaratorContext *context) = 0;

    virtual antlrcpp::Any visitFixed_array_size(IDLParser::Fixed_array_sizeContext *context) = 0;

    virtual antlrcpp::Any visitAnnapps(IDLParser::AnnappsContext *context) = 0;

    virtual antlrcpp::Any visitAnnotation_appl(IDLParser::Annotation_applContext *context) = 0;

    virtual antlrcpp::Any visitAnnotation_appl_params(IDLParser::Annotation_appl_paramsContext *context) = 0;

    virtual antlrcpp::Any visitAnnotation_appl_param(IDLParser::Annotation_appl_paramContext *context) = 0;

    virtual antlrcpp::Any visitIdentifier(IDLParser::IdentifierContext *context) = 0;


};

