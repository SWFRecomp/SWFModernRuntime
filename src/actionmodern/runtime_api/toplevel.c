#include <toplevel.h>

#include <objects.h>
#include <flashbang.h>

#include <initial_strings_decls.h>

void ASSetPropFlags(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	RETURN_VOID();
}

void recompGetLastKey(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	u8 key = FBC->last_key_pressed;
	
	PUSH_INT(key);
}

void recompSetDisplayScale(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar scale_v;
	popVar(app_context, &scale_v);
	
	DISCARD_ARGS(num_args - 1);
	
	convertNumericToInteger(app_context, &scale_v);
	
	flashbang_set_display_scale(FBC, scale_v.s32);
	
	releaseObjectVar(app_context, &scale_v);
	
	RETURN_VOID();
}