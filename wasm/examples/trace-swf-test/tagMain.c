#include <recomp.h>

#include <out.h>
#include "draws.h"

void frame_0()
{
	tagSetBackgroundColor(255, 255, 255);
	script_0(stack, &sp);
	tagShowFrame();
	quit_swf = 1;
}

typedef void (*frame_func)();

frame_func frame_funcs[] =
{
	frame_0,
};

void tagInit()
{
}