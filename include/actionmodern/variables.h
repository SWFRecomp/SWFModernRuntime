#pragma once

#include <common.h>
#include <swf.h>
#include <stackvalue.h>

typedef enum
{
	FUNC_TYPE_1,
	FUNC_TYPE_2,
	FUNC_TYPE_3,
} FunctionType;

typedef struct
{
	ActionStackValueType type;
	
	union
	{
		// function
		struct
		{
			FunctionType func_type;
			
			action_func func;
			void* args;
			u8 reg_count;
			u16 flags;
			u32 func_name_string_id;
		};
		
		// string
		struct
		{
			u32 str_size;
			u32 string_id;
			bool owns_memory;
		};
	};
	
	// value
	union
	{
		u64 value;
		u64 u64;
		s64 s64;
		u32 u32;
		s32 s32;
		char* str;
		f32 f32;
		f64 f64;
		bool b;
		void* object;
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