#pragma once

#include <common.h>

#include <stddef.h>

void grow_ptr(char** ptr, size_t* capacity_ptr, size_t elem_size);

void* aligned_alloc(size_t alignment, size_t size);
void aligned_free(void* memblock);

u32 get_elapsed_ms();
u32 getpagesize();