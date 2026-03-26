#pragma once

#include <windows.h>

#define LOCK_READ(lock, code) \
	mutex_lock_read(&lock); \
	code \
	mutex_unlock_read(&lock);

#define LOCK_WRITE(lock, code) \
	mutex_lock_write(&lock); \
	code \
	mutex_unlock_write(&lock);

typedef SRWLOCK recomp_mutex_t;