#pragma once

#include <recomp.h>

#define EXTDATA(member) (((NumberData*) this->extra_data)->member)

typedef struct
{
	ActionVar num;
} NumberData;

void initNumber(SWFAppContext* app_context, ASObject* this, u32 num_args);

void new_Number(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Number_toString(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Number_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args);