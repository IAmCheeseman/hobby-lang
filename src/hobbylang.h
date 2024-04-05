#ifndef _HOBBYLANG_H
#define _HOBBYLANG_H

#include <stdlib.h>
#include <stdbool.h>

#define HS_VERSION_MAJOR 0
#define HS_VERSION_MINOR 1
#define HS_VERSION_PATCH 0

struct hs_State;

typedef void (*hs_CFunction)(struct hs_State* H, int argCount);

struct hs_State* hs_newState();
void hs_freeState(struct hs_State* state);

void hs_pop(struct hs_State* H);

void hs_setGlobal(struct hs_State* H, const char* name);

void hs_pushNil(struct hs_State* H);
void hs_pushNumber(struct hs_State* H, double v);
void hs_pushBoolean(struct hs_State* H, bool v);
void hs_pushString(struct hs_State* H, const char* str, size_t length);
void hs_pushOwnedString(struct hs_State* H, char* str, size_t length);
void hs_pushCFunction(struct hs_State* H, hs_CFunction function, int argCount);

bool hs_isString(struct hs_State* H, int index);

const char* hs_toString(struct hs_State* H, int index, size_t* length);

#endif // _HOBBYLANG_H
