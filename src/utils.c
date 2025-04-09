#include <utils.h>

#if defined(_MSC_VER)
//  Microsoft

#include <malloc.h>

void* aligned_alloc(size_t alignment, size_t size)
{
	return _aligned_malloc(size, alignment);
}

void aligned_free(void* memblock)
{
	_aligned_free(memblock);
}

#elif defined(__GNUC__)
// GCC

void aligned_free(void* memblock)
{
	free(memblock);
}

#endif