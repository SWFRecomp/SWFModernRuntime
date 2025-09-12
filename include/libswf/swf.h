#pragma once

#include <stackvalue.h>

#define INITIAL_DICTIONARY_CAPACITY 1024
#define INITIAL_DISPLAYLIST_CAPACITY 1024

typedef struct Character
{
	size_t shape_offset;
	size_t size;
} Character;

typedef struct DisplayObject
{
	size_t char_id;
	u32 transform_id;
} DisplayObject;

typedef void (*frame_func)();

extern frame_func frame_funcs[];

typedef struct SWFAppContext
{
	frame_func* frame_funcs;
	int width;
	int height;
	const float* stage_to_ndc;
	char* shape_data;
	size_t shape_data_size;
	char* transform_data;
	size_t transform_data_size;
} SWFAppContext;

extern char* stack;
extern u32 sp;
extern u32 oldSP;

extern int quit_swf;
extern size_t next_frame;
extern int manual_next_frame;

extern Character* dictionary;

extern DisplayObject* display_list;
extern size_t max_depth;

void swfStart(SWFAppContext* app_context);