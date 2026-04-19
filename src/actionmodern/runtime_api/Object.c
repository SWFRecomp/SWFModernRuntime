#include <Object.h>

#include <objects.h>

#include <initial_strings_decls.h>

void Object_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	RETURN_VOID();
}

void Object_toString(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	ActionVar constructor;
	getPropertyVar(this, STR_ID_CONSTRUCTOR, NULL, 0, &constructor);
	
	u32 len = constructor.str_size + 9;
	
	PUSH_STR_STACK(len);
	char* stack_str = (char*) &STACK_TOP_VALUE;
	
	snprintf(stack_str, len + 1, "[object %s]", constructor.str);
}

void Object_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	PUSH_OBJ(this);
}