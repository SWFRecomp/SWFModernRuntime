#include <swf.h>
#include <tag.h>
#include <flashbang.h>
#include <utils.h>

extern FlashbangContext* context;

size_t dictionary_capacity = INITIAL_DICTIONARY_CAPACITY;
size_t display_list_capacity = INITIAL_DISPLAYLIST_CAPACITY;

void tagSetBackgroundColor(u8 red, u8 green, u8 blue)
{
	flashbang_set_window_background(context, red, green, blue);
}

void tagShowFrame()
{
	char* v_buffer = flashbang_map_vertex_transfer_buffer(context);
	char* x_buffer = flashbang_map_xform_transfer_buffer(context);
	
	for (size_t i = 1; i <= max_depth; ++i)
	{
		DisplayObject* obj = &display_list[i];
		
		if (obj->char_id == 0)
		{
			continue;
		}
		
		Character* ch = &dictionary[obj->char_id];
		flashbang_upload_vertices(context, v_buffer, ch->tris, ch->size);
		flashbang_upload_xform(context, x_buffer, (char*) obj->transform);
	}
	
	flashbang_unmap_vertex_transfer_buffer(context);
	flashbang_unmap_xform_transfer_buffer(context);
	
	flashbang_draw(context);
}

void tagDefineShape(size_t char_id, char* tris, size_t tris_size)
{
	if (char_id >= dictionary_capacity)
	{
		grow_ptr((char**) &dictionary, &dictionary_capacity, sizeof(Character));
	}
	
	dictionary[char_id].tris = tris;
	dictionary[char_id].size = tris_size;
}

void tagPlaceObject2(size_t depth, size_t char_id, float* transform)
{
	if (depth >= display_list_capacity)
	{
		grow_ptr((char**) &display_list, &display_list_capacity, sizeof(DisplayObject));
	}
	
	display_list[depth].char_id = char_id;
	display_list[depth].transform = transform;
	
	if (depth > max_depth)
	{
		max_depth = depth;
	}
}