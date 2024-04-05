#ifndef _HOBBYL_DEBUG_H
#define _HOBBYL_DEBUG_H

#include "common.h"
#include "object.h"

void disassembleFunction(struct GcBcFunction* function, void* functionPointer, const char* name);
s32 disassembleInstruction(struct GcBcFunction* function, s32 offset);

#endif // _HOBBYL_DEBUG_H
