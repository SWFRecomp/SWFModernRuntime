#ifndef NO_GRAPHICS

#include <swf.h>
#include <tag.h>
#include <action.h>
#include <variables.h>
#include <flashbang.h>
#include <utils.h>

char* stack;
u32 sp;
u32 oldSP;

int quit_swf;
int bad_poll;
size_t next_frame;
int manual_next_frame;
ActionVar* temp_val;

Character* dictionary = NULL;

DisplayObject* display_list = NULL;
size_t max_depth = 0;

FlashbangContext* context;

void tagMain(frame_func* frame_funcs)
{
	while (!quit_swf)
	{
		frame_funcs[next_frame]();
		if (!manual_next_frame)
		{
			next_frame += 1;
		}
		manual_next_frame = 0;
		bad_poll |= flashbang_poll();
		quit_swf |= bad_poll;
	}

	if (bad_poll)
	{
		return;
	}

	while (!flashbang_poll())
	{
		tagShowFrame();
	}
}

void swfStart(SWFAppContext* app_context)
{
	context = flashbang_new();

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

	flashbang_init(context);

	dictionary = malloc(INITIAL_DICTIONARY_CAPACITY*sizeof(Character));
	display_list = malloc(INITIAL_DISPLAYLIST_CAPACITY*sizeof(DisplayObject));

	stack = (char*) aligned_alloc(8, INITIAL_STACK_SIZE);
	sp = INITIAL_SP;

	quit_swf = 0;
	bad_poll = 0;
	next_frame = 0;

	initTime();
	initMap();

	tagInit();

	tagMain(frame_funcs);

	freeMap();

	aligned_free(stack);

	free(dictionary);
	free(display_list);

	flashbang_free(context);
}

#endif // NO_GRAPHICS