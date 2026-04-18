#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <initial_strings_decls.h>
#include <heap.h>
#include <utils.h>
#include <swap_vector.h>

#include <objects.h>

ASObject* allocObjectCommon(SWFAppContext* app_context)
{
	ASObject* obj = (ASObject*) HALLOC(sizeof(ASObject));
	
	rbtree_init(&obj->t, sizeof(ASProperty));
	obj->refcount = 0;
	obj->extra_data = NULL;
	mutex_init(&obj->lock);
	obj->reached = false;
	obj->used = false;
	obj->blocked = false;
	obj->freed = false;
	SVEC_INIT(&obj->neighbors);
	SVEC_INIT(&obj->blocked_list);
	obj->temp_rc = 0;
	
	ActionVar constructor_var;
	constructor_var.type = ACTION_STACK_VALUE_STRING;
	constructor_var.str = app_context->str_table[STR_ID_OBJECT];
	constructor_var.string_id = STR_ID_OBJECT;
	constructor_var.str_size = 6;
	constructor_var.owns_memory = false;
	setProperty(app_context, obj, STR_ID_CONSTRUCTOR, NULL, 0, &constructor_var);
	
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
	proto_var.object = (ASObject*) app_context->object_prototype;
	setProperty(app_context, obj, STR_ID_PROTO, NULL, 0, &proto_var);
	
	return obj;
}

/**
 * Object Allocation (No Prototype)
 *
 * Allocates a new ASObject (without setting its prototype) and returns it.
 */
ASObject* allocObjectNoPrototype(SWFAppContext* app_context)
{
	ASObject* obj = allocObjectCommon(app_context);
	
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
extern recomp_mutex_t object_queue_lock;

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
	
	if (obj != _global)
	{
		// queue object for free check
		queueObjectFreeCheck(app_context, obj);
	}
}

void destroyObject(SWFAppContext* app_context, ASObject* obj)
{
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
}

/**
 * Get Property
 *
 * Retrieves a property value by name.
 * Returns pointer to the ASProperty if found, or NULL if not found.
 */
ASProperty* getProperty(ASObject* this, u32 string_id, const char* name, u32 name_length)
{
	if (this == NULL || (string_id == 0 && name == NULL))
	{
		return NULL;
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
void getPropertyVar(ASObject* this, u32 string_id, const char* name, u32 name_length, ActionVar* out_var)
{
	OBJ_LOCK_READ(this,
	{
		ASProperty* p = getProperty(this, string_id, name, name_length);
		
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
	if (this == NULL || (string_id == 0 && name == NULL))
	{
		return NULL;
	}
	
	return (ASProperty*) RBT_GET_OR_INS(&this->t, string_id, created);
}

/**
 * Get Property With Prototype Chain
 *
 * Retrieves a property value by name, searching up the prototype chain via __proto__.
 * Returns pointer to ActionVar, or NULL if property not found in entire chain.
 *
 * This implements proper prototype-based inheritance for ActionScript.
 */
ASProperty* getPropertyWithPrototype(ASObject* this, u32 string_id, const char* name, u32 name_length)
{
	if (this == NULL || (string_id == 0 && name == NULL))
	{
		return NULL;
	}
	
	ASObject* current = this;
	
	while (current != NULL)
	{
		// Search own properties first
		
		ASProperty* prop;
		
		OBJ_LOCK_READ(current,
		{
			prop = getProperty(current, string_id, name, name_length);
		});
		
		if (prop != NULL)
		{
			return prop;
		}
		
		// Property not found on this object - walk up to __proto__
		
		ASProperty* proto_prop;
		
		OBJ_LOCK_READ(current,
		{
			proto_prop = getProperty(current, STR_ID_PROTO, NULL, 0);
		});
		
		if (proto_prop == NULL)
		{
			// No __proto__ property - end of chain
			break;
		}
		
		// Move to next object in prototype chain
		current = (ASObject*) proto_prop->value.object;
	}
	
	return NULL;  // Property not found in entire prototype chain
}

/**
 * Set Property
 *
 * Sets a property value by name. Creates property if it doesn't exist.
 * Handles reference counting if value is an object.
 */
void setProperty(SWFAppContext* app_context, ASObject* this, u32 string_id, const char* name, u32 name_length, ActionVar* value)
{
	if (this == NULL || (string_id == 0 && name == NULL) || value == NULL)
	{
		return;
	}
	
	ASProperty* p;
	
	OBJ_LOCK_READ(this,
	{
		p = getProperty(this, string_id, name, name_length);
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
	
	OBJ_LOCK_READ(this,
	{
		// Property doesn't exist - create new one
		p = (ASProperty*) RBT_GET_OR_INS(&this->t, string_id, &created);
	});
	
	OBJ_LOCK_WRITE(this,
	{
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
ASObject* getConstructor(ASObject* obj)
{
	if (obj == NULL)
	{
		return NULL;
	}
	
	// Look for "constructor" property
	static const char* constructor_name = "constructor";
	ActionVar* ctor = &getProperty(obj, 0, constructor_name, 11)->value;
	
	if (ctor != NULL && ctor->type == ACTION_STACK_VALUE_OBJECT)
	{
		return (ASObject*) ctor->value;
	}
	
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
	if (arr == NULL)
	{
		return;
	}
	
	arr->refcount--;
	
	if (arr->refcount == 0)
	{
		// Release all element values
		for (u32 i = 0; i < arr->length; i++)
		{
			// If element is an object, release it recursively
			if (arr->elements[i].type == ACTION_STACK_VALUE_OBJECT)
			{
				ASObject* child_obj = (ASObject*) arr->elements[i].value;
				releaseObject(app_context, child_obj);
			}
			// If element is an array, release it recursively
			else if (arr->elements[i].type == ACTION_STACK_VALUE_ARRAY)
			{
				ASArray* child_arr = (ASArray*) arr->elements[i].value;
				releaseArray(app_context, child_arr);
			}
			// If element is a string that owns memory, free it
			else if (arr->elements[i].type == ACTION_STACK_VALUE_STRING &&
			         arr->elements[i].owns_memory)
			{
				free(arr->elements[i].str);
			}
		}
		
		// Free element array
		if (arr->elements != NULL)
		{
			free(arr->elements);
		}
		
		// Free array itself
		free(arr);
	}
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
	if (arr == NULL || value == NULL)
	{
		return;
	}

	// Grow array if needed
	if (index >= arr->capacity)
	{
		u32 new_capacity = (index + 1) * 2;  // Grow to accommodate index
		ActionVar* new_elements = (ActionVar*) realloc(arr->elements,
		                                                sizeof(ActionVar) * new_capacity);
		if (new_elements == NULL)
		{
			fprintf(stderr, "ERROR: Failed to grow array\n");
			return;
		}

		arr->elements = new_elements;

		// Zero out new slots
		memset(&arr->elements[arr->capacity], 0,
		       sizeof(ActionVar) * (new_capacity - arr->capacity));

		arr->capacity = new_capacity;
	}

	// Release old value if it exists and is an object/array
	if (index < arr->length)
	{
		if (arr->elements[index].type == ACTION_STACK_VALUE_OBJECT)
		{
			ASObject* old_obj = (ASObject*) arr->elements[index].value;
			releaseObject(app_context, old_obj);
		}
		else if (arr->elements[index].type == ACTION_STACK_VALUE_ARRAY)
		{
			ASArray* old_arr = (ASArray*) arr->elements[index].value;
			releaseArray(app_context, old_arr);
		}
		else if (arr->elements[index].type == ACTION_STACK_VALUE_STRING &&
		         arr->elements[index].owns_memory)
		{
			free(arr->elements[index].str);
		}
	}

	// Set new value
	arr->elements[index] = *value;

	// Update length if needed
	if (index >= arr->length)
	{
		arr->length = index + 1;
	}

	// Retain new value if it's an object or array
	if (value->type == ACTION_STACK_VALUE_OBJECT)
	{
		ASObject* new_obj = (ASObject*) value->value;
		retainObject(new_obj);
	}
	else if (value->type == ACTION_STACK_VALUE_ARRAY)
	{
		ASArray* new_arr = (ASArray*) value->value;
		retainArray(new_arr);
	}

#ifdef DEBUG
	printf("[DEBUG] setArrayElement: arr=%p, index=%u, length=%u\n",
		(void*)arr, index, arr->length);
#endif
}
