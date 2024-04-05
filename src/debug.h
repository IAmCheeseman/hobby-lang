#ifndef _HOBBYSCRIPT_DEBUG_H
#define _HOBBYSCRIPT_DEBUG_H

#include "common.h"
#include "object.h"

void disassembleFunction(
  struct hs_State* H, 
  struct GcBcFunction* function, void* functionPointer, const char* name);
s32 disassembleInstruction(
  struct hs_State* H,
  struct GcBcFunction* function, s32 offset);

#endif // _HOBBYSCRIPT_DEBUG_H
