#include <swf.h>

ActionStackValue* stack;
u64 sp;

int quit_swf;
size_t next_frame;
int manual_next_frame;

void swfStart()
{
	stack = malloc(256*sizeof(ActionStackValue));
	sp = 0;
	
	quit_swf = 0;
	next_frame = 0;
	
	tagMain();
}