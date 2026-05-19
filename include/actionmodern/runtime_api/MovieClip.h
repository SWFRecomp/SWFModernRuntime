#pragma once

#include <recomp.h>

#define MC_EXTDATA_OF(o, member) (((MovieClipData*) o->extra_data)->member)

typedef struct
{
	ASObject** children;
	size_t display_list_capacity;
	
	u32 char_id;
	u32 transform_id;
	
	bool has_tris;
	u32 tri_count;
	u32* tris;
	
	u32 bitmap_at;
	
	bool _multiline;
	bool _visible;
} MovieClipData;

void MovieClip_new(SWFAppContext* app_context, ASObject* this, u32 num_args);

ASObject* MovieClip_create(SWFAppContext* app_context);
void MovieClip_placeObject2_internal(SWFAppContext* app_context, ASObject* this, u32 depth, u32 char_id, u32 transform_id);

void MovieClip_attachBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args);
void MovieClip_createTextField(SWFAppContext* app_context, ASObject* this, u32 num_args);
void MovieClip_createEmptyMovieClip(SWFAppContext* app_context, ASObject* this, u32 num_args);