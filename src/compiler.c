#include "compiler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opcodes.h"
#include "common.h"
#include "compiler.h"
#include "tokenizer.h"
#include "object.h"
#include "memory.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

enum Precedence {
  PREC_NONE,
  PREC_ASSIGNMENT,
  PREC_OR,
  PREC_AND,
  PREC_EQUALITY,
  PREC_COMPARISON,
  PREC_TERM,
  PREC_FACTOR,
  PREC_EXPONENT,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY,
};

typedef void (*ParseFn)(struct Parser* parser, bool canAssign);

struct ParseRule {
  ParseFn prefix;
  ParseFn infix;
  enum Precedence precedence;
};

static struct Function* currentFunction(struct Parser* parser) {
  return parser->compiler->function;
}

static void errorAt(struct Parser* parser, struct Token* token, const char* message) {
  if (parser->panicMode) {
    return;
  }
  parser->panicMode = true;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type != TOKEN_ERROR) {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser->hadError = true;
}

static void error(struct Parser* parser, const char* message) {
  errorAt(parser, &parser->previous, message);
}

static void errorAtCurrent(struct Parser* parser, const char* message) {
  errorAt(parser, &parser->current, message);
}

static void advance(struct Parser* parser) {
  parser->previous = parser->current;

  while (true) {
    parser->current = nextToken(parser->tokenizer);
    if (parser->current.type != TOKEN_ERROR) {
      break;
    }

    errorAtCurrent(parser, parser->current.start);
  }
}

static void consume(struct Parser* parser, enum TokenType type, const char* message) {
  if (parser->current.type == type) {
    advance(parser);
    return;
  }

  errorAtCurrent(parser, message);
}

static bool check(struct Parser* parser, enum TokenType type) {
  return parser->current.type == type;
}

static bool match(struct Parser* parser, enum TokenType type) {
  if (!check(parser, type)) {
    return false;
  }
  advance(parser);
  return true;
}

static void emitByte(struct Parser* parser, u8 byte) {
  writeBytecode(parser->H, currentFunction(parser), byte, parser->previous.line);
}

static void emitBytes(struct Parser* parser, u8 byte1, u8 byte2) {
  emitByte(parser, byte1);
  emitByte(parser, byte2);
}

static s32 emitJump(struct Parser* parser, u8 byte) {
  emitByte(parser, byte);
  emitByte(parser, 0xff);
  emitByte(parser, 0xff);
  return currentFunction(parser)->bcCount - 2;
}

static void patchJump(struct Parser* parser, s32 offset) {
  s32 jump = currentFunction(parser)->bcCount - offset - 2;
  if (jump >= UINT16_MAX) {
    error(parser, "Too much code to jump over. Why?");
  }

  currentFunction(parser)->bc[offset] = (jump >> 8) & 0xff;
  currentFunction(parser)->bc[offset + 1] = jump & 0xff;
}

static void emitLoop(struct Parser* parser, s32 loopStart) {
  emitByte(parser, BC_LOOP);

  s32 offset = currentFunction(parser)->bcCount - loopStart + 2;
  if (offset > UINT16_MAX) {
    error(parser, "Loop is too big. I'm not quite sure why you made a loop this big.");
  }

  emitByte(parser, (offset >> 8) & 0xff);
  emitByte(parser, offset & 0xff);
}

static void emitReturn(struct Parser* parser) {
  emitByte(parser, BC_NIL);
  emitByte(parser, BC_RETURN);
}

static u8 makeConstant(struct Parser* parser, Value value) {
  s32 constant = addFunctionConstant(parser->H, currentFunction(parser), value);
  if (constant > UINT8_MAX) {
    error(parser, "Too many constants in the global scope or functions.");
    return 0;
  }

  return (u8)constant;
}

static void emitConstant(struct Parser* parser, Value value) {
  emitBytes(parser, BC_CONSTANT, makeConstant(parser, value));
}

static void initCompiler(struct Parser* parser,
                         struct Compiler* compiler,
                         enum FunctionType type) {
  compiler->enclosing = parser->compiler;

  compiler->function = NULL;
  compiler->type = type;

  compiler->localOffset = 0;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction(parser->H);
  compiler->loop = NULL;
  parser->compiler = compiler;

  if (type != FUNCTION_TYPE_SCRIPT) {
    struct Token name = parser->previous;
    if (name.type == TOKEN_IDENTIFIER) {
      parser->compiler->function->name = copyString(
          parser->H, name.start, name.length);
    } else if (name.type == TOKEN_FUNC) { // lambda
      parser->compiler->function->name = copyString(parser->H, "@lambda@", 8);
    }
  }

  struct Local* local = 
      &parser->compiler->locals[parser->compiler->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  if (type != FUNCTION_TYPE_FUNCTION) {
    local->name.start = "self";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static struct Function* endCompiler(struct Parser* parser) {
  emitReturn(parser);
  struct Function* function = parser->compiler->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser->hadError) {
    disassembleFunction(
        parser->compiler->function,
        function, function->name != NULL ? function->name->chars : "<script>");
  }
#endif

  parser->compiler = parser->compiler->enclosing;
  return function;
}

static s32 discardLocals(struct Parser* parser, s32 toScope) {
  s32 discarded = 0;
  while (parser->compiler->localCount - discarded > 0
      && parser->compiler->locals[parser->compiler->localCount - discarded - 1].depth
         > toScope) {
    if (parser->compiler->locals
        [parser->compiler->localCount - discarded - 1].isCaptured) {
      emitByte(parser, BC_CLOSE_UPVALUE);
    } else {
      emitByte(parser, BC_POP);
    }
    discarded++;
  }

  return discarded;
}

static void beginLoop(struct Parser* parser, struct Loop* loop) {
  loop->start = currentFunction(parser)->bcCount;
  loop->scopeDepth = parser->compiler->scopeDepth;
  loop->enclosing = parser->compiler->loop;
  loop->isNamed = false;
  loop->breakCount = 0;
  parser->compiler->loop = loop;
}

static void endLoop(struct Parser* parser, struct Loop* loop) {
  // s32 end = currentFunction(parser)->bcCount;

  for (s32 i = 0; i < loop->breakCount; i++) {
    s32 breakIndex = loop->breakIndices[i];
    currentFunction(parser)->bc[breakIndex] = BC_JUMP;
    patchJump(parser, breakIndex + 1);
  }

  parser->compiler->loop = loop->enclosing;
}

static bool identifiersEqual(struct Token* a, struct Token* b) {
  if (a->length != b->length) {
    return false;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

static struct Loop* resolveLoopLabel(struct Parser* parser, struct Token* name) {
  struct Loop* loop = parser->compiler->loop;

  while (loop != NULL) {
    if (!loop->isNamed) {
      loop = loop->enclosing;
    }

    if (identifiersEqual(name, &loop->name)) {
      return loop;
    }
    loop = loop->enclosing;
  }

  return NULL;
}

static void beginScope(struct Parser* parser) {
  parser->compiler->scopeDepth++;
}

static void endScope(struct Parser* parser) {
  parser->compiler->scopeDepth--;
  s32 discarded = discardLocals(parser, parser->compiler->scopeDepth);
  parser->compiler->localCount -= discarded;
}

static void synchronize(struct Parser* parser);
static void expression(struct Parser* parser);
static void statement(struct Parser* parser);
static void declaration(struct Parser* parser);
static struct ParseRule* getRule(enum TokenType type);
static void parsePrecedence(struct Parser* parser, enum Precedence precedence);
static void block(struct Parser* parser);

static void markInitialized(struct Parser* parser, bool isGlobal) {
  if (isGlobal) {
    return;
  }

  parser->compiler->locals[parser->compiler->localCount - 1].depth
      = parser->compiler->scopeDepth;
}

static void defineVariable(struct Parser* parser, u8 global, bool isGlobal) {
  if (!isGlobal) {
    markInitialized(parser, isGlobal);
    return;
  }

  emitBytes(parser, BC_DEFINE_GLOBAL, global);
}

static u8 identifierConstant(struct Parser* parser, struct Token* name) {
  return makeConstant(parser, NEW_OBJ(copyString(parser->H, name->start, name->length)));
}

static s32 resolveLocal(
    struct Parser* parser, struct Compiler* compiler, struct Token* name) {
  for (s32 i = compiler->localCount - 1; i >= 0; i--) {
    struct Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error(parser, "Can't read local variable in it's own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static s32 addUpvalue(struct Parser* parser, struct Compiler* compiler, u8 index, bool isLocal) {
  s32 upvalueCount = compiler->function->upvalueCount;

  for (s32 i = 0; i < upvalueCount; i++) {
    struct CompilerUpvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == U8_COUNT) {
    error(parser, "Too many upvalues in a function.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

static s32 resolveUpvalue(struct Parser* parser, struct Compiler* compiler, struct Token* name) {
  if (compiler->enclosing == NULL) {
    return -1;
  }

  s32 local = resolveLocal(parser, compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(parser, compiler, (u8)local, true);
  }

  s32 upvalue = resolveUpvalue(parser, compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(parser, compiler, (u8)upvalue, false);
  }

  return -1;
}

static void addLocal(struct Parser* parser, struct Token name) {
  if (parser->compiler->localCount == U8_COUNT) {
    error(parser, "Too many local variables in function.");
    return;
  }

  // s32 offset = parser->compiler->localOffset;
  struct Local* local = 
      &parser->compiler->locals[parser->compiler->localCount];
  parser->compiler->localCount++;
  local->name = name;
  local->isCaptured = false;
  local->depth = -1;
  local->depth = parser->compiler->scopeDepth;
}

static void declareVariable(struct Parser* parser, bool isGlobal) {
  if (isGlobal) {
    return;
  }
  
  struct Token* name = &parser->previous;

  for (s32 i = parser->compiler->localCount - 1; i >= 0; i--) {
    struct Local* local = &parser->compiler->locals[i];
    if (local->depth != -1 && local->depth < parser->compiler->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      error(parser, "Redefinition of variable.");
    }
  }

  addLocal(parser, *name);
}

static u8 parseVariable(struct Parser* parser, bool isGlobal, const char* errorMessage) {
  consume(parser, TOKEN_IDENTIFIER, errorMessage);

  declareVariable(parser, isGlobal);
  if (!isGlobal) {
    return 0;
  }

  return identifierConstant(parser, &parser->previous);
}

static void grouping(struct Parser* parser, UNUSED bool canAssign) {
  expression(parser);
  consume(parser, TOKEN_RPAREN, "Expected ')' after expression.");
}

static void number(struct Parser* parser, UNUSED bool canAssign) {
  f64 value = strtod(parser->previous.start, NULL);
  emitConstant(parser, NEW_NUMBER(value));
}

static void string(struct Parser* parser, UNUSED bool canAssign) {
  emitConstant(parser, parser->previous.value);
}

static void namedVariable(struct Parser* parser, struct Token name, bool canAssign) {
  u8 getter, setter;
  s32 arg = resolveLocal(parser, parser->compiler, &name);

  if (arg != -1) {
    getter = BC_GET_LOCAL;
    setter = BC_SET_LOCAL;
  } else if ((arg = resolveUpvalue(parser, parser->compiler, &name)) != -1) {
    getter = BC_GET_UPVALUE;
    setter = BC_SET_UPVALUE;
  } else {
    arg = identifierConstant(parser, &name);
    getter = BC_GET_GLOBAL;
    setter = BC_SET_GLOBAL;
  }

#define COMPOUND_ASSIGNMENT(operator) \
    do { \
      emitBytes(parser, getter, (u8)arg); \
      expression(parser); \
      emitByte(parser, operator); \
      emitBytes(parser, setter, (u8)arg); \
    } while (false)

  if (canAssign && match(parser, TOKEN_EQUAL)) {
    expression(parser);
    emitBytes(parser, setter, (u8)arg);
  } else if (canAssign && match(parser, TOKEN_PLUS_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_ADD);
  } else if (canAssign && match(parser, TOKEN_MINUS_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_SUBTRACT);
  } else if (canAssign && match(parser, TOKEN_STAR_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_MULTIPLY);
  } else if (canAssign && match(parser, TOKEN_SLASH_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_DIVIDE);
  } else if (canAssign && match(parser, TOKEN_STAR_STAR_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_POW);
  } else if (canAssign && match(parser, TOKEN_PERCENT_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_MODULO);
  } else if (canAssign && match(parser, TOKEN_DOT_DOT_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_CONCAT);
  } else {
    emitBytes(parser, getter, (u8)arg);
  }
#undef COMPOUND_ASSIGNMENT
}

static void variable(struct Parser* parser, UNUSED bool canAssign) {
  struct Token name = parser->previous;
  if (match(parser, TOKEN_LBRACE)) { // Struct initalization
    namedVariable(parser, name, canAssign);
    emitByte(parser, BC_INSTANCE);

    if (check(parser, TOKEN_DOT)) {
      do {
        consume(parser, TOKEN_DOT, "Expected '.' before identifier.");
        consume(parser, TOKEN_IDENTIFIER, "Expected identifier.");
        struct Token name = parser->previous;
        consume(parser, TOKEN_EQUAL, "Expected '=' after identifier.");
        expression(parser);

        emitBytes(parser, BC_INIT_PROPERTY, identifierConstant(parser, &name));

        if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACE)) {
          error(parser, "Expected ','.");
        }
      } while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF));
    }

    consume(parser, TOKEN_RBRACE, "Unterminated struct initializer.");
  } else { // Variable reference
    namedVariable(parser, name, canAssign);
  }
}

static void self(struct Parser* parser, UNUSED bool canAssign) {
  if (parser->structCompiler == NULL) {
    error(parser, "Can only use 'self' inside struct methods.");
    return;
  }

  variable(parser, false);
}

static void unary(struct Parser* parser, UNUSED bool canAssign) {
  enum TokenType op = parser->previous.type; 

  parsePrecedence(parser, PREC_UNARY);

  switch (op) {
    case TOKEN_MINUS: emitByte(parser, BC_NEGATE); break;
    case TOKEN_BANG:  emitByte(parser, BC_NOT); break;
    default: return;
  }
}

static void binary(struct Parser* parser, UNUSED bool canAssign) {
  enum TokenType op = parser->previous.type;
  struct ParseRule* rule = getRule(op);
  parsePrecedence(parser, (enum Precedence)(rule->precedence + 1));

  switch (op) {
    case TOKEN_PLUS:          emitByte(parser, BC_ADD); break;
    case TOKEN_MINUS:         emitByte(parser, BC_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(parser, BC_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(parser, BC_DIVIDE); break;
    case TOKEN_PERCENT:       emitByte(parser, BC_MODULO); break;
    case TOKEN_DOT_DOT:       emitByte(parser, BC_CONCAT); break;
    case TOKEN_STAR_STAR:     emitByte(parser, BC_POW); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(parser, BC_EQUAL); break;
    case TOKEN_BANG_EQUAL:    emitByte(parser, BC_NOT_EQUAL); break;
    case TOKEN_GREATER:       emitByte(parser, BC_GREATER); break;
    case TOKEN_LESS:          emitByte(parser, BC_LESSER); break;
    case TOKEN_GREATER_EQUAL: emitByte(parser, BC_GREATER_EQUAL); break;
    case TOKEN_LESS_EQUAL:    emitByte(parser, BC_LESSER_EQUAL); break;
    default: return;
  }
}

static u8 argumentList(struct Parser* parser) {
  u8 argCount = 0;
  if (!check(parser, TOKEN_RPAREN)) {
    do {
      expression(parser);
      if (argCount == UINT8_MAX) {
        error(parser, "Can't have more than 255 arguments.");
      }
      argCount++;
    } while (match(parser, TOKEN_COMMA));
  }
  consume(parser, TOKEN_RPAREN, "Unclosed call.");
  return argCount;
}

static void call(struct Parser* parser, UNUSED bool canAssign) {
  u8 argCount = argumentList(parser);
  emitBytes(parser, BC_CALL, argCount);
}

static void ternery(struct Parser* parser, UNUSED bool canAssign) {
  consume(parser, TOKEN_LPAREN, "Expected '('.");
  expression(parser);
  consume(parser, TOKEN_RPAREN, "Expected ')'.");

  s32 thenJump = emitJump(parser, BC_JUMP_IF_FALSE);
  emitByte(parser, BC_POP);

  expression(parser);

  s32 elseJump = emitJump(parser, BC_JUMP);
  patchJump(parser, thenJump);
  emitByte(parser, BC_POP);

  consume(parser, TOKEN_ELSE, "Expected 'else' in ternery operator.");

  expression(parser);

  patchJump(parser, elseJump);
}

static void dot(struct Parser* parser, bool canAssign) {
  consume(parser, TOKEN_IDENTIFIER, "Expected property name.");
  u8 name = identifierConstant(parser, &parser->previous);

#define COMPOUND_ASSIGNMENT(operator) \
    do { \
      emitBytes(parser, BC_PUSH_PROPERTY, name); \
      expression(parser); \
      emitByte(parser, operator); \
      emitBytes(parser, BC_SET_PROPERTY, name); \
    } while (false)

  if (canAssign && match(parser, TOKEN_EQUAL)) {
    expression(parser);
    emitBytes(parser, BC_SET_PROPERTY, name);
  } else if (match(parser, TOKEN_LPAREN)) {
    u8 argCount = argumentList(parser);
    emitBytes(parser, BC_INVOKE, name);
    emitByte(parser, argCount);
  } else if (canAssign && match(parser, TOKEN_PLUS_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_ADD);
  } else if (canAssign && match(parser, TOKEN_MINUS_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_SUBTRACT);
  } else if (canAssign && match(parser, TOKEN_STAR_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_MULTIPLY);
  } else if (canAssign && match(parser, TOKEN_SLASH_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_DIVIDE);
  } else if (canAssign && match(parser, TOKEN_STAR_STAR_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_POW);
  } else if (canAssign && match(parser, TOKEN_PERCENT_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_MODULO);
  } else if (canAssign && match(parser, TOKEN_DOT_DOT_EQUAL)) {
    COMPOUND_ASSIGNMENT(BC_CONCAT);
  } else {
    emitBytes(parser, BC_GET_PROPERTY, name);
  }
#undef COMPOUND_ASSIGNMENT
}

static void subscript(struct Parser* parser, bool canAssign) {
  expression(parser);
  consume(parser, TOKEN_RBRACKET, "Unterminated subscript operator.");

  if (canAssign && match(parser, TOKEN_EQUAL)) {
    expression(parser);
    emitByte(parser, BC_SET_SUBSCRIPT);
  } else {
    emitByte(parser, BC_GET_SUBSCRIPT);
  }
}

static void staticDot(struct Parser* parser, UNUSED bool canAssign) {
  consume(parser, TOKEN_IDENTIFIER, "Expected static method name.");
  u8 name = identifierConstant(parser, &parser->previous);
  emitBytes(parser, BC_GET_STATIC, name);
}

static void function(struct Parser* parser, enum FunctionType type, bool isLambda) {
  struct Compiler compiler;
  initCompiler(parser, &compiler, type);
  beginScope(parser);

  if (match(parser, TOKEN_LPAREN)) {
    if (!check(parser, TOKEN_RPAREN)) {
      do {
        parser->compiler->function->arity++;
        if (parser->compiler->function->arity > UINT8_MAX) {
          errorAtCurrent(parser, "Too many parameters. Max is 255.");
          advance(parser);
          break;
        }
        u8 constant = parseVariable(parser, false, "Expected variable name.");
        defineVariable(parser, constant, false);
      } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RPAREN, "Expected ')'.");
  }
  
  if (match(parser, TOKEN_LBRACE)) {
    block(parser);
  } else if (match(parser, TOKEN_RIGHT_ARROW)) {
    expression(parser);
    emitByte(parser, BC_RETURN);
    if (!isLambda) {
      consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression.");
    }
    
    // If there's an error, can mess up inside structs without this.
    if (parser->panicMode) {
      synchronize(parser);
    }
  } else {
    error(parser, "Expected '{' or '=>'.");
  }

  struct Function* function = endCompiler(parser);
  emitBytes(parser, BC_CLOSURE, makeConstant(parser, NEW_OBJ(function)));

  for (s32 i = 0; i < function->upvalueCount; i++) {
    emitByte(parser, compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(parser, compiler.upvalues[i].index);
  }
}

static void lambda(struct Parser* parser, UNUSED bool canAssign) {
  function(parser, FUNCTION_TYPE_FUNCTION, true);
}

static void literal(struct Parser* parser, UNUSED bool canAssign) {
  switch (parser->previous.type) {
    case TOKEN_FALSE: emitByte(parser, BC_FALSE); break;
    case TOKEN_TRUE: emitByte(parser, BC_TRUE); break;
    case TOKEN_NIL: emitByte(parser, BC_NIL); break;
    default: return;
  }
}

static void array(struct Parser* parser, UNUSED bool canAssign) {
  u8 count = 0;
  if (!check(parser, TOKEN_RBRACKET)) {
    do {
      expression(parser);
      if (count == UINT8_MAX) {
        error(parser, "Can't have more than 255 elements in an array literal.");
      }
      count++;

      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACKET)) {
        error(parser, "Expected ','.");
      }
    } while (!check(parser, TOKEN_RBRACKET));
  }
  consume(parser, TOKEN_RBRACKET, "Unterminated array literal.");

  emitBytes(parser, BC_ARRAY, count);
}

static void and_(struct Parser* parser, UNUSED bool canAssign) {
  s32 endJump = emitJump(parser, BC_JUMP_IF_FALSE);
  emitByte(parser, BC_POP);
  parsePrecedence(parser, PREC_AND);
  patchJump(parser, endJump);
}

static void or_(struct Parser* parser, UNUSED bool canAssign) {
  s32 elseJump = emitJump(parser, BC_JUMP_IF_FALSE);
  s32 endJump = emitJump(parser, BC_JUMP);

  patchJump(parser, elseJump);
  emitByte(parser, BC_POP);

  parsePrecedence(parser, PREC_OR);
  patchJump(parser, endJump);
}

static struct ParseRule rules[] = {
  [TOKEN_LPAREN]          = {grouping, call,       PREC_CALL},
  [TOKEN_RPAREN]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_LBRACE]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_RBRACE]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_LBRACKET]        = {array,    subscript,  PREC_CALL},
  [TOKEN_RBRACKET]        = {NULL,     NULL,       PREC_NONE},
  [TOKEN_COMMA]           = {NULL,     NULL,       PREC_NONE},
  [TOKEN_DOT]             = {NULL,     dot,        PREC_CALL},
  [TOKEN_SEMICOLON]       = {NULL,     NULL,       PREC_NONE},
  [TOKEN_COLON]           = {NULL,     staticDot,  PREC_CALL},
  [TOKEN_MINUS]           = {unary,    binary,     PREC_TERM},
  [TOKEN_PLUS]            = {NULL,     binary,     PREC_TERM},
  [TOKEN_STAR]            = {NULL,     binary,     PREC_FACTOR},
  [TOKEN_SLASH]           = {NULL,     binary,     PREC_FACTOR},
  [TOKEN_STAR_STAR]       = {NULL,     binary,     PREC_EXPONENT},
  [TOKEN_PERCENT]         = {NULL,     binary,     PREC_FACTOR},
  [TOKEN_DOT_DOT]         = {NULL,     binary,     PREC_TERM},
  [TOKEN_EQUAL]           = {NULL,     NULL,       PREC_NONE},
  [TOKEN_EQUAL_EQUAL]     = {NULL,     binary,     PREC_EQUALITY},
  [TOKEN_BANG]            = {unary,    NULL,       PREC_NONE},
  [TOKEN_BANG_EQUAL]      = {NULL,     binary,     PREC_EQUALITY},
  [TOKEN_LESS]            = {NULL,     binary,     PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]      = {NULL,     binary,     PREC_COMPARISON},
  [TOKEN_GREATER]         = {NULL,     binary,     PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL]   = {NULL,     binary,     PREC_COMPARISON},
  [TOKEN_AMP_AMP]         = {NULL,     and_,       PREC_AND},
  [TOKEN_PIPE_PIPE]       = {NULL,     or_,        PREC_OR},
  [TOKEN_VAR]             = {NULL,     NULL,       PREC_NONE},
  [TOKEN_WHILE]           = {NULL,     NULL,       PREC_NONE},
  [TOKEN_FOR]             = {NULL,     NULL,       PREC_NONE},
  [TOKEN_LOOP]            = {NULL,     NULL,       PREC_NONE},
  [TOKEN_IF]              = {ternery,  NULL,       PREC_ASSIGNMENT},
  [TOKEN_ELSE]            = {NULL,     NULL,       PREC_NONE},
  [TOKEN_MATCH]           = {NULL,     NULL,       PREC_NONE},
  [TOKEN_CASE]            = {NULL,     NULL,       PREC_NONE},
  [TOKEN_STRUCT]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_SELF]            = {self,     NULL,       PREC_NONE},
  [TOKEN_FUNC]            = {lambda,   NULL,       PREC_NONE},
  [TOKEN_TRUE]            = {literal,  NULL,       PREC_NONE},
  [TOKEN_FALSE]           = {literal,  NULL,       PREC_NONE},
  [TOKEN_NIL]             = {literal,  NULL,       PREC_NONE},
  [TOKEN_IDENTIFIER]      = {variable, NULL,       PREC_NONE},
  [TOKEN_STRING]          = {string,   NULL,       PREC_NONE},
  [TOKEN_NUMBER]          = {number,   NULL,       PREC_NONE},
  [TOKEN_ERROR]           = {NULL,     NULL,       PREC_NONE},
  [TOKEN_RETURN]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_ENUM]            = {NULL,     NULL,       PREC_NONE},
  [TOKEN_GLOBAL]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_BREAK]           = {NULL,     NULL,       PREC_NONE},
  [TOKEN_CONTINUE]        = {NULL,     NULL,       PREC_NONE},
  [TOKEN_STATIC]          = {NULL,     NULL,       PREC_NONE},
  [TOKEN_DOT_DOT_EQUAL]   = {NULL,     NULL,       PREC_NONE},
  [TOKEN_PLUS_EQUAL]      = {NULL,     NULL,       PREC_NONE},
  [TOKEN_MINUS_EQUAL]     = {NULL,     NULL,       PREC_NONE},
  [TOKEN_STAR_EQUAL]      = {NULL,     NULL,       PREC_NONE},
  [TOKEN_SLASH_EQUAL]     = {NULL,     NULL,       PREC_NONE},
  [TOKEN_PERCENT_EQUAL]   = {NULL,     NULL,       PREC_NONE},
  [TOKEN_STAR_STAR_EQUAL] = {NULL,     NULL,       PREC_NONE},
  [TOKEN_EOF]             = {NULL,     NULL,       PREC_NONE},
};

static void parsePrecedence(struct Parser* parser, enum Precedence precedence) {
  advance(parser);

  ParseFn prefixRule = getRule(parser->previous.type)->prefix;
  if (prefixRule == NULL) {
    error(parser, "Expected expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(parser, canAssign);
  
  while (precedence <= getRule(parser->current.type)->precedence) {
    advance(parser);
    ParseFn infixRule = getRule(parser->previous.type)->infix;
    infixRule(parser, canAssign);
  }

  if (canAssign && match(parser, TOKEN_EQUAL)) {
    error(parser, "Cannot assign to that expression.");
  }
}

static struct ParseRule* getRule(enum TokenType type) {
  return &rules[type];
}

static void expression(struct Parser* parser) {
  parsePrecedence(parser, PREC_ASSIGNMENT);
}

static void block(struct Parser* parser) {
  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    declaration(parser);
  }

  consume(parser, TOKEN_RBRACE, "Unterminated block.");
}

static void method(struct Parser* parser, bool isStatic) {
  consume(parser, TOKEN_IDENTIFIER, "Expected method name.");
  u8 constant = identifierConstant(parser, &parser->previous);

  enum FunctionType type = isStatic 
      ? FUNCTION_TYPE_FUNCTION
      : FUNCTION_TYPE_METHOD;
  function(parser, type, false);
  emitBytes(parser, isStatic ? BC_STATIC_METHOD : BC_METHOD, constant);
}

static void functionDeclaration(struct Parser* parser, bool isGlobal) {
  if (parser->compiler->scopeDepth > 0) {
    error(parser, "Can only define functions in top level code.");
  }
  u8 global = parseVariable(parser, isGlobal, "Expected function name.");
  markInitialized(parser, isGlobal);
  function(parser, FUNCTION_TYPE_FUNCTION, false);
  defineVariable(parser, global, isGlobal);
}

static void arrayDestructAssignment(struct Parser* parser) {
  u8 setters[UINT8_MAX];
  u8 variables[UINT8_MAX];
  u8 variableCount = 0;
  do {
    if (variableCount == UINT8_MAX) {
      error(parser, "Cannot have more than 255 variables per assignment.");
      return;
    }

    consume(parser, TOKEN_IDENTIFIER, "Expected identifier.");
    struct Token name = parser->previous;

    u8 setter;
    s32 arg = resolveLocal(parser, parser->compiler, &name);

    if (arg != -1) {
      setter = BC_SET_LOCAL;
    } else if ((arg = resolveUpvalue(parser, parser->compiler, &name)) != -1) {
      setter = BC_SET_UPVALUE;
    } else {
      arg = identifierConstant(parser, &name);
      setter = BC_SET_GLOBAL;
    }

    variables[variableCount] = arg;
    setters[variableCount] = setter;

    variableCount++;
  } while (match(parser, TOKEN_COMMA));

  consume(parser, TOKEN_RBRACKET, "Expected ']'.");
  consume(parser, TOKEN_EQUAL, "Expected '='.");
  expression(parser);

  for (u8 i = 0; i < variableCount; i++) {
    emitBytes(parser, BC_DESTRUCT_ARRAY, i);
    emitBytes(parser, setters[i], variables[i]);
    emitByte(parser, BC_POP);
  }

  emitByte(parser, BC_POP);
  consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
}

static void varDeclaration(struct Parser* parser, bool isGlobal) {
  if (match(parser, TOKEN_LBRACKET)) {
    u8 variables[UINT8_MAX];
    struct Token tokens[UINT8_MAX];
    u8 variableCount = 0;
    // parse [x, y]
    do {
      if (variableCount == UINT8_MAX) {
        error(parser, "Cannot have more than 255 variables per var.");
        return;
      }

      struct Token identifier = parser->current;
      u8 nameConstant = parseVariable(parser, isGlobal, "Expected identifier.");
      variables[variableCount] = nameConstant;
      tokens[variableCount] = identifier;
      variableCount++;

      if (!isGlobal) {
        emitByte(parser, BC_NIL); // Reserve
      }
    } while (match(parser, TOKEN_COMMA));

    consume(parser, TOKEN_RBRACKET, "Expected ']'.");
    consume(parser, TOKEN_EQUAL, "Expected '='.");
    expression(parser);

    // Assign the values of the array.
    for (u8 i = 0; i < variableCount; i++) {
      emitBytes(parser, BC_DESTRUCT_ARRAY, i);
      defineVariable(parser, variables[i], isGlobal);

      // Can't set locals like this.
      if (!isGlobal) {
        u8 local = resolveLocal(parser, parser->compiler, &tokens[i]);
        emitBytes(parser, BC_SET_LOCAL, local);
        emitByte(parser, BC_POP);
      }
    }

    emitByte(parser, BC_POP);
  } else {
    u8 global = parseVariable(parser, isGlobal, "Expected identifier.");

    if (match(parser, TOKEN_EQUAL)) {
      expression(parser);
    } else {
      emitByte(parser, BC_NIL);
    }

    defineVariable(parser, global, isGlobal);
  }

  consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
}

static void localVarDeclaration(struct Parser* parser) {
  varDeclaration(parser, false);
}

static void structDeclaration(struct Parser* parser, bool isGlobal) {
  if (parser->compiler->scopeDepth != 0) {
    error(parser, "Structs must be defined in top-level code.");
  }

  struct StructCompiler structCompiler;
  structCompiler.enclosing = parser->structCompiler;
  parser->structCompiler = &structCompiler;

  consume(parser, TOKEN_IDENTIFIER, "Expected struct identifier.");
  struct Token structName = parser->previous;
  u8 nameConstant = identifierConstant(parser, &parser->previous);
  declareVariable(parser, isGlobal);

  emitBytes(parser, BC_STRUCT, nameConstant);
  defineVariable(parser, nameConstant, isGlobal);

  namedVariable(parser, structName, false);

  consume(parser, TOKEN_LBRACE, "Expected struct body.");

  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    if (match(parser, TOKEN_VAR)) {
      // Default value can't be done at compile time, so we'll need an
      // instruction to do that
      consume(parser, TOKEN_IDENTIFIER, "Expected field identifier.");
      struct Token name = parser->previous;

      if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
      } else {
        emitByte(parser, BC_NIL);
      }

      emitBytes(parser, BC_STRUCT_FIELD, identifierConstant(parser, &name));

      consume(parser, TOKEN_SEMICOLON, "Expected ';' after field.");
    } else if (match(parser, TOKEN_FUNC)) {
      method(parser, false);
    } else if (match(parser, TOKEN_STATIC)) {
      consume(parser, TOKEN_FUNC, "Expected 'func' after 'static'.");
      method(parser, true);
    }
  }

  consume(parser, TOKEN_RBRACE, "Unterminated struct declaration.");

  emitByte(parser, BC_POP); // Struct

  parser->structCompiler = parser->structCompiler->enclosing;
}

void enumDeclaration(struct Parser* parser, bool isGlobal) {
  if (parser->compiler->scopeDepth != 0) {
    error(parser, "Enums must be defined in top-level code.");
  }

  consume(parser, TOKEN_IDENTIFIER, "Expected enum identifier.");
  struct Token enumName = parser->previous;
  u8 nameConstant = identifierConstant(parser, &parser->previous);
  declareVariable(parser, isGlobal);

  emitBytes(parser, BC_ENUM, nameConstant);
  defineVariable(parser, nameConstant, isGlobal);

  namedVariable(parser, enumName, false);

  consume(parser, TOKEN_LBRACE, "Expected '{'.");

  u8 enumValue = 0;
  if (!check(parser, TOKEN_RBRACE)) {
    do {
      if (enumValue == UINT8_MAX) {
        error(parser, "Cannot have more than 255 enum values.");
      }

      consume(parser, TOKEN_IDENTIFIER, "Expected enum value.");
      emitByte(parser, BC_ENUM_VALUE);
      emitByte(parser, identifierConstant(parser, &parser->previous));
      emitByte(parser, enumValue++);

      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACE)) {
        error(parser, "Expected ','.");
      }
    } while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF));
  }

  consume(parser, TOKEN_RBRACE, "Unterminated enum declaration.");

  emitByte(parser, BC_POP); // Enum
}

static void globalDeclaration(struct Parser* parser) {
  if (match(parser, TOKEN_VAR)) {
    varDeclaration(parser, true);
  } else if (match(parser, TOKEN_FUNC)) {
    functionDeclaration(parser, true);
  } else if (match(parser, TOKEN_STRUCT)) {
    structDeclaration(parser, true);
  } else if (match(parser, TOKEN_ENUM)) {
    enumDeclaration(parser, true);
  } else {
    error(parser, "Expected a declaration after 'global'.");
  }
}

static void expressionStatement(struct Parser* parser) {
  expression(parser);
  emitByte(parser, BC_POP);
  consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression.");
}

static void ifStatement(struct Parser* parser) {
  consume(parser, TOKEN_LPAREN, "Expected '('.");
  expression(parser);
  consume(parser, TOKEN_RPAREN, "Expected ')'.");
  
  s32 thenJump = emitJump(parser, BC_JUMP_IF_FALSE);
  emitByte(parser, BC_POP);

  statement(parser);

  s32 elseJump = emitJump(parser, BC_JUMP);
  patchJump(parser, thenJump);
  emitByte(parser, BC_POP);

  if (match(parser, TOKEN_ELSE)) {
    statement(parser);
  }

  patchJump(parser, elseJump);
}

void matchStatement(struct Parser* parser) {
  consume(parser, TOKEN_LPAREN, "Expected '('.");
  expression(parser);
  consume(parser, TOKEN_RPAREN, "Expected ')'.");

  s32 caseEnds[UINT8_MAX];
  u8 caseCount = 0;

  consume(parser, TOKEN_LBRACE, "Expected '{'");

  if (match(parser, TOKEN_CASE)) {
    do {
      expression(parser);

      s32 inequalityJump = emitJump(parser, BC_INEQUALITY_JUMP);

      consume(parser, TOKEN_RIGHT_ARROW, "Expected '=>' after case expression.");
      statement(parser);

      if (caseCount == UINT8_MAX) {
        error(parser, "Cannot have more than 255 cases in a single match statement.");
      }

      caseEnds[caseCount++] = emitJump(parser, BC_JUMP);

      patchJump(parser, inequalityJump);
    } while (match(parser, TOKEN_CASE));
  }

  if (match(parser, TOKEN_ELSE)) {
    consume(parser, TOKEN_RIGHT_ARROW, "Expected '=>' after 'else'.");
    statement(parser);
  }

  if (match(parser, TOKEN_CASE)) {
    error(parser, "Default case must be the last case.");
  }

  for (s32 i = 0; i < caseCount; i++) {
    patchJump(parser, caseEnds[i]);
  }

  emitByte(parser, BC_POP); // Expression

  consume(parser, TOKEN_RBRACE, "Expected '}'");
}

static void continueStatement(struct Parser* parser) {
  if (parser->compiler->loop == NULL) {
    error(parser, "Cannot use 'continue' outside of a loop.");
    return;
  }

  struct Loop* loop = parser->compiler->loop;
  if (match(parser, TOKEN_IDENTIFIER)) {
    loop = resolveLoopLabel(parser, &parser->previous);
    if (loop == NULL) {
      error(parser, "Invalid continue target.");
      return;
    }
  }

  discardLocals(parser, loop->scopeDepth);

  emitLoop(parser, loop->start);
  consume(parser, TOKEN_SEMICOLON, "Expected semicolon after 'break'.");
}

static void breakStatement(struct Parser* parser) {
  if (parser->compiler->loop == NULL) {
    error(parser, "Cannot use 'break' outside of a loop.");
    return;
  }

  struct Loop* loop = parser->compiler->loop;
  if (match(parser, TOKEN_IDENTIFIER)) {
    loop = resolveLoopLabel(parser, &parser->previous);
    if (loop == NULL) {
      error(parser, "Invalid break target.");
      return;
    }
  }

  discardLocals(parser, loop->scopeDepth);

  s32 index = emitJump(parser, BC_BREAK) - 1;

  if (loop->breakCount == UINT8_MAX) {
    error(parser, "Too many break statements in a loop.");
    return;
  }
  loop->breakIndices[loop->breakCount++] = index;

  consume(parser, TOKEN_SEMICOLON, "Expected semicolon after 'break'.");
}

static void whileStatement(struct Parser* parser) {
  struct Loop loop;
  beginLoop(parser, &loop);

  consume(parser, TOKEN_LPAREN, "Expected '(' before while condition.");
  expression(parser);
  consume(parser, TOKEN_RPAREN, "Expected ')' after while condition.");

  if (match(parser, TOKEN_COLON)) {
    consume(parser, TOKEN_IDENTIFIER, "Expected loop label.");
    loop.isNamed = true;
    loop.name = parser->previous;
  }

  s32 exitJump = emitJump(parser, BC_JUMP_IF_FALSE);
  emitByte(parser, BC_POP);

  loop.bodyStart = currentFunction(parser)->bcCount;
  statement(parser);

  emitLoop(parser, loop.start);

  patchJump(parser, exitJump);
  emitByte(parser, BC_POP);

  endLoop(parser, &loop);
}

static void loopStatement(struct Parser* parser) {
  struct Loop loop;
  beginLoop(parser, &loop);

  if (match(parser, TOKEN_COLON)) {
    consume(parser, TOKEN_IDENTIFIER, "Expected loop label.");
    loop.isNamed = true;
    loop.name = parser->previous;
  }

  loop.start = currentFunction(parser)->bcCount;
  statement(parser);
  emitLoop(parser, loop.start);

  endLoop(parser, &loop);
}

static void returnStatement(struct Parser* parser) {
  if (parser->compiler->type == FUNCTION_TYPE_SCRIPT) {
    error(parser, "Can only return in functions.");
  }

  if (match(parser, TOKEN_SEMICOLON)) {
    emitReturn(parser);
    return;
  }

  expression(parser);
  consume(parser, TOKEN_SEMICOLON, "Expected ';' after return value.");
  emitByte(parser, BC_RETURN);
}

static void synchronize(struct Parser* parser) {
  parser->panicMode = false;

  while (parser->current.type != TOKEN_EOF) {
    if (parser->previous.type == TOKEN_SEMICOLON) {
      return;
    }

    switch (parser->current.type) {
      case TOKEN_STRUCT:
      case TOKEN_STATIC:
      case TOKEN_FUNC:
      case TOKEN_ENUM:
      case TOKEN_MATCH:
      case TOKEN_GLOBAL:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_LOOP:
      case TOKEN_RETURN:
        return;
      default:
        break;
    }

    advance(parser);
  }
}

static void declaration(struct Parser* parser) {
  if (match(parser, TOKEN_VAR)) {
    localVarDeclaration(parser);
  } else if (match(parser, TOKEN_GLOBAL)) {
    globalDeclaration(parser);
  } else if (match(parser, TOKEN_FUNC)) {
    functionDeclaration(parser, false);
  } else if (match(parser, TOKEN_STRUCT)) {
    structDeclaration(parser, false);
  } else if (match(parser, TOKEN_ENUM)) {
    enumDeclaration(parser, false);
  } else {
    statement(parser);
  }

  if (parser->panicMode) {
    synchronize(parser);
  }
}

static void statement(struct Parser* parser) {
  if (match(parser, TOKEN_IF)) {
    ifStatement(parser);
  } else if (match(parser, TOKEN_MATCH)) {
    matchStatement(parser);
  } else if (match(parser, TOKEN_WHILE)) {
    whileStatement(parser);
  } else if (match(parser, TOKEN_LOOP)) {
    loopStatement(parser);
  } else if (match(parser, TOKEN_LBRACE)) {
    beginScope(parser);
    block(parser);
    endScope(parser);
  } else if (match(parser, TOKEN_BREAK)) {
    breakStatement(parser);
  } else if (match(parser, TOKEN_CONTINUE)) {
    continueStatement(parser);
  } else if (match(parser, TOKEN_RETURN)) {
    returnStatement(parser);
  } else if (match(parser, TOKEN_LBRACKET)) {
    arrayDestructAssignment(parser);
  } else {
    expressionStatement(parser);
  }
}

struct Function* compile(struct State* H, struct Parser* parser, const char* source) {
  parser->H = H;
  parser->compiler = NULL;
  parser->structCompiler = NULL;
  parser->tokenizer = NULL;
  parser->hadError = false;
  parser->panicMode = false;

  struct Tokenizer tokenizer;
  initTokenizer(H, &tokenizer, source);
  parser->tokenizer = &tokenizer;

  struct Compiler compiler;
  initCompiler(parser, &compiler, FUNCTION_TYPE_SCRIPT);
  compiler.localOffset = 1;

  advance(parser);
  while (!match(parser, TOKEN_EOF)) {
    declaration(parser);
  }

  struct Function* function = endCompiler(parser);
  return parser->hadError ? NULL : function;
}

void markCompilerRoots(struct State* H, struct Parser* parser) {
  if (parser == NULL) {
    return;
  }

  markValue(H, parser->current.value);
  markValue(H, parser->previous.value);

  struct Compiler* compiler = parser->compiler;
  while (compiler != NULL) {
    markObject(H, (struct Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
