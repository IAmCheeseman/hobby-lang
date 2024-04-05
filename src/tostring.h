#ifndef _HOBBYSCRIPT_TOSTRING_H
#define _HOBBYSCRIPT_TOSTRING_H

#include "common.h"
#include "object.h"

struct GcString* toString(struct hs_State* H, Value* value);
struct GcString* numToString(struct hs_State* H, f64 num);
struct GcString* boolToString(struct hs_State* H, bool b);
struct GcString* strFormat(struct hs_State* H, const char* format, ...);

#endif // _HOBBYSCRIPT_TOSTRING_H
