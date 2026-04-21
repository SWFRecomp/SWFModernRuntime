#pragma once

#include <common.h>
#include <swf.h>

#include <stddef.h>

#include <utils_lock.h>

#define LIKELY(exp) exp
#define UNLIKELY(exp) exp

#define ENSURE_SIZE(ptr, new_size, capac, elem_size) \
	if (UNLIKELY(new_size >= capac)) \
	{ \
		grow_ptr(app_context, (char**) &ptr, &capac, elem_size); \
	}

#define ENSURE_SIZE_FAR(ptr, new_size, capac, elem_size) \
	if (UNLIKELY(new_size >= capac)) \
	{ \
		grow_ptr_far(app_context, (char**) &ptr, &capac, elem_size, new_size); \
	}

size_t get_power_two_size(size_t old_size, size_t size);

void grow_ptr(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size);
void grow_ptr_far(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size, size_t new_size);

u32 get_elapsed_ms();
void recomp_sleep(u32 ms);
int getpagesize();

char* vmem_reserve(size_t size);
void vmem_release(char* addr, size_t size);

typedef unsigned int (*runtime_thread_func)(void* arg);

uintptr_t thread_start(SWFAppContext* app_context, runtime_thread_func f);
void thread_exit();
void thread_join(uintptr_t handle);

#include <windows.h>

#define DECLARE_RUNTIME_THREAD_FUNC(f) unsigned int f(SWFAppContext* app_context)

void mutex_init(recomp_mutex_t* mutex);
void mutex_lock_read(recomp_mutex_t* mutex);
void mutex_unlock_read(recomp_mutex_t* mutex);
void mutex_lock_write(recomp_mutex_t* mutex);
void mutex_unlock_write(recomp_mutex_t* mutex);