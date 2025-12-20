#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "o1heap.h"
#include "memory/heap.h"
#include "libswf/swf.h"
#include "utils.h"

/**
 * Virtual Memory-based Heap Implementation
 *
 * Strategy:
 * - Reserve full virtual address space (1 GB) upfront - no physical RAM used
 * - Commit all pages immediately - still no physical RAM used!
 * - Initialize o1heap with the full 1 GB committed space
 * - Physical memory is lazily allocated by OS on first access (spreads overhead across frames)
 * - No expansion logic needed - heap has full space from start
 * - Heap state stored in app_context for proper lifecycle management
 *
 * Key Insights:
 * 1. Reserving virtual address space is cheap (no physical RAM)
 * 2. Committing pages is also cheap (<1 ms for 1 GB) - still no physical RAM!
 * 3. Physical RAM only allocated when memory is first touched (lazy allocation)
 * 4. This spreads allocation overhead across many frames, preventing stutter
 * 5. Pre-touching pages to force allocation is too slow (150 ms for 512 MB)
 * 6. Trusting OS lazy allocation is fastest and smoothest
 *
 * Performance: Committing 1 GB upfront is faster than trying to be "smart" about
 * incremental expansion. The OS handles lazy physical allocation better than we can.
 */

#define DEFAULT_FULL_HEAP_SIZE (1ULL * 1024 * 1024 * 1024)  // 1 GB virtual space

bool heap_init(SWFAppContext* app_context, size_t initial_size)
{
	if (app_context == NULL)
	{
		fprintf(stderr, "ERROR: heap_init() called with NULL app_context\n");
		return false;
	}

	if (app_context->heap_inited)
	{
		fprintf(stderr, "WARNING: heap_init() called when already initialized\n");
		return true;
	}

	// Reserve large virtual address space (1 GB)
	app_context->heap_full_size = DEFAULT_FULL_HEAP_SIZE;
	app_context->heap = vmem_reserve(app_context->heap_full_size);

	if (app_context->heap == NULL)
	{
		fprintf(stderr, "ERROR: Failed to reserve %llu bytes of virtual address space\n",
			(unsigned long long)app_context->heap_full_size);
		return false;
	}

	// vmem_reserve now does both reserve and commit in one step
	// Physical memory is still allocated lazily by OS on first access
	app_context->heap_current_size = app_context->heap_full_size;

	// Initialize o1heap with the full committed size
	app_context->heap_instance = o1heapInit(app_context->heap, app_context->heap_full_size);

	if (app_context->heap_instance == NULL)
	{
		fprintf(stderr, "ERROR: Failed to initialize o1heap (size=%zu, arena=%p)\n",
			app_context->heap_full_size, (void*)app_context->heap);
		vmem_release(app_context->heap, app_context->heap_full_size);
		app_context->heap = NULL;
		return false;
	}

	app_context->heap_inited = 1;

	printf("[HEAP] Initialized: %.1f GB reserved and committed (physical RAM allocated on access)\n",
		app_context->heap_full_size / (1024.0 * 1024.0 * 1024.0));

	return true;
}

void* heap_alloc(SWFAppContext* app_context, size_t size)
{
	if (app_context == NULL || !app_context->heap_inited)
	{
		fprintf(stderr, "ERROR: heap_alloc() called before heap_init()\n");
		return NULL;
	}

	if (size == 0)
	{
		return NULL;  // Standard malloc behavior
	}

	// Allocate from the heap
	// All pages are already committed, so no expansion logic needed
	// Physical RAM is allocated lazily by the OS when memory is first accessed
	void* ptr = o1heapAllocate(app_context->heap_instance, size);

	if (ptr == NULL)
	{
		fprintf(stderr, "ERROR: heap_alloc(%zu) failed - out of memory\n", size);
	}

	return ptr;
}

void* heap_calloc(SWFAppContext* app_context, size_t num, size_t size)
{
	// Check for overflow
	if (num != 0 && size > SIZE_MAX / num)
	{
		return NULL;
	}

	size_t total = num * size;
	void* ptr = heap_alloc(app_context, total);

	if (ptr != NULL)
	{
		memset(ptr, 0, total);
	}

	return ptr;
}

void heap_free(SWFAppContext* app_context, void* ptr)
{
	if (ptr == NULL)
	{
		return;  // Standard free behavior
	}

	if (app_context == NULL || !app_context->heap_inited)
	{
		fprintf(stderr, "ERROR: heap_free() called before heap_init()\n");
		return;
	}

	// Check if pointer is within our heap bounds
	if (ptr < (void*)app_context->heap ||
		ptr >= (void*)(app_context->heap + app_context->heap_current_size))
	{
		fprintf(stderr, "ERROR: heap_free() called with invalid pointer %p\n", ptr);
		fprintf(stderr, "       This pointer was not allocated by heap_alloc()\n");
		assert(0);  // Crash in debug builds
		return;
	}

	o1heapFree(app_context->heap_instance, ptr);
}

void heap_stats(SWFAppContext* app_context)
{
	if (app_context == NULL || !app_context->heap_inited)
	{
		printf("[HEAP] Not initialized\n");
		return;
	}

	O1HeapDiagnostics diag = o1heapGetDiagnostics(app_context->heap_instance);

	printf("\n========== Heap Statistics ==========\n");
	printf("Reserved space:  %.1f GB (%llu bytes)\n",
		app_context->heap_full_size / (1024.0 * 1024.0 * 1024.0),
		(unsigned long long)app_context->heap_full_size);
	printf("Committed space: %zu MB (%zu bytes)\n",
		app_context->heap_current_size / (1024 * 1024),
		app_context->heap_current_size);
	printf("Capacity:        %zu MB (%zu bytes)\n",
		diag.capacity / (1024 * 1024), diag.capacity);
	printf("Allocated:       %zu MB (%zu bytes, %.1f%%)\n",
		diag.allocated / (1024 * 1024), diag.allocated,
		100.0 * diag.allocated / diag.capacity);
	printf("Peak allocated:  %zu MB (%zu bytes, %.1f%%)\n",
		diag.peak_allocated / (1024 * 1024), diag.peak_allocated,
		100.0 * diag.peak_allocated / diag.capacity);
	printf("Peak request:    %zu bytes\n", diag.peak_request_size);
	printf("OOM count:       %llu\n", (unsigned long long)diag.oom_count);
	printf("=====================================\n\n");
}

void heap_shutdown(SWFAppContext* app_context)
{
	if (app_context == NULL || !app_context->heap_inited)
	{
		return;
	}

	printf("[HEAP] Shutting down - releasing virtual memory\n");

	// Release all virtual memory
	vmem_release(app_context->heap, app_context->heap_full_size);

	app_context->heap_instance = NULL;
	app_context->heap = NULL;
	app_context->heap_inited = 0;
	app_context->heap_current_size = 0;
	app_context->heap_full_size = 0;
}