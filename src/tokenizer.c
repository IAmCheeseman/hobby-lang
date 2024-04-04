#include "tokenizer.h"

#include <string.h>

#include "common.h"

void initTokenizer(struct State* H, struct Tokenizer* tokenizer, const char* source) {
  if (strncmp(source, "\xEF\xBB\xBF", 3) == 0) {
    source += 3;
  }

  tokenizer->H = H;
  tokenizer->start = source;
  tokenizer->end = source;
  tokenizer->line = 1;
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z')
      || (c >= 'A' && c <= 'Z')
      ||  c == '_';
}

static bool isAtEnd(struct Tokenizer* tokenizer) {
  return *tokenizer->end == '\0';
}

static char advance(struct Tokenizer* tokenizer) {
  tokenizer->end++;
  return tokenizer->end[-1];
}

static char peek(struct Tokenizer* tokenizer) {
  return *tokenizer->end;
}

static char peekNext(struct Tokenizer* tokenizer) {
  if (isAtEnd(tokenizer)) {
    return '\0';
  }
  return *(tokenizer->end + 1);
}

static bool match(struct Tokenizer* tokenizer, char expected) {
  if (isAtEnd(tokenizer)) {
    return false;
  }
  if (*tokenizer->end != expected) {
    return false;
  }
  tokenizer->end++;
  return true;
}

static struct Token makeToken(struct Tokenizer* tokenizer, enum TokenType type) {
  struct Token token;
  token.type = type;
  token.start = tokenizer->start;
  token.length = (s32)(tokenizer->end - tokenizer->start);
  token.line = tokenizer->line;
  return token;
}

static struct Token errorToken(struct Tokenizer* tokenizer, const char* message) {
  struct Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (s32)strlen(message);
  token.line = tokenizer->line;
  return token;
}

static void skipWhitespace(struct Tokenizer* tokenizer) {
  while (true) {
    char c = peek(tokenizer);
    switch (c) {
      case '\n':
        tokenizer->line++;
        FALLTHROUGH;
      case ' ':
      case '\r':
      case '\t':
        advance(tokenizer);
        break;
      case '/':
        if (peekNext(tokenizer) == '/') {
          while (peek(tokenizer) != '\n' && !isAtEnd(tokenizer)) {
            advance(tokenizer);
          }
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static enum TokenType checkKeyword(
    struct Tokenizer* tokenizer,
    s32 start, s32 length,
    const char* rest, enum TokenType type) {
  if (tokenizer->end - tokenizer->start == start + length &&
      memcmp(tokenizer->start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static enum TokenType identifierType(struct Tokenizer* tokenizer) {
  switch (*tokenizer->start) {
    case 'g': return checkKeyword(tokenizer, 1, 5, "lobal", TOKEN_GLOBAL);
    case 'v': return checkKeyword(tokenizer, 1, 2, "ar", TOKEN_VAR);
    case 'b': return checkKeyword(tokenizer, 1, 4, "reak", TOKEN_BREAK);
    case 'c': {
      if (tokenizer->end - tokenizer->start > 1) {
        switch (*(tokenizer->start + 1)) {
          case 'a': return checkKeyword(tokenizer, 2, 2, "se", TOKEN_CASE);
          case 'o': return checkKeyword(tokenizer, 2, 6, "ntinue", TOKEN_CONTINUE);
        }
      }
      break;
    }
    case 'w': return checkKeyword(tokenizer, 1, 4, "hile", TOKEN_WHILE);
    case 'f': {
      if (tokenizer->end - tokenizer->start > 1) {
        switch (*(tokenizer->start + 1)) {
          case 'a': return checkKeyword(tokenizer, 2, 3, "lse", TOKEN_FALSE);
          case 'o': return checkKeyword(tokenizer, 2, 2, "or", TOKEN_FOR);
          case 'u': return checkKeyword(tokenizer, 2, 2, "nc", TOKEN_FUNC);
        }
      }
      break;
    }
    case 'l': return checkKeyword(tokenizer, 1, 3, "oop", TOKEN_LOOP);
    case 'i': return checkKeyword(tokenizer, 1, 1, "f", TOKEN_IF);
    case 'e':
      if (tokenizer->end - tokenizer->start > 1) {
        switch (*(tokenizer->start + 1)) {
          case 'l': return checkKeyword(tokenizer, 2, 2, "se", TOKEN_ELSE);
          case 'n': return checkKeyword(tokenizer, 2, 2, "um", TOKEN_ENUM);
        }
      }
      break;
    case 'm': return checkKeyword(tokenizer, 1, 4, "atch", TOKEN_MATCH);
    case 's': {
      if (tokenizer->end - tokenizer->start > 1) {
        switch (*(tokenizer->start + 1)) {
          case 't': {
            if (tokenizer->end - tokenizer->start > 1) {
              switch (*(tokenizer->start + 2)) {
                case 'a': return checkKeyword(tokenizer, 3, 3, "tic", TOKEN_STATIC);
                case 'r': return checkKeyword(tokenizer, 3, 3, "uct", TOKEN_STRUCT);
              }
            }
            break;
          }
          case 'e': return checkKeyword(tokenizer, 2, 2, "lf", TOKEN_SELF);
        }
      }
      break;
    }
    case 't': return checkKeyword(tokenizer, 1, 3, "rue", TOKEN_TRUE);
    case 'n': return checkKeyword(tokenizer, 1, 2, "il", TOKEN_NIL);
    case 'r': return checkKeyword(tokenizer, 1, 5, "eturn", TOKEN_RETURN);
  }

  return TOKEN_IDENTIFIER;
}

static struct Token identifierOrKeyword(struct Tokenizer* tokenizer) {
  while (isAlpha(peek(tokenizer)) || isDigit(peek(tokenizer))) {
    advance(tokenizer);
  }

  return makeToken(tokenizer, identifierType(tokenizer));
}

static struct Token number(struct Tokenizer* tokenizer) {
  while (isDigit(peek(tokenizer))) {
    advance(tokenizer);
  }

  if (peek(tokenizer) == '.' && isDigit(peekNext(tokenizer))) {
    advance(tokenizer);

    while (isDigit(peek(tokenizer))) {
      advance(tokenizer);
    }
  }

  return makeToken(tokenizer, TOKEN_NUMBER);
}

static struct Token string(struct Tokenizer* tokenizer, char terminator) {
  while (peek(tokenizer) != terminator && !isAtEnd(tokenizer)) {
    if (peek(tokenizer) == '\n') {
      tokenizer->line++;
    }
    advance(tokenizer);
  }

  if (isAtEnd(tokenizer)) {
    return errorToken(tokenizer, "Unterminated string.");
  }

  advance(tokenizer);
  return makeToken(tokenizer, TOKEN_STRING);
}

struct Token nextToken(struct Tokenizer* tokenizer) {
  skipWhitespace(tokenizer);
  tokenizer->start = tokenizer->end;

  if (isAtEnd(tokenizer)) {
    return makeToken(tokenizer, TOKEN_EOF);
  }

  char c = advance(tokenizer);

  if (isAlpha(c)) {
    return identifierOrKeyword(tokenizer);
  }
  if (isDigit(c)) {
    return number(tokenizer);
  }

  switch (c) {
    case '(': return makeToken(tokenizer, TOKEN_LPAREN);
    case ')': return makeToken(tokenizer, TOKEN_RPAREN);
    case '{': return makeToken(tokenizer, TOKEN_LBRACE);
    case '}': return makeToken(tokenizer, TOKEN_RBRACE);
    case '[': return makeToken(tokenizer, TOKEN_LBRACKET);
    case ']': return makeToken(tokenizer, TOKEN_RBRACKET);
    case ';': return makeToken(tokenizer, TOKEN_SEMICOLON);
    case ',': return makeToken(tokenizer, TOKEN_COMMA);
    case '.': {
      if (match(tokenizer, '.')) { // Concat operator
        return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_DOT_DOT_EQUAL : TOKEN_DOT_DOT);
      }
      return makeToken(tokenizer, TOKEN_DOT);
    }
    case ':': return makeToken(tokenizer, TOKEN_COLON);
    case '+': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
    case '-': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
    case '*': {
      if (match(tokenizer, '*')) { // Pow operator
        return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_STAR_STAR_EQUAL : TOKEN_STAR_STAR);
      }
      return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
    }
    case '/': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
    case '%': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
    case '&': return match(tokenizer, '&')
        ? makeToken(tokenizer, TOKEN_AMP_AMP)
        : errorToken(tokenizer, "Did you mean '&&'? Bitwise operators not supported.");
    case '|': return match(tokenizer, '|')
        ? makeToken(tokenizer, TOKEN_PIPE_PIPE)
        : errorToken(tokenizer, "Did you mean '||'? Bitwise operators not supported.");
    case '!': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': {
      if (match(tokenizer, '>')) {
        return makeToken(tokenizer, TOKEN_RIGHT_ARROW);
      }
      return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    }
    case '>': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '<': return makeToken(tokenizer, match(tokenizer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '\'':
    case '"': return string(tokenizer, c);
  }

  return errorToken(tokenizer, "Unexpected character.");
}
