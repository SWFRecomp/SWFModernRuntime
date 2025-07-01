#pragma once

#include <stddef.h>

void* aligned_alloc(size_t alignment, size_t size);
void aligned_free(void* memblock);