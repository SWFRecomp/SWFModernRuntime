#pragma once

#include <recomp.h>
#include <base.h>

#define FU_EXTDATA_OF(o, member) (((FunctionData*) o->extra_data)->member)

typedef struct
{
	BaseExtData base;
	
	action_func func;
	void* args;
	FunctionType func_type;
	u8 reg_count;
	u16 flags;
	u32 func_name_string_id;
} FunctionData;

void Function_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Function_init_object(SWFAppContext* app_context, ASObject* this);

void Function_set_members(SWFAppContext* app_context, ASObject* this, FunctionType func_type, action_func func, void* args, u8 reg_count, u16 flags, u32 func_name_string_id);

action_func Function_get_func(SWFAppContext* app_context, ASObject* this);
void* Function_get_args(SWFAppContext* app_context, ASObject* this);
FunctionType Function_get_func_type(SWFAppContext* app_context, ASObject* this);
u8 Function_get_reg_count(SWFAppContext* app_context, ASObject* this);
u16 Function_get_flags(SWFAppContext* app_context, ASObject* this);
u32 Function_get_func_name_string_id(SWFAppContext* app_context, ASObject* this);