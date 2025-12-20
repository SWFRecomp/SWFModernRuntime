#pragma once

#include <stackvalue.h>
#include <stdbool.h>

typedef struct
{
	ActionStackValueType type;
	u32 str_size;
	u32 string_id;  // String ID for constant strings (0 for dynamic strings)
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

// Array-based variable storage for constant string IDs
extern ActionVar** var_array;
extern size_t var_array_size;

void initVarArray(size_t max_string_id);
ActionVar* getVariableById(u32 string_id);

ActionVar* getVariable(char* var_name, size_t key_size);
bool hasVariable(char* var_name, size_t key_size);
char* materializeStringList(char* stack, u32 sp);
void setVariableWithValue(ActionVar* var, char* stack, u32 sp);