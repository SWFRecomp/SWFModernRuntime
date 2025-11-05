#ifdef NO_GRAPHICS

#include <tag.h>
#include <common.h>

// Stub implementations for console-only mode
// Note: tagInit() is provided by the generated tagMain.c file

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	printf("[Tag] SetBackgroundColor(%d, %d, %d)\n", red, green, blue);
}

void tagShowFrame()
{
	printf("[Tag] ShowFrame()\n");
}

// Stubs for graphics-only tags - should not be called in NO_GRAPHICS mode
// but if they are, we provide empty implementations
#ifdef INCLUDE_GRAPHICS_STUBS
void tagDefineShape(size_t char_id, size_t shape_offset, size_t shape_size)
{
	printf("[Tag] DefineShape(char_id=%zu) [ignored in NO_GRAPHICS mode]\n", char_id);
}

void tagPlaceObject2(size_t depth, size_t char_id, u32 transform_id)
{
	printf("[Tag] PlaceObject2(depth=%zu, char_id=%zu) [ignored in NO_GRAPHICS mode]\n", depth, char_id);
}

void defineBitmap(size_t offset, size_t size, u32 width, u32 height)
{
	printf("[Tag] DefineBitmap(width=%u, height=%u) [ignored in NO_GRAPHICS mode]\n", width, height);
}

void finalizeBitmaps()
{
	printf("[Tag] FinalizeBitmaps() [ignored in NO_GRAPHICS mode]\n");
}
#endif

#endif // NO_GRAPHICS
