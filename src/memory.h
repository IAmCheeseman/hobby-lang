#ifndef _HOBBYL_MEMORY_H
#define _HOBBYL_MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(H, type, count) \
    (type*)reallocate(H, NULL, 0, sizeof(type) * (count))
#define FREE(H, type, pointer) reallocate(H, pointer, sizeof(type), 0)
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(H, type, pointer, oldCount, newCount) \
    (type*)reallocate( \
        H, pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))
#define FREE_ARRAY(H, type, pointer, oldCount) \
    reallocate(H, pointer, sizeof(type) * (oldCount), 0)

void* reallocate(struct hl_State* H, void* pointer, size_t oldSize, size_t newSize);
void markObject(struct hl_State* H, struct GcObj* object);
void markValue(struct hl_State* H, Value value);
void collectGarbage(struct hl_State* H);
void freeObjects(struct hl_State* H);

#endif // _HOBBYL_MEMORY_H
