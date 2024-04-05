#ifndef _HOBBYSCRIPT_TABLE_H
#define _HOBBYSCRIPT_TABLE_H

#include "common.h"
#include "object.h"

void initTable(struct Table* table);
void freeTable(struct hs_State* H, struct Table* table);
bool tableSet(
    struct hs_State* H, struct Table* table, struct GcString* key, Value value);
bool tableGet(
    struct Table* table, struct GcString* key, Value* outValue);
bool tableDelete(struct Table* table, struct GcString* key);
struct GcString* tableFindString(
    struct Table* table, const char* chars, s32 length, u32 hash);
void tableRemoveUnmarked(struct Table* table);
void copyTable(struct hs_State* H, struct Table* dest, struct Table* src);
void markTable(struct hs_State* H, struct Table* table);

#endif // _HOBBYSCRIPT_TABLE_H
