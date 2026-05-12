#ifndef NO_GRAPHICS

#include <swf.h>
#include <tag.h>
#include <MovieClip.h>
#include <flashbang.h>
#include <utils.h>

extern FlashbangContext* context;

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(context, red, green, blue);
}

void tagShowFrame(SWFAppContext* app_context)
{
	flashbang_open_pass(context);
	
	for (size_t i = 1; i <= app_context->max_depth; ++i)
	{
		ASObject* disp_obj = MC_EXTDATA_OF(app_context->_root, children)[i];
		
		u32 char_id = MC_EXTDATA_OF(disp_obj, char_id);
		u32 transform_id = MC_EXTDATA_OF(disp_obj, transform_id);
		
		if (char_id == 0)
		{
			continue;
		}
		
		Character* ch = &dictionary[char_id];
		
		switch (ch->type)
		{
			case CHAR_TYPE_SHAPE:
				flashbang_draw_shape(context, ch->shape_offset, ch->size, transform_id);
				break;
			case CHAR_TYPE_TEXT:
				flashbang_upload_extra_transform_id(context, transform_id);
				flashbang_upload_cxform_id(context, ch->cxform_id);
				for (u32 j = 0; j < ch->text_size; ++j)
				{
					u32 glyph_index = 2*app_context->text_data[ch->text_start + j];
					flashbang_draw_shape(context, app_context->glyph_data[glyph_index], app_context->glyph_data[glyph_index + 1], ch->transform_start + j);
				}
				break;
		}
	}
	
	flashbang_close_pass(context);
}

void tagDefineShape(SWFAppContext* app_context, CharacterType type, u32 char_id, u32 shape_offset, u32 shape_size)
{
	ENSURE_SIZE(dictionary, char_id, app_context->dictionary_capacity, sizeof(Character));
	
	dictionary[char_id].type = type;
	dictionary[char_id].shape_offset = shape_offset;
	dictionary[char_id].size = shape_size;
}

void tagDefineText(SWFAppContext* app_context, u32 char_id, u32 text_start, u32 text_size, u32 transform_start, u32 cxform_id)
{
	ENSURE_SIZE(dictionary, char_id, app_context->dictionary_capacity, sizeof(Character));
	
	dictionary[char_id].type = CHAR_TYPE_TEXT;
	dictionary[char_id].text_start = text_start;
	dictionary[char_id].text_size = text_size;
	dictionary[char_id].transform_start = transform_start;
	dictionary[char_id].cxform_id = cxform_id;
}

void tagPlaceObject2(SWFAppContext* app_context, u32 depth, u32 char_id, u32 transform_id)
{
	MovieClip_placeObject2_internal(app_context, app_context->_root, depth, char_id, transform_id);
}

void defineBitmap(u32 offset, u32 size, u32 width, u32 height)
{
	flashbang_upload_bitmap(context, offset, size, width, height);
}

void finalizeBitmaps()
{
	flashbang_finalize_bitmaps(context);
}

#endif // NO_GRAPHICS