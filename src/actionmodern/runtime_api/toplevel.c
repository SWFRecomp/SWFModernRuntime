#include <math.h>

#include <objects.h>
#include <flashbang.h>
#include <toplevel.h>

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

void recompSin(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar value_v;
	popVar(app_context, &value_v);
	convertNumericToNumber(app_context, &value_v);
	
	DISCARD_ARGS(num_args - 1);
	
	f64 ret = sin(value_v.f64);
	
	releaseObjectVar(app_context, &value_v);
	
	PUSH_F64(ret);
}

void recompCos(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar value_v;
	popVar(app_context, &value_v);
	convertNumericToNumber(app_context, &value_v);
	
	DISCARD_ARGS(num_args - 1);
	
	f64 ret = cos(value_v.f64);
	
	releaseObjectVar(app_context, &value_v);
	
	PUSH_F64(ret);
}

void recompAtan2(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar numer_v;
	popVar(app_context, &numer_v);
	convertNumericToNumber(app_context, &numer_v);
	
	ActionVar denom_v;
	popVar(app_context, &denom_v);
	convertNumericToNumber(app_context, &denom_v);
	
	DISCARD_ARGS(num_args - 2);
	
	f64 ret = atan2(numer_v.f64, denom_v.f64);
	
	releaseObjectVar(app_context, &denom_v);
	releaseObjectVar(app_context, &numer_v);
	
	PUSH_F64(ret);
}

void recompSqrt(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar value_v;
	popVar(app_context, &value_v);
	convertNumericToNumber(app_context, &value_v);
	
	DISCARD_ARGS(num_args - 1);
	
	f64 ret = sqrt(value_v.f64);
	
	releaseObjectVar(app_context, &value_v);
	
	PUSH_F64(ret);
}