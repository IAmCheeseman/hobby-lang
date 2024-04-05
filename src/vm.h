#ifndef _HOBBYSCRIPT_VM_H
#define _HOBBYSCRIPT_VM_H

#include "object.h"

enum InterpretResult {
  INTERPRET_OK,
  COMPILE_ERR,
  RUNTIME_ERR,
};

void bindCFunction(struct hs_State* H, const char* name, hs_CFunction cFunction);
enum InterpretResult interpret(struct hs_State* H, const char* source);

#endif // _HOBBYSCRIPT_VM_H
