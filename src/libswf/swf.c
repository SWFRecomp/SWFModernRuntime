#ifndef NO_GRAPHICS

#include <stdlib.h>
#include <swf.h>
#include <tag.h>
#include <action.h>
#include <variables.h>
#include <flashbang.h>
#include <utils.h>
#include <heap.h>

int quit_swf;
int bad_poll;
size_t current_frame;
size_t next_frame;
int manual_next_frame;
ActionVar* temp_val;

// Global frame access for ActionCall opcode
frame_func* g_frame_funcs = NULL;
size_t g_frame_count = 0;

// Drag state tracking
int is_dragging = 0;
char* dragged_target = NULL;

Character* dictionary = NULL;

DisplayObject* display_list = NULL;
size_t max_depth = 0;

FlashbangContext* context;

void tagMain(SWFAppContext* app_context)
{
	frame_func* frame_funcs = app_context->frame_funcs;
	
	while (!quit_swf)
	{
		current_frame = next_frame;
		frame_funcs[next_frame](app_context);
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
		tagShowFrame(app_context);
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
	context->cxform_data = app_context->cxform_data;
	context->cxform_data_size = app_context->cxform_data_size;
	
	dictionary = malloc(INITIAL_DICTIONARY_CAPACITY*sizeof(Character));
	display_list = malloc(INITIAL_DISPLAYLIST_CAPACITY*sizeof(DisplayObject));
	
	// Allocate stack into app_context (use system malloc, not heap - stack is allocated before heap_init)
	app_context->stack = (char*) malloc(INITIAL_STACK_SIZE);
	app_context->sp = INITIAL_SP;
	app_context->oldSP = 0;
	
	quit_swf = 0;
	bad_poll = 0;
	next_frame = 0;
	
	// Store frame info globally for ActionCall opcode
	g_frame_funcs = app_context->frame_funcs;
	g_frame_count = app_context->frame_count;
	
	initTime(app_context);
	initMap();
	
	// Initialize heap allocator (must be before flashbang_init which uses HALLOC)
	if (!heap_init(app_context, 0)) {  // 0 = use default size (64 MB)
		fprintf(stderr, "Failed to initialize heap allocator\n");
		return;
	}
	
	flashbang_init(app_context, context);
	
	tagInit();
	
	tagMain(app_context);
	
	flashbang_free(app_context, context);
	
	heap_shutdown(app_context);
	freeMap();
	
	free(app_context->stack);
	
	free(dictionary);
	free(display_list);
}

#endif // NO_GRAPHICS