#include <swf.h>
#include <tag.h>
#include <flashbang.h>
#include <utils.h>

extern FlashbangContext* context;

size_t dictionary_capacity = INITIAL_DICTIONARY_CAPACITY;
size_t display_list_capacity = INITIAL_DISPLAYLIST_CAPACITY;

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(context, red, green, blue);
}

void tagShowFrame(SWFAppContext* app_context)
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
		
		switch (ch->type)
		{
			case CHAR_TYPE_SHAPE:
				flashbang_draw_shape(context, ch->shape_offset, ch->size, obj->transform_id);
				break;
			case CHAR_TYPE_TEXT:
				flashbang_upload_extra_transform_id(context, obj->transform_id);
				for (int i = 0; i < ch->text_size; ++i)
				{
					size_t glyph_index = 2*app_context->text_data[ch->text_start + i];
					flashbang_draw_shape(context, app_context->glyph_data[glyph_index], app_context->glyph_data[glyph_index + 1], ch->transform_start + i);
				}
				break;
		}
	}
	
	flashbang_close_pass(context);
}

void tagDefineShape(CharacterType type, size_t char_id, size_t shape_offset, size_t shape_size)
{
	ENSURE_SIZE(dictionary, char_id, dictionary_capacity, sizeof(Character));
	
	dictionary[char_id].type = type;
	dictionary[char_id].shape_offset = shape_offset;
	dictionary[char_id].size = shape_size;
}

void tagDefineText(size_t char_id, size_t text_start, size_t text_size, u32 transform_start)
{
	ENSURE_SIZE(dictionary, char_id, dictionary_capacity, sizeof(Character));
	
	dictionary[char_id].type = CHAR_TYPE_TEXT;
	dictionary[char_id].text_start = text_start;
	dictionary[char_id].text_size = text_size;
	dictionary[char_id].transform_start = transform_start;
}

void tagPlaceObject2(size_t depth, size_t char_id, u32 transform_id)
{
	ENSURE_SIZE(display_list, depth, display_list_capacity, sizeof(DisplayObject));
	
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