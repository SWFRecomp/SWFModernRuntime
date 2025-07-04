#include <tag.h>
#include <flashbang.h>

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(red, green, blue);
}

void tagShowFrame()
{
	flashbang_draw();
}