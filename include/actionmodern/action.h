#pragma once

#include <swf.h>
#include <variables.h>
#include <stackvalue.h>

#define PUSH(t, v) \
	OLDSP = SP; \
	SP -= 4 + 4 + 8 + 8; \
	SP &= ~7; \
	STACK[SP] = t; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u64, &STACK[SP + 16]) = v; \

// Push string with ID (for constant strings from compiler)
#define PUSH_STR_ID(v, n, id) \
	OLDSP = SP; \
	SP -= 4 + 4 + 8 + 8; \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STRING; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n; \
	VAL(u32, &STACK[SP + 12]) = id; \
	VAL(char*, &STACK[SP + 16]) = v; \

// Push string without ID (for dynamic strings, ID = 0)
#define PUSH_STR(v, n) PUSH_STR_ID(v, n, 0)

#define PUSH_STR_LIST(n, size) \
	OLDSP = VAL(u32, &STACK[SP_SECOND_TOP + 4]); \
	SP -= (u32) (4 + 4 + 8 + size); \
	SP &= ~7; \
	STACK[SP] = ACTION_STACK_VALUE_STR_LIST; \
	VAL(u32, &STACK[SP + 4]) = OLDSP; \
	VAL(u32, &STACK[SP + 8]) = n; \

#define PUSH_VAR(p) pushVar(app_context, p);

#define POP() \
	SP = VAL(u32, &STACK[SP + 4]); \

#define POP_2() \
	POP(); \
	POP(); \

#define STACK_TOP_TYPE STACK[SP]
#define STACK_TOP_N VAL(u32, &STACK[SP + 8])
#define STACK_TOP_ID VAL(u32, &STACK[SP + 12])
#define STACK_TOP_VALUE VAL(u64, &STACK[SP + 16])

#define SP_SECOND_TOP VAL(u32, &STACK[SP + 4])
#define STACK_SECOND_TOP_TYPE STACK[SP_SECOND_TOP]
#define STACK_SECOND_TOP_N VAL(u32, &STACK[SP_SECOND_TOP + 8])
#define STACK_SECOND_TOP_ID VAL(u32, &STACK[SP_SECOND_TOP + 12])
#define STACK_SECOND_TOP_VALUE VAL(u64, &STACK[SP_SECOND_TOP + 16])

#define VAL(type, x) *((type*) x)

#define INITIAL_STACK_SIZE 8388608  // 8 MB
#define INITIAL_SP INITIAL_STACK_SIZE

extern ActionVar* temp_val;

void initTime();

void pushVar(SWFAppContext* app_context, ActionVar* p);

void actionAdd(SWFAppContext* app_context);
void actionSubtract(SWFAppContext* app_context);
void actionMultiply(SWFAppContext* app_context);
void actionDivide(SWFAppContext* app_context);
void actionEquals(SWFAppContext* app_context);
void actionLess(SWFAppContext* app_context);
void actionAnd(SWFAppContext* app_context);
void actionOr(SWFAppContext* app_context);
void actionNot(SWFAppContext* app_context);

void actionStringEquals(SWFAppContext* app_context, char* a_str, char* b_str);
void actionStringLength(SWFAppContext* app_context, char* v_str);
void actionStringAdd(SWFAppContext* app_context, char* a_str, char* b_str);

void actionGetVariable(SWFAppContext* app_context);
void actionSetVariable(SWFAppContext* app_context);

void actionTrace(SWFAppContext* app_context);
void actionGetTime(SWFAppContext* app_context);