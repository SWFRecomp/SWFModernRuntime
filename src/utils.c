#include <string.h>

#include <heap.h>
#include <utils.h>

size_t get_power_two_size(size_t old_size, size_t size)
{
	while (old_size < size)
	{
		old_size <<= 1;
	}
	
	return old_size;
}

void grow_ptr(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	char* new_data = HALLOC(old_data_size << 1);
	
	memcpy(new_data, data, old_data_size);
	
	FREE(data);
	
	*ptr = new_data;
	*capacity_ptr = capacity << 1;
}

void grow_ptr_far(SWFAppContext* app_context, char** ptr, size_t* capacity_ptr, size_t elem_size, size_t new_size)
{
	char* data = *ptr;
	size_t capacity = *capacity_ptr;
	size_t old_data_size = capacity*elem_size;
	
	size_t new_capacity = get_power_two_size(capacity, new_size);
	size_t new_data_size = new_capacity*elem_size;
	
	char* new_data = HALLOC(new_data_size);
	
	memcpy(new_data, data, old_data_size);
	
	FREE(data);
	
	*ptr = new_data;
	*capacity_ptr = new_capacity;
}

#if defined(_MSC_VER)
// Microsoft

#include <windows.h>
#include <process.h>
#include <Winbase.h>

u32 get_elapsed_ms()
{
	return (u32) GetTickCount();
}

void recomp_sleep(u32 ms)
{
	Sleep(ms);
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

void thread_start(SWFAppContext* app_context, runtime_thread_func f, recomp_thread_t* handle)
{
	*((uintptr_t*) handle) = _beginthreadex(NULL, 0, f, app_context, 0, NULL);
}

void thread_exit()
{
	_endthreadex(0);
}

void thread_join(recomp_thread_t* handle)
{
	WaitForSingleObject((HANDLE) handle, INFINITE);
	CloseHandle((HANDLE) handle);
}

void rwlock_init(recomp_rwlock_t* rwlock)
{
	InitializeSRWLock((PSRWLOCK) rwlock);
}

void rwlock_lock_read(recomp_rwlock_t* rwlock)
{
	AcquireSRWLockShared((PSRWLOCK) rwlock);
}

void rwlock_unlock_read(recomp_rwlock_t* rwlock)
{
	ReleaseSRWLockShared((PSRWLOCK) rwlock);
}

void rwlock_lock_write(recomp_rwlock_t* rwlock)
{
	AcquireSRWLockExclusive((PSRWLOCK) rwlock);
}

void rwlock_unlock_write(recomp_rwlock_t* rwlock)
{
	ReleaseSRWLockExclusive((PSRWLOCK) rwlock);
}

void rwlock_destroy(recomp_rwlock_t* rwlock)
{
	
}

#elif defined(__GNUC__)
// GCC

#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

u32 get_elapsed_ms()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return (now.tv_sec)*1000 + (now.tv_nsec)/1000000;
}

void recomp_sleep(u32 ms)
{
	struct timespec ms_ts;
	ms_ts.tv_sec = ms/1000;
	ms_ts.tv_nsec = (ms % 1000)*1000000;
	nanosleep(&ms_ts, NULL);
}

char* vmem_reserve(size_t size)
{
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

void vmem_release(char* addr, size_t size)
{
	munmap(addr, size);
}

void thread_start(SWFAppContext* app_context, runtime_thread_func f, recomp_thread_t* handle)
{
	pthread_create(handle, NULL, f, app_context);
}

void thread_exit()
{
	
}

void thread_join(recomp_thread_t* handle)
{
	pthread_join(*handle, NULL);
}

void rwlock_init(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_init(rwlock, NULL);
}

void rwlock_lock_read(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_rdlock(rwlock);
}

void rwlock_unlock_read(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_unlock(rwlock);
}

void rwlock_lock_write(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_wrlock(rwlock);
}

void rwlock_unlock_write(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_unlock(rwlock);
}

void rwlock_destroy(recomp_rwlock_t* rwlock)
{
	pthread_rwlock_destroy(rwlock);
}

#endif