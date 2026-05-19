#ifndef NO_GRAPHICS

#include <swf.h>
#include <tag.h>
#include <MovieClip.h>
#include <BitmapData.h>
#include <flashbang.h>
#include <utils.h>

void tagSetBackgroundColor(SWFAppContext* app_context, u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(app_context->fbc, red, green, blue);
}

u32 last_ms = 0;

typedef struct
{
	u32 offset;
	u32 vertex_count;
	u32* tris;
} VertexTask;

//~ typedef struct
//~ {
	
//~ } UninvTask;

float temp_mat_data[16] =
{
	1.000000000000000f,
	0.000000000000000f,
	0.0f,
	0.0f,
	0.000000000000000f,
	1.000000000000000f,
	0.0f,
	0.0f,
	0.0f,
	0.0f,
	1.0f,
	0.0f,
	0.000000000000000f,
	0.000000000000000f,
	0.0f,
	1.0f,
};

void tagShowFrame(SWFAppContext* app_context)
{
	//~ SwapVector vertex_tasks;
	//~ SwapVector uninv_tasks;
	//~ SwapVector draw_tasks;
	//~ SVEC_SIZED_INIT(&vertex_tasks, sizeof(VertexTask));
	//~ SVEC_SIZED_INIT(&uninv_tasks);
	//~ SVEC_SIZED_INIT(&draw_tasks);
	
	flashbang_open_pass(app_context->fbc, app_context);
	
	for (size_t i = 1; i <= app_context->max_depth; ++i)
	{
		ASObject* disp_obj = MC_EXTDATA_OF(app_context->_root, children)[i];
		
		u32 char_id = MC_EXTDATA_OF(disp_obj, char_id);
		
		if (char_id != 0)
		{
			u32 transform_id = MC_EXTDATA_OF(disp_obj, transform_id);
			
			if (char_id == 0)
			{
				continue;
			}
			
			Character* ch = &dictionary[char_id];
			
			switch (ch->type)
			{
				case CHAR_TYPE_SHAPE:
					flashbang_draw_shape(app_context->fbc, ch->shape_offset, ch->size, transform_id);
					break;
				case CHAR_TYPE_TEXT:
					flashbang_upload_extra_transform_id(app_context->fbc, transform_id);
					flashbang_upload_cxform_id(app_context->fbc, ch->cxform_id);
					for (u32 j = 0; j < ch->text_size; ++j)
					{
						u32 glyph_index = 2*app_context->text_data[ch->text_start + j];
						flashbang_draw_shape(app_context->fbc, app_context->glyph_data[glyph_index], app_context->glyph_data[glyph_index + 1], ch->transform_start + j);
					}
					break;
			}
		}
		
		else
		{
			if (MC_EXTDATA_OF(disp_obj, has_tris))
			{
				u32 vertex_count = 3*MC_EXTDATA_OF(disp_obj, tri_count);
				
				flashbang_open_vertex_transfer(app_context->fbc, vertex_count, 1);
				
				u32 vertex_offset = flashbang_allocate_vertices(app_context->fbc, vertex_count);
				flashbang_upload_vertices(app_context->fbc, MC_EXTDATA_OF(disp_obj, tris), vertex_offset, vertex_count);
				
				flashbang_close_vertex_transfer(app_context->fbc);
				
				flashbang_draw_shape(app_context->fbc, 0, vertex_count, 0);
			}
			
			u32 bitmap_at = MC_EXTDATA_OF(disp_obj, bitmap_at);
			
			if (bitmap_at != 0)
			{
				ASObject* bitmap = MC_EXTDATA_OF(disp_obj, children)[bitmap_at];
				
				u32 tris[6*4];
				
				f32 x = (float) (20.0f*MC_EXTDATA_OF(disp_obj, _x));
				f32 y = (float) (20.0f*MC_EXTDATA_OF(disp_obj, _y));
				
				f32 xscale = (float) (MC_EXTDATA_OF(disp_obj, _xscale)/100.0f);
				f32 yscale = (float) (MC_EXTDATA_OF(disp_obj, _yscale)/100.0f);
				
				VAL(float, &tris[0]) = 0.0f;
				VAL(float, &tris[1]) = (float) (20*BM_EXTDATA_OF(bitmap, height));
				tris[2] = 0x41;
				tris[3] = 0x0;
				
				VAL(float, &tris[4]) = (float) (20*BM_EXTDATA_OF(bitmap, width));
				VAL(float, &tris[5]) = 0.0f;
				tris[6] = 0x41;
				tris[7] = 0x0;
				
				VAL(float, &tris[8]) = 0.0f;
				VAL(float, &tris[9]) = 0.0f;
				tris[10] = 0x41;
				tris[11] = 0x0;
				
				VAL(float, &tris[12]) = 0.0f;
				VAL(float, &tris[13]) = (float) (20*BM_EXTDATA_OF(bitmap, height));
				tris[14] = 0x41;
				tris[15] = 0x0;
				
				VAL(float, &tris[16]) = (float) (20*BM_EXTDATA_OF(bitmap, width));
				VAL(float, &tris[17]) = 0.0f;
				tris[18] = 0x41;
				tris[19] = 0x0;
				
				VAL(float, &tris[20]) = (float) (20*BM_EXTDATA_OF(bitmap, width));
				VAL(float, &tris[21]) = (float) (20*BM_EXTDATA_OF(bitmap, height));
				tris[22] = 0x41;
				tris[23] = 0x0;
				
				u32 vertex_count = 6;
				flashbang_open_vertex_transfer(app_context->fbc, vertex_count, 1);
				
				u32 vertex_offset = flashbang_allocate_vertices(app_context->fbc, vertex_count);
				flashbang_upload_vertices(app_context->fbc, tris, vertex_offset, vertex_count);
				
				temp_mat_data[0] = 20.0f;
				temp_mat_data[5] = 20.0f;
				
				temp_mat_data[12] = 0.0f;
				temp_mat_data[13] = 0.0f;
				
				u32 uninv_offset = flashbang_allocate_uninv(app_context->fbc);
				flashbang_upload_uninv(app_context->fbc, temp_mat_data, uninv_offset);
				
				flashbang_close_vertex_transfer(app_context->fbc);
				
				temp_mat_data[0] = xscale;
				temp_mat_data[5] = yscale;
				
				temp_mat_data[12] = x;
				temp_mat_data[13] = y;
				
				flashbang_upload_extra_transform(app_context->fbc, temp_mat_data);
				
				flashbang_draw_shape(app_context->fbc, 0, vertex_count, 0);
			}
		}
	}
	
	flashbang_close_pass(app_context->fbc, app_context);
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

void defineBitmap(SWFAppContext* app_context, u32 offset, u32 size, u32 width, u32 height)
{
	flashbang_upload_bitmap(app_context->fbc, offset, size, width, height);
}

void finalizeBitmaps(SWFAppContext* app_context)
{
	flashbang_finalize_bitmaps(app_context->fbc);
}

#endif // NO_GRAPHICS