#pragma once

#include <initial_strings_decls.h>

#include <toplevel.h>
#include <Object.h>

RuntimeFunc runtime_funcs[] =
{
	{0, STR_ID_OBJECT, new_Object, true},
	{0, STR_ID_ASSETPROPFLAGS, ASSetPropFlags, false},
};