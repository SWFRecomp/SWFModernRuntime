#include <swf.h>
#include <action.h>
#include <variables.h>
#include <utils.h>

#include <flashbang.h>

char* stack;
u32 sp;
u32 oldSP;

int quit_swf;
int bad_poll;
size_t next_frame;
int manual_next_frame;
ActionVar* temp_val;

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
	
	while (!flashbang_poll());
}

void swfStart(frame_func* frame_funcs)
{
	flashbang_init();
	
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
	
	flashbang_quit();
}