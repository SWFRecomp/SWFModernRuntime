#pragma once

#include <recomp.h>

typedef struct
{
	char* samples;
	size_t byte_count;
	bool loaded;
	
	size_t stream_id;
} SoundData;

void Sound_new(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Sound_loadSound(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Sound_start(SWFAppContext* app_context, ASObject* this, u32 num_args);
void Sound_destroy(SWFAppContext* app_context, ASObject* this, u32 num_args);