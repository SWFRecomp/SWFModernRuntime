#pragma once

#include <common.h>

typedef enum
{
	ACTION_STACK_VALUE_STRING = 0,
	ACTION_STACK_VALUE_F32 = 1
} ActionStackValueType;

typedef struct
{
	ActionStackValueType type;
	u64 value;
} ActionStackValue;