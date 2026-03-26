#include <heap.h>

#include <rbtree.h>

static s32 node_cmp_string_id(const struct rb_node* n, const void* v)
{
	return *((u32*) v) - ((rbnode*) n)->string_id;
}

static s64 node_cmp_u64(const struct rb_node* n, const void* v)
{
	return *((u64*) v) - *((u64*) (((char*) n) + sizeof(struct rb_node)));
}

/** Insert a node into a tree if it doesn't exist, but return it if it does
 *
 * \param   T       The red-black tree into which to insert the new node
 *
 * \param   key     The key to search for
 *
 * \param   cmp     A comparison function to use to order the nodes.
 */
static inline rbnode* rb_tree_get_or_insert(SWFAppContext* app_context,
						rbtree* T, const u32* string_id,
						s32 (*cmp)(const struct rb_node*, const void*))
{
	/* This function is declared inline in the hopes that the compiler can
	 * optimize away the comparison function pointer call.
	 */
	struct rb_node* y = NULL;
	struct rb_node* x = T->t.root;
	s32 c = 0;
	while (x != NULL)
	{
		y = x;
		c = cmp(x, string_id);
		if (c < 0)
			x = x->left;
		else if (c > 0)
			x = x->right;
		else
		{
			return (rbnode*) x;
		}
	}
	
	rbnode* node = HALLOC(T->struct_size);
	node->string_id = *string_id;
	rb_tree_insert_at(&T->t, y, (struct rb_node*) node, c < 0);
	T->length += 1;
	return node;
}

/** Search the tree for a node
 *
 * If a node with a matching key exists, the first matching node found will
 * be returned.  If no matching node exists, NULL is returned.
 *
 * \param   T       The red-black tree to search
 *
 * \param   key     The key to search for
 *
 * \param   cmp     A comparison function to use to order the nodes
 */
static inline rbnode* rb_tree_search_u64(rbtree* T, const void* key,
						s64 (*cmp)(const struct rb_node*, const void*))
{
	/* This function is declared inline in the hopes that the compiler can
	 * optimize away the comparison function pointer call.
	 */
	struct rb_node* x = T->t.root;
	while (x != NULL)
	{
		s64 c = cmp(x, key);
		if (c < 0)
			x = x->left;
		else if (c > 0)
			x = x->right;
		else
			return (rbnode*) x;
	}
	
	return NULL;
}

/** Insert a node into a tree if it doesn't exist, but return it if it does
 *  This function accepts a u64 key
 *
 * \param   T       The red-black tree into which to insert the new node
 *
 * \param   key     The (u64) key to search for
 *
 * \param   cmp     A comparison function to use to order the nodes.
 */
static inline void* rb_tree_get_or_insert_u64(SWFAppContext* app_context,
						rbtree* T, const u64* key,
						s64 (*cmp)(const struct rb_node*, const void*))
{
	/* This function is declared inline in the hopes that the compiler can
	 * optimize away the comparison function pointer call.
	 */
	struct rb_node* y = NULL;
	struct rb_node* x = T->t.root;
	s64 c = 0;
	while (x != NULL)
	{
		y = x;
		c = cmp(x, key);
		if (c < 0)
			x = x->left;
		else if (c > 0)
			x = x->right;
		else
		{
			return (rbnode*) x;
		}
	}
	
	void* node = HALLOC(T->struct_size);
	*((u64*) (((char*) node) + sizeof(struct rb_node))) = *key;
	rb_tree_insert_at(&T->t, y, (struct rb_node*) node, c < 0);
	T->length += 1;
	return node;
}

/** Insert a node into a tree if it doesn't exist
 *  This function accepts a u64 key
 *
 * \param   T       The red-black tree into which to insert the new node
 *
 * \param   key     The (u64) key to search for
 *
 * \param   node    The node to insert
 *
 * \param   cmp     A comparison function to use to order the nodes.
 */
static inline void rb_tree_insert_u64(SWFAppContext* app_context,
						rbtree* T, const u64* key,
						s64 (*cmp)(const struct rb_node*, const void*))
{
	/* This function is declared inline in the hopes that the compiler can
	 * optimize away the comparison function pointer call.
	 */
	struct rb_node* y = NULL;
	struct rb_node* x = T->t.root;
	s64 c = 0;
	while (x != NULL)
	{
		y = x;
		c = cmp(x, key);
		if (c < 0)
			x = x->left;
		else if (c > 0)
			x = x->right;
		else
		{
			return;
		}
	}
	
	void* node = HALLOC(T->struct_size);
	*((u64*) (((char*) node) + sizeof(struct rb_node))) = *key;
	rb_tree_insert_at(&T->t, y, (struct rb_node*) node, c < 0);
	T->length += 1;
}

void rbtree_init(rbtree* t, size_t struct_size)
{
	rb_tree_init((struct rb_tree*) t);
	t->length = 0;
	t->struct_size = struct_size;
}

rbnode* rbtree_get(rbtree* t, u32 string_id)
{
	return (rbnode*) rb_tree_search((struct rb_tree*) t, &string_id, node_cmp_string_id);
}

rbnode* rbtree_get_or_insert(SWFAppContext* app_context, rbtree* t, u32 string_id)
{
	return rb_tree_get_or_insert(app_context, t, &string_id, node_cmp_string_id);
}

void* rbtree_get_u64(rbtree* t, u64 key)
{
	return (void*) rb_tree_search_u64(t, &key, node_cmp_u64);
}

void* rbtree_get_or_insert_u64(SWFAppContext* app_context, rbtree* t, u64 key)
{
	return rb_tree_get_or_insert_u64(app_context, t, &key, node_cmp_u64);
}

void rbtree_insert_u64(SWFAppContext* app_context, rbtree* t, u64 key)
{
	rb_tree_insert_u64(app_context, t, &key, node_cmp_u64);
}

void rbtree_remove_u64(SWFAppContext* app_context, rbtree* t, u64 key)
{
	struct rb_node* n = (struct rb_node*) rb_tree_search_u64(t, &key, node_cmp_u64);
	
	if (n == NULL)
	{
		return;
	}
	
	rb_tree_remove(&t->t, n);
	t->length -= 1;
	FREE(n);
}

void* rbtree_pop_root(rbtree* t)
{
	void* root = (void*) t->t.root;
	rb_tree_remove(&t->t, (struct rb_node*) root);
	t->length -= 1;
	return root;
}