#pragma once

#include <common.h>
#include <swf.h>

// Core tag functions - always available
void tagInit(app_context);
void tagSetBackgroundColor(u8 red, u8 green, u8 blue);
void tagShowFrame(SWFAppContext* app_context);

#ifndef NO_GRAPHICS
// Graphics-only tag functions
void tagDefineShape(SWFAppContext* app_context, CharacterType type, u32 char_id, u32 shape_offset, u32 shape_size);
void tagDefineText(SWFAppContext* app_context, u32 char_id, u32 text_start, u32 text_size, u32 transform_start, u32 cxform_id);
void tagPlaceObject2(SWFAppContext* app_context, u32 depth, u32 char_id, u32 transform_id);
void defineBitmap(u32 offset, u32 size, u32 width, u32 height);
void finalizeBitmaps();
#endif