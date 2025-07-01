#pragma once

#include <stackvalue.h>

typedef struct
{
	ActionStackValueType type;
	u32 str_size;
	u64 value;
} ActionVar;

void initMap();

ActionVar* getVariable(char* var_name, size_t key_size);