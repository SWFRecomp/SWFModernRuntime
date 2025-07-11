#include <swf.h>
#include <tag.h>
#include <flashbang.h>

extern FlashbangContext* context;

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(context, red, green, blue);
}

void tagShowFrame()
{
	for (size_t i = 1; i <= max_depth; ++i)
	{
		size_t char_id = display_list[i];
		flashbang_upload_tris(context, dictionary[char_id], dictionary_sizes[char_id]);
	}
	
	flashbang_draw(context);
}

void tagPlaceObject2(size_t depth, size_t char_id)
{
	display_list[depth] = char_id;
	
	if (depth > max_depth)
	{
		max_depth = depth;
	}
}