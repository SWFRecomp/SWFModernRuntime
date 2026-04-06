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

bool traverseIteration(SWFAppContext* app_context, ASObject* o, SwapVector* path_stack, SwapVector* cycles);

bool detectCycle(SWFAppContext* app_context, ASObject* o, SwapVector* path_stack, SwapVector* cycles)
{
	if (o == (ASObject*) path_stack->data[0])
	{
		SwapVector* cycle = HALLOC(sizeof(SwapVector));
		SVEC_INIT(cycle);
		
		for (size_t i = 0; i < path_stack->length; ++i)
		{
			SVEC_PUSH(cycle, path_stack->data[i]);
		}
		
		SVEC_PUSH(cycles, cycle);
		
		return true;
	}
	
	if (o->blocked)
	{
		return false;
	}
	
	return traverseIteration(app_context, o, path_stack, cycles);
}

bool traverseIteration(SWFAppContext* app_context, ASObject* o, SwapVector* path_stack, SwapVector* cycles)
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
		
		cycle_found |= detectCycle(app_context, neighbor, path_stack, cycles);
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

void johnson(SWFAppContext* app_context, SwapVector* objs, SwapVector* cycles)
{
	SwapVector path_stack;
	SVEC_INIT(&path_stack);
	
	for (size_t i = 0; i < objs->length; ++i)
	{
		for (size_t j = 0; j < objs->length; ++j)
		{
			ASObject* o = (ASObject*) objs->data[j];
			
			o->blocked = false;
			SVEC_CLEAR(&o->blocked_list);
		}
		
		ASObject* o = (ASObject*) objs->data[i];
		
		(void) traverseIteration(app_context, o, &path_stack, cycles);
		o->used = true;
	}
	
	for (size_t i = 0; i < objs->length; ++i)
	{
		ASObject* o = (ASObject*) objs->data[i];
		o->used = false;
	}
	
	SVEC_RELEASE(&path_stack);
}

void pushObjReachable(SWFAppContext* app_context, ASObject* this, ASProperty* p, SwapVector* v)
{
	ASObject* neighbor = (ASObject*) p->value.value;
	
	if (IS_OBJ(p->value))
	{
		bool reached = neighbor->reached;
		neighbor->reached = true;
		SVEC_PUSH(&this->neighbors, neighbor);
		if (!reached)
		{
			SVEC_PUSH(v, neighbor);
		}
	}
}

void pushObjsReachable(SWFAppContext* app_context, ASObject* this, SwapVector* nodes)
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
		
		pushObjReachable(app_context, this, (ASProperty*) node, nodes);
		
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

void getReachable(SWFAppContext* app_context, ASObject* o, SwapVector* reachable)
{
	SVEC_PUSH(reachable, o);
	o->reached = true;
	
	size_t obj_i = 0;
	
	while (obj_i < reachable->length)
	{
		ASObject* this = (ASObject*) reachable->data[obj_i];
		
		SVEC_CLEAR(&this->neighbors);
		
		OBJ_LOCK_READ(this,
		{
			pushObjsReachable(app_context, this, reachable);
			this->temp_rc = this->refcount;
		});
		
		obj_i += 1;
	}
	
	for (size_t i = 0; i < reachable->length; ++i)
	{
		ASObject* this = (ASObject*) reachable->data[i];
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

recomp_mutex_t object_queue_lock;
rbtree object_free_queue;

void attemptFree(SWFAppContext* app_context, ASObject* o);

void freeObject(SWFAppContext* app_context, ASObject* o, SwapVector* reachable)
{
	o->freed = true;
	
	for (size_t i = 0; i < reachable->length; ++i)
	{
		ASObject* r = (ASObject*) reachable->data[i];
		
		if (!r->freed)
		{
			attemptFree(app_context, r);
		}
	}
	
	// TODO: maybe this can be moved above the call to attemptFree,
	//		 and then don't consider objects with freed set to be reachable?
	for (size_t i = 0; i < o->neighbors.length; ++i)
	{
		ASObject* neighbor = (ASObject*) o->neighbors.data[i];
		
		if (!neighbor->freed)
		{
			OBJ_LOCK_WRITE(neighbor,
			{
				neighbor->refcount -= 1;
			});
		}
	}
	
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

bool subRCTest(SWFAppContext* app_context, ASObject* o, SwapVector* reachable)
{
	SwapVector objects_to_test;
	SwapVector cycles;
	
	SVEC_INIT(&objects_to_test);
	SVEC_INIT(&cycles);
	
	johnson(app_context, reachable, &cycles);
	
	SVEC_PUSH(&objects_to_test, o);
	
	for (size_t i = 0; i < cycles.length; ++i)
	{
		SwapVector* cycle = (SwapVector*) cycles.data[i];
		
		for (size_t j = 0; j < cycle->length; ++j)
		{
			ASObject* this = (ASObject*) cycle->data[j];
			
			if (!containsObj(&objects_to_test, this))
			{
				SVEC_PUSH(&objects_to_test, this);
			}
		}
	}
	
	bool pass_test = true;
	
	SwapVector objects_subbed;
	SVEC_INIT(&objects_subbed);
	
	for (size_t i = 0; i < objects_to_test.length; ++i)
	{
		ASObject* test = (ASObject*) objects_to_test.data[i];
		
		// TODO: should find backrefs from cycles here (see below TODO)
		
		for (size_t j = 0; j < cycles.length; ++j)
		{
			SwapVector* cycle = (SwapVector*) cycles.data[j];
			
			// TODO: preprocess to find all backrefs to remove objects_subbed
			ASObject* backref = findBackRef(cycle, test);
			
			if (backref != NULL)
			{
				u32 num_refs = countObjs(&backref->neighbors, test);
				
				if (!containsObj(&objects_subbed, backref) && num_refs > 0)
				{
					SVEC_PUSH(&objects_subbed, backref);
					test->temp_rc -= num_refs;
				}
			}
		}
		
		if (test->temp_rc != 0)
		{
			pass_test = false;
			break;
		}
		
		SVEC_CLEAR(&objects_subbed);
	}
	
	SVEC_RELEASE(&objects_subbed);
	
	for (size_t i = 0; i < cycles.length; ++i)
	{
		SwapVector* cycle = (SwapVector*) cycles.data[i];
		
		SVEC_RELEASE(cycle);
	}
	
	SVEC_RELEASE(&cycles);
	
	return pass_test;
}

void attemptFree(SWFAppContext* app_context, ASObject* o)
{
	u32 rc;
	
	OBJ_LOCK_READ(o,
	{
		rc = o->refcount;
	});
	
	SwapVector reachable;
	SVEC_INIT(&reachable);
	getReachable(app_context, o, &reachable);
	
	if (rc == 0)
	{
		freeObject(app_context, o, &reachable);
	}
	
	else
	{
		if (subRCTest(app_context, o, &reachable))
		{
			freeObject(app_context, o, &reachable);
		}
	}
	
	SVEC_RELEASE(&reachable);
}

uintptr_t free_thread_handle;

DECLARE_RUNTIME_THREAD_FUNC(freeThread)
{
	while (true)
	{
		if (bad_poll)
		{
			break;
		}
		
		for (int i = 0; i < 100; ++i)
		{
			size_t length = 0;
			
			LOCK_READ(object_queue_lock,
			{
				length = object_free_queue.length;
			});
			
			if (length > 0)
			{
				objnode* n;
				
				LOCK_WRITE(object_queue_lock,
				{
					n = (objnode*) rbtree_pop_root(&object_free_queue);
				});
				
				ASObject* o = (ASObject*) n->key;
				
				attemptFree(app_context, o);
				
				FREE(n);
			}
			
			else
			{
				break;
			}
		}
		
		recomp_sleep(16);
	}
	
	thread_exit();
	
	return 0;
}