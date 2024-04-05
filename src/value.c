#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"

void initValueArray(struct ValueArray* array) {
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void copyValueArray(struct hs_State* H, struct ValueArray* dest, struct ValueArray* src) {
  initValueArray(dest);

  dest->count = src->count;
  dest->capacity = src->capacity;
  dest->values = ALLOCATE(H, Value, dest->capacity);

  for (s32 i = 0; i < dest->count; i++) {
    dest->values[i] = src->values[i];
  }
}

void writeValueArray(struct hs_State* H, struct ValueArray* array, Value value) {
  if (array->capacity < array->count + 1) {
    s32 oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(
        H, Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count++] = value;
}

void reserveValueArray(struct hs_State* H, struct ValueArray* array, s32 size) {
  // Make sure the numbers stays at a power of 2.
  // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
  size--;
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size++;

  s32 newCapacity = size < 8 ? 8 : size;
  s32 oldCapacity = array->capacity;
  array->capacity = newCapacity;
  array->values = GROW_ARRAY(
      H, Value, array->values, oldCapacity, array->capacity);
}

void freeValueArray(struct hs_State* H, struct ValueArray* array) {
  FREE_ARRAY(H, Value, array->values, array->capacity);
  initValueArray(array);
}

void printValue(Value value) {
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%.14g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    printObject(value);
  }
}

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else
  if (a.type != b.type) {
    return false;
  }
  
  switch (a.type) {
    case VALTYPE_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VALTYPE_NIL:    return true;
    case VALTYPE_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VALTYPE_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
    default: return false;
  }
#endif
}
