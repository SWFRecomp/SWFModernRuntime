#pragma once

#include <common.h>

// Core tag functions - always available
void tagInit();
void tagSetBackgroundColor(u8 red, u8 green, u8 blue);
void tagShowFrame();

#ifndef NO_GRAPHICS
// Graphics-only tag functions
void tagDefineShape(size_t char_id, size_t shape_offset, size_t shape_size);
void tagPlaceObject2(size_t depth, size_t char_id, u32 transform_id);
void defineBitmap(size_t offset, size_t size, u32 width, u32 height);
void finalizeBitmaps();
#endif