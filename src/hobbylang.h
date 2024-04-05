#ifndef _HOBBYLANG_H
#define _HOBBYLANG_H

#include <stdlib.h>
#include <stdbool.h>

#define HL_VERSION_MAJOR 0
#define HL_VERSION_MINOR 1
#define HL_VERSION_PATCH 0

struct hl_State;

typedef void (*hl_CFunction)(struct hl_State* H, int argCount);

struct hl_State* hl_newState();
void hl_freeState(struct hl_State* state);

void hl_pop(struct hl_State* H);

void hl_setGlobal(struct hl_State* H, const char* name);

void hl_pushNil(struct hl_State* H);
void hl_pushNumber(struct hl_State* H, double v);
void hl_pushBoolean(struct hl_State* H, bool v);
void hl_pushString(struct hl_State* H, const char* str, size_t length);
void hl_pushOwnedString(struct hl_State* H, char* str, size_t length);
void hl_pushCFunction(struct hl_State* H, hl_CFunction function, int argCount);

bool hl_isString(struct hl_State* H, int index);

const char* hl_toString(struct hl_State* H, int index, size_t* length);

#endif
