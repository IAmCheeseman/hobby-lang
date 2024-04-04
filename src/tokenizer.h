#ifndef _HOBBYL_TOKENIZER_H
#define _HOBBYL_TOKENIZER_H

#include "common.h"
#include "object.h"

enum TokenType {
  TOKEN_LPAREN, TOKEN_RPAREN, // ( )
  TOKEN_LBRACE, TOKEN_RBRACE, // { }
  TOKEN_LBRACKET, TOKEN_RBRACKET, // [ ]
  TOKEN_RIGHT_ARROW, // =>
  TOKEN_COMMA, TOKEN_DOT, TOKEN_SEMICOLON, TOKEN_COLON, // , . ; :
  TOKEN_MINUS, TOKEN_PLUS, TOKEN_STAR, TOKEN_SLASH, // - + * /
  TOKEN_STAR_STAR, TOKEN_PERCENT, // ** %
  TOKEN_DOT_DOT, // ..
  
  TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, // += -=
  TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, // *= /=
  TOKEN_STAR_STAR_EQUAL, TOKEN_PERCENT_EQUAL, // **= %=
  TOKEN_DOT_DOT_EQUAL, // ..=

  // Conditions
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, // = ==
  TOKEN_BANG, TOKEN_BANG_EQUAL, // ! !=
  TOKEN_LESS, TOKEN_LESS_EQUAL, // < <=
  TOKEN_GREATER, TOKEN_GREATER_EQUAL, // > >=
  TOKEN_AMP_AMP, TOKEN_PIPE_PIPE, // && ||

  // Keywords
  TOKEN_VAR, // var
  TOKEN_WHILE, TOKEN_FOR, TOKEN_LOOP, // while for loop
  TOKEN_CONTINUE, TOKEN_BREAK, TOKEN_RETURN, // continue break return
  TOKEN_IF, TOKEN_ELSE, TOKEN_MATCH, TOKEN_CASE, // if else match case
  TOKEN_STRUCT, TOKEN_SELF, TOKEN_FUNC, TOKEN_STATIC, // struct self func static
  TOKEN_GLOBAL, TOKEN_ENUM, // global enum

  // literals
  TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL, // true false nil
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

  TOKEN_ERROR, TOKEN_EOF,
};

struct Token {
  enum TokenType type;
  const char* start;
  s32 length;
  s32 line;
  Value value;
};

struct Tokenizer {
  struct State* H;
  const char* start;
  const char* end;
  struct Token prev;
  struct Token current;
  s32 line;
};

void initTokenizer(struct State* H, struct Tokenizer* tokenizer, const char* source);
struct Token nextToken(struct Tokenizer* tokenizer);

#endif // _HOBBYL_TOKENIZER_H
