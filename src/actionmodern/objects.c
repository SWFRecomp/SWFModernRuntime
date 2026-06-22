#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include <initial_strings_decls.h>
#include <Array.h>
#include <Function.h>
#include <MovieClip.h>
#include <Sound.h>
#include <heap.h>
#include <utils.h>
#include <swap_vector.h>

#include <action.h>
#include <objects.h>

extern recomp_rwlock_t object_queue_lock;

u32 next_id = 0;

ASObject* allocObjectCommon(SWFAppContext* app_context)
{
	ASObject* obj = HALLOC(sizeof(ASObject));
	
	obj->id = next_id;
	next_id += 1;
	
	rbtree_init(&obj->t, sizeof(ASProperty));
	obj->refcount = 0;
	rwlock_init(&obj->lock);
	obj->extra_data = NULL;
	obj->reached = false;
	obj->used = false;
	obj->blocked = false;
	obj->freed = false;
	SVEC_INIT(&obj->neighbors);
	SVEC_INIT(&obj->blocked_list);
	obj->temp_rc = 0;
	
	//~ LOCK_WRITE(object_queue_lock,
	//~ {
		//~ SVEC_PUSH(&app_context->active_objects, obj);
	//~ });
	
	return obj;
}

/**
 * Object Allocation
 *
 * Allocates a new ASObject and returns it.
 */
ASObject* allocObject(SWFAppContext* app_context)
{
	ASObject* obj = allocObjectCommon(app_context);
	
	ActionVar proto_var;
	proto_var.type = ACTION_STACK_VALUE_OBJECT;
	proto_var.object = (ASObject*) app_context->Object_prototype;
	setProperty(app_context, obj, STR_ID_PROTO, NULL, 0, &proto_var);
	
	ActionVar constructor_var;
	constructor_var.type = ACTION_STACK_VALUE_OBJECT;
	constructor_var.object = app_context->Object_constructor;
	setProperty(app_context, obj, STR_ID_CONSTRUCTOR, NULL, 0, &constructor_var);
	
	return obj;
}

/**
 * Retain Object
 *
 * Increments the reference count of an object.
 * Called when storing object in a variable, property, or array.
 */
void retainObject(ASObject* obj)
{
	obj->refcount++;
}

extern rbtree object_free_queue;

void queueObjectFreeCheck(SWFAppContext* app_context, ASObject* obj)
{
	LOCK_WRITE(object_queue_lock,
	{
		rbtree_insert_u64(app_context, &object_free_queue, (u64) obj);
	});
}

extern ASObject* _global;

/**
 * Release Object
 *
 * Decrements the reference count of an object.
 * When refcount reaches 0, frees the object and all its properties.
 * Recursively releases any objects stored in properties.
 */
void releaseObject(SWFAppContext* app_context, ASObject* obj)
{
	obj->refcount--;
	
	if (obj != _global || app_context->global_free_override)
	{
		// queue object for free check
		queueObjectFreeCheck(app_context, obj);
	}
}

bool getAndCallMethodIfExists(SWFAppContext* app_context, ASObject* this, u32 method_name, u32 num_args);

void destroyObject(SWFAppContext* app_context, ASObject* obj)
{
	ASObject* ctor = getConstructor(app_context, obj);
	
	// TODO: implement a lock for the stack
	//		 (or something else that doesn't suck lol)
	switch (Function_get_func_name_string_id(app_context, ctor))
	{
		case STR_ID_ARRAY:
		{
			Array_destroy(app_context, obj);
			
			break;
		}
		
		case STR_ID_MOVIECLIP:
		{
			MovieClip_destroy(app_context, obj);
			
			break;
		}
		
		case STR_ID_SOUND:
		{
			Sound_destroy(app_context, obj);
			
			break;
		}
	}
	
	while (obj->t.length > 0)
	{
		ASProperty* p = rbtree_pop_root(&obj->t);
		FREE(p);
	}
	
	if (obj->extra_data != NULL)
	{
		FREE(obj->extra_data);
	}
	
	SVEC_RELEASE(&obj->neighbors);
	SVEC_RELEASE(&obj->blocked_list);
	
	//~ LOCK_WRITE(object_queue_lock,
	//~ {
		//~ for (size_t i = 0; i < app_context->active_objects.length; ++i)
		//~ {
			//~ if ((ASObject*) app_context->active_objects.data[i] == obj)
			//~ {
				//~ SVEC_REMOVE(&app_context->active_objects, i);
				//~ break;
			//~ }
		//~ }
	//~ });
}

/**
 * Get Property
 *
 * Retrieves a property value by name.
 * Returns pointer to the ASProperty if found, or NULL if not found.
 */
ASProperty* getProperty(SWFAppContext* app_context, ASObject* this, u32 string_id, const char* name, u32 name_length)
{
	if (UNLIKELY(this == NULL || (string_id == 0 && name == NULL)))
	{
		if (this == NULL)
		{
			UNREACHABLE("getProperty: 'this' is null");
		}
		
		if (string_id == 0 && name == NULL)
		{
			UNREACHABLE("getProperty: string id is 0 and name is null");
		}
		
		return NULL;
	}
	
	if (UNLIKELY(string_id == 0))
	{
		string_id = getStringId(app_context, (char*) name, name_length);
	}
	
	ASProperty* p = (ASProperty*) rbtree_get(&this->t, string_id);
	return p;
}

/**
 * Get Property Var
 * 
 * Retrieves a property var by name.
 * Copies var into output parameter. This operation is locked for you.
 */
void getPropertyVar(SWFAppContext* app_context, ASObject* this, u32 string_id, const char* name, u32 name_length, ActionVar* out_var)
{
	if (UNLIKELY(this == NULL || (string_id == 0 && name == NULL) || out_var == NULL))
	{
		if (this == NULL)
		{
			UNREACHABLE("getPropertyVar: 'this' is null");
		}
		
		if (string_id == 0 && name == NULL)
		{
			UNREACHABLE("getPropertyVar: string id is 0 and name is null");
		}
		
		if (out_var == NULL)
		{
			UNREACHABLE("getPropertyVar: out_var is null");
		}
		
		return;
	}
	
	if (UNLIKELY(string_id == 0))
	{
		string_id = getStringId(app_context, (char*) name, name_length);
	}
	
	OBJ_LOCK_READ(this,
	{
		ASProperty* p = getProperty(app_context, this, string_id, name, name_length);
		
		if (p != NULL)
		{
			*out_var = p->value;
		}
		
		else
		{
			out_var->type = ACTION_STACK_VALUE_UNDEFINED;
		}
	});
}

/**
 * Get Property
 *
 * Retrieves a property value by name, or creates a new one.
 * Returns pointer to the ASProperty.
 */
ASProperty* getOrCreateProperty(SWFAppContext* app_context, ASObject* this, u32 string_id, const char* name, u32 name_length, bool* created)
{
	if (UNLIKELY(this == NULL || (string_id == 0 && name == NULL)))
	{
		if (this == NULL)
		{
			UNREACHABLE("getOrCreateProperty: 'this' is null");
		}
		
		if (string_id == 0 && name == NULL)
		{
			UNREACHABLE("getOrCreateProperty: string id is 0 and name is null");
		}
		
		return NULL;
	}
	
	return (ASProperty*) RBT_GET_OR_INS(&this->t, string_id, created);
}

/**
 * Get Property With Prototype Chain
 *
 * Retrieves a property value by name, searching up the prototype chain via __proto__.
 *
 * This implements proper prototype-based inheritance for ActionScript.
 */
void getPropertyVarWithPrototype(SWFAppContext* app_context, ASObject* this, u32 string_id, const char* name, u32 name_length, ActionVar* out_v)
{
	out_v->type = ACTION_STACK_VALUE_UNDEFINED;
	
	if (UNLIKELY(this == NULL || (string_id == 0 && name == NULL) || out_v == NULL))
	{
		if (this == NULL)
		{
			UNREACHABLE("getPropertyVarWithPrototype: 'this' is null");
		}
		
		if (string_id == 0 && name == NULL)
		{
			UNREACHABLE("getPropertyVarWithPrototype: string id is 0 and name is null");
		}
		
		if (out_v == NULL)
		{
			UNREACHABLE("getPropertyVarWithPrototype: out_v is null");
		}
		
		return;
	}
	
	ASObject* current = this;
	ASProperty* prop;
	
	while (true)
	{
		// Search own properties first
		
		rwlock_lock_read(&current->lock);
		prop = getProperty(app_context, current, string_id, name, name_length);
		
		if (prop != NULL)
		{
			break;
		}
		
		rwlock_unlock_read(&current->lock);
		
		// Property not found on this object - walk up to __proto__
		
		ASProperty* proto_prop;
		
		rwlock_lock_read(&current->lock);
		proto_prop = getProperty(app_context, current, STR_ID_PROTO, NULL, 0);
		
		if (proto_prop == NULL)
		{
			// No __proto__ property - end of chain
			prop = NULL;
			break;
		}
		
		ASObject* next = proto_prop->value.object;
		
		rwlock_unlock_read(&current->lock);
		
		// Move to next object in prototype chain
		current = next;
	}
	
	if (prop != NULL)
	{
		*out_v = prop->value;
	}
	
	rwlock_unlock_read(&current->lock);
}

/**
 * Set Property
 *
 * Sets a property value by name. Creates property if it doesn't exist.
 * Handles reference counting if value is an object.
 */
void setProperty(SWFAppContext* app_context, ASObject* this, u32 string_id, const char* name, u32 name_length, ActionVar* value)
{
	if (UNLIKELY(this == NULL || (string_id == 0 && name == NULL) || value == NULL))
	{
		if (this == NULL)
		{
			UNREACHABLE("setProperty: 'this' is null");
		}
		
		if (string_id == 0 && name == NULL)
		{
			UNREACHABLE("setProperty: string id is 0 and name is null");
		}
		
		if (value == NULL)
		{
			UNREACHABLE("setProperty: value is null");
		}
		
		return;
	}
	
	if (UNLIKELY(string_id == 0))
	{
		string_id = getStringId(app_context, (char*) name, name_length);
	}
	
	ASProperty* p;
	
	OBJ_LOCK_READ(this,
	{
		p = getProperty(app_context, this, string_id, name, name_length);
	});
	
	// Retain new value if it's an object
	if (IS_OBJ_P(value))
	{
		ASObject* new_obj = (ASObject*) value->value;
		
		OBJ_LOCK_WRITE(new_obj,
		{
			retainObject(new_obj);
		});
	}
	
	if (p != NULL)
	{
		// Property exists - update value
		
		bool release_old = false;
		ASObject* old_obj;
		
		OBJ_LOCK_READ(this,
		{
			// Release old value if it was an object
			if (IS_OBJ(p->value))
			{
				release_old = true;
				old_obj = (ASObject*) p->value.value;
			}
			
			// Free old string if it owned memory
			else if (p->value.type == ACTION_STACK_VALUE_STRING &&
					 p->value.owns_memory)
			{
				FREE(p->value.str);
			}
		});
		
		OBJ_LOCK_WRITE(this,
		{
			// Set new value
			p->value = *value;
		});
		
		if (release_old)
		{
			OBJ_LOCK_WRITE(old_obj,
			{
				releaseObject(app_context, old_obj);
			});
		}
		
		return;
	}
	
	bool created;
	
	OBJ_LOCK_WRITE(this,
	{
		// Property doesn't exist - create new one
		p = (ASProperty*) RBT_GET_OR_INS(&this->t, string_id, &created);
		p->value = *value;
	});
}

/**
 * Delete Property
 *
 * Deletes a property by name. Returns true if deleted or not found (Flash behavior).
 * Handles reference counting if value is an object/array.
 */
bool deleteProperty(SWFAppContext* app_context, ASObject* obj, const char* name, u32 name_length)
{
	//~ if (obj == NULL || name == NULL)
	//~ {
		//~ return true;  // Flash behavior: delete on null returns true
	//~ }
	
	//~ // Find property by name
	//~ for (u32 i = 0; i < obj->num_used; i++)
	//~ {
		//~ if (obj->properties[i].name_length == name_length &&
		    //~ strncmp(obj->properties[i].name, name, name_length) == 0)
		//~ {
			//~ // Property found - delete it
			
			//~ // 1. Release the property value if it's an object/array
			//~ if (obj->properties[i].value.type == ACTION_STACK_VALUE_OBJECT)
			//~ {
				//~ ASObject* child_obj = (ASObject*) obj->properties[i].value.value;
				//~ releaseObject(app_context, child_obj);
			//~ }
			
			//~ else if (obj->properties[i].value.type == ACTION_STACK_VALUE_ARRAY)
			//~ {
				//~ ASArray* child_arr = (ASArray*) obj->properties[i].value.value;
				//~ releaseArray(app_context, child_arr);
			//~ }
			
			//~ // Free string if it owns memory
			//~ else if (obj->properties[i].value.type == ACTION_STACK_VALUE_STRING &&
			         //~ obj->properties[i].value.owns_memory)
			//~ {
				//~ free(obj->properties[i].value.str);
			//~ }
			
			//~ // 2. Free the property name
			//~ if (obj->properties[i].name != NULL)
			//~ {
				//~ FREE(obj->properties[i].name);
			//~ }
			
			//~ // 3. Shift remaining properties down to fill the gap
			//~ for (u32 j = i; j < obj->num_used - 1; j++)
			//~ {
				//~ obj->properties[j] = obj->properties[j + 1];
			//~ }
			
			//~ // 4. Decrement the number of used slots
			//~ obj->num_used--;
			
			//~ // 5. Zero out the last slot
			//~ memset(&obj->properties[obj->num_used], 0, sizeof(ASProperty));
			
			//~ return true;
		//~ }
	//~ }
	
	return true;
}

/**
 * Get Constructor
 *
 * Get the constructor function for an object.
 * Returns the "constructor" property if it exists, NULL otherwise.
 */
ASObject* getConstructor(SWFAppContext* app_context, ASObject* obj)
{
	// Look for "constructor" property
	ActionVar ctor_var;
	getPropertyVar(app_context, obj, STR_ID_CONSTRUCTOR, NULL, 0, &ctor_var);
	
	if (LIKELY(IS_OBJ_T(ctor_var.type)))
	{
		return ctor_var.object;
	}
	
	UNREACHABLE("Object without constructor");
	return NULL;
}

/**
 * Debug Functions
 */

#ifdef DEBUG
void assertRefcount(ASObject* obj, u32 expected)
{
	if (obj == NULL)
	{
		fprintf(stderr, "ERROR: assertRefcount called with NULL object\n");
		assert(0);
	}
	
	if (obj->refcount != expected)
	{
		fprintf(stderr, "ERROR: refcount assertion failed: expected %u, got %u\n",
			expected, obj->refcount);
		assert(0);
	}
	
	printf("[DEBUG] assertRefcount: obj=%p, refcount=%u (OK)\n", (void*)obj, expected);
}

void printObject(ASObject* obj)
{
	if (obj == NULL)
	{
		printf("Object: NULL\n");
		return;
	}
	
	printf("Object: %p\n", (void*)obj);
	printf("  refcount: %u\n", obj->refcount);
	printf("  num_properties: %u\n", obj->num_properties);
	printf("  num_used: %u\n", obj->num_used);
	printf("  properties:\n");
	
	for (u32 i = 0; i < obj->num_used; i++)
	{
		printf("    [%u] '%.*s' = ",
			i, obj->properties[i].name_length, obj->properties[i].name);
		
		switch (obj->properties[i].value.type)
		{
			case ACTION_STACK_VALUE_F32:
				printf("%.15g (F32)\n", *((float*)&obj->properties[i].value.value));
				break;
			
			case ACTION_STACK_VALUE_F64:
				printf("%.15g (F64)\n", *((double*)&obj->properties[i].value.value));
				break;
			
			case ACTION_STACK_VALUE_STRING:
			{
				const char* str = obj->properties[i].value.owns_memory ?
					obj->properties[i].value.str :
					(const char*)obj->properties[i].value.value;
				printf("'%.*s' (STRING)\n", obj->properties[i].value.str_size, str);
				break;
			}
			
			case ACTION_STACK_VALUE_OBJECT:
				printf("%p (OBJECT)\n", (void*)obj->properties[i].value.value);
				break;
			
			default:
				printf("(unknown type %d)\n", obj->properties[i].value.type);
				break;
		}
	}
}

void printArray(ASArray* arr)
{
	if (arr == NULL)
	{
		printf("Array: NULL\n");
		return;
	}
	
	printf("Array: %p\n", (void*)arr);
	printf("  refcount: %u\n", arr->refcount);
	printf("  length: %u\n", arr->length);
	printf("  capacity: %u\n", arr->capacity);
	printf("  elements:\n");
	
	for (u32 i = 0; i < arr->length; i++)
	{
		printf("    [%u] = ", i);
		
		switch (arr->elements[i].type)
		{
			case ACTION_STACK_VALUE_F32:
				printf("%.15g (F32)\n", *((float*)&arr->elements[i].value));
				break;
			
			case ACTION_STACK_VALUE_F64:
				printf("%.15g (F64)\n", *((double*)&arr->elements[i].value));
				break;
			
			case ACTION_STACK_VALUE_STRING:
			{
				const char* str = arr->elements[i].owns_memory ?
					arr->elements[i].str :
					(const char*)arr->elements[i].value;
				printf("'%.*s' (STRING)\n", arr->elements[i].str_size, str);
				break;
			}
			
			case ACTION_STACK_VALUE_OBJECT:
				printf("%p (OBJECT)\n", (void*)arr->elements[i].value);
				break;
			
			case ACTION_STACK_VALUE_ARRAY:
				printf("%p (ARRAY)\n", (void*)arr->elements[i].value);
				break;
			
			default:
				printf("(unknown type %d)\n", arr->elements[i].type);
				break;
		}
	}
}
#endif

/**
 * Array Implementation
 */

ASArray* allocArray(SWFAppContext* app_context, u32 initial_capacity)
{
	ASArray* arr = (ASArray*) malloc(sizeof(ASArray));
	if (arr == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate ASArray\n");
		return NULL;
	}
	
	arr->refcount = 1;  // Initial reference owned by caller
	arr->length = 0;
	arr->capacity = initial_capacity > 0 ? initial_capacity : 4;
	
	// Allocate element array
	arr->elements = (ActionVar*) malloc(sizeof(ActionVar) * arr->capacity);
	if (arr->elements == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate array elements\n");
		free(arr);
		return NULL;
	}
	
	// Initialize elements to zero
	memset(arr->elements, 0, sizeof(ActionVar) * arr->capacity);
	
	return arr;
}

void retainArray(ASArray* arr)
{
	if (arr == NULL)
	{
		return;
	}
	
	arr->refcount++;
}

void releaseArray(SWFAppContext* app_context, ASArray* arr)
{
	//~ if (arr == NULL)
	//~ {
		//~ return;
	//~ }
	
	//~ arr->refcount--;
	
	//~ if (arr->refcount == 0)
	//~ {
		//~ // Release all element values
		//~ for (u32 i = 0; i < arr->length; i++)
		//~ {
			//~ // If element is an object, release it recursively
			//~ if (arr->elements[i].type == ACTION_STACK_VALUE_OBJECT)
			//~ {
				//~ ASObject* child_obj = (ASObject*) arr->elements[i].value;
				//~ releaseObject(app_context, child_obj);
			//~ }
			//~ // If element is an array, release it recursively
			//~ else if (arr->elements[i].type == ACTION_STACK_VALUE_ARRAY)
			//~ {
				//~ ASArray* child_arr = (ASArray*) arr->elements[i].value;
				//~ releaseArray(app_context, child_arr);
			//~ }
			//~ // If element is a string that owns memory, free it
			//~ else if (arr->elements[i].type == ACTION_STACK_VALUE_STRING &&
			         //~ arr->elements[i].owns_memory)
			//~ {
				//~ free(arr->elements[i].str);
			//~ }
		//~ }
		
		//~ // Free element array
		//~ if (arr->elements != NULL)
		//~ {
			//~ free(arr->elements);
		//~ }
		
		//~ // Free array itself
		//~ free(arr);
	//~ }
}

ActionVar* getArrayElement(ASArray* arr, u32 index)
{
	if (arr == NULL || index >= arr->length)
	{
		return NULL;
	}

	return &arr->elements[index];
}

void setArrayElement(SWFAppContext* app_context, ASArray* arr, u32 index, ActionVar* value)
{
	//~ if (arr == NULL || value == NULL)
	//~ {
		//~ return;
	//~ }

	//~ // Grow array if needed
	//~ if (index >= arr->capacity)
	//~ {
		//~ u32 new_capacity = (index + 1) * 2;  // Grow to accommodate index
		//~ ActionVar* new_elements = (ActionVar*) realloc(arr->elements,
		                                                //~ sizeof(ActionVar) * new_capacity);
		//~ if (new_elements == NULL)
		//~ {
			//~ fprintf(stderr, "ERROR: Failed to grow array\n");
			//~ return;
		//~ }

		//~ arr->elements = new_elements;

		//~ // Zero out new slots
		//~ memset(&arr->elements[arr->capacity], 0,
		       //~ sizeof(ActionVar) * (new_capacity - arr->capacity));

		//~ arr->capacity = new_capacity;
	//~ }

	//~ // Release old value if it exists and is an object/array
	//~ if (index < arr->length)
	//~ {
		//~ if (arr->elements[index].type == ACTION_STACK_VALUE_OBJECT)
		//~ {
			//~ ASObject* old_obj = (ASObject*) arr->elements[index].value;
			//~ releaseObject(app_context, old_obj);
		//~ }
		//~ else if (arr->elements[index].type == ACTION_STACK_VALUE_ARRAY)
		//~ {
			//~ ASArray* old_arr = (ASArray*) arr->elements[index].value;
			//~ releaseArray(app_context, old_arr);
		//~ }
		//~ else if (arr->elements[index].type == ACTION_STACK_VALUE_STRING &&
		         //~ arr->elements[index].owns_memory)
		//~ {
			//~ free(arr->elements[index].str);
		//~ }
	//~ }

	//~ // Set new value
	//~ arr->elements[index] = *value;

	//~ // Update length if needed
	//~ if (index >= arr->length)
	//~ {
		//~ arr->length = index + 1;
	//~ }

	//~ // Retain new value if it's an object or array
	//~ if (value->type == ACTION_STACK_VALUE_OBJECT)
	//~ {
		//~ ASObject* new_obj = (ASObject*) value->value;
		//~ retainObject(new_obj);
	//~ }
	//~ else if (value->type == ACTION_STACK_VALUE_ARRAY)
	//~ {
		//~ ASArray* new_arr = (ASArray*) value->value;
		//~ retainArray(new_arr);
	//~ }

//~ #ifdef DEBUG
	//~ printf("[DEBUG] setArrayElement: arr=%p, index=%u, length=%u\n",
		//~ (void*)arr, index, arr->length);
//~ #endif
}