#include "memory.h"

#include <stdlib.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>

#include "debug.h"
#endif

#include "object.h"
#include "compiler.h"
#include "table.h"
#include "state.h"

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(struct hl_State* H, void* pointer, size_t oldSize, size_t newSize) {
  H->bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage(H);
#else
    if (H->bytesAllocated > H->nextGc) {
      collectGarbage(H);
    }
#endif
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* newAllocation = realloc(pointer, newSize);
  if (newAllocation == NULL) {
    exit(1);
  }
  return newAllocation;
}

static void freeObject(struct hl_State* H, struct GcObj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

  switch (object->type) {
    case OBJ_ARRAY: {
      struct GcArray* array = (struct GcArray*)object;
      freeValueArray(H, &array->values);
      FREE(H, struct GcArray, array);
      break;
    }
    case OBJ_ENUM: {
      struct GcEnum* enoom = (struct GcEnum*)object;
      freeTable(H, &enoom->values);
      FREE(H, struct GcEnum, object);
      break;
    }
    case OBJ_STRUCT: {
      struct GcStruct* strooct = (struct GcStruct*)object;
      freeTable(H, &strooct->defaultFields);
      freeTable(H, &strooct->methods);
      freeTable(H, &strooct->staticMethods);
      FREE(H, struct GcStruct, object);
      break;
    }
    case OBJ_INSTANCE: {
      struct GcInstance* instance = (struct GcInstance*)object;
      freeTable(H, &instance->fields);
      FREE(H, struct GcInstance, object);
      break;
    }
    case OBJ_CLOSURE: {
      struct GcClosure* closure = (struct GcClosure*)object;
      FREE_ARRAY(
          H, struct GcUpvalue*, closure->upvalues, closure->upvalueCount);
      FREE(H, struct GcClosure, object);
      break;
    }
    case OBJ_UPVALUE: {
      FREE(H, struct GcUpvalue, object);
      break;
    }
    case OBJ_FUNCTION: {
      struct GcBcFunction* function = (struct GcBcFunction*)object;
      FREE_ARRAY(H, u8, function->bc, function->bcCapacity);
      FREE_ARRAY(H, s32, function->lines, function->bcCapacity);
      freeValueArray(H, &function->constants);
      FREE(H, struct GcBcFunction, object);
      break;
    }
    case OBJ_BOUND_METHOD: {
      FREE(H, struct GcBoundMethod, object);
      break;
    }
    case OBJ_CFUNCTION: {
      FREE(H, struct GcCFunction, object);
      break;
    }
    case OBJ_STRING: {
      struct GcString* string = (struct GcString*)object;
      FREE_ARRAY(H, char, string->chars, string->length + 1);
      FREE(H, struct GcString, object);
      break;
    }
  }
}

void markObject(struct hl_State* H, struct GcObj* object) {
  if (object == NULL) {
    return;
  }

  if (object->isMarked) {
    return;
  }

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(NEW_OBJ(object));
  printf("\n");
#endif

  object->isMarked = true;

  if (H->grayCapacity < H->grayCount + 1) {
    H->grayCapacity = GROW_CAPACITY(H->grayCapacity);
    H->grayStack = (struct GcObj**)realloc(
        H->grayStack, sizeof(struct GcObj*) * H->grayCapacity);
    if (H->grayStack == NULL) {
      exit(1);
    }
  }

  H->grayStack[H->grayCount++] = object;
}

void markValue(struct hl_State* H, Value value) {
  if (IS_OBJ(value)) {
    markObject(H, AS_OBJ(value));
  }
}

static void markArray(struct hl_State* H, struct ValueArray* array) {
  for (s32 i = 0; i < array->count; i++) {
    markValue(H, array->values[i]);
  }
}

static void blackenObject(struct hl_State* H, struct GcObj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(NEW_OBJ(object));
  printf("\n");
#endif

  switch (object->type) {
    // No references.
    case OBJ_CFUNCTION:
    case OBJ_STRING:
      break;
    case OBJ_UPVALUE:
      markValue(H, ((struct GcUpvalue*)object)->closed);
      break;
    case OBJ_FUNCTION: {
      struct GcBcFunction* function = (struct GcBcFunction*)object;
      markObject(H, (struct GcObj*)function->name);
      markArray(H, &function->constants);
      break;
    }
    case OBJ_BOUND_METHOD: {
      struct GcBoundMethod* bound = (struct GcBoundMethod*)object;
      markValue(H, bound->receiver);
      markObject(H, (struct GcObj*)bound->method);
      break;
    }
    case OBJ_CLOSURE: {
      struct GcClosure* closure = (struct GcClosure*)object;
      markObject(H, (struct GcObj*)closure->function);
      for (s32 i = 0; i < closure->upvalueCount; i++) {
        markObject(H, (struct GcObj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_STRUCT: {
      struct GcStruct* strooct = (struct GcStruct*)object;
      markObject(H, (struct GcObj*)strooct->name);
      markTable(H, &strooct->defaultFields);
      markTable(H, &strooct->methods);
      markTable(H, &strooct->staticMethods);
      break;
    }
    case OBJ_INSTANCE: {
      struct GcInstance* instance = (struct GcInstance*)object;
      markObject(H, (struct GcObj*)instance->strooct);
      markTable(H, &instance->fields);
      break;
    }
    case OBJ_ENUM: {
      struct GcEnum* enoom = (struct GcEnum*)object;
      markObject(H, (struct GcObj*)enoom->name);
      markTable(H, &enoom->values);
      break;
    }
    case OBJ_ARRAY: {
      struct GcArray* array = (struct GcArray*)object;
      markArray(H, &array->values);
      break;
    }
  }
}

static void markRoots(struct hl_State* H) {
  for (Value* slot = H->stack; slot < H->stackTop; slot++) {
    markValue(H, *slot);
  }

  for (s32 i = 0; i < H->frameCount; i++) {
    markObject(H, (struct GcObj*)H->frames[i].func);
  }

  for (struct GcUpvalue* upvalue = H->openUpvalues;
      upvalue != NULL;
      upvalue = upvalue->next) {
    markObject(H, (struct GcObj*)upvalue);
  }

  markTable(H, &H->globals);
  markCompilerRoots(H, H->parser);
}

static void traceReferences(struct hl_State* H) {
  while (H->grayCount > 0) {
    struct GcObj* object = H->grayStack[--H->grayCount];
    blackenObject(H, object);
  }
}

static void sweep(struct hl_State* H) {
  struct GcObj* previous = NULL;
  struct GcObj* current = H->objects;

  while (current != NULL) {
    if (current->isMarked) {
      current->isMarked = false;
      previous = current;
      current = current->next;
    } else {
      struct GcObj* unreached = current;
      current = current->next;
      if (previous != NULL) {
        previous->next = current;
      } else {
        H->objects = current;
      }

      freeObject(H, unreached);
    }
  }
}

void collectGarbage(struct hl_State* H) {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = H->bytesAllocated;
#endif

  markRoots(H);
  traceReferences(H);
  tableRemoveUnmarked(&H->strings);
  sweep(H);

  H->nextGc = H->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("Collected %zu bytes (from %zu to %zu) next at %zu.\n",
      before - H->bytesAllocated, before, H->bytesAllocated, H->nextGc);
  printf("-- gc end\n");
#endif
}

void freeObjects(struct hl_State* H) {
  struct GcObj* object = H->objects;
  while (object != NULL) {
    struct GcObj* next = object->next;
    freeObject(H, object);
    object = next;
  }

  free(H->grayStack);
}
