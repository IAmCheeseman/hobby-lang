#include "state.h"

#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "compiler.h"
#include "object.h"
#include "table.h"
#include "tostring.h"

static void wrap_print(struct hl_State* H, s32 argCount) {
  for (s32 i = 0; i < argCount; i++) {
    size_t length;
    const char* str = hl_toString(H, i, &length);
    fwrite(str, sizeof(char), length, stdout);
    if (i != argCount - 1) {
      putc('\t', stdout);
    }
  }
  putc('\n', stdout);
  hl_pushNil(H);
}

void resetStack(struct hl_State* H) {
  H->stackTop = H->stack;
  H->frameCount = 0;
  H->openUpvalues = NULL;
}

static Value* getValueAt(struct hl_State* H, s32 index) {
  if (index >= 0) {
    Value* value = H->frames[H->frameCount - 1].slots + index + 1;
    return (value > H->stackTop) ? NULL : value;
  }

  Value* value = H->stackTop + index;
  return (value > H->stackTop) ? NULL : value;
}

void push(struct hl_State* H, Value value) {
  *H->stackTop = value;
  H->stackTop++;
}

Value pop(struct hl_State* H) {
  H->stackTop--;
  return *H->stackTop;
}

Value peek(struct hl_State* H, s32 distance) {
  return H->stackTop[-1 - distance];
}

struct hl_State* hl_newState() {
  struct hl_State* H = malloc(sizeof(struct hl_State));

  H->objects = NULL;
  H->parser = ALLOCATE(H, struct Parser, 1);
  H->bytesAllocated = 0;
  H->nextGc = 1024 * 1024;
  H->grayCount = 0;
  H->grayCapacity = 0;
  H->grayStack = NULL;
  resetStack(H);
  initTable(&H->strings);
  initTable(&H->globals);

  hl_pushCFunction(H, wrap_print, -1);
  hl_setGlobal(H, "print");

  return H;
}

void hl_freeState(struct hl_State* H) {
  freeTable(H, &H->strings);
  freeTable(H, &H->globals);
  freeObjects(H);
  FREE(H, struct Parser, H->parser);

  free(H);
}

void hl_pop(struct hl_State* H) {
  pop(H);
}

void hl_setGlobal(struct hl_State* H, const char* name) {
  push(H, NEW_OBJ(copyString(H, name, strlen(name))));
  tableSet(H, &H->globals, AS_STRING(peek(H, 0)), peek(H, 1));
  pop(H); // name
  pop(H); // value
}

void hl_pushNil(struct hl_State* H) {
  push(H, NEW_NIL);
}

void hl_pushNumber(struct hl_State* H, double v) {
  push(H, NEW_NUMBER(v));
}

void hl_pushBoolean(struct hl_State* H, bool v) {
  push(H, NEW_BOOL(v));
}

void hl_pushString(struct hl_State* H, const char* str, size_t length) {
  push(H, NEW_OBJ(copyString(H, str, length)));
}

void hl_pushOwnedString(struct hl_State* H, char* str, size_t length) {
  push(H, NEW_OBJ(takeString(H, str, length)));
}

void hl_pushCFunction(struct hl_State* H, hl_CFunction function, int argCount) {
  push(H, NEW_OBJ(newCFunction(H, function, argCount)));
}

bool hl_isString(struct hl_State* H, int index) {
  Value* v = getValueAt(H, index);
  if (v == NULL) {
    return false;
  }
  return IS_STRING(*v);
}

const char* hl_toString(struct hl_State* H, int index, size_t* length) {
  Value* v = getValueAt(H, index);
  if (v == NULL) {
    if (length != NULL) {
      *length = 0;
    }
    return NULL;
  }

  struct GcString* str = toString(H, v);
  if (length != NULL) {
    *length = str->length;
  }
  return str->chars;
}
