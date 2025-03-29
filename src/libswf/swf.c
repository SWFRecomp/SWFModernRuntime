#include <swf.h>
#include <action.h>

ActionStackValue* stack;
u64 sp;

int quit_swf;
size_t next_frame;
int manual_next_frame;

void tagMain();

void swfStart()
{
	stack = malloc(INITIAL_STACK_SIZE*sizeof(ActionStackValue));
	sp = INITIAL_SP;
	
	quit_swf = 0;
	next_frame = 0;
	
	tagMain();
}