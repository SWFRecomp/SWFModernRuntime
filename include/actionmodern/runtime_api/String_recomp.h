#pragma once

#include <recomp.h>

typedef struct
{
	ActionVar str;
} StringData;

void String_init(SWFAppContext* app_context, ASObject* this, u32 num_args);

void String_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void String_toString(SWFAppContext* app_context, ASObject* this, u32 num_args);
void String_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args);