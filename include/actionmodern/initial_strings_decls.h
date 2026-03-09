#pragma once

#include <Object.h>

typedef enum
{
	STR_ID_EMPTY = 1,
	STR_ID_GLOBAL,
	STR_ID_RECOMP,
	STR_ID_ARG1,
	STR_ID_ARG2,
	STR_ID_ARG3,
	STR_ID_ARG4,
	STR_ID_ARG5,
	STR_ID_ARG6,
	STR_ID_OBJECT,
	STR_ID_THIS,
	STR_ID_LENGTH,
	STR_ID_MATH,
	STR_ID_ABS,
	STR_ID_X,
} StringIds;

typedef struct
{
	u32 object_string_id;
	u32 func_string_id;
	action_func func;
	u32* args;
} RuntimeFunc;