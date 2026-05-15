#include <swf.h>
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

FlashbangContext* context;

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
	context = &c;
	
	context->width = app_context->width;
	context->height = app_context->height;
	
	context->stage_to_ndc = app_context->stage_to_ndc;
	
	context->bitmap_count = app_context->bitmap_count;
	context->bitmap_highest_w = app_context->bitmap_highest_w;
	context->bitmap_highest_h = app_context->bitmap_highest_h;
	
	context->shape_data = app_context->shape_data;
	context->shape_data_size = app_context->shape_data_size;
	context->transform_data = app_context->transform_data;
	context->transform_data_size = app_context->transform_data_size;
	context->color_data = app_context->color_data;
	context->color_data_size = app_context->color_data_size;
	context->uninv_mat_data = app_context->uninv_mat_data;
	context->uninv_mat_data_size = app_context->uninv_mat_data_size;
	context->gradient_data = app_context->gradient_data;
	context->gradient_data_size = app_context->gradient_data_size;
	context->bitmap_data = app_context->bitmap_data;
	context->bitmap_data_size = app_context->bitmap_data_size;
	context->cxform_data = app_context->cxform_data;
	context->cxform_data_size = app_context->cxform_data_size;
	
	flashbang_init(context, app_context);
	
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
	
	flashbang_release(context, app_context);
	
	heap_shutdown(app_context);
}