#pragma once

#include <common.h>

void tagSetBackgroundColor(u8 red, u8 green, u8 blue);
void tagShowFrame();
void tagDefineShape(size_t char_id, char* tris, size_t tris_size);
void tagPlaceObject2(size_t depth, size_t char_id, float* transform);