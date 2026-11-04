#include <string.h>

#include <objects.h>
#include <heap.h>

#include <free_thread.h>

extern ASObject* _global;

void block(SWFAppContext* app_context, ASObject* o)
{
	SVEC_CLEAR(&o->blocked_list);
	
	for (size_t i = 0; i < o->neighbors.length; ++i)
	{
		ASObject* neighbor = (ASObject*) o->neighbors.data[i];
		SVEC_PUSH(&o->blocked_list, neighbor);
	}
}

void unblock(ASObject* o)
{
	o->blocked = false;
	
	for (size_t i = 0; i < o->blocked_list.length; ++i)
	{
		ASObject* b = (ASObject*) o->blocked_list.data[i];
		
		if (b->blocked)
		{
			unblock(b);
		}
	}
	
	SVEC_CLEAR(&o->blocked_list);
}

bool traverseIteration(SWFAppContext* app_context, ASObject* o, SwapVector* path_stack);

bool detectCycle(SWFAppContext* app_context, ASObject* o, SwapVector* path_stack)
{
	if (o == (ASObject*) path_stack->data[0])
	{
		SwapVector* cycle = HALLOC(sizeof(SwapVector));
		SVEC_INIT(cycle);
		
		for (size_t i = 0; i < path_stack->length; ++i)
		{
			SVEC_PUSH(cycle, path_stack->data[i]);
		}
		
		SVEC_PUSH(&app_context->cycles, cycle);
		
		return true;
	}
	
	if (o->blocked)
	{
		return false;
	}
	
	return traverseIteration(app_context, o, path_stack);
}

bool traverseIteration(SWFAppContext* app_context, ASObject* o, SwapVector* path_stack)
{
	SVEC_PUSH(path_stack, o);
	
	o->blocked = true;
	
	bool cycle_found = false;
	
	for (size_t i = 0; i < o->neighbors.length; ++i)
	{
		ASObject* neighbor = (ASObject*) o->neighbors.data[i];
		
		if (neighbor->used)
		{
			continue;
		}
		
		cycle_found |= detectCycle(app_context, neighbor, path_stack);
	}
	
	SVEC_POP(path_stack);
	
	if (cycle_found)
	{
		unblock(o);
		return true;
	}
	
	block(app_context, o);
	
	return false;
}

void johnson(SWFAppContext* app_context)
{
	SwapVector path_stack;
	SVEC_INIT(&path_stack);
	
	for (size_t i = 0; i < app_context->reachable.length; ++i)
	{
		for (size_t j = 0; j < app_context->reachable.length; ++j)
		{
			ASObject* o = (ASObject*) app_context->reachable.data[j];
			
			o->blocked = false;
			SVEC_CLEAR(&o->blocked_list);
		}
		
		ASObject* o = (ASObject*) app_context->reachable.data[i];
		
		(void) traverseIteration(app_context, o, &path_stack);
		o->used = true;
	}
	
	for (size_t i = 0; i < app_context->reachable.length; ++i)
	{
		ASObject* o = (ASObject*) app_context->reachable.data[i];
		o->used = false;
	}
	
	SVEC_RELEASE(&path_stack);
}

void pushObjReachable(SWFAppContext* app_context, ASObject* this, ASProperty* p)
{
	ASObject* neighbor = (ASObject*) p->value.value;
	
	if (IS_OBJ(p->value))
	{
		bool reached = neighbor->reached;
		neighbor->reached = true;
		SVEC_PUSH(&this->neighbors, neighbor);
		if (!reached)
		{
			SVEC_PUSH(&app_context->reachable, neighbor);
		}
	}
}

void pushObjsReachable(SWFAppContext* app_context, ASObject* this)
{
	rbtree* t = &this->t;
	
	if (UNLIKELY(t->length == 0 || t->t.root == NULL))
	{
		return;
	}
	
	SwapVector node_stack;
	SVEC_INIT(&node_stack);
	SVEC_PUSH(&node_stack, t->t.root);
	
	while (node_stack.length > 0)
	{
		struct rb_node* node = (struct rb_node*) SVEC_TOP(&node_stack);
		
		SVEC_POP(&node_stack);
		
		pushObjReachable(app_context, this, (ASProperty*) node);
		
		if (node->right)
		{
			SVEC_PUSH(&node_stack, node->right);
		}
		
		if (node->left)
		{
			SVEC_PUSH(&node_stack, node->left);
		}
	}
	
	SVEC_RELEASE(&node_stack);
}

void getReachable(SWFAppContext* app_context, ASObject* o)
{
	SVEC_PUSH(&app_context->reachable, o);
	o->reached = true;
	
	size_t obj_i = 0;
	
	while (obj_i < app_context->reachable.length)
	{
		ASObject* this = (ASObject*) app_context->reachable.data[obj_i];
		
		SVEC_CLEAR(&this->neighbors);
		
		OBJ_LOCK_READ(this,
		{
			pushObjsReachable(app_context, this);
			this->temp_rc = this->refcount;
		});
		
		obj_i += 1;
	}
	
	for (size_t i = 0; i < app_context->reachable.length; ++i)
	{
		ASObject* this = (ASObject*) app_context->reachable.data[i];
		this->reached = false;
	}
}

bool containsObj(SwapVector* v, ASObject* o)
{
	for (size_t i = 0; i < v->length; ++i)
	{
		ASObject* this = (ASObject*) v->data[i];
		
		if (o == this)
		{
			return true;
		}
	}
	
	return false;
}

u32 countObjs(SwapVector* v, ASObject* o)
{
	u32 num_objs = 0;
	
	for (size_t i = 0; i < v->length; ++i)
	{
		ASObject* this = (ASObject*) v->data[i];
		
		if (o == this)
		{
			num_objs += 1;
		}
	}
	
	return num_objs;
}

recomp_rwlock_t object_queue_lock;
rbtree object_free_queue;

bool attemptFree(SWFAppContext* app_context, ASObject* o);

void freeObject(SWFAppContext* app_context, ASObject* o)
{
	o->freed = true;
	
	size_t length = o->neighbors.length;
	
	ASObject** neighbors = HALLOC(length*sizeof(ASObject*));
	memcpy(neighbors, o->neighbors.arena, length*sizeof(ASObject*));
	
	for (size_t i = 0; i < length; ++i)
	{
		ASObject* r = neighbors[i];
		
		if (r->freed || attemptFree(app_context, r))
		{
			continue;
		}
		
		LOCK_WRITE(object_queue_lock,
		{
			rbtree_insert_u64(app_context, &object_free_queue, (u64) r);
		});
		
		OBJ_LOCK_WRITE(r,
		{
			r->refcount -= 1;
		});
	}
	
	FREE(neighbors);
	
	destroyObject(app_context, o);
	FREE(o);
	
	LOCK_WRITE(object_queue_lock,
	{
		rbtree_remove_u64(app_context, &object_free_queue, (u64) o);
	});
}

ASObject* findBackRef(SwapVector* v, ASObject* ref)
{
	for (size_t i = 0; i < v->length; ++i)
	{
		ASObject* this = (ASObject*) v->data[i];
		
		if (this == ref)
		{
			if (UNLIKELY(i == 0))
			{
				return (ASObject*) v->data[v->length - 1];
			}
			
			return (ASObject*) v->data[i - 1];
		}
	}
	
	return NULL;
}

bool subRCTest(SWFAppContext* app_context, ASObject* o)
{
	johnson(app_context);
	
	SVEC_PUSH(&app_context->objects_to_test, o);
	
	for (size_t i = 0; i < app_context->cycles.length; ++i)
	{
		SwapVector* cycle = (SwapVector*) app_context->cycles.data[i];
		
		if (!containsObj(cycle, o))
		{
			continue;
		}
		
		for (size_t j = 0; j < cycle->length; ++j)
		{
			ASObject* this = (ASObject*) cycle->data[j];
			
			if (!containsObj(&app_context->objects_to_test, this))
			{
				SVEC_PUSH(&app_context->objects_to_test, this);
			}
		}
	}
	
	bool pass_test = true;
	
	for (size_t i = 0; i < app_context->objects_to_test.length; ++i)
	{
		ASObject* test = (ASObject*) app_context->objects_to_test.data[i];
		
		// TODO: should find backrefs from cycles here (see below TODO)
		
		SVEC_CLEAR(&app_context->objects_subbed);
		
		for (size_t j = 0; j < app_context->cycles.length; ++j)
		{
			SwapVector* cycle = (SwapVector*) app_context->cycles.data[j];
			
			// TODO: preprocess to find all backrefs to remove objects_subbed
			ASObject* backref = findBackRef(cycle, test);
			
			if (backref != NULL)
			{
				u32 num_refs = countObjs(&backref->neighbors, test);
				
				if (!containsObj(&app_context->objects_subbed, backref) && num_refs > 0)
				{
					SVEC_PUSH(&app_context->objects_subbed, backref);
					test->temp_rc -= num_refs;
				}
			}
		}
		
		if (test->temp_rc != 0)
		{
			pass_test = false;
			break;
		}
	}
	
	SVEC_CLEAR(&app_context->objects_to_test);
	
	for (size_t i = 0; i < app_context->cycles.length; ++i)
	{
		SwapVector* cycle = (SwapVector*) app_context->cycles.data[i];
		
		SVEC_RELEASE(cycle);
		FREE(cycle);
	}
	
	SVEC_CLEAR(&app_context->cycles);
	
	return pass_test;
}

bool attemptFree(SWFAppContext* app_context, ASObject* o)
{
	u32 rc;
	
	OBJ_LOCK_READ(o,
	{
		rc = o->refcount;
	});
	
	SVEC_CLEAR(&app_context->reachable);
	
	getReachable(app_context, o);
	
	if (rc == 0)
	{
		freeObject(app_context, o);
		return true;
	}
	
	if (subRCTest(app_context, o))
	{
		freeObject(app_context, o);
		return true;
	}
	
	return false;
}

recomp_thread_t free_thread_handle;

DECLARE_RUNTIME_THREAD_FUNC(freeThread)
{
	SVEC_INIT(&app_context->reachable);
	SVEC_INIT(&app_context->objects_to_test);
	SVEC_INIT(&app_context->cycles);
	SVEC_INIT(&app_context->objects_subbed);
	
	while (true)
	{
		size_t length;
		
		LOCK_READ(object_queue_lock,
		{
			length = object_free_queue.length;
		});
		
		if (length == 0)
		{
			bool stop;
			
			LOCK_READ(object_queue_lock,
			{
				stop = app_context->stop_free;
			});
			
			if (stop)
			{
				break;
			}
			
			recomp_sleep(16);
			
			continue;
		}
		
		objnode* n;
		
		LOCK_WRITE(object_queue_lock,
		{
			n = (objnode*) rbtree_pop_root(&object_free_queue);
		});
		
		ASObject* o = (ASObject*) n->key;
		
		attemptFree(app_context, o);
		
		FREE(n);
	}
	
	SVEC_RELEASE(&app_context->reachable);
	SVEC_RELEASE(&app_context->objects_to_test);
	SVEC_RELEASE(&app_context->cycles);
	SVEC_RELEASE(&app_context->objects_subbed);
	
	thread_exit();
	
	return 0;
}