#pragma once

#include <common.h>
#include <rbtree_struct.h>
#include <swap_vector.h>
#include <utils_lock.h>

typedef struct
{
	u32 id;
	rbtree t;
	u32 refcount;
	recomp_rwlock_t lock;
	void* extra_data;
	bool reached;
	bool used;
	bool blocked;
	bool freed;
	SwapVector neighbors;
	SwapVector blocked_list;
	u32 temp_rc;
} ASObject;