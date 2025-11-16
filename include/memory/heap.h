#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdbool.h>

#include <swf.h>

/**
 * Memory Heap Manager
 *
 * Wrapper around o1heap allocator providing multi-heap support with automatic expansion.
 *
 * Design:
 * - Starts with initial heap (default 32 MB)
 * - When current heap is full, creates a new larger heap
 * - Keeps chain of heaps alive (no migration needed)
 * - Heap sizes double: 32 MB -> 64 MB -> 128 MB -> 256 MB
 */

/**
 * Initialize the heap system
 *
 * @param initial_size Initial heap size in bytes (default: 32 MB if 0)
 */
void heap_init(SWFAppContext* app_context, size_t initial_size);

/**
 * Allocate memory from the heap
 *
 * Semantics similar to malloc():
 * - Returns pointer aligned to O1HEAP_ALIGNMENT
 * - Returns NULL on allocation failure
 * - Size of 0 returns NULL (standard behavior)
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* heap_alloc(SWFAppContext* app_context, size_t size);

/**
 * Free memory allocated by heap_alloc() or heap_calloc()
 *
 * Semantics similar to free():
 * - Passing NULL is a no-op
 * - Pointer must have been returned by heap_alloc() or heap_calloc()
 *
 * @param ptr Pointer to memory to free
 */
void heap_free(SWFAppContext* app_context, void* ptr);

/**
 * Shutdown the heap system
 *
 * Frees all heap arenas. Should be called at program exit.
 * After calling this, heap_alloc() will fail until heap_init() is called again.
 */
void heap_shutdown(SWFAppContext* app_context);

#endif // HEAP_H
