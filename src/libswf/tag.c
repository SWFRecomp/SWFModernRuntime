#include <tag.h>
#include <flashbang.h>

extern FlashbangContext* context;

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(context, red, green, blue);
}

void tagShowFrame()
{
	flashbang_draw(context);
}