#ifndef _HOBBYL_VM_H
#define _HOBBYL_VM_H

#include "object.h"

enum InterpretResult {
  INTERPRET_OK,
  COMPILE_ERR,
  RUNTIME_ERR,
};

void bindCFunction(struct hl_State* H, const char* name, hl_CFunction cFunction);
enum InterpretResult interpret(struct hl_State* H, const char* source);

#endif // _HOBBYL_VM_H
