#ifndef _HOBBYSCRIPT_COMPILER_H
#define _HOBBYSCRIPT_COMPILER_H

#include "tokenizer.h"
#include "object.h"

struct Loop {
  s32 start;
  s32 bodyStart;
  s32 scopeDepth;
  s32 breakIndices[UINT8_MAX];
  u8 breakCount;
  bool isNamed;
  struct Token name;
  struct Loop* enclosing;
};

struct Local {
  struct Token name;
  s32 depth;
  bool isCaptured;
};

struct CompilerUpvalue {
  u8 index;
  bool isLocal;
};

enum FunctionType {
  FUNCTION_TYPE_FUNCTION,
  FUNCTION_TYPE_METHOD,
  FUNCTION_TYPE_SCRIPT,
};

struct Compiler {
  struct Compiler* enclosing;

  struct GcBcFunction* function;
  enum FunctionType type;

  s32 localOffset;
  struct Local locals[U8_COUNT];
  struct Loop* loop;
  s32 localCount;
  struct CompilerUpvalue upvalues[U8_COUNT];
  s32 scopeDepth;
};

struct StructField {
  struct Token name;
};

struct StructCompiler {
  struct StructCompiler* enclosing;
};

struct Parser {
  struct hs_State* H;
  struct Token current;
  struct Token previous;
  struct Compiler* compiler;
  struct StructCompiler* structCompiler;
  struct Tokenizer* tokenizer;
  bool hadError;
  bool panicMode;
};

struct GcBcFunction* compile(struct hs_State* H, struct Parser* parser, const char* source);
void markCompilerRoots(struct hs_State* H, struct Parser* parser);

#endif // _HOBBYSCRIPT_COMPILER_H
