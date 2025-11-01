#pragma once

#include <variables.h>
#include <stackvalue.h>

#define PUSH(t, v) \
	oldSP = *sp; \
	*sp -= 4 + 4 + 8 + 8; \
	*sp &= ~7; \
	stack[*sp] = t; \
	VAL(u32, &stack[*sp + 4]) = oldSP; \
	VAL(u64, &stack[*sp + 16]) = v; \

// Push string with ID (for constant strings from compiler)
#define PUSH_STR_ID(v, n, id) \
	oldSP = *sp; \
	*sp -= 4 + 4 + 8 + 8; \
	*sp &= ~7; \
	stack[*sp] = ACTION_STACK_VALUE_STRING; \
	VAL(u32, &stack[*sp + 4]) = id; \
	VAL(u32, &stack[*sp + 8]) = n; \
	VAL(char*, &stack[*sp + 16]) = v;

// Push string without ID (for dynamic strings, ID = 0)
#define PUSH_STR(v, n) PUSH_STR_ID(v, n, 0)

#define PUSH_STR_LIST(n, size) \
	oldSP = VAL(u32, &stack[SP_SECOND_TOP + 4]); \
	*sp -= (u32) (4 + 4 + 8 + size); \
	*sp &= ~7; \
	stack[*sp] = ACTION_STACK_VALUE_STR_LIST; \
	VAL(u32, &stack[*sp + 4]) = oldSP; \
	VAL(u32, &stack[*sp + 8]) = n; \

#define PUSH_VAR(p) pushVar(stack, sp, p);

#define POP() \
	*sp = VAL(u32, &stack[*sp + 4]); \

#define POP_2() \
	POP(); \
	POP(); \

#define STACK_TOP_TYPE stack[*sp]
#define STACK_TOP_N VAL(u32, &stack[*sp + 8])
#define STACK_TOP_VALUE VAL(u64, &stack[*sp + 16])

#define SP_SECOND_TOP VAL(u32, &stack[*sp + 4])
#define STACK_SECOND_TOP_TYPE stack[SP_SECOND_TOP]
#define STACK_SECOND_TOP_N VAL(u32, &stack[SP_SECOND_TOP + 8])
#define STACK_SECOND_TOP_VALUE VAL(u64, &stack[SP_SECOND_TOP + 16])

#define SET_VAR(p, t, n, v) setVariableWithValue(p, stack, *sp)

#define VAL(type, x) *((type*) x)

#define INITIAL_STACK_SIZE 8388608  // 8 MB
#define INITIAL_SP INITIAL_STACK_SIZE

extern ActionVar* temp_val;

void initTime();

void pushVar(char* stack, u32* sp, ActionVar* p);

void actionAdd(char* stack, u32* sp);
void actionSubtract(char* stack, u32* sp);
void actionMultiply(char* stack, u32* sp);
void actionDivide(char* stack, u32* sp);
void actionEquals(char* stack, u32* sp);
void actionLess(char* stack, u32* sp);
void actionAnd(char* stack, u32* sp);
void actionOr(char* stack, u32* sp);
void actionNot(char* stack, u32* sp);

void actionStringEquals(char* stack, u32* sp, char* a_str, char* b_str);
void actionStringLength(char* stack, u32* sp, char* v_str);
void actionStringAdd(char* stack, u32* sp, char* a_str, char* b_str);

void actionGetVariable(char* stack, u32* sp);
void actionSetVariable(char* stack, u32* sp);

void actionTrace(char* stack, u32* sp);
void actionGetTime(char* stack, u32* sp);