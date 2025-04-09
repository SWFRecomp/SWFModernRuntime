#pragma once

#include <stackvalue.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	ActionStackValueType type;
	u32 str_size;
	u64 value;
} ActionVar;

ActionVar* getVariable(char* var_name);

#ifdef __cplusplus
}
#endif