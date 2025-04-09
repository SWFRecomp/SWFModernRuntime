#include <swf.h>
#include <action.h>
#include <utils.h>

char* stack;
u32 sp;

int quit_swf;
size_t next_frame;
int manual_next_frame;

void tagMain();

void swfStart()
{
	stack = (char*) aligned_alloc(8, INITIAL_STACK_SIZE);
	sp = INITIAL_SP;
	
	quit_swf = 0;
	next_frame = 0;
	
	tagMain();
}