#include "debug.h"

#include <stdio.h>

#include "opcodes.h"
#include "object.h"

void disassembleFunction(
    struct hs_State* H,
    struct GcBcFunction* function, void* functionPointer, const char* name) {
  printf("== %s (%p) ==\n", name, functionPointer);

  for (int offset = 0; offset < function->bcCount;) {
    offset = disassembleInstruction(H, function, offset);
  }
}

static s32 simpleInstruction(const char* name, s32 offset) {
  printf("%s\n", name);
  return offset + 1;
}

static s32 byteInstruction(const char* name, struct GcBcFunction* function, s32 offset) {
  u8 slot = function->bc[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; 
}

static int jumpInstruction(
    const char* name, s32 sign, struct GcBcFunction* function, s32 offset) {
  u16 jump = (u16)(function->bc[offset + 1] << 8);
  jump |= function->bc[offset + 2];
  printf("%-16s %4d -> %4d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

static s32 constantInstruction(
    struct hs_State* H, const char* name, struct GcBcFunction* function, s32 offset) {
  u8 constant = function->bc[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(H, function->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static s32 invokeInstruction(
    struct hs_State* H, const char* name, struct GcBcFunction* function, s32 offset) {
  u8 constant = function->bc[offset + 1];
  u8 argCount = function->bc[offset + 2];
  printf("%-16s (%d args) %4d '", name, argCount, constant);
  printValue(H, function->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

s32 disassembleInstruction(struct hs_State* H, struct GcBcFunction* function, s32 offset) {
  printf("%04d ", offset);
  if (offset > 0 && function->lines[offset] == function->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", function->lines[offset]);
  }

  u8 instruction = function->bc[offset];
  switch (instruction) {
    case BC_CONSTANT:
      return constantInstruction(H, "OP_CONSTANT", function, offset);
    case BC_NIL:
      return simpleInstruction("OP_NIL", offset);
    case BC_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case BC_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case BC_POP:
      return simpleInstruction("OP_POP", offset);
    case BC_ARRAY:
      return byteInstruction("OP_ARRAY", function, offset);
    case BC_GET_SUBSCRIPT:
      return simpleInstruction("OP_GET_SUBSCRIPT", offset);
    case BC_SET_SUBSCRIPT:
      return simpleInstruction("OP_SET_SUBSCRIPT", offset);
    case BC_DEFINE_GLOBAL:
      return constantInstruction(H, "OP_DEFINE_GLOBAL", function, offset);
    case BC_GET_GLOBAL:
      return constantInstruction(H, "OP_GET_GLOBAL", function, offset);
    case BC_SET_GLOBAL:
      return constantInstruction(H, "OP_SET_GLOBAL", function, offset);
    case BC_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", function, offset);
    case BC_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", function, offset);
    case BC_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", function, offset);
    case BC_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", function, offset);
    case BC_INIT_PROPERTY:
      return byteInstruction("OP_INIT_PROPERTY", function, offset);
    case BC_GET_STATIC:
      return constantInstruction(H, "OP_GET_STATIC_METHOD", function, offset);
    case BC_PUSH_PROPERTY:
      return constantInstruction(H, "OP_PUSH_PROPERTY", function, offset);
    case BC_GET_PROPERTY:
      return constantInstruction(H, "OP_GET_PROPERTY", function, offset);
    case BC_SET_PROPERTY:
      return constantInstruction(H, "OP_SET_PROPERTY", function, offset);
    case BC_DESTRUCT_ARRAY:
      return byteInstruction("OP_DESTRUCT_ARRAY", function, offset);
    case BC_STRUCT_FIELD:
      return simpleInstruction("OP_SET_STRUCT_FIELD", offset);
    case BC_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case BC_NOT_EQUAL:
      return simpleInstruction("OP_NOT_EQUAL", offset);
    case BC_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case BC_GREATER_EQUAL:
      return simpleInstruction("OP_GREATER_EQUAL", offset);
    case BC_LESSER:
      return simpleInstruction("OP_LESSER", offset);
    case BC_LESSER_EQUAL:
      return simpleInstruction("OP_LESSER_EQUAL", offset);
    case BC_ADD:
      return simpleInstruction("OP_ADD", offset);
    case BC_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case BC_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case BC_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case BC_MODULO:
      return simpleInstruction("OP_MODULO", offset);
    case BC_POW:
      return simpleInstruction("OP_POW", offset);
    case BC_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case BC_NOT:
      return simpleInstruction("OP_NOT", offset);
    case BC_JUMP:
      return jumpInstruction("OP_JUMP", 1, function, offset);
    case BC_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, function, offset);
    case BC_INEQUALITY_JUMP:
      return jumpInstruction("OP_INEQUALITY_JUMP", 1, function, offset);
    case BC_LOOP:
      return jumpInstruction("OP_LOOP", -1, function, offset);
    case BC_CALL:
      return byteInstruction("OP_CALL", function, offset);
    case BC_INSTANCE:
      return simpleInstruction("OP_INSTANCE", offset);
    case BC_CLOSURE: {
      offset++;
      u8 constant = function->bc[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printValue(H, function->constants.values[constant]);
      printf("\n");
      struct GcBcFunction* inner = AS_FUNCTION(function->constants.values[constant]);

      for (int j = 0; j < inner->upvalueCount; j++) {
        int isLocal = function->bc[offset++];
        int index = function->bc[offset++];
        printf("%04d      |                     %s %d\n",
            offset - 2, isLocal ? "local" : "upvalue", index);
      }

      return offset;
    }
    case BC_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case BC_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    case BC_ENUM:
      return constantInstruction(H, "OP_ENUM", function, offset);
    case BC_ENUM_VALUE: {
      u8 constant = function->bc[offset + 1];
      u8 slot = function->bc[offset + 2];
      printf("%-16s %4d %4d '", "OP_ENUM_VALUE", slot, constant);
      printValue(H, function->constants.values[constant]);
      printf("'\n");
      return offset + 3; 
    }
    case BC_STRUCT:
      return constantInstruction(H, "OP_STRUCT", function, offset);
    case BC_METHOD:
      return constantInstruction(H, "OP_METHOD", function, offset);
    case BC_STATIC_METHOD:
      return constantInstruction(H, "OP_STATIC_METHOD", function, offset);
    case BC_INVOKE:
      return invokeInstruction(H, "OP_INVOKE", function, offset);
    case BC_BREAK:
      return simpleInstruction("OP_BREAK", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}
