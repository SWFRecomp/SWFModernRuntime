#pragma once

#include <rb_tree.h>

#include <stdint.h>

typedef struct
{
	struct rb_tree t;
	size_t length;
	size_t struct_size;
} rbtree;