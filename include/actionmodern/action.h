#pragma once

#include <stackvalue.h>

#define VAL(type, x) *((type*) x)

void actionTrace(const char* str);
void actionTraceVar(var val);