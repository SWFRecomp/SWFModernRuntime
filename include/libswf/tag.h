#pragma once

#include <common.h>
#include <swf.h>

typedef struct
{
	u32 offset;
	u32 count;
	u32* tris;
	
	bool free_after;
} VertexTask;

typedef struct
{
	u32 offset;
} UninvTask;

typedef struct
{
	u32 offset;
	
	f32 x;
	f32 y;
	
	f32 rotation;
	
	f32 xscale;
	f32 yscale;
} MultTask;

typedef struct
{
	bool has_extra_transform_id;
	u32 extra_transform_id;
	
	bool has_extra_cxform_id;
	u32 extra_cxform_id;
	
	bool has_extra_transform;
	
	f32 x;
	f32 y;
	
	f32 rotation;
	
	f32 xscale;
	f32 yscale;
	
	u32 offset;
	u32 count;
	u32 transform_id;
} DrawTask;

// Core tag functions - always available
void tagInit(SWFAppContext* app_context);
void tagSetBackgroundColor(SWFAppContext* app_context, u8 red, u8 green, u8 blue);
void tagShowFrame(SWFAppContext* app_context);

#ifndef NO_GRAPHICS
// Graphics-only tag functions
void tagDefineShape(SWFAppContext* app_context, CharacterType type, u32 char_id, u32 shape_offset, u32 shape_size);
void tagDefineText(SWFAppContext* app_context, u32 char_id, u32 text_start, u32 text_size, u32 transform_start, u32 cxform_id);
void tagPlaceObject2(SWFAppContext* app_context, u32 depth, u32 char_id, u32 transform_id);
void defineBitmap(SWFAppContext* app_context, u32 offset, u32 size, u32 width, u32 height);
void finalizeBitmaps(SWFAppContext* app_context);
#endif