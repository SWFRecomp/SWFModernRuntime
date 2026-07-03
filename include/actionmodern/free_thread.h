#pragma once

#include <rbtree.h>
#include <utils.h>

extern recomp_rwlock_t object_queue_lock;
extern rbtree object_free_queue;

extern recomp_thread_t free_thread_handle;

DECLARE_RUNTIME_THREAD_FUNC(freeThread);