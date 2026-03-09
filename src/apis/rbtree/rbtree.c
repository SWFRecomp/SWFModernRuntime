#include <heap.h>

#include <rbtree.h>

static int node_cmp_u32(const struct rb_node* n, const void* v)
{
	return *((u32*) v) - ((rbnode*) n)->string_id;
}

/** Insert a node into a tree if it doesn't exist, but return it if it does
 *
 * \param   T       The red-black tree into which to insert the new node
 *
 * \param   key     The key to search for
 *
 * \param   node    The node to insert
 *
 * \param   cmp     A comparison function to use to order the nodes.
 */
static inline rbnode* rb_tree_get_or_insert(SWFAppContext* app_context,
						rbtree* T, const u32* string_id,
						int (*cmp)(const struct rb_node*, const void*))
{
	/* This function is declared inline in the hopes that the compiler can
	 * optimize away the comparison function pointer call.
	 */
	struct rb_node* y = NULL;
	struct rb_node* x = T->t.root;
	int c = 0;
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
	node->string_id = *((u32*) string_id);
	rb_tree_insert_at(&T->t, y, (struct rb_node*) node, c < 0);
	return node;
}

#include <assert.h>

void rbtree_init(rbtree* t, size_t struct_size)
{
	assert(struct_size != 0);
	rb_tree_init((struct rb_tree*) t);
	t->struct_size = struct_size;
}

rbnode* rbtree_get(rbtree* t, u32 string_id)
{
	return (rbnode*) rb_tree_search((struct rb_tree*) t, &string_id, node_cmp_u32);
}

rbnode* rbtree_get_or_insert(SWFAppContext* app_context, rbtree* t, u32 string_id)
{
	return rb_tree_get_or_insert(app_context, t, &string_id, node_cmp_u32);
}