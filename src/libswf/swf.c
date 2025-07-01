#include <swf.h>
#include <action.h>
#include <variables.h>
#include <utils.h>

char* stack;
u32 sp;
u32 oldSP;

int quit_swf;
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
	}
}

void swfStart(frame_func* frame_funcs)
{
	stack = (char*) aligned_alloc(8, INITIAL_STACK_SIZE);
	sp = INITIAL_SP;
	
	quit_swf = 0;
	next_frame = 0;
	
	initMap();
	
	tagMain(frame_funcs);
}