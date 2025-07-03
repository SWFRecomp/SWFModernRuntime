#include <utils.h>

#if defined(_MSC_VER)
//  Microsoft

#include <malloc.h>
#include <windows.h>
#include <Winbase.h>

void* aligned_alloc(size_t alignment, size_t size)
{
	return _aligned_malloc(size, alignment);
}

void aligned_free(void* memblock)
{
	_aligned_free(memblock);
}

u32 get_elapsed_ms()
{
	return (u32) GetTickCount();
}

#elif defined(__GNUC__)
// GCC

#include <stdlib.h>
#include <time.h>

void aligned_free(void* memblock)
{
	free(memblock);
}

u32 get_elapsed_ms()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return (now.tv_sec)*1000 + (now.tv_nsec)/1000000;
}

#endif