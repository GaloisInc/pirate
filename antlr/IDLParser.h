
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"




class  IDLParser : public antlr4::Parser {
public:
  enum {
    INTEGER_LITERAL = 1, OCTAL_LITERAL = 2, HEX_LITERAL = 3, FLOATING_PT_LITERAL = 4, 
    FIXED_PT_LITERAL = 5, WIDE_CHARACTER_LITERAL = 6, CHARACTER_LITERAL = 7, 
    WIDE_STRING_LITERAL = 8, STRING_LITERAL = 9, BOOLEAN_LITERAL = 10, SEMICOLON = 11, 
    COLON = 12, COMMA = 13, LEFT_BRACE = 14, RIGHT_BRACE = 15, LEFT_BRACKET = 16, 
    RIGHT_BRACKET = 17, LEFT_SQUARE_BRACKET = 18, RIGHT_SQUARE_BRACKET = 19, 
    TILDE = 20, SLASH = 21, LEFT_ANG_BRACKET = 22, RIGHT_ANG_BRACKET = 23, 
    STAR = 24, PLUS = 25, MINUS = 26, CARET = 27, AMPERSAND = 28, PIPE = 29, 
    EQUAL = 30, PERCENT = 31, DOUBLE_COLON = 32, RIGHT_SHIFT = 33, LEFT_SHIFT = 34, 
    AT = 35, KW_SETRAISES = 36, KW_OUT = 37, KW_EMITS = 38, KW_STRING = 39, 
    KW_SWITCH = 40, KW_PUBLISHES = 41, KW_TYPEDEF = 42, KW_USES = 43, KW_PRIMARYKEY = 44, 
    KW_CUSTOM = 45, KW_OCTET = 46, KW_SEQUENCE = 47, KW_IMPORT = 48, KW_STRUCT = 49, 
    KW_NATIVE = 50, KW_READONLY = 51, KW_FINDER = 52, KW_RAISES = 53, KW_VOID = 54, 
    KW_PRIVATE = 55, KW_EVENTTYPE = 56, KW_WCHAR = 57, KW_IN = 58, KW_DEFAULT = 59, 
    KW_PUBLIC = 60, KW_SHORT = 61, KW_LONG = 62, KW_ENUM = 63, KW_WSTRING = 64, 
    KW_CONTEXT = 65, KW_HOME = 66, KW_FACTORY = 67, KW_EXCEPTION = 68, KW_GETRAISES = 69, 
    KW_CONST = 70, KW_VALUEBASE = 71, KW_VALUETYPE = 72, KW_SUPPORTS = 73, 
    KW_MODULE = 74, KW_OBJECT = 75, KW_TRUNCATABLE = 76, KW_UNSIGNED = 77, 
    KW_FIXED = 78, KW_UNION = 79, KW_ONEWAY = 80, KW_ANY = 81, KW_CHAR = 82, 
    KW_CASE = 83, KW_FLOAT = 84, KW_BOOLEAN = 85, KW_MULTIPLE = 86, KW_ABSTRACT = 87, 
    KW_INOUT = 88, KW_PROVIDES = 89, KW_CONSUMES = 90, KW_DOUBLE = 91, KW_TYPEPREFIX = 92, 
    KW_TYPEID = 93, KW_ATTRIBUTE = 94, KW_LOCAL = 95, KW_MANAGES = 96, KW_INTERFACE = 97, 
    KW_COMPONENT = 98, KW_SET = 99, KW_MAP = 100, KW_BITFIELD = 101, KW_BITSET = 102, 
    KW_BITMASK = 103, KW_INT8 = 104, KW_UINT8 = 105, KW_INT16 = 106, KW_UINT16 = 107, 
    KW_INT32 = 108, KW_UINT32 = 109, KW_INT64 = 110, KW_UINT64 = 111, KW_AT_ANNOTATION = 112, 
    ID = 113, WS = 114, COMMENT = 115, LINE_COMMENT = 116
  };

  enum {
    RuleSpecification = 0, RuleDefinition = 1, RuleModule = 2, RuleA_scoped_name = 3, 
    RuleScoped_name = 4, RuleConst_exp = 5, RuleLiteral = 6, RulePositive_int_const = 7, 
    RuleType_decl = 8, RuleType_spec = 9, RuleSimple_type_spec = 10, RuleBase_type_spec = 11, 
    RuleConstr_type_spec = 12, RuleDeclarators = 13, RuleDeclarator = 14, 
    RuleSimple_declarator = 15, RuleComplex_declarator = 16, RuleFloating_pt_type = 17, 
    RuleInteger_type = 18, RuleSigned_int = 19, RuleSigned_tiny_int = 20, 
    RuleSigned_short_int = 21, RuleSigned_long_int = 22, RuleSigned_longlong_int = 23, 
    RuleUnsigned_int = 24, RuleUnsigned_tiny_int = 25, RuleUnsigned_short_int = 26, 
    RuleUnsigned_long_int = 27, RuleUnsigned_longlong_int = 28, RuleChar_type = 29, 
    RuleBoolean_type = 30, RuleOctet_type = 31, RuleStruct_type = 32, RuleMember_list = 33, 
    RuleMember = 34, RuleUnion_type = 35, RuleSwitch_type_spec = 36, RuleSwitch_body = 37, 
    RuleCase_stmt = 38, RuleCase_label = 39, RuleElement_spec = 40, RuleEnum_type = 41, 
    RuleEnumerator = 42, RuleArray_declarator = 43, RuleFixed_array_size = 44, 
    RuleAnnapps = 45, RuleAnnotation_appl = 46, RuleAnnotation_appl_params = 47, 
    RuleAnnotation_appl_param = 48, RuleIdentifier = 49
  };

  IDLParser(antlr4::TokenStream *input);
  ~IDLParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class SpecificationContext;
  class DefinitionContext;
  class ModuleContext;
  class A_scoped_nameContext;
  class Scoped_nameContext;
  class Const_expContext;
  class LiteralContext;
  class Positive_int_constContext;
  class Type_declContext;
  class Type_specContext;
  class Simple_type_specContext;
  class Base_type_specContext;
  class Constr_type_specContext;
  class DeclaratorsContext;
  class DeclaratorContext;
  class Simple_declaratorContext;
  class Complex_declaratorContext;
  class Floating_pt_typeContext;
  class Integer_typeContext;
  class Signed_intContext;
  class Signed_tiny_intContext;
  class Signed_short_intContext;
  class Signed_long_intContext;
  class Signed_longlong_intContext;
  class Unsigned_intContext;
  class Unsigned_tiny_intContext;
  class Unsigned_short_intContext;
  class Unsigned_long_intContext;
  class Unsigned_longlong_intContext;
  class Char_typeContext;
  class Boolean_typeContext;
  class Octet_typeContext;
  class Struct_typeContext;
  class Member_listContext;
  class MemberContext;
  class Union_typeContext;
  class Switch_type_specContext;
  class Switch_bodyContext;
  class Case_stmtContext;
  class Case_labelContext;
  class Element_specContext;
  class Enum_typeContext;
  class EnumeratorContext;
  class Array_declaratorContext;
  class Fixed_array_sizeContext;
  class AnnappsContext;
  class Annotation_applContext;
  class Annotation_appl_paramsContext;
  class Annotation_appl_paramContext;
  class IdentifierContext; 

  class  SpecificationContext : public antlr4::ParserRuleContext {
  public:
    SpecificationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SpecificationContext* specification();

  class  DefinitionContext : public antlr4::ParserRuleContext {
  public:
    DefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    Type_declContext *type_decl();
    antlr4::tree::TerminalNode *SEMICOLON();
    ModuleContext *module();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionContext* definition();

  class  ModuleContext : public antlr4::ParserRuleContext {
  public:
    ModuleContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_MODULE();
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *LEFT_BRACE();
    antlr4::tree::TerminalNode *RIGHT_BRACE();
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModuleContext* module();

  class  A_scoped_nameContext : public antlr4::ParserRuleContext {
  public:
    A_scoped_nameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    Scoped_nameContext *scoped_name();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  A_scoped_nameContext* a_scoped_name();

  class  Scoped_nameContext : public antlr4::ParserRuleContext {
  public:
    Scoped_nameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DOUBLE_COLON();
    antlr4::tree::TerminalNode* DOUBLE_COLON(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Scoped_nameContext* scoped_name();

  class  Const_expContext : public antlr4::ParserRuleContext {
  public:
    Const_expContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LiteralContext *literal();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Const_expContext* const_exp();

  class  LiteralContext : public antlr4::ParserRuleContext {
  public:
    LiteralContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *HEX_LITERAL();
    antlr4::tree::TerminalNode *INTEGER_LITERAL();
    antlr4::tree::TerminalNode *OCTAL_LITERAL();
    antlr4::tree::TerminalNode *STRING_LITERAL();
    antlr4::tree::TerminalNode *WIDE_STRING_LITERAL();
    antlr4::tree::TerminalNode *CHARACTER_LITERAL();
    antlr4::tree::TerminalNode *WIDE_CHARACTER_LITERAL();
    antlr4::tree::TerminalNode *FIXED_PT_LITERAL();
    antlr4::tree::TerminalNode *FLOATING_PT_LITERAL();
    antlr4::tree::TerminalNode *BOOLEAN_LITERAL();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LiteralContext* literal();

  class  Positive_int_constContext : public antlr4::ParserRuleContext {
  public:
    Positive_int_constContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Const_expContext *const_exp();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Positive_int_constContext* positive_int_const();

  class  Type_declContext : public antlr4::ParserRuleContext {
  public:
    Type_declContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Struct_typeContext *struct_type();
    Union_typeContext *union_type();
    Enum_typeContext *enum_type();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Type_declContext* type_decl();

  class  Type_specContext : public antlr4::ParserRuleContext {
  public:
    Type_specContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Simple_type_specContext *simple_type_spec();
    Constr_type_specContext *constr_type_spec();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Type_specContext* type_spec();

  class  Simple_type_specContext : public antlr4::ParserRuleContext {
  public:
    Simple_type_specContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Base_type_specContext *base_type_spec();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Simple_type_specContext* simple_type_spec();

  class  Base_type_specContext : public antlr4::ParserRuleContext {
  public:
    Base_type_specContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Floating_pt_typeContext *floating_pt_type();
    Integer_typeContext *integer_type();
    Char_typeContext *char_type();
    Boolean_typeContext *boolean_type();
    Octet_typeContext *octet_type();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Base_type_specContext* base_type_spec();

  class  Constr_type_specContext : public antlr4::ParserRuleContext {
  public:
    Constr_type_specContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Struct_typeContext *struct_type();
    Union_typeContext *union_type();
    Enum_typeContext *enum_type();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Constr_type_specContext* constr_type_spec();

  class  DeclaratorsContext : public antlr4::ParserRuleContext {
  public:
    DeclaratorsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DeclaratorContext *> declarator();
    DeclaratorContext* declarator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DeclaratorsContext* declarators();

  class  DeclaratorContext : public antlr4::ParserRuleContext {
  public:
    DeclaratorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    Simple_declaratorContext *simple_declarator();
    Complex_declaratorContext *complex_declarator();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DeclaratorContext* declarator();

  class  Simple_declaratorContext : public antlr4::ParserRuleContext {
  public:
    Simple_declaratorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Simple_declaratorContext* simple_declarator();

  class  Complex_declaratorContext : public antlr4::ParserRuleContext {
  public:
    Complex_declaratorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Array_declaratorContext *array_declarator();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Complex_declaratorContext* complex_declarator();

  class  Floating_pt_typeContext : public antlr4::ParserRuleContext {
  public:
    Floating_pt_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_FLOAT();
    antlr4::tree::TerminalNode *KW_DOUBLE();
    antlr4::tree::TerminalNode *KW_LONG();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Floating_pt_typeContext* floating_pt_type();

  class  Integer_typeContext : public antlr4::ParserRuleContext {
  public:
    Integer_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Signed_intContext *signed_int();
    Unsigned_intContext *unsigned_int();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Integer_typeContext* integer_type();

  class  Signed_intContext : public antlr4::ParserRuleContext {
  public:
    Signed_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Signed_short_intContext *signed_short_int();
    Signed_long_intContext *signed_long_int();
    Signed_longlong_intContext *signed_longlong_int();
    Signed_tiny_intContext *signed_tiny_int();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Signed_intContext* signed_int();

  class  Signed_tiny_intContext : public antlr4::ParserRuleContext {
  public:
    Signed_tiny_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_INT8();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Signed_tiny_intContext* signed_tiny_int();

  class  Signed_short_intContext : public antlr4::ParserRuleContext {
  public:
    Signed_short_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_SHORT();
    antlr4::tree::TerminalNode *KW_INT16();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Signed_short_intContext* signed_short_int();

  class  Signed_long_intContext : public antlr4::ParserRuleContext {
  public:
    Signed_long_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_LONG();
    antlr4::tree::TerminalNode *KW_INT32();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Signed_long_intContext* signed_long_int();

  class  Signed_longlong_intContext : public antlr4::ParserRuleContext {
  public:
    Signed_longlong_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> KW_LONG();
    antlr4::tree::TerminalNode* KW_LONG(size_t i);
    antlr4::tree::TerminalNode *KW_INT64();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Signed_longlong_intContext* signed_longlong_int();

  class  Unsigned_intContext : public antlr4::ParserRuleContext {
  public:
    Unsigned_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Unsigned_short_intContext *unsigned_short_int();
    Unsigned_long_intContext *unsigned_long_int();
    Unsigned_longlong_intContext *unsigned_longlong_int();
    Unsigned_tiny_intContext *unsigned_tiny_int();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Unsigned_intContext* unsigned_int();

  class  Unsigned_tiny_intContext : public antlr4::ParserRuleContext {
  public:
    Unsigned_tiny_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_UINT8();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Unsigned_tiny_intContext* unsigned_tiny_int();

  class  Unsigned_short_intContext : public antlr4::ParserRuleContext {
  public:
    Unsigned_short_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_UNSIGNED();
    antlr4::tree::TerminalNode *KW_SHORT();
    antlr4::tree::TerminalNode *KW_UINT16();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Unsigned_short_intContext* unsigned_short_int();

  class  Unsigned_long_intContext : public antlr4::ParserRuleContext {
  public:
    Unsigned_long_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_UNSIGNED();
    antlr4::tree::TerminalNode *KW_LONG();
    antlr4::tree::TerminalNode *KW_UINT32();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Unsigned_long_intContext* unsigned_long_int();

  class  Unsigned_longlong_intContext : public antlr4::ParserRuleContext {
  public:
    Unsigned_longlong_intContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_UNSIGNED();
    std::vector<antlr4::tree::TerminalNode *> KW_LONG();
    antlr4::tree::TerminalNode* KW_LONG(size_t i);
    antlr4::tree::TerminalNode *KW_UINT64();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Unsigned_longlong_intContext* unsigned_longlong_int();

  class  Char_typeContext : public antlr4::ParserRuleContext {
  public:
    Char_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_CHAR();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Char_typeContext* char_type();

  class  Boolean_typeContext : public antlr4::ParserRuleContext {
  public:
    Boolean_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_BOOLEAN();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Boolean_typeContext* boolean_type();

  class  Octet_typeContext : public antlr4::ParserRuleContext {
  public:
    Octet_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_OCTET();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Octet_typeContext* octet_type();

  class  Struct_typeContext : public antlr4::ParserRuleContext {
  public:
    Struct_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_STRUCT();
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *LEFT_BRACE();
    Member_listContext *member_list();
    antlr4::tree::TerminalNode *RIGHT_BRACE();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Struct_typeContext* struct_type();

  class  Member_listContext : public antlr4::ParserRuleContext {
  public:
    Member_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<MemberContext *> member();
    MemberContext* member(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Member_listContext* member_list();

  class  MemberContext : public antlr4::ParserRuleContext {
  public:
    MemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    Type_specContext *type_spec();
    DeclaratorsContext *declarators();
    antlr4::tree::TerminalNode *SEMICOLON();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MemberContext* member();

  class  Union_typeContext : public antlr4::ParserRuleContext {
  public:
    Union_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_UNION();
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *KW_SWITCH();
    antlr4::tree::TerminalNode *LEFT_BRACKET();
    AnnappsContext *annapps();
    Switch_type_specContext *switch_type_spec();
    antlr4::tree::TerminalNode *RIGHT_BRACKET();
    antlr4::tree::TerminalNode *LEFT_BRACE();
    Switch_bodyContext *switch_body();
    antlr4::tree::TerminalNode *RIGHT_BRACE();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Union_typeContext* union_type();

  class  Switch_type_specContext : public antlr4::ParserRuleContext {
  public:
    Switch_type_specContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Integer_typeContext *integer_type();
    Enum_typeContext *enum_type();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Switch_type_specContext* switch_type_spec();

  class  Switch_bodyContext : public antlr4::ParserRuleContext {
  public:
    Switch_bodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Case_stmtContext *> case_stmt();
    Case_stmtContext* case_stmt(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Switch_bodyContext* switch_body();

  class  Case_stmtContext : public antlr4::ParserRuleContext {
  public:
    Case_stmtContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Element_specContext *element_spec();
    antlr4::tree::TerminalNode *SEMICOLON();
    std::vector<Case_labelContext *> case_label();
    Case_labelContext* case_label(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Case_stmtContext* case_stmt();

  class  Case_labelContext : public antlr4::ParserRuleContext {
  public:
    Case_labelContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    antlr4::tree::TerminalNode *KW_CASE();
    Const_expContext *const_exp();
    antlr4::tree::TerminalNode *COLON();
    antlr4::tree::TerminalNode *KW_DEFAULT();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Case_labelContext* case_label();

  class  Element_specContext : public antlr4::ParserRuleContext {
  public:
    Element_specContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    Type_specContext *type_spec();
    DeclaratorContext *declarator();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Element_specContext* element_spec();

  class  Enum_typeContext : public antlr4::ParserRuleContext {
  public:
    Enum_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_ENUM();
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *LEFT_BRACE();
    std::vector<EnumeratorContext *> enumerator();
    EnumeratorContext* enumerator(size_t i);
    antlr4::tree::TerminalNode *RIGHT_BRACE();
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Enum_typeContext* enum_type();

  class  EnumeratorContext : public antlr4::ParserRuleContext {
  public:
    EnumeratorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumeratorContext* enumerator();

  class  Array_declaratorContext : public antlr4::ParserRuleContext {
  public:
    Array_declaratorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();
    std::vector<Fixed_array_sizeContext *> fixed_array_size();
    Fixed_array_sizeContext* fixed_array_size(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Array_declaratorContext* array_declarator();

  class  Fixed_array_sizeContext : public antlr4::ParserRuleContext {
  public:
    Fixed_array_sizeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LEFT_SQUARE_BRACKET();
    Positive_int_constContext *positive_int_const();
    antlr4::tree::TerminalNode *RIGHT_SQUARE_BRACKET();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Fixed_array_sizeContext* fixed_array_size();

  class  AnnappsContext : public antlr4::ParserRuleContext {
  public:
    AnnappsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Annotation_applContext *> annotation_appl();
    Annotation_applContext* annotation_appl(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnnappsContext* annapps();

  class  Annotation_applContext : public antlr4::ParserRuleContext {
  public:
    Annotation_applContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *AT();
    Scoped_nameContext *scoped_name();
    antlr4::tree::TerminalNode *LEFT_BRACKET();
    Annotation_appl_paramsContext *annotation_appl_params();
    antlr4::tree::TerminalNode *RIGHT_BRACKET();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Annotation_applContext* annotation_appl();

  class  Annotation_appl_paramsContext : public antlr4::ParserRuleContext {
  public:
    Annotation_appl_paramsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Const_expContext *const_exp();
    std::vector<Annotation_appl_paramContext *> annotation_appl_param();
    Annotation_appl_paramContext* annotation_appl_param(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Annotation_appl_paramsContext* annotation_appl_params();

  class  Annotation_appl_paramContext : public antlr4::ParserRuleContext {
  public:
    Annotation_appl_paramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *EQUAL();
    Const_expContext *const_exp();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Annotation_appl_paramContext* annotation_appl_param();

  class  IdentifierContext : public antlr4::ParserRuleContext {
  public:
    IdentifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnappsContext *annapps();
    antlr4::tree::TerminalNode *ID();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IdentifierContext* identifier();


private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

