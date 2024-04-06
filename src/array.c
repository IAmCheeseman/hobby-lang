#include "common.h"
#include "hobbyscript.h"
#include "modules.h"
#include "object.h"
#include "table.h"
#include <stdio.h>

void array_push(struct hs_State* H, UNUSED s32 argCount) {
  if (!hs_isArray(H, 0)) {
    printf("Expected array.\n");
    hs_pushNil(H);
    return;
  }
  hs_push(H, 1); // Value
  hs_pushArray(H, 0);
  hs_pushNil(H);
}

struct hs_FuncInfo array[] = {
  {array_push, "push", 1},
  {NULL, NULL, -1},
};

void openArray(struct hs_State* H) {
  for (struct hs_FuncInfo* info = array; info->func != NULL; info++) {
    tableSet(
        H, &H->arrayMethods,
        copyString(H, info->name, strlen(info->name)),
        NEW_OBJ(newCFunction(H, info->func, info->argCount)));
  }
}
