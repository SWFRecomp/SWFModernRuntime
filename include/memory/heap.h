#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdbool.h>

// Forward declaration
typedef struct SWFAppContext SWFAppContext;

/**
 * Convenience macros for heap allocation
 *
 * These macros require app_context to be in scope.
 */
#define HALLOC(s) heap_alloc(app_context, s)
#define HCALLOC(n, s) heap_calloc(app_context, n, s)
#define FREE(p) heap_free(app_context, p)

/**
 * Memory Heap Manager
 *
 * Wrapper around o1heap allocator using virtual memory for efficient allocation.
 *
 * Design:
 * - Reserves 1 GB virtual address space upfront (cheap, no physical RAM)
 * - Commits all pages immediately (still cheap, still no physical RAM!)
 * - Initializes o1heap with full 1 GB space (no expansion needed)
 * - Physical RAM only allocated on first access (lazy allocation by OS)
 * - Heap state stored in app_context for proper lifecycle management
 *
 * Key benefit: Lazy physical allocation by OS spreads memory overhead across frames,
 * reducing stutter compared to traditional malloc approaches. Committing the full space
 * upfront is faster and simpler than incremental expansion.
 */

/**
 * Initialize the heap system
 *
 * Reserves and commits 1 GB of virtual address space. Physical RAM is allocated
 * lazily by the OS as memory is accessed.
 *
 * @param app_context The SWF application context to store heap state
 * @param initial_size Unused (kept for API compatibility)
 * @return true on success, false on failure
 */
bool heap_init(SWFAppContext* app_context, size_t initial_size);

/**
 * Allocate memory from the heap
 *
 * Semantics similar to malloc():
 * - Returns pointer aligned to O1HEAP_ALIGNMENT
 * - Returns NULL on allocation failure
 * - Size of 0 returns NULL (standard behavior)
 *
 * @param app_context The SWF application context containing heap state
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* heap_alloc(SWFAppContext* app_context, size_t size);

/**
 * Allocate zero-initialized memory from the heap
 *
 * Semantics similar to calloc():
 * - Allocates num * size bytes
 * - Zeroes the memory before returning
 * - Returns NULL on allocation failure or overflow
 *
 * @param app_context The SWF application context containing heap state
 * @param num Number of elements
 * @param size Size of each element
 * @return Pointer to allocated zero-initialized memory, or NULL on failure
 */
void* heap_calloc(SWFAppContext* app_context, size_t num, size_t size);

/**
 * Free memory allocated by heap_alloc() or heap_calloc()
 *
 * Semantics similar to free():
 * - Passing NULL is a no-op
 * - Pointer must have been returned by heap_alloc() or heap_calloc()
 *
 * @param app_context The SWF application context containing heap state
 * @param ptr Pointer to memory to free
 */
void heap_free(SWFAppContext* app_context, void* ptr);

/**
 * Get heap statistics
 *
 * Prints detailed statistics about heap usage including:
 * - Number of heaps
 * - Size of each heap
 * - Allocated memory
 * - Peak allocation
 * - OOM count
 *
 * @param app_context The SWF application context containing heap state
 */
void heap_stats(SWFAppContext* app_context);

/**
 * Shutdown the heap system
 *
 * Frees all heap arenas. Should be called at program exit.
 * After calling this, heap_alloc() will fail until heap_init() is called again.
 *
 * @param app_context The SWF application context containing heap state
 */
void heap_shutdown(SWFAppContext* app_context);

#endif // HEAP_H
