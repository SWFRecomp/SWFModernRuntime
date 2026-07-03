#include <math.h>

#include <Number.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((NumberData*) this->extra_data)->member)

void Number_init(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	ASObject* Number = getProperty(app_context, _GLOBAL, STR_ID_NUMBER, NULL, 0)->value.object;
	
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

void Number_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	this->extra_data = HALLOC(sizeof(NumberData));
	
	EXTDATA(base.type) = NATIVE_NUMBER;
	
	EXTDATA(num).type = ACTION_STACK_VALUE_F64;
	
	ActionVar num;
	
	if (num_args > 0)
	{
		convertDouble(app_context);
		popVar(app_context, &num);
		
		DISCARD_ARGS(num_args - 1);
		
		EXTDATA(num).f64 = num.f64;
	}
	
	else
	{
		EXTDATA(num).f64 = 0.0;
	}
	
	RETURN_VOID();
}

void Number_toString(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	toString(app_context, &EXTDATA(num));
}

void Number_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	PUSH_VAR(&EXTDATA(num));
}