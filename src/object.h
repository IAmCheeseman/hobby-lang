#ifndef _HOBBYL_OBJECT_H
#define _HOBBYL_OBJECT_H

#include <string.h>

#include "common.h"

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

#define AS_CLOSURE(value)      ((struct Closure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((struct Function*)AS_OBJ(value))
#define AS_CFUNCTION(value)    (((struct CFunctionBinding*)AS_OBJ(value))->cFunc)
#define AS_BOUND_METHOD(value) ((struct BoundMethod*)AS_OBJ(value))
#define AS_STRING(value)       ((struct String*)AS_OBJ(value))
#define AS_CSTRING(value)      (AS_STRING(value)->chars)
#define AS_STRUCT(value)       ((struct Struct*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((struct Instance*)AS_OBJ(value))
#define AS_ENUM(value)         ((struct Enum*)AS_OBJ(value))
#define AS_ARRAY(value)        ((struct Array*)AS_OBJ(value))

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
#define AS_OBJ(value) ((struct Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

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

enum ValueType {
  VALTYPE_BOOL,
  VALTYPE_NIL,
  VALTYPE_NUMBER,
  VALTYPE_OBJ,
};

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

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * U8_COUNT)

struct Entry {
  struct String* key;
  Value value;
};

struct Table {
  s32 count;
  s32 capacity;
  struct Entry* entries;
};

struct CallFrame {
  struct Closure* closure;
  u8* ip;
  Value* slots;
};

struct State {
  struct CallFrame frames[FRAMES_MAX];
  s32 frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  struct Table globals;
  struct Table strings;
  struct Upvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGc;

  struct Obj* objects;

  s32 grayCount;
  s32 grayCapacity;
  struct Obj** grayStack;

  struct Parser* parser;
};

struct Obj {
  enum ObjType type;
  bool isMarked;
  struct Obj* next;
};

struct Function {
  struct Obj obj;
  u16 arity;
  s32 upvalueCount;

  s32 bcCount;
  s32 bcCapacity;
  u8* bc;
  s32* lines;

  struct ValueArray constants;
  struct String* name;
};

struct Closure {
  struct Obj obj;
  struct Function* function;
  struct Upvalue** upvalues;
  u8 upvalueCount;
};

struct Upvalue {
  struct Obj obj;
  Value* location;
  Value closed;
  struct Upvalue* next;
};

typedef Value (*CFunction)(struct State* H);

struct CFunctionBinding {
  struct Obj obj;
  CFunction cFunc;
};

struct String {
  struct Obj obj;
  s32 length;
  char* chars;
  u32 hash;
};

struct Struct {
  struct Obj obj;
  struct String* name;
  struct Table defaultFields;
  struct Table methods;
  struct Table staticMethods;
};

struct Instance {
  struct Obj obj;
  struct Struct* strooct;
  struct Table fields;
};

struct BoundMethod {
  struct Obj obj;
  Value receiver;
  struct Closure* method;
};

struct Enum {
  struct Obj obj;
  struct String* name;
  struct Table values;
};

struct Array {
  struct Obj obj;
  struct ValueArray values;
};

void initValueArray(struct ValueArray* array);
void copyValueArray(struct State* H, struct ValueArray* dest, struct ValueArray* src);
void writeValueArray(struct State* H, struct ValueArray* array, Value value);
void freeValueArray(struct State* H, struct ValueArray* array);
void reserveValueArray(struct State* H, struct ValueArray* array, s32 size);
void printValue(Value value);
bool valuesEqual(Value a, Value b);

struct Array* newArray(struct State* H);
struct Enum* newEnum(struct State* H, struct String* name);
struct String* copyString(struct State* H, const char* chars, int length);
struct String* takeString(struct State* H, char* chars, int length);
struct Struct* newStruct(struct State* H, struct String* name);
struct Instance* newInstance(struct State* H, struct Struct* strooct);

struct Closure* newClosure(struct State* H, struct Function* function);
struct Upvalue* newUpvalue(struct State* H, Value* slot);
struct Function* newFunction(struct State* H);
struct CFunctionBinding* newCFunctionBinding(struct State* H, CFunction cFunc);
struct BoundMethod* newBoundMethod(
    struct State* H, Value receiver, struct Closure* method);
void writeBytecode(struct State* H, struct Function* function, u8 byte, s32 line);
s32 addFunctionConstant(
    struct State* H, struct Function* function, Value value);

void printObject(Value value);

static inline bool isObjOfType(Value value, enum ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // _HOBBYL_OBJECT_H
