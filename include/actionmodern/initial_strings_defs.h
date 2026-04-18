#pragma once

#include <initial_strings_decls.h>

#include <toplevel.h>
#include <Object.h>
#include <Number.h>

RuntimeFunc runtime_funcs[] =
{
	{0, STR_ID_OBJECT, new_Object, true},
	{0, STR_ID_NUMBER, new_Number, true},
	{0, STR_ID_ASSETPROPFLAGS, ASSetPropFlags, false},
};

RuntimeFunc runtime_meths[] =
{
	{STR_ID_OBJECT, STR_ID_TO_STRING, Object_toString, false},
	{STR_ID_OBJECT, STR_ID_VALUE_OF, Object_valueOf, false},
	{STR_ID_NUMBER, STR_ID_TO_STRING, Number_toString, false},
	{STR_ID_NUMBER, STR_ID_VALUE_OF, Number_valueOf, false},
};

action_runtime_func static_initializers[] =
{
	initNumber,
};