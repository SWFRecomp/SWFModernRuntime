#pragma once

#include <recomp.h>

void ASSetPropFlags(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompGetLastKey(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompSetDisplayScale(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompSin(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompCos(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompAtan2(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompSqrt(SWFAppContext* app_context, ASObject* this, u32 num_args);