#pragma once

#include <stackvalue.h>

typedef void (*frame_func)();

extern frame_func frame_funcs[];

extern char* stack;
extern u32 sp;
extern u32 oldSP;

extern int quit_swf;
extern size_t next_frame;
extern int manual_next_frame;

extern char* dictionary[];
extern size_t dictionary_sizes[];

extern size_t display_list[];
extern size_t max_depth;

void swfStart();