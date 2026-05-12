#pragma once

#include <context.h>

#define HALLOC(s) heap_alloc(app_context, s);
#define HREALLOC(p, s) heap_realloc(app_context, p, s);
#define FREE(p) heap_free(app_context, p);

/**
 * Memory Heap Manager
 *
 * Wrapper around o1heap allocator providing multi-heap support.
 */

/**
 * Initialize the heap system
 *
 * @param app_context Main app context
 * @param size Heap size in bytes
 */
void heap_init(SWFAppContext* app_context, size_t size);

/**
 * Allocate memory from the heap
 *
 * @param app_context Main app context
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* heap_alloc(SWFAppContext* app_context, size_t size);

/**
 * Allocate memory from the heap, copying memory from an old ptr and freeing it
 *
 * @param app_context Main app context
 * @param ptr Old ptr to copy and free
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* heap_realloc(SWFAppContext* app_context, void* ptr, size_t size);

/**
 * Free memory allocated by heap_alloc() or heap_calloc()
 *
 * Pointer must have been returned by heap_alloc()
 *
 * @param app_context Main app context
 * @param ptr Pointer to memory to free
 */
void heap_free(SWFAppContext* app_context, void* ptr);

/**
 * Shutdown the heap system
 *
 * Frees all heap arenas. Should be called at program exit.
 *
 * @param app_context Main app context
 */
void heap_shutdown(SWFAppContext* app_context);