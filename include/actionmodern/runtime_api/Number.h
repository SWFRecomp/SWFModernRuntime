#pragma once

#include <recomp.h>

typedef struct
{
	ActionVar num;
} NumberData;

void Number_init(SWFAppContext* app_context, ASObject* this, u32 num_args);

void Number_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Number_toString(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Number_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args);