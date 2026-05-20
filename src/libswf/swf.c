#include <context.h>
#include <tag.h>
#include <action.h>
#include <variables.h>
#include <flashbang.h>
#include <heap.h>
#include <utils.h>

int quit_swf;
int bad_poll;
size_t next_frame;
int manual_next_frame;
ActionVar* temp_val;

Character* dictionary = NULL;

u16 swfGetExportedChar(SWFAppContext* app_context, u32 string_id)
{
	u16 char_id = 0;
	
	for (size_t i = 0; i < app_context->exported_chars_count; ++i)
	{
		if (app_context->exported_string_ids[i] == string_id)
		{
			char_id = app_context->exported_char_ids[i];
			break;
		}
	}
	
	return char_id;
}

u16 swfGetBitmapId(SWFAppContext* app_context, u32 char_id)
{
	u16 bitmap_id = 0;
	
	for (size_t i = 0; i < app_context->bitmap_count; ++i)
	{
		if (app_context->bitmap_char_ids[i] == bitmap_id)
		{
			bitmap_id = app_context->bitmap_ids[i];
			break;
		}
	}
	
	return bitmap_id;
}

void tagMain(SWFAppContext* app_context)
{
	frame_func* frame_funcs = app_context->frame_funcs;
	
	while (!quit_swf)
	{
		frame_funcs[next_frame](app_context);
		if (!manual_next_frame)
		{
			next_frame += 1;
		}
		manual_next_frame = 0;
		
		bad_poll |= flashbang_poll();
		quit_swf |= bad_poll;
	}
	
	while (!(bad_poll = flashbang_poll()))
	{
		tagShowFrame(app_context);
	}
}

void swfStart(SWFAppContext* app_context)
{
	heap_init(app_context, HEAP_SIZE);
	
	FlashbangContext c;
	app_context->fbc = &c;
	
	c.width = app_context->width;
	c.height = app_context->height;
	
	c.stage_to_ndc = app_context->stage_to_ndc;
	
	c.bitmap_count = app_context->bitmap_count;
	c.bitmap_highest_w = app_context->bitmap_highest_w;
	c.bitmap_highest_h = app_context->bitmap_highest_h;
	
	c.shape_data_exists = app_context->shape_data_exists;
	
	c.shape_data = app_context->shape_data;
	c.shape_data_size = app_context->shape_data_size;
	c.transform_data = app_context->transform_data;
	c.transform_data_size = app_context->transform_data_size;
	c.color_data = app_context->color_data;
	c.color_data_size = app_context->color_data_size;
	c.uninv_mat_data = app_context->uninv_mat_data;
	c.uninv_mat_data_size = app_context->uninv_mat_data_size;
	c.gradient_data = app_context->gradient_data;
	c.gradient_data_size = app_context->gradient_data_size;
	c.bitmap_data = app_context->bitmap_data;
	c.bitmap_data_size = app_context->bitmap_data_size;
	c.cxform_data = app_context->cxform_data;
	c.cxform_data_size = app_context->cxform_data_size;
	
	flashbang_init(&c, app_context);
	
	dictionary = HALLOC(INITIAL_DICTIONARY_CAPACITY*sizeof(Character));
	
	app_context->dictionary_capacity = INITIAL_DICTIONARY_CAPACITY;
	app_context->max_depth = 0;
	
	STACK = (char*) HALLOC(INITIAL_STACK_SIZE);
	SP = INITIAL_SP;
	
	quit_swf = 0;
	bad_poll = 0;
	next_frame = 0;
	
	initVarArray(app_context, app_context->max_string_id);
	
	initActions(app_context);
	initMap();
	
	tagInit(app_context);
	
	tagMain(app_context);
	
	freeMap(app_context);
	freeActions(app_context);
	
	FREE(STACK);
	
	FREE(dictionary);
	
	flashbang_release(&c, app_context);
	
	heap_shutdown(app_context);
}