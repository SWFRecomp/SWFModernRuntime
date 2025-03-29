#pragma once

#include <stackvalue.h>

#define PUSH(t, v) \
	stack[sp].type = t; \
	stack[sp].value = v; \
	sp -= 1; \

#define POP() \
	sp += 1; \

#define POP_2() \
	sp += 2; \

#define STACK_TOP stack[sp + 1]
#define STACK_SECOND_TOP stack[sp + 2]

#define SET_STACK_TOP(t, v) \
	stack[sp + 1].type = t; \
	stack[sp + 1].value = v; \

#define SET(n, t, v) \
	n->type = t; \
	n->value = v; \

#define VAL(type, x) *((type*) x)

#ifndef TEMP_VAL
#define TEMP_VAL
var* temp_val;
#endif

#define INITIAL_STACK_SIZE 256
#define INITIAL_SP 255

void actionAdd(var* a, var* b);
void actionSubtract(var* a, var* b);
void actionMultiply(var* a, var* b);
void actionDivide(var* a, var* b);
void actionEquals(var* a, var* b);
void actionLess(var* a, var* b);
void actionAnd(var* a, var* b);
void actionOr(var* a, var* b);
void actionNot(var* a);

void actionStringEquals(var* a, var* b, char* a_str, char* b_str);
void actionStringAdd(var* a, var* b, char* a_str, char* b_str, char* out_str);

void actionTrace(var* val);