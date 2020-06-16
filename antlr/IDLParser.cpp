
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8


#include "IDLListener.h"
#include "IDLVisitor.h"

#include "IDLParser.h"


using namespace antlrcpp;
using namespace antlr4;

IDLParser::IDLParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

IDLParser::~IDLParser() {
  delete _interpreter;
}

std::string IDLParser::getGrammarFileName() const {
  return "IDL.g4";
}

const std::vector<std::string>& IDLParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& IDLParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- SpecificationContext ------------------------------------------------------------------

IDLParser::SpecificationContext::SpecificationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::SpecificationContext::EOF() {
  return getToken(IDLParser::EOF, 0);
}

std::vector<IDLParser::DefinitionContext *> IDLParser::SpecificationContext::definition() {
  return getRuleContexts<IDLParser::DefinitionContext>();
}

IDLParser::DefinitionContext* IDLParser::SpecificationContext::definition(size_t i) {
  return getRuleContext<IDLParser::DefinitionContext>(i);
}


size_t IDLParser::SpecificationContext::getRuleIndex() const {
  return IDLParser::RuleSpecification;
}

void IDLParser::SpecificationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSpecification(this);
}

void IDLParser::SpecificationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSpecification(this);
}


antlrcpp::Any IDLParser::SpecificationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSpecification(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::SpecificationContext* IDLParser::specification() {
  SpecificationContext *_localctx = _tracker.createInstance<SpecificationContext>(_ctx, getState());
  enterRule(_localctx, 0, IDLParser::RuleSpecification);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(101); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(100);
      definition();
      setState(103); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (((((_la - 35) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 35)) & ((1ULL << (IDLParser::AT - 35))
      | (1ULL << (IDLParser::KW_STRUCT - 35))
      | (1ULL << (IDLParser::KW_ENUM - 35))
      | (1ULL << (IDLParser::KW_MODULE - 35))
      | (1ULL << (IDLParser::KW_UNION - 35)))) != 0));
    setState(105);
    match(IDLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DefinitionContext ------------------------------------------------------------------

IDLParser::DefinitionContext::DefinitionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::DefinitionContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

IDLParser::Type_declContext* IDLParser::DefinitionContext::type_decl() {
  return getRuleContext<IDLParser::Type_declContext>(0);
}

tree::TerminalNode* IDLParser::DefinitionContext::SEMICOLON() {
  return getToken(IDLParser::SEMICOLON, 0);
}

IDLParser::ModuleContext* IDLParser::DefinitionContext::module() {
  return getRuleContext<IDLParser::ModuleContext>(0);
}


size_t IDLParser::DefinitionContext::getRuleIndex() const {
  return IDLParser::RuleDefinition;
}

void IDLParser::DefinitionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDefinition(this);
}

void IDLParser::DefinitionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDefinition(this);
}


antlrcpp::Any IDLParser::DefinitionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitDefinition(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::DefinitionContext* IDLParser::definition() {
  DefinitionContext *_localctx = _tracker.createInstance<DefinitionContext>(_ctx, getState());
  enterRule(_localctx, 2, IDLParser::RuleDefinition);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(107);
    annapps();
    setState(114);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_STRUCT:
      case IDLParser::KW_ENUM:
      case IDLParser::KW_UNION: {
        setState(108);
        type_decl();
        setState(109);
        match(IDLParser::SEMICOLON);
        break;
      }

      case IDLParser::KW_MODULE: {
        setState(111);
        module();
        setState(112);
        match(IDLParser::SEMICOLON);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ModuleContext ------------------------------------------------------------------

IDLParser::ModuleContext::ModuleContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::ModuleContext::KW_MODULE() {
  return getToken(IDLParser::KW_MODULE, 0);
}

IDLParser::IdentifierContext* IDLParser::ModuleContext::identifier() {
  return getRuleContext<IDLParser::IdentifierContext>(0);
}

tree::TerminalNode* IDLParser::ModuleContext::LEFT_BRACE() {
  return getToken(IDLParser::LEFT_BRACE, 0);
}

tree::TerminalNode* IDLParser::ModuleContext::RIGHT_BRACE() {
  return getToken(IDLParser::RIGHT_BRACE, 0);
}

std::vector<IDLParser::DefinitionContext *> IDLParser::ModuleContext::definition() {
  return getRuleContexts<IDLParser::DefinitionContext>();
}

IDLParser::DefinitionContext* IDLParser::ModuleContext::definition(size_t i) {
  return getRuleContext<IDLParser::DefinitionContext>(i);
}


size_t IDLParser::ModuleContext::getRuleIndex() const {
  return IDLParser::RuleModule;
}

void IDLParser::ModuleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterModule(this);
}

void IDLParser::ModuleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitModule(this);
}


antlrcpp::Any IDLParser::ModuleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitModule(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::ModuleContext* IDLParser::module() {
  ModuleContext *_localctx = _tracker.createInstance<ModuleContext>(_ctx, getState());
  enterRule(_localctx, 4, IDLParser::RuleModule);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(116);
    match(IDLParser::KW_MODULE);
    setState(117);
    identifier();
    setState(118);
    match(IDLParser::LEFT_BRACE);
    setState(120); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(119);
      definition();
      setState(122); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (((((_la - 35) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 35)) & ((1ULL << (IDLParser::AT - 35))
      | (1ULL << (IDLParser::KW_STRUCT - 35))
      | (1ULL << (IDLParser::KW_ENUM - 35))
      | (1ULL << (IDLParser::KW_MODULE - 35))
      | (1ULL << (IDLParser::KW_UNION - 35)))) != 0));
    setState(124);
    match(IDLParser::RIGHT_BRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- A_scoped_nameContext ------------------------------------------------------------------

IDLParser::A_scoped_nameContext::A_scoped_nameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::A_scoped_nameContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

IDLParser::Scoped_nameContext* IDLParser::A_scoped_nameContext::scoped_name() {
  return getRuleContext<IDLParser::Scoped_nameContext>(0);
}


size_t IDLParser::A_scoped_nameContext::getRuleIndex() const {
  return IDLParser::RuleA_scoped_name;
}

void IDLParser::A_scoped_nameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterA_scoped_name(this);
}

void IDLParser::A_scoped_nameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitA_scoped_name(this);
}


antlrcpp::Any IDLParser::A_scoped_nameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitA_scoped_name(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::A_scoped_nameContext* IDLParser::a_scoped_name() {
  A_scoped_nameContext *_localctx = _tracker.createInstance<A_scoped_nameContext>(_ctx, getState());
  enterRule(_localctx, 6, IDLParser::RuleA_scoped_name);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(126);
    annapps();
    setState(127);
    scoped_name();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Scoped_nameContext ------------------------------------------------------------------

IDLParser::Scoped_nameContext::Scoped_nameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> IDLParser::Scoped_nameContext::ID() {
  return getTokens(IDLParser::ID);
}

tree::TerminalNode* IDLParser::Scoped_nameContext::ID(size_t i) {
  return getToken(IDLParser::ID, i);
}

std::vector<tree::TerminalNode *> IDLParser::Scoped_nameContext::DOUBLE_COLON() {
  return getTokens(IDLParser::DOUBLE_COLON);
}

tree::TerminalNode* IDLParser::Scoped_nameContext::DOUBLE_COLON(size_t i) {
  return getToken(IDLParser::DOUBLE_COLON, i);
}


size_t IDLParser::Scoped_nameContext::getRuleIndex() const {
  return IDLParser::RuleScoped_name;
}

void IDLParser::Scoped_nameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterScoped_name(this);
}

void IDLParser::Scoped_nameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitScoped_name(this);
}


antlrcpp::Any IDLParser::Scoped_nameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitScoped_name(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Scoped_nameContext* IDLParser::scoped_name() {
  Scoped_nameContext *_localctx = _tracker.createInstance<Scoped_nameContext>(_ctx, getState());
  enterRule(_localctx, 8, IDLParser::RuleScoped_name);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(130);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == IDLParser::DOUBLE_COLON) {
      setState(129);
      match(IDLParser::DOUBLE_COLON);
    }
    setState(132);
    match(IDLParser::ID);
    setState(137);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(133);
        match(IDLParser::DOUBLE_COLON);
        setState(134);
        match(IDLParser::ID); 
      }
      setState(139);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Const_expContext ------------------------------------------------------------------

IDLParser::Const_expContext::Const_expContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::LiteralContext* IDLParser::Const_expContext::literal() {
  return getRuleContext<IDLParser::LiteralContext>(0);
}


size_t IDLParser::Const_expContext::getRuleIndex() const {
  return IDLParser::RuleConst_exp;
}

void IDLParser::Const_expContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConst_exp(this);
}

void IDLParser::Const_expContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConst_exp(this);
}


antlrcpp::Any IDLParser::Const_expContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitConst_exp(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Const_expContext* IDLParser::const_exp() {
  Const_expContext *_localctx = _tracker.createInstance<Const_expContext>(_ctx, getState());
  enterRule(_localctx, 10, IDLParser::RuleConst_exp);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(140);
    literal();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LiteralContext ------------------------------------------------------------------

IDLParser::LiteralContext::LiteralContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::LiteralContext::HEX_LITERAL() {
  return getToken(IDLParser::HEX_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::INTEGER_LITERAL() {
  return getToken(IDLParser::INTEGER_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::OCTAL_LITERAL() {
  return getToken(IDLParser::OCTAL_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::STRING_LITERAL() {
  return getToken(IDLParser::STRING_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::WIDE_STRING_LITERAL() {
  return getToken(IDLParser::WIDE_STRING_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::CHARACTER_LITERAL() {
  return getToken(IDLParser::CHARACTER_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::WIDE_CHARACTER_LITERAL() {
  return getToken(IDLParser::WIDE_CHARACTER_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::FIXED_PT_LITERAL() {
  return getToken(IDLParser::FIXED_PT_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::FLOATING_PT_LITERAL() {
  return getToken(IDLParser::FLOATING_PT_LITERAL, 0);
}

tree::TerminalNode* IDLParser::LiteralContext::BOOLEAN_LITERAL() {
  return getToken(IDLParser::BOOLEAN_LITERAL, 0);
}


size_t IDLParser::LiteralContext::getRuleIndex() const {
  return IDLParser::RuleLiteral;
}

void IDLParser::LiteralContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLiteral(this);
}

void IDLParser::LiteralContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLiteral(this);
}


antlrcpp::Any IDLParser::LiteralContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitLiteral(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::LiteralContext* IDLParser::literal() {
  LiteralContext *_localctx = _tracker.createInstance<LiteralContext>(_ctx, getState());
  enterRule(_localctx, 12, IDLParser::RuleLiteral);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(142);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << IDLParser::INTEGER_LITERAL)
      | (1ULL << IDLParser::OCTAL_LITERAL)
      | (1ULL << IDLParser::HEX_LITERAL)
      | (1ULL << IDLParser::FLOATING_PT_LITERAL)
      | (1ULL << IDLParser::FIXED_PT_LITERAL)
      | (1ULL << IDLParser::WIDE_CHARACTER_LITERAL)
      | (1ULL << IDLParser::CHARACTER_LITERAL)
      | (1ULL << IDLParser::WIDE_STRING_LITERAL)
      | (1ULL << IDLParser::STRING_LITERAL)
      | (1ULL << IDLParser::BOOLEAN_LITERAL))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Positive_int_constContext ------------------------------------------------------------------

IDLParser::Positive_int_constContext::Positive_int_constContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Const_expContext* IDLParser::Positive_int_constContext::const_exp() {
  return getRuleContext<IDLParser::Const_expContext>(0);
}


size_t IDLParser::Positive_int_constContext::getRuleIndex() const {
  return IDLParser::RulePositive_int_const;
}

void IDLParser::Positive_int_constContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPositive_int_const(this);
}

void IDLParser::Positive_int_constContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPositive_int_const(this);
}


antlrcpp::Any IDLParser::Positive_int_constContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitPositive_int_const(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Positive_int_constContext* IDLParser::positive_int_const() {
  Positive_int_constContext *_localctx = _tracker.createInstance<Positive_int_constContext>(_ctx, getState());
  enterRule(_localctx, 14, IDLParser::RulePositive_int_const);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(144);
    const_exp();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Type_declContext ------------------------------------------------------------------

IDLParser::Type_declContext::Type_declContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Struct_typeContext* IDLParser::Type_declContext::struct_type() {
  return getRuleContext<IDLParser::Struct_typeContext>(0);
}

IDLParser::Union_typeContext* IDLParser::Type_declContext::union_type() {
  return getRuleContext<IDLParser::Union_typeContext>(0);
}

IDLParser::Enum_typeContext* IDLParser::Type_declContext::enum_type() {
  return getRuleContext<IDLParser::Enum_typeContext>(0);
}


size_t IDLParser::Type_declContext::getRuleIndex() const {
  return IDLParser::RuleType_decl;
}

void IDLParser::Type_declContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterType_decl(this);
}

void IDLParser::Type_declContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitType_decl(this);
}


antlrcpp::Any IDLParser::Type_declContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitType_decl(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Type_declContext* IDLParser::type_decl() {
  Type_declContext *_localctx = _tracker.createInstance<Type_declContext>(_ctx, getState());
  enterRule(_localctx, 16, IDLParser::RuleType_decl);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(149);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_STRUCT: {
        enterOuterAlt(_localctx, 1);
        setState(146);
        struct_type();
        break;
      }

      case IDLParser::KW_UNION: {
        enterOuterAlt(_localctx, 2);
        setState(147);
        union_type();
        break;
      }

      case IDLParser::KW_ENUM: {
        enterOuterAlt(_localctx, 3);
        setState(148);
        enum_type();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Type_specContext ------------------------------------------------------------------

IDLParser::Type_specContext::Type_specContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Simple_type_specContext* IDLParser::Type_specContext::simple_type_spec() {
  return getRuleContext<IDLParser::Simple_type_specContext>(0);
}

IDLParser::Constr_type_specContext* IDLParser::Type_specContext::constr_type_spec() {
  return getRuleContext<IDLParser::Constr_type_specContext>(0);
}


size_t IDLParser::Type_specContext::getRuleIndex() const {
  return IDLParser::RuleType_spec;
}

void IDLParser::Type_specContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterType_spec(this);
}

void IDLParser::Type_specContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitType_spec(this);
}


antlrcpp::Any IDLParser::Type_specContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitType_spec(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Type_specContext* IDLParser::type_spec() {
  Type_specContext *_localctx = _tracker.createInstance<Type_specContext>(_ctx, getState());
  enterRule(_localctx, 18, IDLParser::RuleType_spec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(153);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_OCTET:
      case IDLParser::KW_SHORT:
      case IDLParser::KW_LONG:
      case IDLParser::KW_UNSIGNED:
      case IDLParser::KW_CHAR:
      case IDLParser::KW_FLOAT:
      case IDLParser::KW_BOOLEAN:
      case IDLParser::KW_DOUBLE:
      case IDLParser::KW_INT8:
      case IDLParser::KW_UINT8:
      case IDLParser::KW_INT16:
      case IDLParser::KW_UINT16:
      case IDLParser::KW_INT32:
      case IDLParser::KW_UINT32:
      case IDLParser::KW_INT64:
      case IDLParser::KW_UINT64: {
        enterOuterAlt(_localctx, 1);
        setState(151);
        simple_type_spec();
        break;
      }

      case IDLParser::KW_STRUCT:
      case IDLParser::KW_ENUM:
      case IDLParser::KW_UNION: {
        enterOuterAlt(_localctx, 2);
        setState(152);
        constr_type_spec();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Simple_type_specContext ------------------------------------------------------------------

IDLParser::Simple_type_specContext::Simple_type_specContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Base_type_specContext* IDLParser::Simple_type_specContext::base_type_spec() {
  return getRuleContext<IDLParser::Base_type_specContext>(0);
}


size_t IDLParser::Simple_type_specContext::getRuleIndex() const {
  return IDLParser::RuleSimple_type_spec;
}

void IDLParser::Simple_type_specContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSimple_type_spec(this);
}

void IDLParser::Simple_type_specContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSimple_type_spec(this);
}


antlrcpp::Any IDLParser::Simple_type_specContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSimple_type_spec(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Simple_type_specContext* IDLParser::simple_type_spec() {
  Simple_type_specContext *_localctx = _tracker.createInstance<Simple_type_specContext>(_ctx, getState());
  enterRule(_localctx, 20, IDLParser::RuleSimple_type_spec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(155);
    base_type_spec();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Base_type_specContext ------------------------------------------------------------------

IDLParser::Base_type_specContext::Base_type_specContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Floating_pt_typeContext* IDLParser::Base_type_specContext::floating_pt_type() {
  return getRuleContext<IDLParser::Floating_pt_typeContext>(0);
}

IDLParser::Integer_typeContext* IDLParser::Base_type_specContext::integer_type() {
  return getRuleContext<IDLParser::Integer_typeContext>(0);
}

IDLParser::Char_typeContext* IDLParser::Base_type_specContext::char_type() {
  return getRuleContext<IDLParser::Char_typeContext>(0);
}

IDLParser::Boolean_typeContext* IDLParser::Base_type_specContext::boolean_type() {
  return getRuleContext<IDLParser::Boolean_typeContext>(0);
}

IDLParser::Octet_typeContext* IDLParser::Base_type_specContext::octet_type() {
  return getRuleContext<IDLParser::Octet_typeContext>(0);
}


size_t IDLParser::Base_type_specContext::getRuleIndex() const {
  return IDLParser::RuleBase_type_spec;
}

void IDLParser::Base_type_specContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBase_type_spec(this);
}

void IDLParser::Base_type_specContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBase_type_spec(this);
}


antlrcpp::Any IDLParser::Base_type_specContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitBase_type_spec(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Base_type_specContext* IDLParser::base_type_spec() {
  Base_type_specContext *_localctx = _tracker.createInstance<Base_type_specContext>(_ctx, getState());
  enterRule(_localctx, 22, IDLParser::RuleBase_type_spec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(162);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 7, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(157);
      floating_pt_type();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(158);
      integer_type();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(159);
      char_type();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(160);
      boolean_type();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(161);
      octet_type();
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Constr_type_specContext ------------------------------------------------------------------

IDLParser::Constr_type_specContext::Constr_type_specContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Struct_typeContext* IDLParser::Constr_type_specContext::struct_type() {
  return getRuleContext<IDLParser::Struct_typeContext>(0);
}

IDLParser::Union_typeContext* IDLParser::Constr_type_specContext::union_type() {
  return getRuleContext<IDLParser::Union_typeContext>(0);
}

IDLParser::Enum_typeContext* IDLParser::Constr_type_specContext::enum_type() {
  return getRuleContext<IDLParser::Enum_typeContext>(0);
}


size_t IDLParser::Constr_type_specContext::getRuleIndex() const {
  return IDLParser::RuleConstr_type_spec;
}

void IDLParser::Constr_type_specContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstr_type_spec(this);
}

void IDLParser::Constr_type_specContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstr_type_spec(this);
}


antlrcpp::Any IDLParser::Constr_type_specContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitConstr_type_spec(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Constr_type_specContext* IDLParser::constr_type_spec() {
  Constr_type_specContext *_localctx = _tracker.createInstance<Constr_type_specContext>(_ctx, getState());
  enterRule(_localctx, 24, IDLParser::RuleConstr_type_spec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(167);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_STRUCT: {
        enterOuterAlt(_localctx, 1);
        setState(164);
        struct_type();
        break;
      }

      case IDLParser::KW_UNION: {
        enterOuterAlt(_localctx, 2);
        setState(165);
        union_type();
        break;
      }

      case IDLParser::KW_ENUM: {
        enterOuterAlt(_localctx, 3);
        setState(166);
        enum_type();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DeclaratorsContext ------------------------------------------------------------------

IDLParser::DeclaratorsContext::DeclaratorsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<IDLParser::DeclaratorContext *> IDLParser::DeclaratorsContext::declarator() {
  return getRuleContexts<IDLParser::DeclaratorContext>();
}

IDLParser::DeclaratorContext* IDLParser::DeclaratorsContext::declarator(size_t i) {
  return getRuleContext<IDLParser::DeclaratorContext>(i);
}

std::vector<tree::TerminalNode *> IDLParser::DeclaratorsContext::COMMA() {
  return getTokens(IDLParser::COMMA);
}

tree::TerminalNode* IDLParser::DeclaratorsContext::COMMA(size_t i) {
  return getToken(IDLParser::COMMA, i);
}


size_t IDLParser::DeclaratorsContext::getRuleIndex() const {
  return IDLParser::RuleDeclarators;
}

void IDLParser::DeclaratorsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDeclarators(this);
}

void IDLParser::DeclaratorsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDeclarators(this);
}


antlrcpp::Any IDLParser::DeclaratorsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitDeclarators(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::DeclaratorsContext* IDLParser::declarators() {
  DeclaratorsContext *_localctx = _tracker.createInstance<DeclaratorsContext>(_ctx, getState());
  enterRule(_localctx, 26, IDLParser::RuleDeclarators);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(169);
    declarator();
    setState(174);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == IDLParser::COMMA) {
      setState(170);
      match(IDLParser::COMMA);
      setState(171);
      declarator();
      setState(176);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DeclaratorContext ------------------------------------------------------------------

IDLParser::DeclaratorContext::DeclaratorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::DeclaratorContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

IDLParser::Simple_declaratorContext* IDLParser::DeclaratorContext::simple_declarator() {
  return getRuleContext<IDLParser::Simple_declaratorContext>(0);
}

IDLParser::Complex_declaratorContext* IDLParser::DeclaratorContext::complex_declarator() {
  return getRuleContext<IDLParser::Complex_declaratorContext>(0);
}


size_t IDLParser::DeclaratorContext::getRuleIndex() const {
  return IDLParser::RuleDeclarator;
}

void IDLParser::DeclaratorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDeclarator(this);
}

void IDLParser::DeclaratorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDeclarator(this);
}


antlrcpp::Any IDLParser::DeclaratorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitDeclarator(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::DeclaratorContext* IDLParser::declarator() {
  DeclaratorContext *_localctx = _tracker.createInstance<DeclaratorContext>(_ctx, getState());
  enterRule(_localctx, 28, IDLParser::RuleDeclarator);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(177);
    annapps();
    setState(180);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx)) {
    case 1: {
      setState(178);
      simple_declarator();
      break;
    }

    case 2: {
      setState(179);
      complex_declarator();
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Simple_declaratorContext ------------------------------------------------------------------

IDLParser::Simple_declaratorContext::Simple_declaratorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Simple_declaratorContext::ID() {
  return getToken(IDLParser::ID, 0);
}


size_t IDLParser::Simple_declaratorContext::getRuleIndex() const {
  return IDLParser::RuleSimple_declarator;
}

void IDLParser::Simple_declaratorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSimple_declarator(this);
}

void IDLParser::Simple_declaratorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSimple_declarator(this);
}


antlrcpp::Any IDLParser::Simple_declaratorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSimple_declarator(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Simple_declaratorContext* IDLParser::simple_declarator() {
  Simple_declaratorContext *_localctx = _tracker.createInstance<Simple_declaratorContext>(_ctx, getState());
  enterRule(_localctx, 30, IDLParser::RuleSimple_declarator);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(182);
    match(IDLParser::ID);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Complex_declaratorContext ------------------------------------------------------------------

IDLParser::Complex_declaratorContext::Complex_declaratorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Array_declaratorContext* IDLParser::Complex_declaratorContext::array_declarator() {
  return getRuleContext<IDLParser::Array_declaratorContext>(0);
}


size_t IDLParser::Complex_declaratorContext::getRuleIndex() const {
  return IDLParser::RuleComplex_declarator;
}

void IDLParser::Complex_declaratorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterComplex_declarator(this);
}

void IDLParser::Complex_declaratorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitComplex_declarator(this);
}


antlrcpp::Any IDLParser::Complex_declaratorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitComplex_declarator(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Complex_declaratorContext* IDLParser::complex_declarator() {
  Complex_declaratorContext *_localctx = _tracker.createInstance<Complex_declaratorContext>(_ctx, getState());
  enterRule(_localctx, 32, IDLParser::RuleComplex_declarator);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(184);
    array_declarator();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Floating_pt_typeContext ------------------------------------------------------------------

IDLParser::Floating_pt_typeContext::Floating_pt_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Floating_pt_typeContext::KW_FLOAT() {
  return getToken(IDLParser::KW_FLOAT, 0);
}

tree::TerminalNode* IDLParser::Floating_pt_typeContext::KW_DOUBLE() {
  return getToken(IDLParser::KW_DOUBLE, 0);
}

tree::TerminalNode* IDLParser::Floating_pt_typeContext::KW_LONG() {
  return getToken(IDLParser::KW_LONG, 0);
}


size_t IDLParser::Floating_pt_typeContext::getRuleIndex() const {
  return IDLParser::RuleFloating_pt_type;
}

void IDLParser::Floating_pt_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFloating_pt_type(this);
}

void IDLParser::Floating_pt_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFloating_pt_type(this);
}


antlrcpp::Any IDLParser::Floating_pt_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitFloating_pt_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Floating_pt_typeContext* IDLParser::floating_pt_type() {
  Floating_pt_typeContext *_localctx = _tracker.createInstance<Floating_pt_typeContext>(_ctx, getState());
  enterRule(_localctx, 34, IDLParser::RuleFloating_pt_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(190);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_FLOAT: {
        setState(186);
        match(IDLParser::KW_FLOAT);
        break;
      }

      case IDLParser::KW_DOUBLE: {
        setState(187);
        match(IDLParser::KW_DOUBLE);
        break;
      }

      case IDLParser::KW_LONG: {
        setState(188);
        match(IDLParser::KW_LONG);
        setState(189);
        match(IDLParser::KW_DOUBLE);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Integer_typeContext ------------------------------------------------------------------

IDLParser::Integer_typeContext::Integer_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Signed_intContext* IDLParser::Integer_typeContext::signed_int() {
  return getRuleContext<IDLParser::Signed_intContext>(0);
}

IDLParser::Unsigned_intContext* IDLParser::Integer_typeContext::unsigned_int() {
  return getRuleContext<IDLParser::Unsigned_intContext>(0);
}


size_t IDLParser::Integer_typeContext::getRuleIndex() const {
  return IDLParser::RuleInteger_type;
}

void IDLParser::Integer_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInteger_type(this);
}

void IDLParser::Integer_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInteger_type(this);
}


antlrcpp::Any IDLParser::Integer_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitInteger_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Integer_typeContext* IDLParser::integer_type() {
  Integer_typeContext *_localctx = _tracker.createInstance<Integer_typeContext>(_ctx, getState());
  enterRule(_localctx, 36, IDLParser::RuleInteger_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(194);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_SHORT:
      case IDLParser::KW_LONG:
      case IDLParser::KW_INT8:
      case IDLParser::KW_INT16:
      case IDLParser::KW_INT32:
      case IDLParser::KW_INT64: {
        enterOuterAlt(_localctx, 1);
        setState(192);
        signed_int();
        break;
      }

      case IDLParser::KW_UNSIGNED:
      case IDLParser::KW_UINT8:
      case IDLParser::KW_UINT16:
      case IDLParser::KW_UINT32:
      case IDLParser::KW_UINT64: {
        enterOuterAlt(_localctx, 2);
        setState(193);
        unsigned_int();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Signed_intContext ------------------------------------------------------------------

IDLParser::Signed_intContext::Signed_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Signed_short_intContext* IDLParser::Signed_intContext::signed_short_int() {
  return getRuleContext<IDLParser::Signed_short_intContext>(0);
}

IDLParser::Signed_long_intContext* IDLParser::Signed_intContext::signed_long_int() {
  return getRuleContext<IDLParser::Signed_long_intContext>(0);
}

IDLParser::Signed_longlong_intContext* IDLParser::Signed_intContext::signed_longlong_int() {
  return getRuleContext<IDLParser::Signed_longlong_intContext>(0);
}

IDLParser::Signed_tiny_intContext* IDLParser::Signed_intContext::signed_tiny_int() {
  return getRuleContext<IDLParser::Signed_tiny_intContext>(0);
}


size_t IDLParser::Signed_intContext::getRuleIndex() const {
  return IDLParser::RuleSigned_int;
}

void IDLParser::Signed_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSigned_int(this);
}

void IDLParser::Signed_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSigned_int(this);
}


antlrcpp::Any IDLParser::Signed_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSigned_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Signed_intContext* IDLParser::signed_int() {
  Signed_intContext *_localctx = _tracker.createInstance<Signed_intContext>(_ctx, getState());
  enterRule(_localctx, 38, IDLParser::RuleSigned_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(200);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(196);
      signed_short_int();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(197);
      signed_long_int();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(198);
      signed_longlong_int();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(199);
      signed_tiny_int();
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Signed_tiny_intContext ------------------------------------------------------------------

IDLParser::Signed_tiny_intContext::Signed_tiny_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Signed_tiny_intContext::KW_INT8() {
  return getToken(IDLParser::KW_INT8, 0);
}


size_t IDLParser::Signed_tiny_intContext::getRuleIndex() const {
  return IDLParser::RuleSigned_tiny_int;
}

void IDLParser::Signed_tiny_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSigned_tiny_int(this);
}

void IDLParser::Signed_tiny_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSigned_tiny_int(this);
}


antlrcpp::Any IDLParser::Signed_tiny_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSigned_tiny_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Signed_tiny_intContext* IDLParser::signed_tiny_int() {
  Signed_tiny_intContext *_localctx = _tracker.createInstance<Signed_tiny_intContext>(_ctx, getState());
  enterRule(_localctx, 40, IDLParser::RuleSigned_tiny_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(202);
    match(IDLParser::KW_INT8);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Signed_short_intContext ------------------------------------------------------------------

IDLParser::Signed_short_intContext::Signed_short_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Signed_short_intContext::KW_SHORT() {
  return getToken(IDLParser::KW_SHORT, 0);
}

tree::TerminalNode* IDLParser::Signed_short_intContext::KW_INT16() {
  return getToken(IDLParser::KW_INT16, 0);
}


size_t IDLParser::Signed_short_intContext::getRuleIndex() const {
  return IDLParser::RuleSigned_short_int;
}

void IDLParser::Signed_short_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSigned_short_int(this);
}

void IDLParser::Signed_short_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSigned_short_int(this);
}


antlrcpp::Any IDLParser::Signed_short_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSigned_short_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Signed_short_intContext* IDLParser::signed_short_int() {
  Signed_short_intContext *_localctx = _tracker.createInstance<Signed_short_intContext>(_ctx, getState());
  enterRule(_localctx, 42, IDLParser::RuleSigned_short_int);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(204);
    _la = _input->LA(1);
    if (!(_la == IDLParser::KW_SHORT

    || _la == IDLParser::KW_INT16)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Signed_long_intContext ------------------------------------------------------------------

IDLParser::Signed_long_intContext::Signed_long_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Signed_long_intContext::KW_LONG() {
  return getToken(IDLParser::KW_LONG, 0);
}

tree::TerminalNode* IDLParser::Signed_long_intContext::KW_INT32() {
  return getToken(IDLParser::KW_INT32, 0);
}


size_t IDLParser::Signed_long_intContext::getRuleIndex() const {
  return IDLParser::RuleSigned_long_int;
}

void IDLParser::Signed_long_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSigned_long_int(this);
}

void IDLParser::Signed_long_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSigned_long_int(this);
}


antlrcpp::Any IDLParser::Signed_long_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSigned_long_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Signed_long_intContext* IDLParser::signed_long_int() {
  Signed_long_intContext *_localctx = _tracker.createInstance<Signed_long_intContext>(_ctx, getState());
  enterRule(_localctx, 44, IDLParser::RuleSigned_long_int);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(206);
    _la = _input->LA(1);
    if (!(_la == IDLParser::KW_LONG

    || _la == IDLParser::KW_INT32)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Signed_longlong_intContext ------------------------------------------------------------------

IDLParser::Signed_longlong_intContext::Signed_longlong_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> IDLParser::Signed_longlong_intContext::KW_LONG() {
  return getTokens(IDLParser::KW_LONG);
}

tree::TerminalNode* IDLParser::Signed_longlong_intContext::KW_LONG(size_t i) {
  return getToken(IDLParser::KW_LONG, i);
}

tree::TerminalNode* IDLParser::Signed_longlong_intContext::KW_INT64() {
  return getToken(IDLParser::KW_INT64, 0);
}


size_t IDLParser::Signed_longlong_intContext::getRuleIndex() const {
  return IDLParser::RuleSigned_longlong_int;
}

void IDLParser::Signed_longlong_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSigned_longlong_int(this);
}

void IDLParser::Signed_longlong_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSigned_longlong_int(this);
}


antlrcpp::Any IDLParser::Signed_longlong_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSigned_longlong_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Signed_longlong_intContext* IDLParser::signed_longlong_int() {
  Signed_longlong_intContext *_localctx = _tracker.createInstance<Signed_longlong_intContext>(_ctx, getState());
  enterRule(_localctx, 46, IDLParser::RuleSigned_longlong_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(211);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_LONG: {
        enterOuterAlt(_localctx, 1);
        setState(208);
        match(IDLParser::KW_LONG);
        setState(209);
        match(IDLParser::KW_LONG);
        break;
      }

      case IDLParser::KW_INT64: {
        enterOuterAlt(_localctx, 2);
        setState(210);
        match(IDLParser::KW_INT64);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unsigned_intContext ------------------------------------------------------------------

IDLParser::Unsigned_intContext::Unsigned_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Unsigned_short_intContext* IDLParser::Unsigned_intContext::unsigned_short_int() {
  return getRuleContext<IDLParser::Unsigned_short_intContext>(0);
}

IDLParser::Unsigned_long_intContext* IDLParser::Unsigned_intContext::unsigned_long_int() {
  return getRuleContext<IDLParser::Unsigned_long_intContext>(0);
}

IDLParser::Unsigned_longlong_intContext* IDLParser::Unsigned_intContext::unsigned_longlong_int() {
  return getRuleContext<IDLParser::Unsigned_longlong_intContext>(0);
}

IDLParser::Unsigned_tiny_intContext* IDLParser::Unsigned_intContext::unsigned_tiny_int() {
  return getRuleContext<IDLParser::Unsigned_tiny_intContext>(0);
}


size_t IDLParser::Unsigned_intContext::getRuleIndex() const {
  return IDLParser::RuleUnsigned_int;
}

void IDLParser::Unsigned_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsigned_int(this);
}

void IDLParser::Unsigned_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsigned_int(this);
}


antlrcpp::Any IDLParser::Unsigned_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitUnsigned_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Unsigned_intContext* IDLParser::unsigned_int() {
  Unsigned_intContext *_localctx = _tracker.createInstance<Unsigned_intContext>(_ctx, getState());
  enterRule(_localctx, 48, IDLParser::RuleUnsigned_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(217);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(213);
      unsigned_short_int();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(214);
      unsigned_long_int();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(215);
      unsigned_longlong_int();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(216);
      unsigned_tiny_int();
      break;
    }

    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unsigned_tiny_intContext ------------------------------------------------------------------

IDLParser::Unsigned_tiny_intContext::Unsigned_tiny_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Unsigned_tiny_intContext::KW_UINT8() {
  return getToken(IDLParser::KW_UINT8, 0);
}


size_t IDLParser::Unsigned_tiny_intContext::getRuleIndex() const {
  return IDLParser::RuleUnsigned_tiny_int;
}

void IDLParser::Unsigned_tiny_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsigned_tiny_int(this);
}

void IDLParser::Unsigned_tiny_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsigned_tiny_int(this);
}


antlrcpp::Any IDLParser::Unsigned_tiny_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitUnsigned_tiny_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Unsigned_tiny_intContext* IDLParser::unsigned_tiny_int() {
  Unsigned_tiny_intContext *_localctx = _tracker.createInstance<Unsigned_tiny_intContext>(_ctx, getState());
  enterRule(_localctx, 50, IDLParser::RuleUnsigned_tiny_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(219);
    match(IDLParser::KW_UINT8);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unsigned_short_intContext ------------------------------------------------------------------

IDLParser::Unsigned_short_intContext::Unsigned_short_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Unsigned_short_intContext::KW_UNSIGNED() {
  return getToken(IDLParser::KW_UNSIGNED, 0);
}

tree::TerminalNode* IDLParser::Unsigned_short_intContext::KW_SHORT() {
  return getToken(IDLParser::KW_SHORT, 0);
}

tree::TerminalNode* IDLParser::Unsigned_short_intContext::KW_UINT16() {
  return getToken(IDLParser::KW_UINT16, 0);
}


size_t IDLParser::Unsigned_short_intContext::getRuleIndex() const {
  return IDLParser::RuleUnsigned_short_int;
}

void IDLParser::Unsigned_short_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsigned_short_int(this);
}

void IDLParser::Unsigned_short_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsigned_short_int(this);
}


antlrcpp::Any IDLParser::Unsigned_short_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitUnsigned_short_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Unsigned_short_intContext* IDLParser::unsigned_short_int() {
  Unsigned_short_intContext *_localctx = _tracker.createInstance<Unsigned_short_intContext>(_ctx, getState());
  enterRule(_localctx, 52, IDLParser::RuleUnsigned_short_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(224);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_UNSIGNED: {
        enterOuterAlt(_localctx, 1);
        setState(221);
        match(IDLParser::KW_UNSIGNED);
        setState(222);
        match(IDLParser::KW_SHORT);
        break;
      }

      case IDLParser::KW_UINT16: {
        enterOuterAlt(_localctx, 2);
        setState(223);
        match(IDLParser::KW_UINT16);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unsigned_long_intContext ------------------------------------------------------------------

IDLParser::Unsigned_long_intContext::Unsigned_long_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Unsigned_long_intContext::KW_UNSIGNED() {
  return getToken(IDLParser::KW_UNSIGNED, 0);
}

tree::TerminalNode* IDLParser::Unsigned_long_intContext::KW_LONG() {
  return getToken(IDLParser::KW_LONG, 0);
}

tree::TerminalNode* IDLParser::Unsigned_long_intContext::KW_UINT32() {
  return getToken(IDLParser::KW_UINT32, 0);
}


size_t IDLParser::Unsigned_long_intContext::getRuleIndex() const {
  return IDLParser::RuleUnsigned_long_int;
}

void IDLParser::Unsigned_long_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsigned_long_int(this);
}

void IDLParser::Unsigned_long_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsigned_long_int(this);
}


antlrcpp::Any IDLParser::Unsigned_long_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitUnsigned_long_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Unsigned_long_intContext* IDLParser::unsigned_long_int() {
  Unsigned_long_intContext *_localctx = _tracker.createInstance<Unsigned_long_intContext>(_ctx, getState());
  enterRule(_localctx, 54, IDLParser::RuleUnsigned_long_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(229);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_UNSIGNED: {
        enterOuterAlt(_localctx, 1);
        setState(226);
        match(IDLParser::KW_UNSIGNED);
        setState(227);
        match(IDLParser::KW_LONG);
        break;
      }

      case IDLParser::KW_UINT32: {
        enterOuterAlt(_localctx, 2);
        setState(228);
        match(IDLParser::KW_UINT32);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unsigned_longlong_intContext ------------------------------------------------------------------

IDLParser::Unsigned_longlong_intContext::Unsigned_longlong_intContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Unsigned_longlong_intContext::KW_UNSIGNED() {
  return getToken(IDLParser::KW_UNSIGNED, 0);
}

std::vector<tree::TerminalNode *> IDLParser::Unsigned_longlong_intContext::KW_LONG() {
  return getTokens(IDLParser::KW_LONG);
}

tree::TerminalNode* IDLParser::Unsigned_longlong_intContext::KW_LONG(size_t i) {
  return getToken(IDLParser::KW_LONG, i);
}

tree::TerminalNode* IDLParser::Unsigned_longlong_intContext::KW_UINT64() {
  return getToken(IDLParser::KW_UINT64, 0);
}


size_t IDLParser::Unsigned_longlong_intContext::getRuleIndex() const {
  return IDLParser::RuleUnsigned_longlong_int;
}

void IDLParser::Unsigned_longlong_intContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsigned_longlong_int(this);
}

void IDLParser::Unsigned_longlong_intContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsigned_longlong_int(this);
}


antlrcpp::Any IDLParser::Unsigned_longlong_intContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitUnsigned_longlong_int(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Unsigned_longlong_intContext* IDLParser::unsigned_longlong_int() {
  Unsigned_longlong_intContext *_localctx = _tracker.createInstance<Unsigned_longlong_intContext>(_ctx, getState());
  enterRule(_localctx, 56, IDLParser::RuleUnsigned_longlong_int);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(235);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_UNSIGNED: {
        enterOuterAlt(_localctx, 1);
        setState(231);
        match(IDLParser::KW_UNSIGNED);
        setState(232);
        match(IDLParser::KW_LONG);
        setState(233);
        match(IDLParser::KW_LONG);
        break;
      }

      case IDLParser::KW_UINT64: {
        enterOuterAlt(_localctx, 2);
        setState(234);
        match(IDLParser::KW_UINT64);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Char_typeContext ------------------------------------------------------------------

IDLParser::Char_typeContext::Char_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Char_typeContext::KW_CHAR() {
  return getToken(IDLParser::KW_CHAR, 0);
}


size_t IDLParser::Char_typeContext::getRuleIndex() const {
  return IDLParser::RuleChar_type;
}

void IDLParser::Char_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterChar_type(this);
}

void IDLParser::Char_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitChar_type(this);
}


antlrcpp::Any IDLParser::Char_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitChar_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Char_typeContext* IDLParser::char_type() {
  Char_typeContext *_localctx = _tracker.createInstance<Char_typeContext>(_ctx, getState());
  enterRule(_localctx, 58, IDLParser::RuleChar_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(237);
    match(IDLParser::KW_CHAR);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Boolean_typeContext ------------------------------------------------------------------

IDLParser::Boolean_typeContext::Boolean_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Boolean_typeContext::KW_BOOLEAN() {
  return getToken(IDLParser::KW_BOOLEAN, 0);
}


size_t IDLParser::Boolean_typeContext::getRuleIndex() const {
  return IDLParser::RuleBoolean_type;
}

void IDLParser::Boolean_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBoolean_type(this);
}

void IDLParser::Boolean_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBoolean_type(this);
}


antlrcpp::Any IDLParser::Boolean_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitBoolean_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Boolean_typeContext* IDLParser::boolean_type() {
  Boolean_typeContext *_localctx = _tracker.createInstance<Boolean_typeContext>(_ctx, getState());
  enterRule(_localctx, 60, IDLParser::RuleBoolean_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(239);
    match(IDLParser::KW_BOOLEAN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Octet_typeContext ------------------------------------------------------------------

IDLParser::Octet_typeContext::Octet_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Octet_typeContext::KW_OCTET() {
  return getToken(IDLParser::KW_OCTET, 0);
}


size_t IDLParser::Octet_typeContext::getRuleIndex() const {
  return IDLParser::RuleOctet_type;
}

void IDLParser::Octet_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterOctet_type(this);
}

void IDLParser::Octet_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitOctet_type(this);
}


antlrcpp::Any IDLParser::Octet_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitOctet_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Octet_typeContext* IDLParser::octet_type() {
  Octet_typeContext *_localctx = _tracker.createInstance<Octet_typeContext>(_ctx, getState());
  enterRule(_localctx, 62, IDLParser::RuleOctet_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(241);
    match(IDLParser::KW_OCTET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Struct_typeContext ------------------------------------------------------------------

IDLParser::Struct_typeContext::Struct_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Struct_typeContext::KW_STRUCT() {
  return getToken(IDLParser::KW_STRUCT, 0);
}

IDLParser::IdentifierContext* IDLParser::Struct_typeContext::identifier() {
  return getRuleContext<IDLParser::IdentifierContext>(0);
}

tree::TerminalNode* IDLParser::Struct_typeContext::LEFT_BRACE() {
  return getToken(IDLParser::LEFT_BRACE, 0);
}

IDLParser::Member_listContext* IDLParser::Struct_typeContext::member_list() {
  return getRuleContext<IDLParser::Member_listContext>(0);
}

tree::TerminalNode* IDLParser::Struct_typeContext::RIGHT_BRACE() {
  return getToken(IDLParser::RIGHT_BRACE, 0);
}


size_t IDLParser::Struct_typeContext::getRuleIndex() const {
  return IDLParser::RuleStruct_type;
}

void IDLParser::Struct_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStruct_type(this);
}

void IDLParser::Struct_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStruct_type(this);
}


antlrcpp::Any IDLParser::Struct_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitStruct_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Struct_typeContext* IDLParser::struct_type() {
  Struct_typeContext *_localctx = _tracker.createInstance<Struct_typeContext>(_ctx, getState());
  enterRule(_localctx, 64, IDLParser::RuleStruct_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(243);
    match(IDLParser::KW_STRUCT);
    setState(244);
    identifier();
    setState(245);
    match(IDLParser::LEFT_BRACE);
    setState(246);
    member_list();
    setState(247);
    match(IDLParser::RIGHT_BRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Member_listContext ------------------------------------------------------------------

IDLParser::Member_listContext::Member_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<IDLParser::MemberContext *> IDLParser::Member_listContext::member() {
  return getRuleContexts<IDLParser::MemberContext>();
}

IDLParser::MemberContext* IDLParser::Member_listContext::member(size_t i) {
  return getRuleContext<IDLParser::MemberContext>(i);
}


size_t IDLParser::Member_listContext::getRuleIndex() const {
  return IDLParser::RuleMember_list;
}

void IDLParser::Member_listContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMember_list(this);
}

void IDLParser::Member_listContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMember_list(this);
}


antlrcpp::Any IDLParser::Member_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitMember_list(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Member_listContext* IDLParser::member_list() {
  Member_listContext *_localctx = _tracker.createInstance<Member_listContext>(_ctx, getState());
  enterRule(_localctx, 66, IDLParser::RuleMember_list);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(252);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << IDLParser::AT)
      | (1ULL << IDLParser::KW_OCTET)
      | (1ULL << IDLParser::KW_STRUCT)
      | (1ULL << IDLParser::KW_SHORT)
      | (1ULL << IDLParser::KW_LONG)
      | (1ULL << IDLParser::KW_ENUM))) != 0) || ((((_la - 77) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 77)) & ((1ULL << (IDLParser::KW_UNSIGNED - 77))
      | (1ULL << (IDLParser::KW_UNION - 77))
      | (1ULL << (IDLParser::KW_CHAR - 77))
      | (1ULL << (IDLParser::KW_FLOAT - 77))
      | (1ULL << (IDLParser::KW_BOOLEAN - 77))
      | (1ULL << (IDLParser::KW_DOUBLE - 77))
      | (1ULL << (IDLParser::KW_INT8 - 77))
      | (1ULL << (IDLParser::KW_UINT8 - 77))
      | (1ULL << (IDLParser::KW_INT16 - 77))
      | (1ULL << (IDLParser::KW_UINT16 - 77))
      | (1ULL << (IDLParser::KW_INT32 - 77))
      | (1ULL << (IDLParser::KW_UINT32 - 77))
      | (1ULL << (IDLParser::KW_INT64 - 77))
      | (1ULL << (IDLParser::KW_UINT64 - 77)))) != 0)) {
      setState(249);
      member();
      setState(254);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MemberContext ------------------------------------------------------------------

IDLParser::MemberContext::MemberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::MemberContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

IDLParser::Type_specContext* IDLParser::MemberContext::type_spec() {
  return getRuleContext<IDLParser::Type_specContext>(0);
}

IDLParser::DeclaratorsContext* IDLParser::MemberContext::declarators() {
  return getRuleContext<IDLParser::DeclaratorsContext>(0);
}

tree::TerminalNode* IDLParser::MemberContext::SEMICOLON() {
  return getToken(IDLParser::SEMICOLON, 0);
}


size_t IDLParser::MemberContext::getRuleIndex() const {
  return IDLParser::RuleMember;
}

void IDLParser::MemberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMember(this);
}

void IDLParser::MemberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMember(this);
}


antlrcpp::Any IDLParser::MemberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitMember(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::MemberContext* IDLParser::member() {
  MemberContext *_localctx = _tracker.createInstance<MemberContext>(_ctx, getState());
  enterRule(_localctx, 68, IDLParser::RuleMember);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(255);
    annapps();
    setState(256);
    type_spec();
    setState(257);
    declarators();
    setState(258);
    match(IDLParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Union_typeContext ------------------------------------------------------------------

IDLParser::Union_typeContext::Union_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Union_typeContext::KW_UNION() {
  return getToken(IDLParser::KW_UNION, 0);
}

IDLParser::IdentifierContext* IDLParser::Union_typeContext::identifier() {
  return getRuleContext<IDLParser::IdentifierContext>(0);
}

tree::TerminalNode* IDLParser::Union_typeContext::KW_SWITCH() {
  return getToken(IDLParser::KW_SWITCH, 0);
}

tree::TerminalNode* IDLParser::Union_typeContext::LEFT_BRACKET() {
  return getToken(IDLParser::LEFT_BRACKET, 0);
}

IDLParser::AnnappsContext* IDLParser::Union_typeContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

IDLParser::Switch_type_specContext* IDLParser::Union_typeContext::switch_type_spec() {
  return getRuleContext<IDLParser::Switch_type_specContext>(0);
}

tree::TerminalNode* IDLParser::Union_typeContext::RIGHT_BRACKET() {
  return getToken(IDLParser::RIGHT_BRACKET, 0);
}

tree::TerminalNode* IDLParser::Union_typeContext::LEFT_BRACE() {
  return getToken(IDLParser::LEFT_BRACE, 0);
}

IDLParser::Switch_bodyContext* IDLParser::Union_typeContext::switch_body() {
  return getRuleContext<IDLParser::Switch_bodyContext>(0);
}

tree::TerminalNode* IDLParser::Union_typeContext::RIGHT_BRACE() {
  return getToken(IDLParser::RIGHT_BRACE, 0);
}


size_t IDLParser::Union_typeContext::getRuleIndex() const {
  return IDLParser::RuleUnion_type;
}

void IDLParser::Union_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnion_type(this);
}

void IDLParser::Union_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnion_type(this);
}


antlrcpp::Any IDLParser::Union_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitUnion_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Union_typeContext* IDLParser::union_type() {
  Union_typeContext *_localctx = _tracker.createInstance<Union_typeContext>(_ctx, getState());
  enterRule(_localctx, 70, IDLParser::RuleUnion_type);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(260);
    match(IDLParser::KW_UNION);
    setState(261);
    identifier();
    setState(262);
    match(IDLParser::KW_SWITCH);
    setState(263);
    match(IDLParser::LEFT_BRACKET);
    setState(264);
    annapps();
    setState(265);
    switch_type_spec();
    setState(266);
    match(IDLParser::RIGHT_BRACKET);
    setState(267);
    match(IDLParser::LEFT_BRACE);
    setState(268);
    switch_body();
    setState(269);
    match(IDLParser::RIGHT_BRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Switch_type_specContext ------------------------------------------------------------------

IDLParser::Switch_type_specContext::Switch_type_specContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Integer_typeContext* IDLParser::Switch_type_specContext::integer_type() {
  return getRuleContext<IDLParser::Integer_typeContext>(0);
}

IDLParser::Enum_typeContext* IDLParser::Switch_type_specContext::enum_type() {
  return getRuleContext<IDLParser::Enum_typeContext>(0);
}


size_t IDLParser::Switch_type_specContext::getRuleIndex() const {
  return IDLParser::RuleSwitch_type_spec;
}

void IDLParser::Switch_type_specContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSwitch_type_spec(this);
}

void IDLParser::Switch_type_specContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSwitch_type_spec(this);
}


antlrcpp::Any IDLParser::Switch_type_specContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSwitch_type_spec(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Switch_type_specContext* IDLParser::switch_type_spec() {
  Switch_type_specContext *_localctx = _tracker.createInstance<Switch_type_specContext>(_ctx, getState());
  enterRule(_localctx, 72, IDLParser::RuleSwitch_type_spec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(273);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_SHORT:
      case IDLParser::KW_LONG:
      case IDLParser::KW_UNSIGNED:
      case IDLParser::KW_INT8:
      case IDLParser::KW_UINT8:
      case IDLParser::KW_INT16:
      case IDLParser::KW_UINT16:
      case IDLParser::KW_INT32:
      case IDLParser::KW_UINT32:
      case IDLParser::KW_INT64:
      case IDLParser::KW_UINT64: {
        enterOuterAlt(_localctx, 1);
        setState(271);
        integer_type();
        break;
      }

      case IDLParser::KW_ENUM: {
        enterOuterAlt(_localctx, 2);
        setState(272);
        enum_type();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Switch_bodyContext ------------------------------------------------------------------

IDLParser::Switch_bodyContext::Switch_bodyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<IDLParser::Case_stmtContext *> IDLParser::Switch_bodyContext::case_stmt() {
  return getRuleContexts<IDLParser::Case_stmtContext>();
}

IDLParser::Case_stmtContext* IDLParser::Switch_bodyContext::case_stmt(size_t i) {
  return getRuleContext<IDLParser::Case_stmtContext>(i);
}


size_t IDLParser::Switch_bodyContext::getRuleIndex() const {
  return IDLParser::RuleSwitch_body;
}

void IDLParser::Switch_bodyContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSwitch_body(this);
}

void IDLParser::Switch_bodyContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSwitch_body(this);
}


antlrcpp::Any IDLParser::Switch_bodyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitSwitch_body(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Switch_bodyContext* IDLParser::switch_body() {
  Switch_bodyContext *_localctx = _tracker.createInstance<Switch_bodyContext>(_ctx, getState());
  enterRule(_localctx, 74, IDLParser::RuleSwitch_body);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(276); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(275);
      case_stmt();
      setState(278); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (((((_la - 35) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 35)) & ((1ULL << (IDLParser::AT - 35))
      | (1ULL << (IDLParser::KW_DEFAULT - 35))
      | (1ULL << (IDLParser::KW_CASE - 35)))) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Case_stmtContext ------------------------------------------------------------------

IDLParser::Case_stmtContext::Case_stmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Element_specContext* IDLParser::Case_stmtContext::element_spec() {
  return getRuleContext<IDLParser::Element_specContext>(0);
}

tree::TerminalNode* IDLParser::Case_stmtContext::SEMICOLON() {
  return getToken(IDLParser::SEMICOLON, 0);
}

std::vector<IDLParser::Case_labelContext *> IDLParser::Case_stmtContext::case_label() {
  return getRuleContexts<IDLParser::Case_labelContext>();
}

IDLParser::Case_labelContext* IDLParser::Case_stmtContext::case_label(size_t i) {
  return getRuleContext<IDLParser::Case_labelContext>(i);
}


size_t IDLParser::Case_stmtContext::getRuleIndex() const {
  return IDLParser::RuleCase_stmt;
}

void IDLParser::Case_stmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCase_stmt(this);
}

void IDLParser::Case_stmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCase_stmt(this);
}


antlrcpp::Any IDLParser::Case_stmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitCase_stmt(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Case_stmtContext* IDLParser::case_stmt() {
  Case_stmtContext *_localctx = _tracker.createInstance<Case_stmtContext>(_ctx, getState());
  enterRule(_localctx, 76, IDLParser::RuleCase_stmt);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(281); 
    _errHandler->sync(this);
    alt = 1;
    do {
      switch (alt) {
        case 1: {
              setState(280);
              case_label();
              break;
            }

      default:
        throw NoViableAltException(this);
      }
      setState(283); 
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx);
    } while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER);
    setState(285);
    element_spec();
    setState(286);
    match(IDLParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Case_labelContext ------------------------------------------------------------------

IDLParser::Case_labelContext::Case_labelContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::Case_labelContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

tree::TerminalNode* IDLParser::Case_labelContext::KW_CASE() {
  return getToken(IDLParser::KW_CASE, 0);
}

IDLParser::Const_expContext* IDLParser::Case_labelContext::const_exp() {
  return getRuleContext<IDLParser::Const_expContext>(0);
}

tree::TerminalNode* IDLParser::Case_labelContext::COLON() {
  return getToken(IDLParser::COLON, 0);
}

tree::TerminalNode* IDLParser::Case_labelContext::KW_DEFAULT() {
  return getToken(IDLParser::KW_DEFAULT, 0);
}


size_t IDLParser::Case_labelContext::getRuleIndex() const {
  return IDLParser::RuleCase_label;
}

void IDLParser::Case_labelContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCase_label(this);
}

void IDLParser::Case_labelContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCase_label(this);
}


antlrcpp::Any IDLParser::Case_labelContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitCase_label(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Case_labelContext* IDLParser::case_label() {
  Case_labelContext *_localctx = _tracker.createInstance<Case_labelContext>(_ctx, getState());
  enterRule(_localctx, 78, IDLParser::RuleCase_label);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(288);
    annapps();
    setState(295);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::KW_CASE: {
        setState(289);
        match(IDLParser::KW_CASE);
        setState(290);
        const_exp();
        setState(291);
        match(IDLParser::COLON);
        break;
      }

      case IDLParser::KW_DEFAULT: {
        setState(293);
        match(IDLParser::KW_DEFAULT);
        setState(294);
        match(IDLParser::COLON);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Element_specContext ------------------------------------------------------------------

IDLParser::Element_specContext::Element_specContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::Element_specContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

IDLParser::Type_specContext* IDLParser::Element_specContext::type_spec() {
  return getRuleContext<IDLParser::Type_specContext>(0);
}

IDLParser::DeclaratorContext* IDLParser::Element_specContext::declarator() {
  return getRuleContext<IDLParser::DeclaratorContext>(0);
}


size_t IDLParser::Element_specContext::getRuleIndex() const {
  return IDLParser::RuleElement_spec;
}

void IDLParser::Element_specContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterElement_spec(this);
}

void IDLParser::Element_specContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitElement_spec(this);
}


antlrcpp::Any IDLParser::Element_specContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitElement_spec(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Element_specContext* IDLParser::element_spec() {
  Element_specContext *_localctx = _tracker.createInstance<Element_specContext>(_ctx, getState());
  enterRule(_localctx, 80, IDLParser::RuleElement_spec);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(297);
    annapps();
    setState(298);
    type_spec();
    setState(299);
    declarator();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Enum_typeContext ------------------------------------------------------------------

IDLParser::Enum_typeContext::Enum_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Enum_typeContext::KW_ENUM() {
  return getToken(IDLParser::KW_ENUM, 0);
}

IDLParser::IdentifierContext* IDLParser::Enum_typeContext::identifier() {
  return getRuleContext<IDLParser::IdentifierContext>(0);
}

tree::TerminalNode* IDLParser::Enum_typeContext::LEFT_BRACE() {
  return getToken(IDLParser::LEFT_BRACE, 0);
}

std::vector<IDLParser::EnumeratorContext *> IDLParser::Enum_typeContext::enumerator() {
  return getRuleContexts<IDLParser::EnumeratorContext>();
}

IDLParser::EnumeratorContext* IDLParser::Enum_typeContext::enumerator(size_t i) {
  return getRuleContext<IDLParser::EnumeratorContext>(i);
}

tree::TerminalNode* IDLParser::Enum_typeContext::RIGHT_BRACE() {
  return getToken(IDLParser::RIGHT_BRACE, 0);
}

std::vector<tree::TerminalNode *> IDLParser::Enum_typeContext::COMMA() {
  return getTokens(IDLParser::COMMA);
}

tree::TerminalNode* IDLParser::Enum_typeContext::COMMA(size_t i) {
  return getToken(IDLParser::COMMA, i);
}


size_t IDLParser::Enum_typeContext::getRuleIndex() const {
  return IDLParser::RuleEnum_type;
}

void IDLParser::Enum_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEnum_type(this);
}

void IDLParser::Enum_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEnum_type(this);
}


antlrcpp::Any IDLParser::Enum_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitEnum_type(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Enum_typeContext* IDLParser::enum_type() {
  Enum_typeContext *_localctx = _tracker.createInstance<Enum_typeContext>(_ctx, getState());
  enterRule(_localctx, 82, IDLParser::RuleEnum_type);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(301);
    match(IDLParser::KW_ENUM);
    setState(302);
    identifier();
    setState(303);
    match(IDLParser::LEFT_BRACE);
    setState(304);
    enumerator();
    setState(309);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == IDLParser::COMMA) {
      setState(305);
      match(IDLParser::COMMA);
      setState(306);
      enumerator();
      setState(311);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(312);
    match(IDLParser::RIGHT_BRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EnumeratorContext ------------------------------------------------------------------

IDLParser::EnumeratorContext::EnumeratorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::IdentifierContext* IDLParser::EnumeratorContext::identifier() {
  return getRuleContext<IDLParser::IdentifierContext>(0);
}


size_t IDLParser::EnumeratorContext::getRuleIndex() const {
  return IDLParser::RuleEnumerator;
}

void IDLParser::EnumeratorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEnumerator(this);
}

void IDLParser::EnumeratorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEnumerator(this);
}


antlrcpp::Any IDLParser::EnumeratorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitEnumerator(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::EnumeratorContext* IDLParser::enumerator() {
  EnumeratorContext *_localctx = _tracker.createInstance<EnumeratorContext>(_ctx, getState());
  enterRule(_localctx, 84, IDLParser::RuleEnumerator);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(314);
    identifier();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Array_declaratorContext ------------------------------------------------------------------

IDLParser::Array_declaratorContext::Array_declaratorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Array_declaratorContext::ID() {
  return getToken(IDLParser::ID, 0);
}

std::vector<IDLParser::Fixed_array_sizeContext *> IDLParser::Array_declaratorContext::fixed_array_size() {
  return getRuleContexts<IDLParser::Fixed_array_sizeContext>();
}

IDLParser::Fixed_array_sizeContext* IDLParser::Array_declaratorContext::fixed_array_size(size_t i) {
  return getRuleContext<IDLParser::Fixed_array_sizeContext>(i);
}


size_t IDLParser::Array_declaratorContext::getRuleIndex() const {
  return IDLParser::RuleArray_declarator;
}

void IDLParser::Array_declaratorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArray_declarator(this);
}

void IDLParser::Array_declaratorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArray_declarator(this);
}


antlrcpp::Any IDLParser::Array_declaratorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitArray_declarator(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Array_declaratorContext* IDLParser::array_declarator() {
  Array_declaratorContext *_localctx = _tracker.createInstance<Array_declaratorContext>(_ctx, getState());
  enterRule(_localctx, 86, IDLParser::RuleArray_declarator);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(316);
    match(IDLParser::ID);
    setState(318); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(317);
      fixed_array_size();
      setState(320); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == IDLParser::LEFT_SQUARE_BRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Fixed_array_sizeContext ------------------------------------------------------------------

IDLParser::Fixed_array_sizeContext::Fixed_array_sizeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Fixed_array_sizeContext::LEFT_SQUARE_BRACKET() {
  return getToken(IDLParser::LEFT_SQUARE_BRACKET, 0);
}

IDLParser::Positive_int_constContext* IDLParser::Fixed_array_sizeContext::positive_int_const() {
  return getRuleContext<IDLParser::Positive_int_constContext>(0);
}

tree::TerminalNode* IDLParser::Fixed_array_sizeContext::RIGHT_SQUARE_BRACKET() {
  return getToken(IDLParser::RIGHT_SQUARE_BRACKET, 0);
}


size_t IDLParser::Fixed_array_sizeContext::getRuleIndex() const {
  return IDLParser::RuleFixed_array_size;
}

void IDLParser::Fixed_array_sizeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFixed_array_size(this);
}

void IDLParser::Fixed_array_sizeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFixed_array_size(this);
}


antlrcpp::Any IDLParser::Fixed_array_sizeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitFixed_array_size(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Fixed_array_sizeContext* IDLParser::fixed_array_size() {
  Fixed_array_sizeContext *_localctx = _tracker.createInstance<Fixed_array_sizeContext>(_ctx, getState());
  enterRule(_localctx, 88, IDLParser::RuleFixed_array_size);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(322);
    match(IDLParser::LEFT_SQUARE_BRACKET);
    setState(323);
    positive_int_const();
    setState(324);
    match(IDLParser::RIGHT_SQUARE_BRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnnappsContext ------------------------------------------------------------------

IDLParser::AnnappsContext::AnnappsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<IDLParser::Annotation_applContext *> IDLParser::AnnappsContext::annotation_appl() {
  return getRuleContexts<IDLParser::Annotation_applContext>();
}

IDLParser::Annotation_applContext* IDLParser::AnnappsContext::annotation_appl(size_t i) {
  return getRuleContext<IDLParser::Annotation_applContext>(i);
}


size_t IDLParser::AnnappsContext::getRuleIndex() const {
  return IDLParser::RuleAnnapps;
}

void IDLParser::AnnappsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAnnapps(this);
}

void IDLParser::AnnappsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAnnapps(this);
}


antlrcpp::Any IDLParser::AnnappsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitAnnapps(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::AnnappsContext* IDLParser::annapps() {
  AnnappsContext *_localctx = _tracker.createInstance<AnnappsContext>(_ctx, getState());
  enterRule(_localctx, 90, IDLParser::RuleAnnapps);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(329);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == IDLParser::AT) {
      setState(326);
      annotation_appl();
      setState(331);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Annotation_applContext ------------------------------------------------------------------

IDLParser::Annotation_applContext::Annotation_applContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Annotation_applContext::AT() {
  return getToken(IDLParser::AT, 0);
}

IDLParser::Scoped_nameContext* IDLParser::Annotation_applContext::scoped_name() {
  return getRuleContext<IDLParser::Scoped_nameContext>(0);
}

tree::TerminalNode* IDLParser::Annotation_applContext::LEFT_BRACKET() {
  return getToken(IDLParser::LEFT_BRACKET, 0);
}

IDLParser::Annotation_appl_paramsContext* IDLParser::Annotation_applContext::annotation_appl_params() {
  return getRuleContext<IDLParser::Annotation_appl_paramsContext>(0);
}

tree::TerminalNode* IDLParser::Annotation_applContext::RIGHT_BRACKET() {
  return getToken(IDLParser::RIGHT_BRACKET, 0);
}


size_t IDLParser::Annotation_applContext::getRuleIndex() const {
  return IDLParser::RuleAnnotation_appl;
}

void IDLParser::Annotation_applContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAnnotation_appl(this);
}

void IDLParser::Annotation_applContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAnnotation_appl(this);
}


antlrcpp::Any IDLParser::Annotation_applContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitAnnotation_appl(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Annotation_applContext* IDLParser::annotation_appl() {
  Annotation_applContext *_localctx = _tracker.createInstance<Annotation_applContext>(_ctx, getState());
  enterRule(_localctx, 92, IDLParser::RuleAnnotation_appl);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(332);
    match(IDLParser::AT);
    setState(333);
    scoped_name();
    setState(338);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == IDLParser::LEFT_BRACKET) {
      setState(334);
      match(IDLParser::LEFT_BRACKET);
      setState(335);
      annotation_appl_params();
      setState(336);
      match(IDLParser::RIGHT_BRACKET);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Annotation_appl_paramsContext ------------------------------------------------------------------

IDLParser::Annotation_appl_paramsContext::Annotation_appl_paramsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::Const_expContext* IDLParser::Annotation_appl_paramsContext::const_exp() {
  return getRuleContext<IDLParser::Const_expContext>(0);
}

std::vector<IDLParser::Annotation_appl_paramContext *> IDLParser::Annotation_appl_paramsContext::annotation_appl_param() {
  return getRuleContexts<IDLParser::Annotation_appl_paramContext>();
}

IDLParser::Annotation_appl_paramContext* IDLParser::Annotation_appl_paramsContext::annotation_appl_param(size_t i) {
  return getRuleContext<IDLParser::Annotation_appl_paramContext>(i);
}

std::vector<tree::TerminalNode *> IDLParser::Annotation_appl_paramsContext::COMMA() {
  return getTokens(IDLParser::COMMA);
}

tree::TerminalNode* IDLParser::Annotation_appl_paramsContext::COMMA(size_t i) {
  return getToken(IDLParser::COMMA, i);
}


size_t IDLParser::Annotation_appl_paramsContext::getRuleIndex() const {
  return IDLParser::RuleAnnotation_appl_params;
}

void IDLParser::Annotation_appl_paramsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAnnotation_appl_params(this);
}

void IDLParser::Annotation_appl_paramsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAnnotation_appl_params(this);
}


antlrcpp::Any IDLParser::Annotation_appl_paramsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitAnnotation_appl_params(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Annotation_appl_paramsContext* IDLParser::annotation_appl_params() {
  Annotation_appl_paramsContext *_localctx = _tracker.createInstance<Annotation_appl_paramsContext>(_ctx, getState());
  enterRule(_localctx, 94, IDLParser::RuleAnnotation_appl_params);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(349);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case IDLParser::INTEGER_LITERAL:
      case IDLParser::OCTAL_LITERAL:
      case IDLParser::HEX_LITERAL:
      case IDLParser::FLOATING_PT_LITERAL:
      case IDLParser::FIXED_PT_LITERAL:
      case IDLParser::WIDE_CHARACTER_LITERAL:
      case IDLParser::CHARACTER_LITERAL:
      case IDLParser::WIDE_STRING_LITERAL:
      case IDLParser::STRING_LITERAL:
      case IDLParser::BOOLEAN_LITERAL: {
        enterOuterAlt(_localctx, 1);
        setState(340);
        const_exp();
        break;
      }

      case IDLParser::ID: {
        enterOuterAlt(_localctx, 2);
        setState(341);
        annotation_appl_param();
        setState(346);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == IDLParser::COMMA) {
          setState(342);
          match(IDLParser::COMMA);
          setState(343);
          annotation_appl_param();
          setState(348);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Annotation_appl_paramContext ------------------------------------------------------------------

IDLParser::Annotation_appl_paramContext::Annotation_appl_paramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* IDLParser::Annotation_appl_paramContext::ID() {
  return getToken(IDLParser::ID, 0);
}

tree::TerminalNode* IDLParser::Annotation_appl_paramContext::EQUAL() {
  return getToken(IDLParser::EQUAL, 0);
}

IDLParser::Const_expContext* IDLParser::Annotation_appl_paramContext::const_exp() {
  return getRuleContext<IDLParser::Const_expContext>(0);
}


size_t IDLParser::Annotation_appl_paramContext::getRuleIndex() const {
  return IDLParser::RuleAnnotation_appl_param;
}

void IDLParser::Annotation_appl_paramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAnnotation_appl_param(this);
}

void IDLParser::Annotation_appl_paramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAnnotation_appl_param(this);
}


antlrcpp::Any IDLParser::Annotation_appl_paramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitAnnotation_appl_param(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::Annotation_appl_paramContext* IDLParser::annotation_appl_param() {
  Annotation_appl_paramContext *_localctx = _tracker.createInstance<Annotation_appl_paramContext>(_ctx, getState());
  enterRule(_localctx, 96, IDLParser::RuleAnnotation_appl_param);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(351);
    match(IDLParser::ID);
    setState(352);
    match(IDLParser::EQUAL);
    setState(353);
    const_exp();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IdentifierContext ------------------------------------------------------------------

IDLParser::IdentifierContext::IdentifierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

IDLParser::AnnappsContext* IDLParser::IdentifierContext::annapps() {
  return getRuleContext<IDLParser::AnnappsContext>(0);
}

tree::TerminalNode* IDLParser::IdentifierContext::ID() {
  return getToken(IDLParser::ID, 0);
}


size_t IDLParser::IdentifierContext::getRuleIndex() const {
  return IDLParser::RuleIdentifier;
}

void IDLParser::IdentifierContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIdentifier(this);
}

void IDLParser::IdentifierContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<IDLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIdentifier(this);
}


antlrcpp::Any IDLParser::IdentifierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<IDLVisitor*>(visitor))
    return parserVisitor->visitIdentifier(this);
  else
    return visitor->visitChildren(this);
}

IDLParser::IdentifierContext* IDLParser::identifier() {
  IdentifierContext *_localctx = _tracker.createInstance<IdentifierContext>(_ctx, getState());
  enterRule(_localctx, 98, IDLParser::RuleIdentifier);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(355);
    annapps();
    setState(356);
    match(IDLParser::ID);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> IDLParser::_decisionToDFA;
atn::PredictionContextCache IDLParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN IDLParser::_atn;
std::vector<uint16_t> IDLParser::_serializedATN;

std::vector<std::string> IDLParser::_ruleNames = {
  "specification", "definition", "module", "a_scoped_name", "scoped_name", 
  "const_exp", "literal", "positive_int_const", "type_decl", "type_spec", 
  "simple_type_spec", "base_type_spec", "constr_type_spec", "declarators", 
  "declarator", "simple_declarator", "complex_declarator", "floating_pt_type", 
  "integer_type", "signed_int", "signed_tiny_int", "signed_short_int", "signed_long_int", 
  "signed_longlong_int", "unsigned_int", "unsigned_tiny_int", "unsigned_short_int", 
  "unsigned_long_int", "unsigned_longlong_int", "char_type", "boolean_type", 
  "octet_type", "struct_type", "member_list", "member", "union_type", "switch_type_spec", 
  "switch_body", "case_stmt", "case_label", "element_spec", "enum_type", 
  "enumerator", "array_declarator", "fixed_array_size", "annapps", "annotation_appl", 
  "annotation_appl_params", "annotation_appl_param", "identifier"
};

std::vector<std::string> IDLParser::_literalNames = {
  "", "", "", "", "", "", "", "", "", "", "", "';'", "':'", "','", "'{'", 
  "'}'", "'('", "')'", "'['", "']'", "'~'", "'/'", "'<'", "'>'", "'*'", 
  "'+'", "'-'", "'^'", "'&'", "'|'", "'='", "'%'", "'::'", "'>>'", "'<<'", 
  "'@'", "'setraises'", "'out'", "'emits'", "'string'", "'switch'", "'publishes'", 
  "'typedef'", "'uses'", "'primarykey'", "'custom'", "'octet'", "'sequence'", 
  "'import'", "'struct'", "'native'", "'readonly'", "'finder'", "'raises'", 
  "'void'", "'private'", "'eventtype'", "'wchar'", "'in'", "'default'", 
  "'public'", "'short'", "'long'", "'enum'", "'wstring'", "'context'", "'home'", 
  "'factory'", "'exception'", "'getraises'", "'const'", "'ValueBase'", "'valuetype'", 
  "'supports'", "'module'", "'Object'", "'truncatable'", "'unsigned'", "'fixed'", 
  "'union'", "'oneway'", "'any'", "'char'", "'case'", "'float'", "'boolean'", 
  "'multiple'", "'abstract'", "'inout'", "'provides'", "'consumes'", "'double'", 
  "'typeprefix'", "'typeid'", "'attribute'", "'local'", "'manages'", "'interface'", 
  "'component'", "'set'", "'map'", "'bitfield'", "'bitset'", "'bitmask'", 
  "'int8'", "'uint8'", "'int16'", "'uint16'", "'int32'", "'uint32'", "'int64'", 
  "'uint64'", "'@annotation'"
};

std::vector<std::string> IDLParser::_symbolicNames = {
  "", "INTEGER_LITERAL", "OCTAL_LITERAL", "HEX_LITERAL", "FLOATING_PT_LITERAL", 
  "FIXED_PT_LITERAL", "WIDE_CHARACTER_LITERAL", "CHARACTER_LITERAL", "WIDE_STRING_LITERAL", 
  "STRING_LITERAL", "BOOLEAN_LITERAL", "SEMICOLON", "COLON", "COMMA", "LEFT_BRACE", 
  "RIGHT_BRACE", "LEFT_BRACKET", "RIGHT_BRACKET", "LEFT_SQUARE_BRACKET", 
  "RIGHT_SQUARE_BRACKET", "TILDE", "SLASH", "LEFT_ANG_BRACKET", "RIGHT_ANG_BRACKET", 
  "STAR", "PLUS", "MINUS", "CARET", "AMPERSAND", "PIPE", "EQUAL", "PERCENT", 
  "DOUBLE_COLON", "RIGHT_SHIFT", "LEFT_SHIFT", "AT", "KW_SETRAISES", "KW_OUT", 
  "KW_EMITS", "KW_STRING", "KW_SWITCH", "KW_PUBLISHES", "KW_TYPEDEF", "KW_USES", 
  "KW_PRIMARYKEY", "KW_CUSTOM", "KW_OCTET", "KW_SEQUENCE", "KW_IMPORT", 
  "KW_STRUCT", "KW_NATIVE", "KW_READONLY", "KW_FINDER", "KW_RAISES", "KW_VOID", 
  "KW_PRIVATE", "KW_EVENTTYPE", "KW_WCHAR", "KW_IN", "KW_DEFAULT", "KW_PUBLIC", 
  "KW_SHORT", "KW_LONG", "KW_ENUM", "KW_WSTRING", "KW_CONTEXT", "KW_HOME", 
  "KW_FACTORY", "KW_EXCEPTION", "KW_GETRAISES", "KW_CONST", "KW_VALUEBASE", 
  "KW_VALUETYPE", "KW_SUPPORTS", "KW_MODULE", "KW_OBJECT", "KW_TRUNCATABLE", 
  "KW_UNSIGNED", "KW_FIXED", "KW_UNION", "KW_ONEWAY", "KW_ANY", "KW_CHAR", 
  "KW_CASE", "KW_FLOAT", "KW_BOOLEAN", "KW_MULTIPLE", "KW_ABSTRACT", "KW_INOUT", 
  "KW_PROVIDES", "KW_CONSUMES", "KW_DOUBLE", "KW_TYPEPREFIX", "KW_TYPEID", 
  "KW_ATTRIBUTE", "KW_LOCAL", "KW_MANAGES", "KW_INTERFACE", "KW_COMPONENT", 
  "KW_SET", "KW_MAP", "KW_BITFIELD", "KW_BITSET", "KW_BITMASK", "KW_INT8", 
  "KW_UINT8", "KW_INT16", "KW_UINT16", "KW_INT32", "KW_UINT32", "KW_INT64", 
  "KW_UINT64", "KW_AT_ANNOTATION", "ID", "WS", "COMMENT", "LINE_COMMENT"
};

dfa::Vocabulary IDLParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> IDLParser::_tokenNames;

IDLParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  _serializedATN = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
    0x3, 0x76, 0x169, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
    0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 0x7, 
    0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 0x4, 0xb, 
    0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 0xe, 0x9, 0xe, 
    0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 0x9, 0x11, 0x4, 
    0x12, 0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 0x9, 0x14, 0x4, 0x15, 
    0x9, 0x15, 0x4, 0x16, 0x9, 0x16, 0x4, 0x17, 0x9, 0x17, 0x4, 0x18, 0x9, 
    0x18, 0x4, 0x19, 0x9, 0x19, 0x4, 0x1a, 0x9, 0x1a, 0x4, 0x1b, 0x9, 0x1b, 
    0x4, 0x1c, 0x9, 0x1c, 0x4, 0x1d, 0x9, 0x1d, 0x4, 0x1e, 0x9, 0x1e, 0x4, 
    0x1f, 0x9, 0x1f, 0x4, 0x20, 0x9, 0x20, 0x4, 0x21, 0x9, 0x21, 0x4, 0x22, 
    0x9, 0x22, 0x4, 0x23, 0x9, 0x23, 0x4, 0x24, 0x9, 0x24, 0x4, 0x25, 0x9, 
    0x25, 0x4, 0x26, 0x9, 0x26, 0x4, 0x27, 0x9, 0x27, 0x4, 0x28, 0x9, 0x28, 
    0x4, 0x29, 0x9, 0x29, 0x4, 0x2a, 0x9, 0x2a, 0x4, 0x2b, 0x9, 0x2b, 0x4, 
    0x2c, 0x9, 0x2c, 0x4, 0x2d, 0x9, 0x2d, 0x4, 0x2e, 0x9, 0x2e, 0x4, 0x2f, 
    0x9, 0x2f, 0x4, 0x30, 0x9, 0x30, 0x4, 0x31, 0x9, 0x31, 0x4, 0x32, 0x9, 
    0x32, 0x4, 0x33, 0x9, 0x33, 0x3, 0x2, 0x6, 0x2, 0x68, 0xa, 0x2, 0xd, 
    0x2, 0xe, 0x2, 0x69, 0x3, 0x2, 0x3, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x5, 0x3, 0x75, 0xa, 0x3, 0x3, 
    0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x6, 0x4, 0x7b, 0xa, 0x4, 0xd, 0x4, 
    0xe, 0x4, 0x7c, 0x3, 0x4, 0x3, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 
    0x6, 0x5, 0x6, 0x85, 0xa, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x7, 0x6, 
    0x8a, 0xa, 0x6, 0xc, 0x6, 0xe, 0x6, 0x8d, 0xb, 0x6, 0x3, 0x7, 0x3, 0x7, 
    0x3, 0x8, 0x3, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 
    0x5, 0xa, 0x98, 0xa, 0xa, 0x3, 0xb, 0x3, 0xb, 0x5, 0xb, 0x9c, 0xa, 0xb, 
    0x3, 0xc, 0x3, 0xc, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 
    0x5, 0xd, 0xa5, 0xa, 0xd, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x5, 0xe, 0xaa, 
    0xa, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x7, 0xf, 0xaf, 0xa, 0xf, 0xc, 
    0xf, 0xe, 0xf, 0xb2, 0xb, 0xf, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x5, 
    0x10, 0xb7, 0xa, 0x10, 0x3, 0x11, 0x3, 0x11, 0x3, 0x12, 0x3, 0x12, 0x3, 
    0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x5, 0x13, 0xc1, 0xa, 0x13, 0x3, 
    0x14, 0x3, 0x14, 0x5, 0x14, 0xc5, 0xa, 0x14, 0x3, 0x15, 0x3, 0x15, 0x3, 
    0x15, 0x3, 0x15, 0x5, 0x15, 0xcb, 0xa, 0x15, 0x3, 0x16, 0x3, 0x16, 0x3, 
    0x17, 0x3, 0x17, 0x3, 0x18, 0x3, 0x18, 0x3, 0x19, 0x3, 0x19, 0x3, 0x19, 
    0x5, 0x19, 0xd6, 0xa, 0x19, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 
    0x5, 0x1a, 0xdc, 0xa, 0x1a, 0x3, 0x1b, 0x3, 0x1b, 0x3, 0x1c, 0x3, 0x1c, 
    0x3, 0x1c, 0x5, 0x1c, 0xe3, 0xa, 0x1c, 0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1d, 
    0x5, 0x1d, 0xe8, 0xa, 0x1d, 0x3, 0x1e, 0x3, 0x1e, 0x3, 0x1e, 0x3, 0x1e, 
    0x5, 0x1e, 0xee, 0xa, 0x1e, 0x3, 0x1f, 0x3, 0x1f, 0x3, 0x20, 0x3, 0x20, 
    0x3, 0x21, 0x3, 0x21, 0x3, 0x22, 0x3, 0x22, 0x3, 0x22, 0x3, 0x22, 0x3, 
    0x22, 0x3, 0x22, 0x3, 0x23, 0x7, 0x23, 0xfd, 0xa, 0x23, 0xc, 0x23, 0xe, 
    0x23, 0x100, 0xb, 0x23, 0x3, 0x24, 0x3, 0x24, 0x3, 0x24, 0x3, 0x24, 
    0x3, 0x24, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 
    0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 0x25, 0x3, 0x26, 
    0x3, 0x26, 0x5, 0x26, 0x114, 0xa, 0x26, 0x3, 0x27, 0x6, 0x27, 0x117, 
    0xa, 0x27, 0xd, 0x27, 0xe, 0x27, 0x118, 0x3, 0x28, 0x6, 0x28, 0x11c, 
    0xa, 0x28, 0xd, 0x28, 0xe, 0x28, 0x11d, 0x3, 0x28, 0x3, 0x28, 0x3, 0x28, 
    0x3, 0x29, 0x3, 0x29, 0x3, 0x29, 0x3, 0x29, 0x3, 0x29, 0x3, 0x29, 0x3, 
    0x29, 0x5, 0x29, 0x12a, 0xa, 0x29, 0x3, 0x2a, 0x3, 0x2a, 0x3, 0x2a, 
    0x3, 0x2a, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2b, 0x3, 
    0x2b, 0x7, 0x2b, 0x136, 0xa, 0x2b, 0xc, 0x2b, 0xe, 0x2b, 0x139, 0xb, 
    0x2b, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2c, 0x3, 0x2c, 0x3, 0x2d, 0x3, 0x2d, 
    0x6, 0x2d, 0x141, 0xa, 0x2d, 0xd, 0x2d, 0xe, 0x2d, 0x142, 0x3, 0x2e, 
    0x3, 0x2e, 0x3, 0x2e, 0x3, 0x2e, 0x3, 0x2f, 0x7, 0x2f, 0x14a, 0xa, 0x2f, 
    0xc, 0x2f, 0xe, 0x2f, 0x14d, 0xb, 0x2f, 0x3, 0x30, 0x3, 0x30, 0x3, 0x30, 
    0x3, 0x30, 0x3, 0x30, 0x3, 0x30, 0x5, 0x30, 0x155, 0xa, 0x30, 0x3, 0x31, 
    0x3, 0x31, 0x3, 0x31, 0x3, 0x31, 0x7, 0x31, 0x15b, 0xa, 0x31, 0xc, 0x31, 
    0xe, 0x31, 0x15e, 0xb, 0x31, 0x5, 0x31, 0x160, 0xa, 0x31, 0x3, 0x32, 
    0x3, 0x32, 0x3, 0x32, 0x3, 0x32, 0x3, 0x33, 0x3, 0x33, 0x3, 0x33, 0x3, 
    0x33, 0x2, 0x2, 0x34, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 
    0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 
    0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 
    0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 
    0x5c, 0x5e, 0x60, 0x62, 0x64, 0x2, 0x5, 0x3, 0x2, 0x3, 0xc, 0x4, 0x2, 
    0x3f, 0x3f, 0x6c, 0x6c, 0x4, 0x2, 0x40, 0x40, 0x6e, 0x6e, 0x2, 0x15e, 
    0x2, 0x67, 0x3, 0x2, 0x2, 0x2, 0x4, 0x6d, 0x3, 0x2, 0x2, 0x2, 0x6, 0x76, 
    0x3, 0x2, 0x2, 0x2, 0x8, 0x80, 0x3, 0x2, 0x2, 0x2, 0xa, 0x84, 0x3, 0x2, 
    0x2, 0x2, 0xc, 0x8e, 0x3, 0x2, 0x2, 0x2, 0xe, 0x90, 0x3, 0x2, 0x2, 0x2, 
    0x10, 0x92, 0x3, 0x2, 0x2, 0x2, 0x12, 0x97, 0x3, 0x2, 0x2, 0x2, 0x14, 
    0x9b, 0x3, 0x2, 0x2, 0x2, 0x16, 0x9d, 0x3, 0x2, 0x2, 0x2, 0x18, 0xa4, 
    0x3, 0x2, 0x2, 0x2, 0x1a, 0xa9, 0x3, 0x2, 0x2, 0x2, 0x1c, 0xab, 0x3, 
    0x2, 0x2, 0x2, 0x1e, 0xb3, 0x3, 0x2, 0x2, 0x2, 0x20, 0xb8, 0x3, 0x2, 
    0x2, 0x2, 0x22, 0xba, 0x3, 0x2, 0x2, 0x2, 0x24, 0xc0, 0x3, 0x2, 0x2, 
    0x2, 0x26, 0xc4, 0x3, 0x2, 0x2, 0x2, 0x28, 0xca, 0x3, 0x2, 0x2, 0x2, 
    0x2a, 0xcc, 0x3, 0x2, 0x2, 0x2, 0x2c, 0xce, 0x3, 0x2, 0x2, 0x2, 0x2e, 
    0xd0, 0x3, 0x2, 0x2, 0x2, 0x30, 0xd5, 0x3, 0x2, 0x2, 0x2, 0x32, 0xdb, 
    0x3, 0x2, 0x2, 0x2, 0x34, 0xdd, 0x3, 0x2, 0x2, 0x2, 0x36, 0xe2, 0x3, 
    0x2, 0x2, 0x2, 0x38, 0xe7, 0x3, 0x2, 0x2, 0x2, 0x3a, 0xed, 0x3, 0x2, 
    0x2, 0x2, 0x3c, 0xef, 0x3, 0x2, 0x2, 0x2, 0x3e, 0xf1, 0x3, 0x2, 0x2, 
    0x2, 0x40, 0xf3, 0x3, 0x2, 0x2, 0x2, 0x42, 0xf5, 0x3, 0x2, 0x2, 0x2, 
    0x44, 0xfe, 0x3, 0x2, 0x2, 0x2, 0x46, 0x101, 0x3, 0x2, 0x2, 0x2, 0x48, 
    0x106, 0x3, 0x2, 0x2, 0x2, 0x4a, 0x113, 0x3, 0x2, 0x2, 0x2, 0x4c, 0x116, 
    0x3, 0x2, 0x2, 0x2, 0x4e, 0x11b, 0x3, 0x2, 0x2, 0x2, 0x50, 0x122, 0x3, 
    0x2, 0x2, 0x2, 0x52, 0x12b, 0x3, 0x2, 0x2, 0x2, 0x54, 0x12f, 0x3, 0x2, 
    0x2, 0x2, 0x56, 0x13c, 0x3, 0x2, 0x2, 0x2, 0x58, 0x13e, 0x3, 0x2, 0x2, 
    0x2, 0x5a, 0x144, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x14b, 0x3, 0x2, 0x2, 0x2, 
    0x5e, 0x14e, 0x3, 0x2, 0x2, 0x2, 0x60, 0x15f, 0x3, 0x2, 0x2, 0x2, 0x62, 
    0x161, 0x3, 0x2, 0x2, 0x2, 0x64, 0x165, 0x3, 0x2, 0x2, 0x2, 0x66, 0x68, 
    0x5, 0x4, 0x3, 0x2, 0x67, 0x66, 0x3, 0x2, 0x2, 0x2, 0x68, 0x69, 0x3, 
    0x2, 0x2, 0x2, 0x69, 0x67, 0x3, 0x2, 0x2, 0x2, 0x69, 0x6a, 0x3, 0x2, 
    0x2, 0x2, 0x6a, 0x6b, 0x3, 0x2, 0x2, 0x2, 0x6b, 0x6c, 0x7, 0x2, 0x2, 
    0x3, 0x6c, 0x3, 0x3, 0x2, 0x2, 0x2, 0x6d, 0x74, 0x5, 0x5c, 0x2f, 0x2, 
    0x6e, 0x6f, 0x5, 0x12, 0xa, 0x2, 0x6f, 0x70, 0x7, 0xd, 0x2, 0x2, 0x70, 
    0x75, 0x3, 0x2, 0x2, 0x2, 0x71, 0x72, 0x5, 0x6, 0x4, 0x2, 0x72, 0x73, 
    0x7, 0xd, 0x2, 0x2, 0x73, 0x75, 0x3, 0x2, 0x2, 0x2, 0x74, 0x6e, 0x3, 
    0x2, 0x2, 0x2, 0x74, 0x71, 0x3, 0x2, 0x2, 0x2, 0x75, 0x5, 0x3, 0x2, 
    0x2, 0x2, 0x76, 0x77, 0x7, 0x4c, 0x2, 0x2, 0x77, 0x78, 0x5, 0x64, 0x33, 
    0x2, 0x78, 0x7a, 0x7, 0x10, 0x2, 0x2, 0x79, 0x7b, 0x5, 0x4, 0x3, 0x2, 
    0x7a, 0x79, 0x3, 0x2, 0x2, 0x2, 0x7b, 0x7c, 0x3, 0x2, 0x2, 0x2, 0x7c, 
    0x7a, 0x3, 0x2, 0x2, 0x2, 0x7c, 0x7d, 0x3, 0x2, 0x2, 0x2, 0x7d, 0x7e, 
    0x3, 0x2, 0x2, 0x2, 0x7e, 0x7f, 0x7, 0x11, 0x2, 0x2, 0x7f, 0x7, 0x3, 
    0x2, 0x2, 0x2, 0x80, 0x81, 0x5, 0x5c, 0x2f, 0x2, 0x81, 0x82, 0x5, 0xa, 
    0x6, 0x2, 0x82, 0x9, 0x3, 0x2, 0x2, 0x2, 0x83, 0x85, 0x7, 0x22, 0x2, 
    0x2, 0x84, 0x83, 0x3, 0x2, 0x2, 0x2, 0x84, 0x85, 0x3, 0x2, 0x2, 0x2, 
    0x85, 0x86, 0x3, 0x2, 0x2, 0x2, 0x86, 0x8b, 0x7, 0x73, 0x2, 0x2, 0x87, 
    0x88, 0x7, 0x22, 0x2, 0x2, 0x88, 0x8a, 0x7, 0x73, 0x2, 0x2, 0x89, 0x87, 
    0x3, 0x2, 0x2, 0x2, 0x8a, 0x8d, 0x3, 0x2, 0x2, 0x2, 0x8b, 0x89, 0x3, 
    0x2, 0x2, 0x2, 0x8b, 0x8c, 0x3, 0x2, 0x2, 0x2, 0x8c, 0xb, 0x3, 0x2, 
    0x2, 0x2, 0x8d, 0x8b, 0x3, 0x2, 0x2, 0x2, 0x8e, 0x8f, 0x5, 0xe, 0x8, 
    0x2, 0x8f, 0xd, 0x3, 0x2, 0x2, 0x2, 0x90, 0x91, 0x9, 0x2, 0x2, 0x2, 
    0x91, 0xf, 0x3, 0x2, 0x2, 0x2, 0x92, 0x93, 0x5, 0xc, 0x7, 0x2, 0x93, 
    0x11, 0x3, 0x2, 0x2, 0x2, 0x94, 0x98, 0x5, 0x42, 0x22, 0x2, 0x95, 0x98, 
    0x5, 0x48, 0x25, 0x2, 0x96, 0x98, 0x5, 0x54, 0x2b, 0x2, 0x97, 0x94, 
    0x3, 0x2, 0x2, 0x2, 0x97, 0x95, 0x3, 0x2, 0x2, 0x2, 0x97, 0x96, 0x3, 
    0x2, 0x2, 0x2, 0x98, 0x13, 0x3, 0x2, 0x2, 0x2, 0x99, 0x9c, 0x5, 0x16, 
    0xc, 0x2, 0x9a, 0x9c, 0x5, 0x1a, 0xe, 0x2, 0x9b, 0x99, 0x3, 0x2, 0x2, 
    0x2, 0x9b, 0x9a, 0x3, 0x2, 0x2, 0x2, 0x9c, 0x15, 0x3, 0x2, 0x2, 0x2, 
    0x9d, 0x9e, 0x5, 0x18, 0xd, 0x2, 0x9e, 0x17, 0x3, 0x2, 0x2, 0x2, 0x9f, 
    0xa5, 0x5, 0x24, 0x13, 0x2, 0xa0, 0xa5, 0x5, 0x26, 0x14, 0x2, 0xa1, 
    0xa5, 0x5, 0x3c, 0x1f, 0x2, 0xa2, 0xa5, 0x5, 0x3e, 0x20, 0x2, 0xa3, 
    0xa5, 0x5, 0x40, 0x21, 0x2, 0xa4, 0x9f, 0x3, 0x2, 0x2, 0x2, 0xa4, 0xa0, 
    0x3, 0x2, 0x2, 0x2, 0xa4, 0xa1, 0x3, 0x2, 0x2, 0x2, 0xa4, 0xa2, 0x3, 
    0x2, 0x2, 0x2, 0xa4, 0xa3, 0x3, 0x2, 0x2, 0x2, 0xa5, 0x19, 0x3, 0x2, 
    0x2, 0x2, 0xa6, 0xaa, 0x5, 0x42, 0x22, 0x2, 0xa7, 0xaa, 0x5, 0x48, 0x25, 
    0x2, 0xa8, 0xaa, 0x5, 0x54, 0x2b, 0x2, 0xa9, 0xa6, 0x3, 0x2, 0x2, 0x2, 
    0xa9, 0xa7, 0x3, 0x2, 0x2, 0x2, 0xa9, 0xa8, 0x3, 0x2, 0x2, 0x2, 0xaa, 
    0x1b, 0x3, 0x2, 0x2, 0x2, 0xab, 0xb0, 0x5, 0x1e, 0x10, 0x2, 0xac, 0xad, 
    0x7, 0xf, 0x2, 0x2, 0xad, 0xaf, 0x5, 0x1e, 0x10, 0x2, 0xae, 0xac, 0x3, 
    0x2, 0x2, 0x2, 0xaf, 0xb2, 0x3, 0x2, 0x2, 0x2, 0xb0, 0xae, 0x3, 0x2, 
    0x2, 0x2, 0xb0, 0xb1, 0x3, 0x2, 0x2, 0x2, 0xb1, 0x1d, 0x3, 0x2, 0x2, 
    0x2, 0xb2, 0xb0, 0x3, 0x2, 0x2, 0x2, 0xb3, 0xb6, 0x5, 0x5c, 0x2f, 0x2, 
    0xb4, 0xb7, 0x5, 0x20, 0x11, 0x2, 0xb5, 0xb7, 0x5, 0x22, 0x12, 0x2, 
    0xb6, 0xb4, 0x3, 0x2, 0x2, 0x2, 0xb6, 0xb5, 0x3, 0x2, 0x2, 0x2, 0xb7, 
    0x1f, 0x3, 0x2, 0x2, 0x2, 0xb8, 0xb9, 0x7, 0x73, 0x2, 0x2, 0xb9, 0x21, 
    0x3, 0x2, 0x2, 0x2, 0xba, 0xbb, 0x5, 0x58, 0x2d, 0x2, 0xbb, 0x23, 0x3, 
    0x2, 0x2, 0x2, 0xbc, 0xc1, 0x7, 0x56, 0x2, 0x2, 0xbd, 0xc1, 0x7, 0x5d, 
    0x2, 0x2, 0xbe, 0xbf, 0x7, 0x40, 0x2, 0x2, 0xbf, 0xc1, 0x7, 0x5d, 0x2, 
    0x2, 0xc0, 0xbc, 0x3, 0x2, 0x2, 0x2, 0xc0, 0xbd, 0x3, 0x2, 0x2, 0x2, 
    0xc0, 0xbe, 0x3, 0x2, 0x2, 0x2, 0xc1, 0x25, 0x3, 0x2, 0x2, 0x2, 0xc2, 
    0xc5, 0x5, 0x28, 0x15, 0x2, 0xc3, 0xc5, 0x5, 0x32, 0x1a, 0x2, 0xc4, 
    0xc2, 0x3, 0x2, 0x2, 0x2, 0xc4, 0xc3, 0x3, 0x2, 0x2, 0x2, 0xc5, 0x27, 
    0x3, 0x2, 0x2, 0x2, 0xc6, 0xcb, 0x5, 0x2c, 0x17, 0x2, 0xc7, 0xcb, 0x5, 
    0x2e, 0x18, 0x2, 0xc8, 0xcb, 0x5, 0x30, 0x19, 0x2, 0xc9, 0xcb, 0x5, 
    0x2a, 0x16, 0x2, 0xca, 0xc6, 0x3, 0x2, 0x2, 0x2, 0xca, 0xc7, 0x3, 0x2, 
    0x2, 0x2, 0xca, 0xc8, 0x3, 0x2, 0x2, 0x2, 0xca, 0xc9, 0x3, 0x2, 0x2, 
    0x2, 0xcb, 0x29, 0x3, 0x2, 0x2, 0x2, 0xcc, 0xcd, 0x7, 0x6a, 0x2, 0x2, 
    0xcd, 0x2b, 0x3, 0x2, 0x2, 0x2, 0xce, 0xcf, 0x9, 0x3, 0x2, 0x2, 0xcf, 
    0x2d, 0x3, 0x2, 0x2, 0x2, 0xd0, 0xd1, 0x9, 0x4, 0x2, 0x2, 0xd1, 0x2f, 
    0x3, 0x2, 0x2, 0x2, 0xd2, 0xd3, 0x7, 0x40, 0x2, 0x2, 0xd3, 0xd6, 0x7, 
    0x40, 0x2, 0x2, 0xd4, 0xd6, 0x7, 0x70, 0x2, 0x2, 0xd5, 0xd2, 0x3, 0x2, 
    0x2, 0x2, 0xd5, 0xd4, 0x3, 0x2, 0x2, 0x2, 0xd6, 0x31, 0x3, 0x2, 0x2, 
    0x2, 0xd7, 0xdc, 0x5, 0x36, 0x1c, 0x2, 0xd8, 0xdc, 0x5, 0x38, 0x1d, 
    0x2, 0xd9, 0xdc, 0x5, 0x3a, 0x1e, 0x2, 0xda, 0xdc, 0x5, 0x34, 0x1b, 
    0x2, 0xdb, 0xd7, 0x3, 0x2, 0x2, 0x2, 0xdb, 0xd8, 0x3, 0x2, 0x2, 0x2, 
    0xdb, 0xd9, 0x3, 0x2, 0x2, 0x2, 0xdb, 0xda, 0x3, 0x2, 0x2, 0x2, 0xdc, 
    0x33, 0x3, 0x2, 0x2, 0x2, 0xdd, 0xde, 0x7, 0x6b, 0x2, 0x2, 0xde, 0x35, 
    0x3, 0x2, 0x2, 0x2, 0xdf, 0xe0, 0x7, 0x4f, 0x2, 0x2, 0xe0, 0xe3, 0x7, 
    0x3f, 0x2, 0x2, 0xe1, 0xe3, 0x7, 0x6d, 0x2, 0x2, 0xe2, 0xdf, 0x3, 0x2, 
    0x2, 0x2, 0xe2, 0xe1, 0x3, 0x2, 0x2, 0x2, 0xe3, 0x37, 0x3, 0x2, 0x2, 
    0x2, 0xe4, 0xe5, 0x7, 0x4f, 0x2, 0x2, 0xe5, 0xe8, 0x7, 0x40, 0x2, 0x2, 
    0xe6, 0xe8, 0x7, 0x6f, 0x2, 0x2, 0xe7, 0xe4, 0x3, 0x2, 0x2, 0x2, 0xe7, 
    0xe6, 0x3, 0x2, 0x2, 0x2, 0xe8, 0x39, 0x3, 0x2, 0x2, 0x2, 0xe9, 0xea, 
    0x7, 0x4f, 0x2, 0x2, 0xea, 0xeb, 0x7, 0x40, 0x2, 0x2, 0xeb, 0xee, 0x7, 
    0x40, 0x2, 0x2, 0xec, 0xee, 0x7, 0x71, 0x2, 0x2, 0xed, 0xe9, 0x3, 0x2, 
    0x2, 0x2, 0xed, 0xec, 0x3, 0x2, 0x2, 0x2, 0xee, 0x3b, 0x3, 0x2, 0x2, 
    0x2, 0xef, 0xf0, 0x7, 0x54, 0x2, 0x2, 0xf0, 0x3d, 0x3, 0x2, 0x2, 0x2, 
    0xf1, 0xf2, 0x7, 0x57, 0x2, 0x2, 0xf2, 0x3f, 0x3, 0x2, 0x2, 0x2, 0xf3, 
    0xf4, 0x7, 0x30, 0x2, 0x2, 0xf4, 0x41, 0x3, 0x2, 0x2, 0x2, 0xf5, 0xf6, 
    0x7, 0x33, 0x2, 0x2, 0xf6, 0xf7, 0x5, 0x64, 0x33, 0x2, 0xf7, 0xf8, 0x7, 
    0x10, 0x2, 0x2, 0xf8, 0xf9, 0x5, 0x44, 0x23, 0x2, 0xf9, 0xfa, 0x7, 0x11, 
    0x2, 0x2, 0xfa, 0x43, 0x3, 0x2, 0x2, 0x2, 0xfb, 0xfd, 0x5, 0x46, 0x24, 
    0x2, 0xfc, 0xfb, 0x3, 0x2, 0x2, 0x2, 0xfd, 0x100, 0x3, 0x2, 0x2, 0x2, 
    0xfe, 0xfc, 0x3, 0x2, 0x2, 0x2, 0xfe, 0xff, 0x3, 0x2, 0x2, 0x2, 0xff, 
    0x45, 0x3, 0x2, 0x2, 0x2, 0x100, 0xfe, 0x3, 0x2, 0x2, 0x2, 0x101, 0x102, 
    0x5, 0x5c, 0x2f, 0x2, 0x102, 0x103, 0x5, 0x14, 0xb, 0x2, 0x103, 0x104, 
    0x5, 0x1c, 0xf, 0x2, 0x104, 0x105, 0x7, 0xd, 0x2, 0x2, 0x105, 0x47, 
    0x3, 0x2, 0x2, 0x2, 0x106, 0x107, 0x7, 0x51, 0x2, 0x2, 0x107, 0x108, 
    0x5, 0x64, 0x33, 0x2, 0x108, 0x109, 0x7, 0x2a, 0x2, 0x2, 0x109, 0x10a, 
    0x7, 0x12, 0x2, 0x2, 0x10a, 0x10b, 0x5, 0x5c, 0x2f, 0x2, 0x10b, 0x10c, 
    0x5, 0x4a, 0x26, 0x2, 0x10c, 0x10d, 0x7, 0x13, 0x2, 0x2, 0x10d, 0x10e, 
    0x7, 0x10, 0x2, 0x2, 0x10e, 0x10f, 0x5, 0x4c, 0x27, 0x2, 0x10f, 0x110, 
    0x7, 0x11, 0x2, 0x2, 0x110, 0x49, 0x3, 0x2, 0x2, 0x2, 0x111, 0x114, 
    0x5, 0x26, 0x14, 0x2, 0x112, 0x114, 0x5, 0x54, 0x2b, 0x2, 0x113, 0x111, 
    0x3, 0x2, 0x2, 0x2, 0x113, 0x112, 0x3, 0x2, 0x2, 0x2, 0x114, 0x4b, 0x3, 
    0x2, 0x2, 0x2, 0x115, 0x117, 0x5, 0x4e, 0x28, 0x2, 0x116, 0x115, 0x3, 
    0x2, 0x2, 0x2, 0x117, 0x118, 0x3, 0x2, 0x2, 0x2, 0x118, 0x116, 0x3, 
    0x2, 0x2, 0x2, 0x118, 0x119, 0x3, 0x2, 0x2, 0x2, 0x119, 0x4d, 0x3, 0x2, 
    0x2, 0x2, 0x11a, 0x11c, 0x5, 0x50, 0x29, 0x2, 0x11b, 0x11a, 0x3, 0x2, 
    0x2, 0x2, 0x11c, 0x11d, 0x3, 0x2, 0x2, 0x2, 0x11d, 0x11b, 0x3, 0x2, 
    0x2, 0x2, 0x11d, 0x11e, 0x3, 0x2, 0x2, 0x2, 0x11e, 0x11f, 0x3, 0x2, 
    0x2, 0x2, 0x11f, 0x120, 0x5, 0x52, 0x2a, 0x2, 0x120, 0x121, 0x7, 0xd, 
    0x2, 0x2, 0x121, 0x4f, 0x3, 0x2, 0x2, 0x2, 0x122, 0x129, 0x5, 0x5c, 
    0x2f, 0x2, 0x123, 0x124, 0x7, 0x55, 0x2, 0x2, 0x124, 0x125, 0x5, 0xc, 
    0x7, 0x2, 0x125, 0x126, 0x7, 0xe, 0x2, 0x2, 0x126, 0x12a, 0x3, 0x2, 
    0x2, 0x2, 0x127, 0x128, 0x7, 0x3d, 0x2, 0x2, 0x128, 0x12a, 0x7, 0xe, 
    0x2, 0x2, 0x129, 0x123, 0x3, 0x2, 0x2, 0x2, 0x129, 0x127, 0x3, 0x2, 
    0x2, 0x2, 0x12a, 0x51, 0x3, 0x2, 0x2, 0x2, 0x12b, 0x12c, 0x5, 0x5c, 
    0x2f, 0x2, 0x12c, 0x12d, 0x5, 0x14, 0xb, 0x2, 0x12d, 0x12e, 0x5, 0x1e, 
    0x10, 0x2, 0x12e, 0x53, 0x3, 0x2, 0x2, 0x2, 0x12f, 0x130, 0x7, 0x41, 
    0x2, 0x2, 0x130, 0x131, 0x5, 0x64, 0x33, 0x2, 0x131, 0x132, 0x7, 0x10, 
    0x2, 0x2, 0x132, 0x137, 0x5, 0x56, 0x2c, 0x2, 0x133, 0x134, 0x7, 0xf, 
    0x2, 0x2, 0x134, 0x136, 0x5, 0x56, 0x2c, 0x2, 0x135, 0x133, 0x3, 0x2, 
    0x2, 0x2, 0x136, 0x139, 0x3, 0x2, 0x2, 0x2, 0x137, 0x135, 0x3, 0x2, 
    0x2, 0x2, 0x137, 0x138, 0x3, 0x2, 0x2, 0x2, 0x138, 0x13a, 0x3, 0x2, 
    0x2, 0x2, 0x139, 0x137, 0x3, 0x2, 0x2, 0x2, 0x13a, 0x13b, 0x7, 0x11, 
    0x2, 0x2, 0x13b, 0x55, 0x3, 0x2, 0x2, 0x2, 0x13c, 0x13d, 0x5, 0x64, 
    0x33, 0x2, 0x13d, 0x57, 0x3, 0x2, 0x2, 0x2, 0x13e, 0x140, 0x7, 0x73, 
    0x2, 0x2, 0x13f, 0x141, 0x5, 0x5a, 0x2e, 0x2, 0x140, 0x13f, 0x3, 0x2, 
    0x2, 0x2, 0x141, 0x142, 0x3, 0x2, 0x2, 0x2, 0x142, 0x140, 0x3, 0x2, 
    0x2, 0x2, 0x142, 0x143, 0x3, 0x2, 0x2, 0x2, 0x143, 0x59, 0x3, 0x2, 0x2, 
    0x2, 0x144, 0x145, 0x7, 0x14, 0x2, 0x2, 0x145, 0x146, 0x5, 0x10, 0x9, 
    0x2, 0x146, 0x147, 0x7, 0x15, 0x2, 0x2, 0x147, 0x5b, 0x3, 0x2, 0x2, 
    0x2, 0x148, 0x14a, 0x5, 0x5e, 0x30, 0x2, 0x149, 0x148, 0x3, 0x2, 0x2, 
    0x2, 0x14a, 0x14d, 0x3, 0x2, 0x2, 0x2, 0x14b, 0x149, 0x3, 0x2, 0x2, 
    0x2, 0x14b, 0x14c, 0x3, 0x2, 0x2, 0x2, 0x14c, 0x5d, 0x3, 0x2, 0x2, 0x2, 
    0x14d, 0x14b, 0x3, 0x2, 0x2, 0x2, 0x14e, 0x14f, 0x7, 0x25, 0x2, 0x2, 
    0x14f, 0x154, 0x5, 0xa, 0x6, 0x2, 0x150, 0x151, 0x7, 0x12, 0x2, 0x2, 
    0x151, 0x152, 0x5, 0x60, 0x31, 0x2, 0x152, 0x153, 0x7, 0x13, 0x2, 0x2, 
    0x153, 0x155, 0x3, 0x2, 0x2, 0x2, 0x154, 0x150, 0x3, 0x2, 0x2, 0x2, 
    0x154, 0x155, 0x3, 0x2, 0x2, 0x2, 0x155, 0x5f, 0x3, 0x2, 0x2, 0x2, 0x156, 
    0x160, 0x5, 0xc, 0x7, 0x2, 0x157, 0x15c, 0x5, 0x62, 0x32, 0x2, 0x158, 
    0x159, 0x7, 0xf, 0x2, 0x2, 0x159, 0x15b, 0x5, 0x62, 0x32, 0x2, 0x15a, 
    0x158, 0x3, 0x2, 0x2, 0x2, 0x15b, 0x15e, 0x3, 0x2, 0x2, 0x2, 0x15c, 
    0x15a, 0x3, 0x2, 0x2, 0x2, 0x15c, 0x15d, 0x3, 0x2, 0x2, 0x2, 0x15d, 
    0x160, 0x3, 0x2, 0x2, 0x2, 0x15e, 0x15c, 0x3, 0x2, 0x2, 0x2, 0x15f, 
    0x156, 0x3, 0x2, 0x2, 0x2, 0x15f, 0x157, 0x3, 0x2, 0x2, 0x2, 0x160, 
    0x61, 0x3, 0x2, 0x2, 0x2, 0x161, 0x162, 0x7, 0x73, 0x2, 0x2, 0x162, 
    0x163, 0x7, 0x20, 0x2, 0x2, 0x163, 0x164, 0x5, 0xc, 0x7, 0x2, 0x164, 
    0x63, 0x3, 0x2, 0x2, 0x2, 0x165, 0x166, 0x5, 0x5c, 0x2f, 0x2, 0x166, 
    0x167, 0x7, 0x73, 0x2, 0x2, 0x167, 0x65, 0x3, 0x2, 0x2, 0x2, 0x20, 0x69, 
    0x74, 0x7c, 0x84, 0x8b, 0x97, 0x9b, 0xa4, 0xa9, 0xb0, 0xb6, 0xc0, 0xc4, 
    0xca, 0xd5, 0xdb, 0xe2, 0xe7, 0xed, 0xfe, 0x113, 0x118, 0x11d, 0x129, 
    0x137, 0x142, 0x14b, 0x154, 0x15c, 0x15f, 
  };

  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

IDLParser::Initializer IDLParser::_init;
