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

#define PUSH_STR(v, n) \
	oldSP = *sp; \
	*sp -= 4 + 4 + 8 + 8; \
	*sp &= ~7; \
	stack[*sp] = ACTION_STACK_VALUE_STRING; \
	VAL(u32, &stack[*sp + 4]) = oldSP; \
	VAL(u32, &stack[*sp + 8]) = n; \
	VAL(char*, &stack[*sp + 16]) = v; \

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

#define SET_VAR(p, t, n, v) \
	p->type = t; \
	p->str_size = n; \
	p->value = v; \

#define VAL(type, x) *((type*) x)

#define INITIAL_STACK_SIZE 8388608  // 8 MB
#define INITIAL_SP INITIAL_STACK_SIZE

extern ActionVar* temp_val;

void pushVar(char* stack, u32* sp, ActionVar* p);

void actionAdd(char* stack, u32* sp);
//~ void actionSubtract(u64 a, u64 b);
//~ void actionMultiply(u64 a, u64 b);
void actionDivide(char* stack, u32* sp);
void actionEquals(char* stack, u32* sp);
//~ void actionLess(u64 a, u64 b);
//~ void actionAnd(u64 a, u64 b);
//~ void actionOr(u64 a, u64 b);
//~ void actionNot(u64 a);

void actionStringEquals(char* stack, u32* sp, char* a_str, char* b_str);
void actionStringLength(char* stack, u32* sp, char* v_str);
void actionStringAdd(char* stack, u32* sp, char* a_str, char* b_str);

void actionTrace(char* stack, u32* sp);