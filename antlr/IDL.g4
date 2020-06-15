/*
[The "BSD licence"]
Copyright (c) 2014 AutoTest Technologies, LLC
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** CORBA IDL v3.5 grammar built from the OMG IDL language spec 'ptc-13-02-02'
    http://www.omg.org/spec/IDL35/Beta1/PDF/

    Initial IDL spec implementation in ANTLR v3 by Dong Nguyen.
    Migrated to ANTLR v4 by Steve Osselton.
    Current revision prepared by Nikita Visnevski.
    Renaming of COMA to COMMA and addition of OCTAL_LITERAL in `literal`
    by Oliver Kellogg.
    Support for IDL4 annotation applications, integration of eProsima IDL4
    updates by Oliver Kellogg.
*/

grammar IDL;

specification
   : definition +
   ;

definition
   : annapps
     ( type_decl SEMICOLON
     | module SEMICOLON
     )
   ;

module
   : KW_MODULE identifier LEFT_BRACE definition + RIGHT_BRACE
   ;

// scoped_name with optional prefixed annotations
a_scoped_name
   : annapps scoped_name
   ;

scoped_name
   : (DOUBLE_COLON)? ID (DOUBLE_COLON ID)*
   ;

const_exp
   : literal
   ;

literal
   : (HEX_LITERAL | INTEGER_LITERAL | OCTAL_LITERAL | STRING_LITERAL | WIDE_STRING_LITERAL | CHARACTER_LITERAL | WIDE_CHARACTER_LITERAL | FIXED_PT_LITERAL | FLOATING_PT_LITERAL | BOOLEAN_LITERAL)
   ;

positive_int_const
   : const_exp
   ;

type_decl
   : struct_type
   | union_type
   | enum_type
   | bitset_type
   | bitmask_type
   ;

type_spec
   : simple_type_spec
   | constr_type_spec
   ;

simple_type_spec
   : base_type_spec
   | scoped_name
   ;

bitfield_type_spec
   : integer_type
   | boolean_type
   | octet_type
   ;

base_type_spec
   : floating_pt_type
   | integer_type
   | char_type
   | wide_char_type
   | boolean_type
   | octet_type
   | any_type
   | object_type
   ;

constr_type_spec
   : struct_type
   | union_type
   | enum_type
   | bitset_type
   | bitmask_type
   ;

simple_declarators
   : identifier (COMMA identifier)*
   ;

declarators
   : declarator (COMMA declarator)*
   ;

declarator
   : annapps
     ( simple_declarator
     | complex_declarator
     )
   ;

simple_declarator
   : ID
   ;

complex_declarator
   : array_declarator
   ;

floating_pt_type
   : (KW_FLOAT | KW_DOUBLE | KW_LONG KW_DOUBLE)
   ;

integer_type
   : signed_int
   | unsigned_int
   ;

signed_int
   : signed_short_int
   | signed_long_int
   | signed_longlong_int
   | signed_tiny_int
   ;

signed_tiny_int
   : KW_INT8
   ;

signed_short_int
   : KW_SHORT
   | KW_INT16
   ;

signed_long_int
   : KW_LONG
   | KW_INT32
   ;

signed_longlong_int
   : KW_LONG KW_LONG
   | KW_INT64
   ;

unsigned_int
   : unsigned_short_int
   | unsigned_long_int
   | unsigned_longlong_int
   | unsigned_tiny_int
   ;

unsigned_tiny_int
   : KW_UINT8
   ;

unsigned_short_int
   : KW_UNSIGNED KW_SHORT
   | KW_UINT16
   ;

unsigned_long_int
   : KW_UNSIGNED KW_LONG
   | KW_UINT32
   ;

unsigned_longlong_int
   : KW_UNSIGNED KW_LONG KW_LONG
   | KW_UINT64
   ;

char_type
   : KW_CHAR
   ;

wide_char_type
   : KW_WCHAR
   ;

boolean_type
   : KW_BOOLEAN
   ;

octet_type
   : KW_OCTET
   ;

any_type
   : KW_ANY
   ;

object_type
   : KW_OBJECT
   ;

bitset_type
   : KW_BITSET identifier (COLON scoped_name)? LEFT_BRACE bitfield RIGHT_BRACE
   ;

bitfield
   : ( bitfield_spec (simple_declarators)? SEMICOLON )+
   ;

bitfield_spec
   : annapps KW_BITFIELD LEFT_ANG_BRACKET positive_int_const (COMMA bitfield_type_spec)? RIGHT_ANG_BRACKET
   ;

bitmask_type
   : KW_BITMASK identifier LEFT_BRACE bit_values RIGHT_BRACE
   ;

bit_values
   : identifier (COMMA identifier)*
   ;

struct_type
   : KW_STRUCT identifier (COLON scoped_name)? LEFT_BRACE member_list RIGHT_BRACE
   ;

member_list
   : member *
   ;

member
   : annapps type_spec declarators SEMICOLON
   ;

union_type
   : KW_UNION identifier KW_SWITCH LEFT_BRACKET annapps switch_type_spec RIGHT_BRACKET LEFT_BRACE switch_body RIGHT_BRACE
   ;

switch_type_spec
   : integer_type
   | char_type
   | wide_char_type
   | octet_type
   | boolean_type
   | enum_type
   | scoped_name
   ;

switch_body
   : case_stmt +
   ;

case_stmt
   : case_label + element_spec SEMICOLON
   ;

case_label
   : annapps
     ( KW_CASE const_exp COLON
     | KW_DEFAULT COLON
     )
   ;

element_spec
   : annapps type_spec declarator
   ;

enum_type
   : KW_ENUM identifier LEFT_BRACE enumerator (COMMA enumerator)* RIGHT_BRACE
   ;

enumerator
   : identifier
   ;

array_declarator
   : ID fixed_array_size +
   ;

fixed_array_size
   : LEFT_SQUARE_BRACKET positive_int_const RIGHT_SQUARE_BRACKET
   ;

annapps
   : ( annotation_appl )*
   ;

annotation_appl
   : AT scoped_name ( LEFT_BRACKET annotation_appl_params RIGHT_BRACKET )?
   ;

annotation_appl_params
   : const_exp
   | annotation_appl_param ( COMMA annotation_appl_param )*
   ;

annotation_appl_param
   : ID EQUAL const_exp
   ;

identifier
   : annapps ID
   ;


INTEGER_LITERAL
   : ('0' | '1' .. '9' '0' .. '9'*) INTEGER_TYPE_SUFFIX?
   ;


OCTAL_LITERAL
   : '0' ('0' .. '7') + INTEGER_TYPE_SUFFIX?
   ;


HEX_LITERAL
   : '0' ('x' | 'X') HEX_DIGIT + INTEGER_TYPE_SUFFIX?
   ;


fragment HEX_DIGIT
   : ('0' .. '9' | 'a' .. 'f' | 'A' .. 'F')
   ;


fragment INTEGER_TYPE_SUFFIX
   : ('l' | 'L')
   ;


FLOATING_PT_LITERAL
   : ('0' .. '9') + '.' ('0' .. '9')* EXPONENT? FLOAT_TYPE_SUFFIX?
   | '.' ('0' .. '9') + EXPONENT? FLOAT_TYPE_SUFFIX?
   | ('0' .. '9') + EXPONENT FLOAT_TYPE_SUFFIX?
   | ('0' .. '9') + EXPONENT? FLOAT_TYPE_SUFFIX
   ;


FIXED_PT_LITERAL
   : FLOATING_PT_LITERAL
   ;


fragment EXPONENT
   : ('e' | 'E') (PLUS | MINUS)? ('0' .. '9') +
   ;


fragment FLOAT_TYPE_SUFFIX
   : ('f' | 'F' | 'd' | 'D')
   ;


WIDE_CHARACTER_LITERAL
   : 'L' CHARACTER_LITERAL
   ;


CHARACTER_LITERAL
   : '\'' (ESCAPE_SEQUENCE | ~ ('\'' | '\\')) '\''
   ;


WIDE_STRING_LITERAL
   : 'L' STRING_LITERAL
   ;


STRING_LITERAL
   : '"' (ESCAPE_SEQUENCE | ~ ('\\' | '"'))* '"'
   ;


BOOLEAN_LITERAL
   : 'TRUE'
   | 'FALSE'
   ;


fragment ESCAPE_SEQUENCE
   : '\\' ('b' | 't' | 'n' | 'f' | 'r' | '"' | '\'' | '\\')
   | UNICODE_ESCAPE
   | OCTAL_ESCAPE
   ;


fragment OCTAL_ESCAPE
   : '\\' ('0' .. '3') ('0' .. '7') ('0' .. '7')
   | '\\' ('0' .. '7') ('0' .. '7')
   | '\\' ('0' .. '7')
   ;


fragment UNICODE_ESCAPE
   : '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
   ;


fragment LETTER
   : '\u0024'
   | '\u0041' .. '\u005a'
   | '\u005f'
   | '\u0061' .. '\u007a'
   | '\u00c0' .. '\u00d6'
   | '\u00d8' .. '\u00f6'
   | '\u00f8' .. '\u00ff'
   | '\u0100' .. '\u1fff'
   | '\u3040' .. '\u318f'
   | '\u3300' .. '\u337f'
   | '\u3400' .. '\u3d2d'
   | '\u4e00' .. '\u9fff'
   | '\uf900' .. '\ufaff'
   ;


fragment ID_DIGIT
   : '\u0030' .. '\u0039'
   | '\u0660' .. '\u0669'
   | '\u06f0' .. '\u06f9'
   | '\u0966' .. '\u096f'
   | '\u09e6' .. '\u09ef'
   | '\u0a66' .. '\u0a6f'
   | '\u0ae6' .. '\u0aef'
   | '\u0b66' .. '\u0b6f'
   | '\u0be7' .. '\u0bef'
   | '\u0c66' .. '\u0c6f'
   | '\u0ce6' .. '\u0cef'
   | '\u0d66' .. '\u0d6f'
   | '\u0e50' .. '\u0e59'
   | '\u0ed0' .. '\u0ed9'
   | '\u1040' .. '\u1049'
   ;


SEMICOLON
   : ';'
   ;


COLON
   : ':'
   ;


COMMA
   : ','
   ;


LEFT_BRACE
   : '{'
   ;


RIGHT_BRACE
   : '}'
   ;


LEFT_BRACKET
   : '('
   ;


RIGHT_BRACKET
   : ')'
   ;


LEFT_SQUARE_BRACKET
   : '['
   ;


RIGHT_SQUARE_BRACKET
   : ']'
   ;


TILDE
   : '~'
   ;


SLASH
   : '/'
   ;


LEFT_ANG_BRACKET
   : '<'
   ;


RIGHT_ANG_BRACKET
   : '>'
   ;


STAR
   : '*'
   ;


PLUS
   : '+'
   ;


MINUS
   : '-'
   ;


CARET
   : '^'
   ;


AMPERSAND
   : '&'
   ;


PIPE
   : '|'
   ;


EQUAL
   : '='
   ;


PERCENT
   : '%'
   ;


DOUBLE_COLON
   : '::'
   ;


RIGHT_SHIFT
   : '>>'
   ;


LEFT_SHIFT
   : '<<'
   ;


AT : '@'
   ;


KW_SETRAISES
   : 'setraises'
   ;


KW_OUT
   : 'out'
   ;


KW_EMITS
   : 'emits'
   ;


KW_STRING
   : 'string'
   ;


KW_SWITCH
   : 'switch'
   ;


KW_PUBLISHES
   : 'publishes'
   ;


KW_TYPEDEF
   : 'typedef'
   ;


KW_USES
   : 'uses'
   ;


KW_PRIMARYKEY
   : 'primarykey'
   ;


KW_CUSTOM
   : 'custom'
   ;


KW_OCTET
   : 'octet'
   ;


KW_SEQUENCE
   : 'sequence'
   ;


KW_IMPORT
   : 'import'
   ;


KW_STRUCT
   : 'struct'
   ;


KW_NATIVE
   : 'native'
   ;


KW_READONLY
   : 'readonly'
   ;


KW_FINDER
   : 'finder'
   ;


KW_RAISES
   : 'raises'
   ;


KW_VOID
   : 'void'
   ;


KW_PRIVATE
   : 'private'
   ;


KW_EVENTTYPE
   : 'eventtype'
   ;


KW_WCHAR
   : 'wchar'
   ;


KW_IN
   : 'in'
   ;


KW_DEFAULT
   : 'default'
   ;


KW_PUBLIC
   : 'public'
   ;


KW_SHORT
   : 'short'
   ;


KW_LONG
   : 'long'
   ;


KW_ENUM
   : 'enum'
   ;


KW_WSTRING
   : 'wstring'
   ;


KW_CONTEXT
   : 'context'
   ;


KW_HOME
   : 'home'
   ;


KW_FACTORY
   : 'factory'
   ;


KW_EXCEPTION
   : 'exception'
   ;


KW_GETRAISES
   : 'getraises'
   ;


KW_CONST
   : 'const'
   ;


KW_VALUEBASE
   : 'ValueBase'
   ;


KW_VALUETYPE
   : 'valuetype'
   ;


KW_SUPPORTS
   : 'supports'
   ;


KW_MODULE
   : 'module'
   ;


KW_OBJECT
   : 'Object'
   ;


KW_TRUNCATABLE
   : 'truncatable'
   ;


KW_UNSIGNED
   : 'unsigned'
   ;


KW_FIXED
   : 'fixed'
   ;


KW_UNION
   : 'union'
   ;


KW_ONEWAY
   : 'oneway'
   ;


KW_ANY
   : 'any'
   ;


KW_CHAR
   : 'char'
   ;


KW_CASE
   : 'case'
   ;


KW_FLOAT
   : 'float'
   ;


KW_BOOLEAN
   : 'boolean'
   ;


KW_MULTIPLE
   : 'multiple'
   ;


KW_ABSTRACT
   : 'abstract'
   ;


KW_INOUT
   : 'inout'
   ;


KW_PROVIDES
   : 'provides'
   ;


KW_CONSUMES
   : 'consumes'
   ;


KW_DOUBLE
   : 'double'
   ;


KW_TYPEPREFIX
   : 'typeprefix'
   ;


KW_TYPEID
   : 'typeid'
   ;


KW_ATTRIBUTE
   : 'attribute'
   ;


KW_LOCAL
   : 'local'
   ;


KW_MANAGES
   : 'manages'
   ;


KW_INTERFACE
   : 'interface'
   ;


KW_COMPONENT
   : 'component'
   ;

KW_SET
   : 'set'
   ;

KW_MAP
   : 'map'
   ;

KW_BITFIELD
   : 'bitfield'
   ;

KW_BITSET
   : 'bitset'
   ;

KW_BITMASK
   : 'bitmask'
   ;

KW_INT8
   : 'int8'
   ;

KW_UINT8
   : 'uint8'
   ;

KW_INT16
   : 'int16'
   ;

KW_UINT16
   : 'uint16'
   ;

KW_INT32
   : 'int32'
   ;

KW_UINT32
   : 'uint32'
   ;

KW_INT64
   : 'int64'
   ;

KW_UINT64
   : 'uint64'
   ;

KW_AT_ANNOTATION
   : '@annotation'
   ;

ID
   : LETTER (LETTER | ID_DIGIT)*
   ;


WS
   : (' ' | '\r' | '\t' | '\u000C' | '\n') -> channel (HIDDEN)
   ;


COMMENT
   : '/*' .*? '*/' -> channel (HIDDEN)
   ;


LINE_COMMENT
   : '//' ~ ('\n' | '\r')* '\r'? '\n' -> channel (HIDDEN)
   ;

