#include <math.h>

#include <Function.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((FunctionData*) this->extra_data)->member)

void Function_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	UNREACHABLE("Function constructor");
}

void Function_init_object(SWFAppContext* app_context, ASObject* this)
{
	this->extra_data = HALLOC(sizeof(FunctionData));
	
	ActionVar ctor_v;
	ctor_v.type = ACTION_STACK_VALUE_OBJECT;
	ctor_v.object = app_context->Function_constructor;
	setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &ctor_v);
}

void Function_set_members(SWFAppContext* app_context, ASObject* this, FunctionType func_type, action_func func, void* args, u8 reg_count, u16 flags, u32 func_name_string_id)
{
	EXTDATA(func_type) = func_type;
	EXTDATA(func) = func;
	EXTDATA(args) = args;
	EXTDATA(reg_count) = reg_count;
	EXTDATA(flags) = flags;
	EXTDATA(func_name_string_id) = func_name_string_id;
}

action_func Function_get_func(SWFAppContext* app_context, ASObject* this)
{
	return EXTDATA(func);
}

void* Function_get_args(SWFAppContext* app_context, ASObject* this)
{
	return EXTDATA(args);
}

FunctionType Function_get_func_type(SWFAppContext* app_context, ASObject* this)
{
	return EXTDATA(func_type);
}

u8 Function_get_reg_count(SWFAppContext* app_context, ASObject* this)
{
	return EXTDATA(reg_count);
}

u16 Function_get_flags(SWFAppContext* app_context, ASObject* this)
{
	return EXTDATA(flags);
}

u32 Function_get_func_name_string_id(SWFAppContext* app_context, ASObject* this)
{
	return EXTDATA(func_name_string_id);
}