#pragma once

#include <recomp.h>

#define AR_EXTDATA_OF(o, member) (((ArrayData*) o->extra_data)->member)

typedef struct
{
	ActionVar* data;
	size_t length;
	size_t capacity;
	
	ActionVar undef;
} ArrayData;

void Array_init(SWFAppContext* app_context, ASObject* this, u32 num_args);

void Array_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Array_push(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Array_pop(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Array_toString(SWFAppContext* app_context, ASObject* this, u32 num_args);

ActionVar* Array_getElement(SWFAppContext* app_context, ASObject* this, s32 i);
void Array_setElement(SWFAppContext* app_context, ASObject* this, s32 i, ActionVar* v);
void Array_setLength(SWFAppContext* app_context, ASObject* this, size_t new_length);