#pragma once

#include <common.h>

#include <stddef.h>

void* aligned_alloc(size_t alignment, size_t size);
void aligned_free(void* memblock);

u32 get_elapsed_ms();