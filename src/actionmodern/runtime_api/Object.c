#include <Object.h>
#include <Function.h>
#include <MovieClip.h>

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
	getPropertyVar(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &constructor);
	
	u32 ctor_name_id = Function_get_func_name_string_id(app_context, constructor.object);
	u32 len = 8 + app_context->str_len_table[ctor_name_id] + 1;
	
	PUSH_STR_STACK(len);
	char* stack_str = (char*) &STACK_TOP_VALUE;
	
	snprintf(stack_str, len + 1, "[object %s]", app_context->str_table[ctor_name_id]);
}

void Object_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	PUSH_OBJ(this);
}

void Object_recompId(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	PUSH_INT(this->id);
}