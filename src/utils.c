#include <string.h>

#include <utils.h>

void grow_ptr(char** ptr, size_t* capacity_ptr, size_t elem_size)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	char* new_data = malloc(old_data_size << 1);
	
	memcpy(new_data, data, old_data_size);
	
	free(data);
	
	*ptr = new_data;
	*capacity_ptr = capacity << 1;
}

void grow_ptr_aligned(char** ptr, size_t* capacity_ptr, size_t elem_size, size_t alignment)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	char* new_data = aligned_alloc(alignment, old_data_size << 1);
	
	memcpy(new_data, data, old_data_size);
	
	aligned_free(data);
	
	*ptr = new_data;
	*capacity_ptr = capacity << 1;
}

#if defined(_MSC_VER)
// Microsoft

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

int getpagesize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	
	return si.dwPageSize;
}

char* vmem_reserve(size_t size)
{
	return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void vmem_release(char* addr, size_t size)
{
	VirtualFree(addr, 0, MEM_RELEASE);
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