#include <utils.h>

void grow_ptr(char** ptr, size_t* capacity_ptr, size_t elem_size)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	char* new_data = malloc(old_data_size << 1);
	
	for (size_t i = 0; i < old_data_size; ++i)
	{
		new_data[i] = data[i];
	}
	
	free(data);
	
	*ptr = new_data;
	*capacity_ptr = capacity << 1;
}

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

u32 getpagesize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	
	return si.dwPageSize;
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