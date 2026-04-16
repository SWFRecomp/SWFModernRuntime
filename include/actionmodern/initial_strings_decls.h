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
} StringIds;

typedef struct
{
	u32 object_string_id;
	u32 func_string_id;
	action_func func;
	u32* args;
	bool constructor;
} RuntimeFunc;