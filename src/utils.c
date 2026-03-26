#include <string.h>

#include <heap.h>
#include <utils.h>

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

uintptr_t thread_start(SWFAppContext* app_context, runtime_thread_func f)
{
	return _beginthreadex(NULL, 0, f, app_context, 0, NULL);
}

void thread_exit()
{
	_endthreadex(0);
}

void thread_join(uintptr_t handle)
{
	WaitForSingleObject((HANDLE) handle, INFINITE);
	CloseHandle((HANDLE) handle);
}

void mutex_init(recomp_mutex_t* mutex)
{
	InitializeSRWLock((PSRWLOCK) mutex);
}

void mutex_lock_read(recomp_mutex_t* mutex)
{
	AcquireSRWLockShared((PSRWLOCK) mutex);
}

void mutex_unlock_read(recomp_mutex_t* mutex)
{
	ReleaseSRWLockShared((PSRWLOCK) mutex);
}

void mutex_lock_write(recomp_mutex_t* mutex)
{
	AcquireSRWLockExclusive((PSRWLOCK) mutex);
}

void mutex_unlock_write(recomp_mutex_t* mutex)
{
	ReleaseSRWLockExclusive((PSRWLOCK) mutex);
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

char* vmem_reserve(size_t size)
{
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

void vmem_release(char* addr, size_t size)
{
	munmap(addr, size);
}

#endif