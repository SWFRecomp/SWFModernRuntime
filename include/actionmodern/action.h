#pragma once

#include <stackvalue.h>

#define VAL(type, x) *((type*) x)

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

void actionTrace(var* val);