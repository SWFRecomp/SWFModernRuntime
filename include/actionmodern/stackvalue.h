#pragma once

#include <common.h>

typedef enum
{
	ACTION_STACK_VALUE_STRING = 0,
	ACTION_STACK_VALUE_F32 = 1,
	ACTION_STACK_VALUE_UNKNOWN = 10,
	ACTION_STACK_VALUE_VARIABLE = 11
} ActionStackValueType;

typedef struct
{
	ActionStackValueType type;
	u64 value;
} ActionStackValue;

typedef ActionStackValue var;