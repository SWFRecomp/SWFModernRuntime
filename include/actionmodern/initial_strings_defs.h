#pragma once

#include <initial_strings_decls.h>

#include <toplevel.h>
#include <Object.h>
#include <Math.h>

RuntimeFunc runtime_funcs[] =
{
	{0, STR_ID_OBJECT, new_Object, NULL, true},
	{0, STR_ID_ASSETPROPFLAGS, ASSetPropFlags, (u32*) &(u32[]){ STR_ID_LENGTH, STR_ID_MATH, STR_ID_ABS }, true},
};