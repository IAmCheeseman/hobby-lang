#include "tostring.h"

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "state.h"

#define NUMBER_FORMAT "%.14g"

static struct GcString* functionToString(struct hl_State* H, struct GcBcFunction* func) {
  return strFormat(H, "<function @>", func->name);
}

struct GcString* toString(struct hl_State* H, Value* value) {
  if (value == NULL) {
    return copyString(H, "Null", 4);
  }

  if (IS_STRING(*value)) {
    return AS_STRING(*value);
  } else if (IS_NUMBER(*value)) {
    return numToString(H, AS_NUMBER(*value));
  } else if (IS_BOOL(*value)) {
    return boolToString(H, AS_BOOL(*value));
  } else if (IS_NIL(*value)) {
    return copyString(H, "nil", 3);
  } else if (IS_INSTANCE(*value)) {
    return strFormat(H, "<@ instance>", AS_INSTANCE(*value)->strooct->name);
  } else if (IS_STRUCT(*value)) {
    return strFormat(H, "<@>", AS_STRUCT(*value)->name);
  } else if (IS_ENUM(*value)) {
    return strFormat(H, "<@>", AS_ENUM(*value)->name);
  } else if (IS_FUNCTION(*value)) {
    return functionToString(H, AS_FUNCTION(*value));
  } else if (IS_CLOSURE(*value)) {
    return functionToString(H, AS_CLOSURE(*value)->function);
  } else if (IS_BOUND_METHOD(*value)) {
    return functionToString(H, AS_BOUND_METHOD(*value)->method->function);
  } else if (IS_UPVALUE(*value)) {
    return copyString(H, "<upvalue>", 9);
  } else if (IS_ARRAY(*value)) {
    return copyString(H, "<array>", 7);
  }

  return copyString(H, "unknown", 7);
}

struct GcString* numToString(struct hl_State* H, f64 num) {
  if (isnan(num)) {
    return copyString(H, "nan", 3);
  }
  
  if (isinf(num)) {
    if (num > 0) {
      return copyString(H, "inf", 3);
    } else {
      return copyString(H, "-inf", 4);
    }
  }

  s32 len = snprintf(NULL, 0, NUMBER_FORMAT, num);
  char* string = ALLOCATE(H, char, len + 1);
  snprintf(string, len + 1, NUMBER_FORMAT, num);
  return takeString(H, string, len);
}

struct GcString* boolToString(struct hl_State* H, bool b) {
  return b ? copyString(H, "true", 4) : copyString(H, "false", 5);
}

struct GcString* strFormat(struct hl_State* H, const char* format, ...) {
  va_list args;

  va_start(args, format);
  size_t total = 0;

  for (const char* c = format; *c != '\0'; c++) {
    switch (*c) {
      case '$': {
        total += strlen(va_arg(args, const char*));
        break;
      }
      case '@': {
        total += va_arg(args, struct GcString*)->length;
        break;
      }
      default: {
        total += 1;
        break;
      }
    }
  }
  va_end(args);

  char* chars = ALLOCATE(H, char, total + 1);
  chars[total] = '\0';

  va_start(args, format);
  char* start = chars;
  for (const char* c = format; *c != '\0'; c++) {
    switch (*c) {
      case '$': {
        const char* s = va_arg(args, const char*);
        size_t length = strlen(s);
        memcpy(start, s, length);
        start += length;
        break;
      }
      case '@': {
        struct GcString* s = va_arg(args, struct GcString*);
        memcpy(start, s->chars, s->length);
        start += s->length;
        break;
      }
      default: {
        *start++ = *c;
        break;
      }
    }
  }

  return takeString(H, chars, total);
}
