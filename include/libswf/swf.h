#pragma once

#include <stackvalue.h>

#define INITIAL_DICTIONARY_CAPACITY 1024
#define INITIAL_DISPLAYLIST_CAPACITY 1024

typedef struct Character
{
	char* tris;
	size_t size;
} Character;

typedef struct DisplayObject
{
	size_t char_id;
	float* transform;
} DisplayObject;

typedef void (*frame_func)();

extern frame_func frame_funcs[];

extern char* stack;
extern u32 sp;
extern u32 oldSP;

extern int quit_swf;
extern size_t next_frame;
extern int manual_next_frame;

extern Character* dictionary;

extern DisplayObject* display_list;
extern size_t max_depth;

void swfStart();