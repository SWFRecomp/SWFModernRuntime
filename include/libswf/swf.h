#pragma once

#include <stackvalue.h>

extern char* stack;
extern u32 sp;

extern int quit_swf;
extern size_t next_frame;
extern int manual_next_frame;

void swfStart();