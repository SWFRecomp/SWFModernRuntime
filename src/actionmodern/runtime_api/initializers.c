#include <math.h>

#include <recomp.h>

#include <initial_strings_decls.h>

void initNumber(SWFAppContext* app_context, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	ASObject* Number = getProperty(_global, STR_ID_NUMBER, NULL, 0)->value.object;
	
	ActionVar v;
	v.type = ACTION_STACK_VALUE_F64;
	
	v.f64 = NAN;
	setProperty(app_context, Number, STR_ID_NAN, NULL, 0, &v);
	
	v.f64 = INFINITY;
	setProperty(app_context, Number, STR_ID_POSITIVE_INFINITY, NULL, 0, &v);
	
	v.f64 = -INFINITY;
	setProperty(app_context, Number, STR_ID_NEGATIVE_INFINITY, NULL, 0, &v);
	
	v.u64 = 0x7FEFFFFFFFFFFFFF;
	setProperty(app_context, Number, STR_ID_MAX_VALUE, NULL, 0, &v);
	
	v.u64 = 0x0000000000000001;
	setProperty(app_context, Number, STR_ID_MIN_VALUE, NULL, 0, &v);
	
	RETURN_VOID();
}