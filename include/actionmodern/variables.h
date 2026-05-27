#pragma once

#include <common.h>
#include <swf.h>
#include <stackvalue.h>

typedef enum
{
	FUNC_TYPE_1 = 1,
	FUNC_TYPE_2,
	FUNC_TYPE_3,
} FunctionType;

typedef struct
{
	void* var_map;
	size_t next_str_id;
} VarCtx;

typedef struct
{
	ActionStackValueType type;
	
	union
	{
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
		ASObject* object;
	};
} ActionVar;

void initMap(SWFAppContext* app_context);
void freeMap(SWFAppContext* app_context);

u32 getStringId(SWFAppContext* app_context, char* str, size_t str_size);
char* materializeStringList(SWFAppContext* app_context);