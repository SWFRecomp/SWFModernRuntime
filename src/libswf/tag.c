#ifndef NO_GRAPHICS

#include <swf.h>
#include <tag.h>
#include <flashbang.h>
#include <utils.h>

extern FlashbangContext* context;

size_t dictionary_capacity = INITIAL_DICTIONARY_CAPACITY;
size_t display_list_capacity = INITIAL_DISPLAYLIST_CAPACITY;

void tagInit()
{
	// Graphics initialization happens in flashbang_init
	// This is called after flashbang is set up
}

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(context, red, green, blue);
}

void tagShowFrame()
{
	flashbang_open_pass(context);
	
	for (size_t i = 1; i <= max_depth; ++i)
	{
		DisplayObject* obj = &display_list[i];
		
		if (obj->char_id == 0)
		{
			continue;
		}
		
		Character* ch = &dictionary[obj->char_id];
		flashbang_draw_shape(context, ch->shape_offset, ch->size, obj->transform_id);
	}
	
	flashbang_close_pass(context);
}

void tagDefineShape(size_t char_id, size_t shape_offset, size_t shape_size)
{
	if (char_id >= dictionary_capacity)
	{
		grow_ptr((char**) &dictionary, &dictionary_capacity, sizeof(Character));
	}
	
	dictionary[char_id].shape_offset = shape_offset;
	dictionary[char_id].size = shape_size;
}

void tagPlaceObject2(size_t depth, size_t char_id, u32 transform_id)
{
	if (depth >= display_list_capacity)
	{
		grow_ptr((char**) &display_list, &display_list_capacity, sizeof(DisplayObject));
	}
	
	display_list[depth].char_id = char_id;
	display_list[depth].transform_id = transform_id;
	
	if (depth > max_depth)
	{
		max_depth = depth;
	}
}

void defineBitmap(size_t offset, size_t size, u32 width, u32 height)
{
	flashbang_upload_bitmap(context, offset, size, width, height);
}

void finalizeBitmaps()
{
	flashbang_finalize_bitmaps(context);
}

#endif // NO_GRAPHICS