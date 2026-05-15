#include <string.h>
#include <math.h>

#include <String_recomp.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((StringData*) this->extra_data)->member)

void String_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	this->extra_data = HALLOC(sizeof(StringData));
	
	ActionVar* str = &EXTDATA(str);
	
	if (num_args > 0)
	{
		convertString(app_context);
		popVar(app_context, str);
		
		DISCARD_ARGS(num_args - 1);
		
		char* old_str = str->str;
		u32 len = str->str_size + 1;
		
		str->str = HALLOC(len);
		memcpy(str->str, old_str, len);
	}
	
	else
	{
		str->type = ACTION_STACK_VALUE_STRING;
		str->str = NULL;
		str->str_size = 0;
		str->string_id = 0;
		str->owns_memory = false;
	}
	
	RETURN_VOID();
}

void String_toString(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	PUSH_VAR(&EXTDATA(str));
}

void String_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	PUSH_VAR(&EXTDATA(str));
}