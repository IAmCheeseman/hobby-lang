#ifndef _HOBBYSCRIPT_OBJECT_H
#define _HOBBYSCRIPT_OBJECT_H

#include <string.h>

#include "common.h"
#include "hobbyscript.h"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_CLOSURE(value)      isObjOfType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjOfType(value, OBJ_FUNCTION)
#define IS_CFUNCTION(value)    isObjOfType(value, OBJ_CFUNCTION)
#define IS_BOUND_METHOD(value) isObjOfType(value, OBJ_BOUND_METHOD)
#define IS_STRING(value)       isObjOfType(value, OBJ_STRING)
#define IS_STRUCT(value)       isObjOfType(value, OBJ_STRUCT)
#define IS_INSTANCE(value)     isObjOfType(value, OBJ_INSTANCE)
#define IS_ENUM(value)         isObjOfType(value, OBJ_ENUM)
#define IS_ARRAY(value)        isObjOfType(value, OBJ_ARRAY)
#define IS_UPVALUE(value)      isObjOfType(value, OBJ_UPVALUE)

#define AS_CLOSURE(value)      ((struct GcClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((struct GcBcFunction*)AS_OBJ(value))
#define AS_CFUNCTION(value)    ((struct GcCFunction*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((struct GcBoundMethod*)AS_OBJ(value))
#define AS_STRING(value)       ((struct GcString*)AS_OBJ(value))
#define AS_CSTRING(value)      (AS_STRING(value)->chars)
#define AS_STRUCT(value)       ((struct GcStruct*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((struct GcInstance*)AS_OBJ(value))
#define AS_ENUM(value)         ((struct GcEnum*)AS_OBJ(value))
#define AS_ARRAY(value)        ((struct GcArray*)AS_OBJ(value))

enum ObjType {
  OBJ_CLOSURE,
  OBJ_UPVALUE,
  OBJ_FUNCTION,
  OBJ_CFUNCTION,
  OBJ_BOUND_METHOD,
  OBJ_STRING,
  OBJ_STRUCT,
  OBJ_INSTANCE,
  OBJ_ENUM,
  OBJ_ARRAY,
};

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((u64)0x7ffc000000000000)
#define TAG_NIL   1
#define TAG_FALSE 2
#define TAG_TRUE  3

typedef u64 Value;

#define IS_BOOL(value)     (((value) | 1) == NEW_TRUE)
#define IS_NIL(value)      ((value) == NEW_NIL)
#define IS_NUMBER(value)   (((value) & QNAN) != QNAN)
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)     ((value) == NEW_TRUE)
#define AS_NUMBER(value)   valueToNumber(value)
#define AS_OBJ(value) ((struct GcObj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define NEW_FALSE          ((Value)(u64)(QNAN | TAG_FALSE))
#define NEW_TRUE           ((Value)(u64)(QNAN | TAG_TRUE))
#define NEW_NIL            ((Value)(u64)(QNAN | TAG_NIL))
#define NEW_BOOL(boolean)  (boolean ? NEW_TRUE : NEW_FALSE)
#define NEW_NUMBER(number) numberToValue(number)
#define NEW_OBJ(obj) (Value)(SIGN_BIT | QNAN | (u64)(uintptr_t)(obj))

static inline f64 valueToNumber(Value value) {
  f64 number;
  memcpy(&number, &value, sizeof(Value));
  return number;
}

static inline Value numberToValue(f64 number) {
  Value value;
  memcpy(&value, &number, sizeof(f64));
  return value;
}

#else

typedef struct {
  enum ValueType type;
  union {
    bool boolean;
    f64 number;
    struct Obj* obj;
  } as;
} Value;

#define IS_BOOL(value)    ((value).type == VALTYPE_BOOL)
#define IS_NIL(value)     ((value).type == VALTYPE_NIL)
#define IS_NUMBER(value)  ((value).type == VALTYPE_NUMBER)
#define IS_OBJ(value)     ((value).type == VALTYPE_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

#define NEW_BOOL(value)   ((struct Value){VALTYPE_BOOL, {.boolean = value}})
#define NEW_NIL           ((struct Value){VALTYPE_NIL, {.number = 0}})
#define NEW_NUMBER(value) ((struct Value){VALTYPE_NUMBER, {.number = value}})
#define NEW_OBJ(value)    ((struct Value){VALTYPE_OBJ, {.obj = (struct Obj*)value}})

#endif

struct ValueArray {
  s32 capacity;
  s32 count;
  Value* values;
};

struct TableEntry {
  struct GcString* key;
  Value value;
};

struct Table {
  s32 count;
  s32 capacity;
  struct TableEntry* entries;
};

struct GcObj {
  enum ObjType type;
  bool isMarked;
  struct GcObj* next;
};

struct GcUpvalue {
  struct GcObj obj;
  Value* location;
  Value closed;
  struct GcUpvalue* next;
};

struct GcCFunction {
  struct GcObj obj;
  hs_CFunction cFunc;
  s32 arity;
};

struct GcClosure {
  struct GcObj obj;
  struct GcBcFunction* function;
  struct GcUpvalue** upvalues;
  u8 upvalueCount;
};

struct GcBcFunction {
  struct GcObj obj;
  u8 arity;
  u8 upvalueCount;

  s32 bcCount;
  s32 bcCapacity;
  u8* bc;
  s32* lines;

  struct ValueArray constants;
  struct GcString* name;
};

struct GcString {
  struct GcObj obj;
  s32 length;
  char* chars;
  u32 hash;
};

struct GcStruct {
  struct GcObj obj;
  struct GcString* name;
  struct Table defaultFields;
  struct Table methods;
  struct Table staticMethods;
};

struct GcInstance {
  struct GcObj obj;
  struct GcStruct* strooct;
  struct Table fields;
};

struct GcBoundMethod {
  struct GcObj obj;
  Value receiver;
  struct GcClosure* method;
};

struct GcEnum {
  struct GcObj obj;
  struct GcString* name;
  struct Table values;
};

struct GcArray {
  struct GcObj obj;
  struct ValueArray values;
};

void initValueArray(struct ValueArray* array);
void copyValueArray(struct hs_State* H, struct ValueArray* dest, struct ValueArray* src);
void writeValueArray(struct hs_State* H, struct ValueArray* array, Value value);
void freeValueArray(struct hs_State* H, struct ValueArray* array);
void reserveValueArray(struct hs_State* H, struct ValueArray* array, s32 size);
void printValue(struct hs_State* H, Value value);
bool valuesEqual(Value a, Value b);

struct GcArray* newArray(struct hs_State* H);
struct GcEnum* newEnum(struct hs_State* H, struct GcString* name);
struct GcString* copyString(struct hs_State* H, const char* chars, int length);
struct GcString* takeString(struct hs_State* H, char* chars, int length);
struct GcStruct* newStruct(struct hs_State* H, struct GcString* name);
struct GcInstance* newInstance(struct hs_State* H, struct GcStruct* strooct);

struct GcClosure* newClosure(struct hs_State* H, struct GcBcFunction* function);
struct GcUpvalue* newUpvalue(struct hs_State* H, Value* slot);
struct GcBcFunction* newBcFunction(struct hs_State* H);
struct GcCFunction* newCFunction(struct hs_State* H, hs_CFunction cFunc, int argCount);
struct GcBoundMethod* newBoundMethod(
    struct hs_State* H, Value receiver, struct GcClosure* method);
void writeBytecode(struct hs_State* H, struct GcBcFunction* function, u8 byte, s32 line);
s32 addFunctionConstant(
    struct hs_State* H, struct GcBcFunction* function, Value value);

static inline bool isObjOfType(Value value, enum ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // _HOBBYSCRIPT_OBJECT_H
