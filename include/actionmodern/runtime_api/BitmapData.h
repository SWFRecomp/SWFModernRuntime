#pragma once

#include <recomp.h>

#define BM_EXTDATA_OF(o, member) (((BitmapData*) o->extra_data)->member)

typedef struct
{
	u16 char_id;
	ASObject* _parent;
	size_t parent_depth;
	
	u16 bitmap_id;
	
	u32 width;
	u32 height;
} BitmapData;

void BitmapData_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void BitmapData_loadBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args);

bool BitmapData_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v);