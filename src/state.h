#ifndef _HOBBYSCRIPT_STATE_H
#define _HOBBYSCRIPT_STATE_H

#include "object.h"
#include "common.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * U8_COUNT)

struct CallFrame {
  struct GcClosure* func;
  u8* ip;
  Value* slots;
};

struct hs_State {
  struct CallFrame frames[FRAMES_MAX];
  s32 frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  struct Table globals;
  struct Table strings;
  struct Table arrayMethods;
  struct GcUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGc;

  struct GcObj* objects;

  s32 grayCount;
  s32 grayCapacity;
  struct GcObj** grayStack;

  struct Parser* parser;
};

void resetStack(struct hs_State* H);

inline void push(struct hs_State* H, Value value) {
  *H->stackTop = value;
  H->stackTop++;
}

inline Value pop(struct hs_State* H) {
  H->stackTop--;
  return *H->stackTop;
}

inline Value peek(struct hs_State* H, s32 distance) {
  return H->stackTop[-1 - distance];
}

#endif // _HOBBYSCRIPT_STATE_H
