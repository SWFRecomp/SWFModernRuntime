#pragma once

#include <common.h>
#include <swf.h>
#include <stackvalue.h>

typedef struct
{
	ActionStackValueType type;
	u32 str_size;
	u32 string_id;
	union
	{
		u64 value;
		struct
		{
			char* heap_ptr;
			bool owns_memory;
		};
	};
} ActionVar;

void initMap();
void freeMap(SWFAppContext* app_context);

// Array-based variable storage for constant string IDs
extern ActionVar** var_array;
extern size_t var_array_size;

void initVarArray(SWFAppContext* app_context, size_t max_string_id);
ActionVar* getVariableById(SWFAppContext* app_context, u32 string_id);

ActionVar* getVariable(SWFAppContext* app_context, char* var_name, size_t key_size);
char* materializeStringList(SWFAppContext* app_context);
void setVariableWithValue(SWFAppContext* app_context, ActionVar* var);