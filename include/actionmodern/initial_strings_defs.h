#pragma once

#include <initial_strings_decls.h>

#include <toplevel.h>
#include <Object.h>
#include <Function.h>
#include <Number.h>
#include <String_recomp.h>
#include <Array.h>
#include <MovieClip.h>
#include <BitmapData.h>

RuntimeFunc runtime_funcs[] =
{
	{0, STR_ID_OBJECT, Object_new, true},
	{0, STR_ID_FUNCTION, Function_new, true},
	{0, STR_ID_NUMBER, Number_new, true},
	{0, STR_ID_STRING, String_new, true},
	{0, STR_ID_ARRAY, Array_new, true},
	{0, STR_ID_MOVIECLIP, MovieClip_new, true},
	{0, STR_ID_ASSETPROPFLAGS, ASSetPropFlags, false},
};

RuntimeFunc runtime_meths[] =
{
	{STR_ID_OBJECT, STR_ID_TO_STRING, Object_toString, false},
	{STR_ID_OBJECT, STR_ID_VALUE_OF, Object_valueOf, false},
	{STR_ID_OBJECT, STR_ID_RECOMP_ID, Object_recompId, false},
	{STR_ID_NUMBER, STR_ID_TO_STRING, Number_toString, false},
	{STR_ID_NUMBER, STR_ID_VALUE_OF, Number_valueOf, false},
	{STR_ID_STRING, STR_ID_TO_STRING, String_toString, false},
	{STR_ID_STRING, STR_ID_VALUE_OF, String_valueOf, false},
	{STR_ID_ARRAY, STR_ID_PUSH, Array_push, false},
	{STR_ID_ARRAY, STR_ID_POP, Array_pop, false},
	{STR_ID_MOVIECLIP, STR_ID_CREATE_EMPTY_MOVIECLIP, MovieClip_createEmptyMovieClip, false},
};

action_runtime_func static_initializers[] =
{
	Number_init,
};