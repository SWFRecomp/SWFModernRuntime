#ifndef NO_GRAPHICS

#define _USE_MATH_DEFINES
#include <math.h>

#include <swf.h>
#include <tag.h>
#include <MovieClip.h>
#include <BitmapData.h>
#include <flashbang.h>
#include <heap.h>
#include <utils.h>

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

void tagSetBackgroundColor(SWFAppContext* app_context, u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(app_context->fbc, red, green, blue);
}

void tagShowFrame(SWFAppContext* app_context)
{
	app_context->frame_vertex_count = 0;
	
	SwapVector* stack = &app_context->movieclip_stack;
	ASObject* root = app_context->_root;
	
	SVEC_PUSH(stack, root);
	
	while (stack->length > 0)
	{
		ASObject* disp_obj = (ASObject*) SVEC_TOP(stack);
		SVEC_POP(stack);
		
		if (disp_obj == NULL)
		{
			continue;
		}
		
		size_t max_depth = MC_EXTDATA_OF(disp_obj, max_depth);
		
		for (size_t i = max_depth; i >= 1; --i)
		{
			if (MC_EXTDATA_OF(disp_obj, bitmap_at) == i)
			{
				continue;
			}
			
			SVEC_PUSH(stack, MC_EXTDATA_OF(disp_obj, children)[i]);
		}
		
		u32 char_id = MC_EXTDATA_OF(disp_obj, char_id);
		
		if (char_id != 0)
		{
			u32 transform_id = MC_EXTDATA_OF(disp_obj, transform_id);
			
			Character* ch = &dictionary[char_id];
			
			switch (ch->type)
			{
				case CHAR_TYPE_SHAPE:
					SVEC_BUMP(&app_context->draw_tasks);
					
					DrawTask* dt = SVEC_GET_TOP(&app_context->draw_tasks, DrawTask);
					
					dt->has_extra_transform_id = false;
					dt->has_extra_cxform_id = false;
					dt->has_extra_transform = false;
					
					dt->offset = ch->shape_offset;
					dt->count = ch->size;
					dt->transform_id = transform_id;
					break;
				case CHAR_TYPE_TEXT:
					for (u32 j = 0; j < ch->text_size; ++j)
					{
						SVEC_BUMP(&app_context->draw_tasks);
						
						DrawTask* dt = SVEC_GET_TOP(&app_context->draw_tasks, DrawTask);
						
						dt->has_extra_transform_id = true;
						dt->extra_transform_id = transform_id;
						
						dt->has_extra_cxform_id = true;
						dt->extra_cxform_id = ch->cxform_id;
						
						dt->has_extra_transform = false;
						
						u32 glyph_index = 2*app_context->text_data[ch->text_start + j];
						
						dt->offset = app_context->glyph_data[glyph_index];
						dt->count = app_context->glyph_data[glyph_index + 1];
						dt->transform_id = ch->transform_start + j;
					}
					break;
			}
		}
		
		else
		{
			if (MC_EXTDATA_OF(disp_obj, has_tris))
			{
				SVEC_BUMP(&app_context->vertex_tasks);
				
				VertexTask* vt = SVEC_GET_TOP(&app_context->vertex_tasks, VertexTask);
				
				u32 vertex_count = 3*MC_EXTDATA_OF(disp_obj, tri_count);
				u32 vertex_offset = flashbang_allocate_vertices(app_context->fbc, vertex_count);
				
				app_context->frame_vertex_count += vertex_count;
				
				vt->count = vertex_count;
				vt->offset = vertex_offset;
				vt->tris = MC_EXTDATA_OF(disp_obj, tris);
				
				vt->free_after = false;
				
				SVEC_BUMP(&app_context->draw_tasks);
				
				DrawTask* dt = SVEC_GET_TOP(&app_context->draw_tasks, DrawTask);
				
				dt->has_extra_transform_id = false;
				dt->has_extra_cxform_id = false;
				dt->has_extra_transform = false;
				
				dt->offset = vertex_offset;
				dt->count = vertex_count;
				dt->transform_id = 0;
			}
			
			u32 bitmap_at = MC_EXTDATA_OF(disp_obj, bitmap_at);
			
			if (bitmap_at != 0)
			{
				ASObject* bitmap = MC_EXTDATA_OF(disp_obj, children)[bitmap_at];
				
				u32* tris = HALLOC(6*4*sizeof(float));
				
				u32 uninv_offset = flashbang_allocate_uninv(app_context->fbc);
				u32 uninv_id = uninv_offset/(16*sizeof(float));
				
				u32 bitmap_id = BM_EXTDATA_OF(bitmap, bitmap_id);
				
				// TODO: change 0x41 to 0x43
				
				VAL(float, &tris[0]) = 0.0f;
				VAL(float, &tris[1]) = (float) (20*BM_EXTDATA_OF(bitmap, height));
				tris[2] = 0x41;
				tris[3] = (uninv_id << 16) | bitmap_id;
				
				VAL(float, &tris[4]) = (float) (20*BM_EXTDATA_OF(bitmap, width));
				VAL(float, &tris[5]) = 0.0f;
				tris[6] = 0x41;
				tris[7] = (uninv_id << 16) | bitmap_id;
				
				VAL(float, &tris[8]) = 0.0f;
				VAL(float, &tris[9]) = 0.0f;
				tris[10] = 0x41;
				tris[11] = (uninv_id << 16) | bitmap_id;
				
				VAL(float, &tris[12]) = 0.0f;
				VAL(float, &tris[13]) = (float) (20*BM_EXTDATA_OF(bitmap, height));
				tris[14] = 0x41;
				tris[15] = (uninv_id << 16) | bitmap_id;
				
				VAL(float, &tris[16]) = (float) (20*BM_EXTDATA_OF(bitmap, width));
				VAL(float, &tris[17]) = 0.0f;
				tris[18] = 0x41;
				tris[19] = (uninv_id << 16) | bitmap_id;
				
				VAL(float, &tris[20]) = (float) (20*BM_EXTDATA_OF(bitmap, width));
				VAL(float, &tris[21]) = (float) (20*BM_EXTDATA_OF(bitmap, height));
				tris[22] = 0x41;
				tris[23] = (uninv_id << 16) | bitmap_id;
				
				SVEC_BUMP(&app_context->draw_tasks);
				
				DrawTask* dt = SVEC_GET_TOP(&app_context->draw_tasks, DrawTask);
				
				dt->has_extra_transform_id = false;
				dt->has_extra_cxform_id = false;
				
				dt->has_extra_transform = true;
				
				dt->x = (f32) (20.0f*MovieClip_getTotalX(app_context, disp_obj));
				dt->y = (f32) (20.0f*MovieClip_getTotalY(app_context, disp_obj));
				
				dt->rotation = (f32) (MovieClip_getTotalRotation(app_context, disp_obj)*M_PI/180.0);
				
				dt->xscale = (f32) (MovieClip_getTotalXScale(app_context, disp_obj)/100.0f);
				dt->yscale = (f32) (MovieClip_getTotalYScale(app_context, disp_obj)/100.0f);
				
				SVEC_BUMP(&app_context->vertex_tasks);
				
				VertexTask* vt = SVEC_GET_TOP(&app_context->vertex_tasks, VertexTask);
				
				u32 vertex_count = 6;
				app_context->frame_vertex_count += vertex_count;
				vt->count = vertex_count;
				
				u32 vertex_offset = flashbang_allocate_vertices(app_context->fbc, vertex_count);
				vt->offset = vertex_offset;
				
				dt->count = vertex_count;
				dt->offset = vertex_offset;
				dt->transform_id = 0;
				
				vt->tris = tris;
				
				vt->free_after = true;
				
				SVEC_BUMP(&app_context->uninv_tasks);
				
				UninvTask* ut = SVEC_GET_TOP(&app_context->uninv_tasks, UninvTask);
				
				ut->offset = uninv_offset;
				
				ut->x = 0.0f;
				ut->y = 0.0f;
				
				ut->xscale = 20.0f;
				ut->yscale = 20.0f;
			}
		}
	}
	
	flashbang_open_pass(app_context->fbc, app_context);
	
	if (app_context->vertex_tasks.length > 0)
	{
		flashbang_open_vertex_transfer(app_context->fbc, app_context->frame_vertex_count, app_context->uninv_tasks.length);
		
		for (size_t i = 0; i < app_context->vertex_tasks.length; ++i)
		{
			VertexTask* vt = SVEC_GET(&app_context->vertex_tasks, VertexTask, i);
			
			flashbang_upload_vertices(app_context->fbc, vt->tris, vt->offset, vt->count);
			
			bool free_after = vt->free_after;
			
			if (free_after)
			{
				FREE(vt->tris);
			}
		}
		
		for (size_t i = 0; i < app_context->uninv_tasks.length; ++i)
		{
			UninvTask* ut = SVEC_GET(&app_context->uninv_tasks, UninvTask, i);
			
			u32 offset = ut->offset;
			
			temp_mat_data[0] = ut->xscale;
			temp_mat_data[1] = 0.0f;
			temp_mat_data[4] = 0.0f;
			temp_mat_data[5] = ut->yscale;
			
			temp_mat_data[12] = ut->x;
			temp_mat_data[13] = ut->y;
			
			flashbang_upload_uninv(app_context->fbc, temp_mat_data, offset);
		}
		
		flashbang_close_vertex_transfer(app_context->fbc);
	}
	
	for (size_t i = 0; i < app_context->draw_tasks.length; ++i)
	{
		DrawTask* t = SVEC_GET(&app_context->draw_tasks, DrawTask, i);
		
		u32 extra_transform_id = t->has_extra_transform_id ? t->extra_transform_id : 0;
		flashbang_upload_extra_transform_id(app_context->fbc, extra_transform_id);
		
		u32 extra_cxform_id = t->has_extra_cxform_id ? t->extra_cxform_id : 0;
		flashbang_upload_cxform_id(app_context->fbc, extra_cxform_id);
		
		if (t->has_extra_transform)
		{
			temp_mat_data[0] = cosf(t->rotation)*t->xscale;
			temp_mat_data[1] = sinf(t->rotation)*t->yscale;
			temp_mat_data[4] = -sinf(t->rotation)*t->xscale;
			temp_mat_data[5] = cosf(t->rotation)*t->yscale;
			
			temp_mat_data[12] = t->x;
			temp_mat_data[13] = t->y;
			
			flashbang_upload_extra_transform(app_context->fbc, temp_mat_data);
		}
		
		else
		{
			flashbang_upload_extra_transform(app_context->fbc, (float*) identity);
		}
		
		flashbang_draw_shape(app_context->fbc, t->offset, t->count, t->transform_id);
	}
	
	flashbang_close_pass(app_context->fbc, app_context);
	
	SVEC_CLEAR(&app_context->vertex_tasks);
	SVEC_CLEAR(&app_context->uninv_tasks);
	SVEC_CLEAR(&app_context->draw_tasks);
	
	SVEC_CLEAR(&app_context->movieclip_stack);
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