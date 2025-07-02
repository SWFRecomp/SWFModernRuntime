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

void swfStart();