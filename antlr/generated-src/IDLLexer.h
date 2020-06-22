
// Generated from /home/michaelspiegel/Projects/gaps/pirate/antlr/IDL.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"




class  IDLLexer : public antlr4::Lexer {
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

  IDLLexer(antlr4::CharStream *input);
  ~IDLLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

