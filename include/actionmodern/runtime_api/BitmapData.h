#pragma once

#include <recomp.h>

typedef struct
{
	u32 char_id;
} BitmapData;

void BitmapData_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void BitmapData_loadBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args);