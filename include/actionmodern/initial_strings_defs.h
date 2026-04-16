#pragma once

#include <initial_strings_decls.h>

#include <toplevel.h>
#include <Object.h>

RuntimeFunc runtime_funcs[] =
{
	{0, STR_ID_OBJECT, new_Object, NULL, true},
	{0, STR_ID_ASSETPROPFLAGS, ASSetPropFlags, (u32*) &(u32[]){ STR_ID_X, STR_ID_Y, STR_ID_Z }, false},
};