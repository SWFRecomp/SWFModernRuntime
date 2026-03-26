#pragma once

#include <initial_strings_decls.h>

#include <Object.h>
#include <Math.h>

RuntimeFunc runtime_funcs[] =
{
	{0, STR_ID_OBJECT, new_Object, NULL},
	{STR_ID_MATH, STR_ID_ABS, Math_abs, (u32*) &(u32[]){ STR_ID_X }},
};