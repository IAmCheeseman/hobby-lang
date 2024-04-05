#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "table.h"
#include "vm.h"
#include "state.h"

#define ALLOCATE_OBJ(H, type, objectType) \
    (type*)allocateObject(H, sizeof(type), objectType)

static struct GcObj* allocateObject(struct hs_State* H, size_t size, enum ObjType type) {
  struct GcObj* object = (struct GcObj*)reallocate(H, NULL, 0, size);
  object->type = type;
  object->isMarked = false;
  
  object->next = H->objects;
  H->objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

struct GcArray* newArray(struct hs_State* H) {
  struct GcArray* array = ALLOCATE_OBJ(H, struct GcArray, OBJ_ARRAY);
  initValueArray(&array->values);
  return array;
}

struct GcEnum* newEnum(struct hs_State* H, struct GcString* name) {
  struct GcEnum* enoom = ALLOCATE_OBJ(H, struct GcEnum, OBJ_ENUM);
  enoom->name = name;
  initTable(&enoom->values);
  return enoom;
}

struct GcBoundMethod* newBoundMethod(
    struct hs_State* H, Value receiver, struct GcClosure* method) {
  struct GcBoundMethod* bound = ALLOCATE_OBJ(H, struct GcBoundMethod, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

struct GcStruct* newStruct(struct hs_State* H, struct GcString* name) {
  struct GcStruct* strooct = ALLOCATE_OBJ(H, struct GcStruct, OBJ_STRUCT);

  strooct->name = name;
  initTable(&strooct->defaultFields);
  initTable(&strooct->methods);
  initTable(&strooct->staticMethods);
  return strooct;
}

struct GcInstance* newInstance(struct hs_State* H, struct GcStruct* strooct) {
  struct GcInstance* instance = ALLOCATE_OBJ(H, struct GcInstance, OBJ_INSTANCE);
  instance->strooct = strooct;
  initTable(&instance->fields);
  copyTable(H, &instance->fields, &strooct->defaultFields);
  return instance;
}

struct GcClosure* newClosure(struct hs_State* H, struct GcBcFunction* function) {
  struct GcUpvalue** upvalues = ALLOCATE(
      H, struct GcUpvalue*, function->upvalueCount);
  for (s32 i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  struct GcClosure* closure = ALLOCATE_OBJ(H, struct GcClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

struct GcUpvalue* newUpvalue(struct hs_State* H, Value* slot) {
  struct GcUpvalue* upvalue = ALLOCATE_OBJ(H, struct GcUpvalue, OBJ_UPVALUE);
  upvalue->location = slot;
  upvalue->closed = NEW_NIL;
  upvalue->next = NULL;
  return upvalue;
}

struct GcBcFunction* newBcFunction(struct hs_State* H) {
  struct GcBcFunction* function = ALLOCATE_OBJ(H, struct GcBcFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;

  function->bcCount = 0;
  function->bcCapacity = 0;
  function->bc = NULL;
  function->lines = NULL;
  initValueArray(&function->constants);

  return function;
}

struct GcCFunction* newCFunction(struct hs_State* H, hs_CFunction cFunc, s32 argCount) {
  struct GcCFunction* cFunction = ALLOCATE_OBJ(
      H, struct GcCFunction, OBJ_CFUNCTION);
  cFunction->cFunc = cFunc;
  cFunction->arity = argCount;
  return cFunction;
}

static struct GcString* allocateString(struct hs_State* H, char* chars, s32 length, u32 hash) {
  struct GcString* string = ALLOCATE_OBJ(H, struct GcString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  
  push(H, NEW_OBJ(string));
  tableSet(H, &H->strings, string, NEW_NIL);
  pop(H);

  return string;
}

static u32 hashString(const char* key, int length) {
  u32 hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (u8)key[i];
    hash *= 16777619;
  }
  return hash;
}

struct GcString* copyString(struct hs_State* H, const char* chars, s32 length) {
  u32 hash = hashString(chars, length);
  struct GcString* interned = tableFindString(&H->strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }
  char* ownedChars = ALLOCATE(H, char, length + 1);
  memcpy(ownedChars, chars, length);
  ownedChars[length] = '\0';
  return allocateString(H, ownedChars, length, hash);
}

struct GcString* takeString(struct hs_State* H, char* chars, s32 length) {
  u32 hash = hashString(chars, length);
  struct GcString* interned = tableFindString(&H->strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(H, char, chars, length + 1);
    return interned;
  }
  return allocateString(H, chars, length, hash);
}

void writeBytecode(struct hs_State* H, struct GcBcFunction* function, u8 byte, s32 line) {
  if (function->bcCapacity < function->bcCount + 1) {
    s32 oldCapacity = function->bcCapacity;
    function->bcCapacity = GROW_CAPACITY(oldCapacity);
    function->bc = GROW_ARRAY(H, u8, function->bc, oldCapacity, function->bcCapacity);
    function->lines = GROW_ARRAY(H, s32, function->lines, oldCapacity, function->bcCapacity);
  }

  function->bc[function->bcCount] = byte;
  function->lines[function->bcCount] = line;
  function->bcCount++;
}

s32 addFunctionConstant(
    struct hs_State* H, struct GcBcFunction* function, Value value) {
  push(H, value);
  writeValueArray(H, &function->constants, value);
  pop(H);
  return function->constants.count - 1;
}

static void printFunction(struct GcBcFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<function %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_UPVALUE:
      printf("<upvalue %p>", AS_OBJ(value));
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_BOUND_METHOD:
      printFunction(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CFUNCTION:
      printf("<cfunction>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_STRUCT:
      printf("<struct %s>", AS_STRUCT(value)->name->chars);
      break;
    case OBJ_INSTANCE:
      printf("<%s instance>",
          AS_INSTANCE(value)->strooct->name->chars);
      break;
    case OBJ_ENUM:
      printf("<enum %s>", AS_ENUM(value)->name->chars);
      break;
    case OBJ_ARRAY:
      printf("<array>");
      break;
  }
}
