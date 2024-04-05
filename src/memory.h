#ifndef _HOBBYSCRIPT_MEMORY_H
#define _HOBBYSCRIPT_MEMORY_H

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

void* reallocate(struct hs_State* H, void* pointer, size_t oldSize, size_t newSize);
void markObject(struct hs_State* H, struct GcObj* object);
void markValue(struct hs_State* H, Value value);
void collectGarbage(struct hs_State* H);
void freeObjects(struct hs_State* H);

#endif // _HOBBYSCRIPT_MEMORY_H
