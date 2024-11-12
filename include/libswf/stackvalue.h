#pragma once

enum ActionStackValueType
{
	ACTION_STACK_VALUE_STRING = 0,
	ACTION_STACK_VALUE_F32 = 1
};

struct ActionStackValue
{
	ActionStackValueType type;
	u64 value;
};