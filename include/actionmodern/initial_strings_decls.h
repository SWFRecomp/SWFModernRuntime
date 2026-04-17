#pragma once

#include <common.h>
#include <swf.h>

typedef enum
{
	STR_ID_EMPTY = 1,
	STR_ID_GLOBAL,
	STR_ID_ROOT,
	STR_ID_PARENT,
	STR_ID_RECOMP,
	STR_ID_OBJECT,
	STR_ID_NUMBER,
	STR_ID_THIS,
	STR_ID_ARGUMENTS,
	STR_ID_SUPER,
	STR_ID_PROTOTYPE,
	STR_ID_PROTO,
	STR_ID_LENGTH,
	STR_ID_ASSETPROPFLAGS,
	STR_ID_MATH,
	STR_ID_ABS,
	STR_ID_X,
	STR_ID_Y,
	STR_ID_Z,
	STR_ID_NAN,
	STR_ID_POSITIVE_INFINITY,
	STR_ID_NEGATIVE_INFINITY,
	STR_ID_MAX_VALUE,
	STR_ID_MIN_VALUE,
} StringIds;

typedef struct
{
	u32 object_string_id;
	u32 func_string_id;
	action_runtime_func func;
	bool constructor;
} RuntimeFunc;