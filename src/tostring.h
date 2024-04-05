#ifndef _HOBBYL_TOSTRING_H
#define _HOBBYL_TOSTRING_H

#include "common.h"
#include "object.h"

struct GcString* toString(struct hl_State* H, Value* value);
struct GcString* numToString(struct hl_State* H, f64 num);
struct GcString* boolToString(struct hl_State* H, bool b);
struct GcString* strFormat(struct hl_State* H, const char* format, ...);

#endif // _HOBBYL_TOSTRING_H
