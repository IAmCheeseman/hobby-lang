#ifndef _HOBBYL_STATE_H
#define _HOBBYL_STATE_H

#include "object.h"
#include "common.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * U8_COUNT)

struct CallFrame {
  union GcFunction* func;
  u8* ip;
  Value* slots;
};

struct hl_State {
  struct CallFrame frames[FRAMES_MAX];
  s32 frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  struct Table globals;
  struct Table strings;
  struct GcUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGc;

  struct GcObj* objects;

  s32 grayCount;
  s32 grayCapacity;
  struct GcObj** grayStack;

  struct Parser* parser;
};

void resetStack(struct hl_State* H);
void push(struct hl_State* H, Value value);
Value pop(struct hl_State* H);
Value peek(struct hl_State* H, s32 distance);

#endif // _HOBBYL_STATE_H
