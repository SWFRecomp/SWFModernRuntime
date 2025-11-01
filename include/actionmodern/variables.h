#pragma once

#include <stackvalue.h>
#include <stdbool.h>

typedef struct
{
	ActionStackValueType type;
	u32 str_size;
	union {
		u64 numeric_value;
		struct {
			char* heap_ptr;
			bool owns_memory;
		} string_data;
	} data;
} ActionVar;

void initMap();
void freeMap();

ActionVar* getVariable(char* var_name, size_t key_size);
char* materializeStringList(char* stack, u32 sp);
void setVariableWithValue(ActionVar* var, char* stack, u32 sp);