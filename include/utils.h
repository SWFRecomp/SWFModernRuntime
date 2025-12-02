#pragma once

#include <common.h>

#include <stddef.h>

#define ENSURE_SIZE(ptr, new_size, capac, elem_size) \
	if (new_size >= capac) \
	{ \
		grow_ptr(app_context, (char**) &ptr, &capac, elem_size); \
	}

void grow_ptr(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size);

u32 get_elapsed_ms();
int getpagesize();

char* vmem_reserve(size_t size);
void vmem_release(char* addr, size_t size);