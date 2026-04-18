#pragma once

#include <recomp.h>

void new_Object(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Object_toString(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Object_valueOf(SWFAppContext* app_context, ASObject* this, u32 num_args);