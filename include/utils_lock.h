#pragma once

#define LOCK_READ(lock, code) \
	rwlock_lock_read(&lock); \
	code \
	rwlock_unlock_read(&lock);

#define LOCK_WRITE(lock, code) \
	rwlock_lock_write(&lock); \
	code \
	rwlock_unlock_write(&lock);

#if defined(_MSC_VER)
// Microsoft

#include <windows.h>

typedef SRWLOCK recomp_rwlock_t;

#elif defined(__GNUC__)
// GCC

#include <pthread.h>

typedef pthread_rwlock_t recomp_rwlock_t;

#endif