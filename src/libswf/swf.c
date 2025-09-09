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

void swfStart(frame_func* frame_funcs, int width, int height)
{
	context = flashbang_new();
	flashbang_init(context, width, height);
	
	dictionary = malloc(INITIAL_DICTIONARY_CAPACITY*sizeof(Character));
	display_list = malloc(INITIAL_DISPLAYLIST_CAPACITY*sizeof(DisplayObject));
	
	stack = (char*) aligned_alloc(8, INITIAL_STACK_SIZE);
	sp = INITIAL_SP;
	
	quit_swf = 0;
	bad_poll = 0;
	next_frame = 0;
	
	initTime();
	initMap();
	
	tagMain(frame_funcs);
	
	freeMap();
	
	aligned_free(stack);
	
	free(dictionary);
	free(display_list);
	
	flashbang_free(context);
}