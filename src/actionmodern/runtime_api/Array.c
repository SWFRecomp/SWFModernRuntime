#include <math.h>

#include <Array.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((ArrayData*) this->extra_data)->member)

void Array_init(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	ASObject* Array = getProperty(app_context, _global, STR_ID_ARRAY, NULL, 0)->value.object;
	
	ActionVar v;
	v.type = ACTION_STACK_VALUE_INT;
	
	//~ v.s32 = 0;
	//~ setProperty(app_context, Array, STR_ID_CASE_INSENSITIVE, NULL, 0, &v);
	
	RETURN_VOID();
}

void Array_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	this->extra_data = HALLOC(sizeof(ArrayData));
	
	EXTDATA(base.type) = NATIVE_ARRAY;
	
	EXTDATA(undef).type = ACTION_STACK_VALUE_UNDEFINED;
	
	if (num_args == 0)
	{
		size_t length = 0;
		size_t capacity = 8;
		
		EXTDATA(data) = HALLOC(capacity*sizeof(ActionVar));
		EXTDATA(length) = length;
		EXTDATA(capacity) = capacity;
		
		for (size_t i = 0; i < length; ++i)
		{
			EXTDATA(data)[i].type = ACTION_STACK_VALUE_UNDEFINED;
		}
		
		RETURN_VOID();
		return;
	}
	
	if (num_args == 1)
	{
		convertIntECMA(app_context);
		
		ActionVar length_v;
		popVar(app_context, &length_v);
		
		if (UNLIKELY(length_v.f64 == INFINITY || length_v.f64 == -INFINITY))
		{
			EXC("Array constructor got inf\n");
		}
		
		size_t length = (size_t) length_v.f64;
		size_t capacity = get_power_two_size(8, length);
		
		EXTDATA(data) = HALLOC(capacity*sizeof(ActionVar));
		EXTDATA(length) = length;
		EXTDATA(capacity) = capacity;
		
		for (size_t i = 0; i < length; ++i)
		{
			EXTDATA(data)[i].type = ACTION_STACK_VALUE_UNDEFINED;
		}
		
		RETURN_VOID();
		return;
	}
	
	if (num_args > 1)
	{
		UNIMPLEMENTED("Array constructor with initial arguments");
	}
	
	RETURN_VOID();
}

void Array_push(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar v;
	peekVar(app_context, &v);
	
	size_t length = ++EXTDATA(length);
	
	ENSURE_SIZE(EXTDATA(data), length, EXTDATA(capacity), sizeof(ActionVar));
	
	if (IS_OBJ_T(v.type))
	{
		OBJ_LOCK_WRITE(v.object,
		{
			retainObject(v.object);
		});
	}
	
	EXTDATA(data)[length - 1] = v;
	
	DISCARD_ARGS(num_args);
	
	releaseObjectVar(app_context, &v);
	
	f64 length_f64 = (f64) length;
	PUSH_F64(length_f64);
}

void Array_pop(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	ActionVar* data = EXTDATA(data);
	size_t length = --EXTDATA(length);
	
	if (IS_OBJ_T(data[length].type))
	{
		OBJ_LOCK_WRITE(data[length].object,
		{
			releaseObject(app_context, data[length].object);
		});
	}
	
	PUSH_VAR(&data[length]);
}

void Array_toString(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	UNIMPLEMENTED("Array toString");
	
	size_t length = EXTDATA(length);
	
	if (UNLIKELY(length == 0))
	{
		PUSH_STR_ID("", STR_ID_EMPTY, 0);
		return;
	}
	
	// add first element separately without comma
	
	//~ for (size_t i = 1; i < length; ++i)
	//~ {
		
	//~ }
}

ActionVar* Array_getElement(SWFAppContext* app_context, ASObject* this, s32 i)
{
	if (UNLIKELY(i >= EXTDATA(length)))
	{
		return &EXTDATA(undef);
	}
	
	return &(EXTDATA(data)[i]);
}

void Array_setElement(SWFAppContext* app_context, ASObject* this, s32 i, ActionVar* v)
{
	ENSURE_SIZE_FAR(EXTDATA(data), i + 1, EXTDATA(capacity), sizeof(ActionVar));
	
	if (i >= EXTDATA(length))
	{
		EXTDATA(length) = i + 1;
	}
	
	if (IS_OBJ_T(v->type))
	{
		OBJ_LOCK_WRITE(v->object,
		{
			retainObject(v->object);
		});
	}
	
	if (IS_OBJ_T(EXTDATA(data)[i].type))
	{
		OBJ_LOCK_WRITE(EXTDATA(data)[i].object,
		{
			releaseObject(app_context, EXTDATA(data)[i].object);
		});
	}
	
	EXTDATA(data)[i] = *v;
}

void Array_setLength(SWFAppContext* app_context, ASObject* this, size_t new_length)
{
	if (new_length < EXTDATA(length))
	{
		for (size_t i = new_length; i < EXTDATA(length); ++i)
		{
			ActionVar* v = &EXTDATA(data[i]);
			
			if (IS_OBJ_T(v->type))
			{
				OBJ_LOCK_WRITE(v->object,
				{
					releaseObject(app_context, v->object);
				});
			}
			
			EXTDATA(data)[i].type = ACTION_STACK_VALUE_UNDEFINED;
		}
	}
	
	EXTDATA(length) = new_length;
}

void Array_destroy(SWFAppContext* app_context, ASObject* this)
{
	for (size_t i = 0; i < EXTDATA(length); ++i)
	{
		ActionVar* v = &EXTDATA(data[i]);
		
		if (IS_OBJ_T(v->type))
		{
			OBJ_LOCK_WRITE(v->object,
			{
				releaseObject(app_context, v->object);
			});
		}
	}
	
	FREE(EXTDATA(data));
}