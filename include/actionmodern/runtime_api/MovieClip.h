#pragma once

#include <recomp.h>

#define MC_EXTDATA_OF(o, member) (((MovieClipData*) o->extra_data)->member)

typedef struct
{
	ASObject** children;
	size_t display_list_capacity;
	
	u32 char_id;
	u32 transform_id;
	
	bool _multiline;
	bool _visible;
} MovieClipData;

ASObject* MovieClip_create(SWFAppContext* app_context);
void MovieClip_placeObject2_internal(SWFAppContext* app_context, ASObject* this, u32 depth, u32 char_id, u32 transform_id);