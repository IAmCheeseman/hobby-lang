#include "common.h"
#include "modules.h"

#include <stdio.h>
#include <time.h>

static void core_print(struct hs_State* H, UNUSED s32 argCount) {
  for (s32 i = 0; i < argCount; i++) {
    size_t length;
    const char* str = hs_toString(H, i, &length);
    fwrite(str, sizeof(char), length, stdout);
    if (i != argCount - 1) {
      putc('\t', stdout);
    }
  }
  putc('\n', stdout);
  hs_pushNil(H);
}

static void core_toString(struct hs_State* H, UNUSED s32 argCount) {
  size_t length;
  const char* str = hs_toString(H, 0, &length);
  hs_pushString(H, str, length);
}

static void core_clock(struct hs_State* H, UNUSED s32 argCount) {
  hs_pushNumber(H, (f64)clock() / CLOCKS_PER_SEC);
}

struct hs_FuncInfo core[] = {
  {core_print, "print", -1},
  {core_toString, "toString", 1},
  {core_clock, "clock", 0},
  {NULL, NULL, -1},
};

void openCore(struct hs_State* H) {
  hs_registerGlobalFunctions(H, core);
}
