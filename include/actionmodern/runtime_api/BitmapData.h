#pragma once

#include <recomp.h>

#define BM_EXTDATA_OF(o, member) (((BitmapData*) o->extra_data)->member)

typedef struct
{
	u32 width;
	u32 height;
	
	u32 char_id;
} BitmapData;

void BitmapData_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void BitmapData_loadBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args);