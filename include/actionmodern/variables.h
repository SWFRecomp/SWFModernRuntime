#pragma once

#include <swf.h>
#include <stackvalue.h>

#define GETVAR(name, name_size) getVariable(app_context, name, name_size)

typedef struct
{
	ActionStackValueType type;
	u32 str_size;
	u64 value;
} ActionVar;

void initMap();
void freeMap();

ActionVar* getVariable(SWFAppContext* app_context, char* var_name, size_t key_size);