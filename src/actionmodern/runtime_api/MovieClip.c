#define _USE_MATH_DEFINES
#include <math.h>

#include <MovieClip.h>
#include <BitmapData.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((MovieClipData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((MovieClipData*) o->extra_data)->member)

#define RAD(r) (r*M_PI/180.0)

void MovieClip_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	EXC("who just tried to call new MovieClip() LMFAO");
}

ASObject* MovieClip_create(SWFAppContext* app_context)
{
	ASObject* this = allocObject(app_context);
	
	ActionVar proto_v;
	proto_v.type = ACTION_STACK_VALUE_OBJECT;
	proto_v.object = app_context->MovieClip_prototype;
	setProperty(app_context, this, STR_ID_PROTO, NULL, 0, &proto_v);
	
	ActionVar ctor_v;
	ctor_v.type = ACTION_STACK_VALUE_OBJECT;
	ctor_v.object = app_context->MovieClip_constructor;
	setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &ctor_v);
	
	this->extra_data = HALLOC(sizeof(MovieClipData));
	
	EXTDATA(char_id) = 0;
	
	EXTDATA(has_tris) = false;
	EXTDATA(bitmap_at) = 0;
	
	EXTDATA(_parent) = NULL;
	EXTDATA(parent_depth) = 0;
	EXTDATA(_rotation) = 0.0;
	EXTDATA(_x) = 0.0;
	EXTDATA(_y) = 0.0;
	EXTDATA(_xscale) = 100.0;
	EXTDATA(_yscale) = 100.0;
	EXTDATA(transform) = NULL;
	
	size_t capacity = 8;
	
	EXTDATA(children) = HALLOC(capacity*sizeof(ASObject*));
	EXTDATA(max_depth) = 0;
	EXTDATA(display_list_capacity) = capacity;
	
	for (size_t i = 0; i < capacity; ++i)
	{
		EXTDATA(children)[i] = NULL;
	}
	
	return this;
}

void MovieClip_applyTransformsParents(SWFAppContext* app_context, ASObject* this, f32 mat[16])
{
	ASObject* current = EXTDATA(_parent);
	
	SwapVector parent_chain;
	
	SVEC_INIT(&parent_chain);
	SVEC_PUSH(&parent_chain, this);
	
	while (current != app_context->_root)
	{
		SVEC_PUSH(&parent_chain, current);
		
		current = EXTDATA_OF(current, _parent);
	}
	
	current = (ASObject*) SVEC_TOP(&parent_chain);
	
	f32 x = (f32) (20.0f*EXTDATA_OF(current, _x));
	f32 y = (f32) (20.0f*EXTDATA_OF(current, _y));
	f32 r = (f32) -EXTDATA_OF(current, _rotation);
	f32 sx = (f32) EXTDATA_OF(current, _xscale)/100.0f;
	f32 sy = (f32) EXTDATA_OF(current, _yscale)/100.0f;
	
	f32 t[6];
	f32 m[6];
	
	mat[0] = cosf((f32) RAD(r))*sx;
	mat[1] = sinf((f32) RAD(r))*sx;
	mat[4] = -sinf((f32) RAD(r))*sy;
	mat[5] = cosf((f32) RAD(r))*sy;
	
	mat[12] = x;
	mat[13] = y;
	
	SVEC_POP(&parent_chain);
	
	for (s64 i = parent_chain.length - 1; i >= 0; --i)
	{
		m[0] = mat[0];
		m[1] = mat[1];
		m[2] = mat[4];
		m[3] = mat[5];
		m[4] = mat[12];
		m[5] = mat[13];
		
		current = (ASObject*) parent_chain.data[i];
		
		x = (f32) (20.0f*EXTDATA_OF(current, _x));
		y = (f32) (20.0f*EXTDATA_OF(current, _y));
		r = (f32) EXTDATA_OF(current, _rotation);
		sx = (f32) EXTDATA_OF(current, _xscale)/100.0f;
		sy = (f32) EXTDATA_OF(current, _yscale)/100.0f;
		
		// row-major because i'm a disgusting pagan
		t[0] = cosf((f32) RAD(r))*sx;
		t[1] = -sinf((f32) RAD(r))*sy;
		t[2] = sinf((f32) RAD(r))*sx;
		t[3] = cosf((f32) RAD(r))*sy;
		
		// just kidding
		t[4] = x;
		t[5] = y;
		
		mat[0] = m[0]*t[0] + m[2]*t[2];
		mat[1] = m[1]*t[0] + m[3]*t[2];
		mat[4] = m[0]*t[1] + m[2]*t[3];
		mat[5] = m[1]*t[1] + m[3]*t[3];
		
		mat[12] = m[0]*t[4] + m[2]*t[5] + m[4];
		mat[13] = m[1]*t[4] + m[3]*t[5] + m[5];
	}
	
	SVEC_RELEASE(&parent_chain);
}

void MovieClip_growChildren_internal(SWFAppContext* app_context, ASObject* this, u32 depth)
{
	size_t old_capacity = EXTDATA(display_list_capacity);
	
	ENSURE_SIZE_FAR(EXTDATA(children), depth + 1, EXTDATA(display_list_capacity), sizeof(ASObject*));
	
	for (size_t i = old_capacity; i < EXTDATA(display_list_capacity); ++i)
	{
		EXTDATA(children)[i] = NULL;
	}
}

void MovieClip_setChild_internal(SWFAppContext* app_context, ASObject* this, u32 depth, ASObject* new_child)
{
	MovieClip_growChildren_internal(app_context, this, depth);
	ASObject* old_child = EXTDATA(children)[depth];
	
	OBJ_LOCK_WRITE(new_child,
	{
		retainObject(new_child);
	});
	
	if (old_child != NULL)
	{
		OBJ_LOCK_WRITE(old_child,
		{
			releaseObject(app_context, old_child);
		});
	}
	
	EXTDATA(children)[depth] = new_child;
	
	//~ ASObject* old_parent = EXTDATA_OF(new_child, _parent);
	
	//~ OBJ_LOCK_WRITE(this,
	//~ {
		//~ retainObject(this);
	//~ });
	
	//~ if (old_parent != NULL)
	//~ {
		//~ OBJ_LOCK_WRITE(old_parent,
		//~ {
			//~ releaseObject(app_context, old_parent);
		//~ });
	//~ }
	
	EXTDATA_OF(new_child, _parent) = this;
	EXTDATA_OF(new_child, parent_depth) = depth;
}

void MovieClip_removeChild_internal(SWFAppContext* app_context, ASObject* this, u32 depth)
{
	ASObject* old_child = EXTDATA(children)[depth];
	
	EXTDATA_OF(old_child, _parent) = NULL;
	EXTDATA_OF(old_child, parent_depth) = 0;
	
	// TODO: also remove old child from the rbtree
	
	if (old_child != NULL)
	{
		OBJ_LOCK_WRITE(old_child,
		{
			releaseObject(app_context, old_child);
		});
	}
	
	EXTDATA(children)[depth] = NULL;
}

void MovieClip_placeObject2_internal(SWFAppContext* app_context, ASObject* this, u32 depth, u32 char_id, u32 transform_id)
{
	MovieClip_setChild_internal(app_context, this, depth, MovieClip_create(app_context));
	
	EXTDATA_OF(EXTDATA(children)[depth], char_id) = char_id;
	EXTDATA_OF(EXTDATA(children)[depth], transform_id) = transform_id;
	
	if (depth > EXTDATA(max_depth))
	{
		EXTDATA(max_depth) = depth;
	}
}

void MovieClip_attachBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar bitmap_v;
	popVar(app_context, &bitmap_v);
	ActionVar depth_v;
	popVar(app_context, &depth_v);
	
	ASObject* bitmap = bitmap_v.object;
	
	releaseObjectVar(app_context, &depth_v);
	toNumber(app_context, &depth_v);
	popVar(app_context, &depth_v);
	
	u32 depth = (u32) depth_v.f64;
	MovieClip_setChild_internal(app_context, this, depth, bitmap);
	EXTDATA(bitmap_at) = depth;
	
	if (depth > EXTDATA(max_depth))
	{
		EXTDATA(max_depth) = depth;
	}
	
	releaseObjectVar(app_context, &depth_v);
	releaseObjectVar(app_context, &bitmap_v);
	
	DISCARD_ARGS(num_args - 2);
	
	RETURN_VOID();
}

ASObject* MovieClip_createTextField_internal(SWFAppContext* app_context, ASObject* this, ActionVar* name_v)
{
	ASObject* tf = allocObject(app_context);
	
	ActionVar tf_v;
	tf_v.type = ACTION_STACK_VALUE_OBJECT;
	tf_v.object = tf;
	
	setProperty(app_context, this, name_v->string_id, NULL, 0, &tf_v);
	
	return tf;
}

void MovieClip_createTextField(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar name_v;
	popVar(app_context, &name_v);
	
	ASObject* tf = MovieClip_createTextField_internal(app_context, this, &name_v);
	
	releaseObjectVar(app_context, &name_v);
	
	DISCARD_ARGS(num_args - 1);
	
	PUSH_OBJ(tf);
}

ASObject* MovieClip_createEmptyMovieClip_internal(SWFAppContext* app_context, ASObject* this, ActionVar* name_v, ActionVar* depth_v)
{
	ASObject* mc = MovieClip_create(app_context);
	
	ActionVar mc_v;
	mc_v.type = ACTION_STACK_VALUE_OBJECT;
	mc_v.object = mc;
	
	setProperty(app_context, this, name_v->string_id, name_v->str, name_v->str_size, &mc_v);
	
	releaseObjectVar(app_context, depth_v);
	toNumber(app_context, depth_v);
	popVar(app_context, depth_v);
	
	u32 depth = (u32) depth_v->f64;
	
	if (depth > EXTDATA(max_depth))
	{
		EXTDATA(max_depth) = depth;
	}
	
	MovieClip_setChild_internal(app_context, this, depth, mc);
	
	return mc;
}

void MovieClip_createEmptyMovieClip(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar name_v;
	popVar(app_context, &name_v);
	ActionVar depth_v;
	popVar(app_context, &depth_v);
	
	ASObject* mc = MovieClip_createEmptyMovieClip_internal(app_context, this, &name_v, &depth_v);
	
	releaseObjectVar(app_context, &depth_v);
	releaseObjectVar(app_context, &name_v);
	
	DISCARD_ARGS(num_args - 2);
	
	PUSH_OBJ(mc);
}

void MovieClip_removeMovieClip(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	ASObject* parent = EXTDATA(_parent);
	size_t depth = EXTDATA(parent_depth);
	
	MovieClip_removeChild_internal(app_context, parent, (u32) depth);
	
	RETURN_VOID();
}

void MovieClip_destroy(SWFAppContext* app_context, ASObject* this)
{
	for (size_t i = 1; i <= EXTDATA(max_depth); ++i)
	{
		ASObject* child = EXTDATA(children)[i];
		
		if (child != NULL)
		{
			OBJ_LOCK_WRITE(child,
			{
				releaseObject(app_context, child);
			});
		}
	}
	
	FREE(EXTDATA(children));
	
	if (EXTDATA(transform) != NULL)
	{
		OBJ_LOCK_WRITE(EXTDATA(transform),
		{
			releaseObject(app_context, EXTDATA(transform));
		});
	}
	
	//~ ASObject* old_parent = EXTDATA(_parent);
	
	//~ if (old_parent != NULL)
	//~ {
		//~ OBJ_LOCK_WRITE(old_parent,
		//~ {
			//~ releaseObject(app_context, old_parent);
		//~ });
	//~ }
}

bool MovieClip_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v)
{
	switch (string_id)
	{
		case STR_ID_PARENT:
		{
			out_v->type = ACTION_STACK_VALUE_OBJECT;
			out_v->object = EXTDATA(_parent);
			
			break;
		}
		
		case STR_ID__ROTATION:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(_rotation);
			
			break;
		}
		
		case STR_ID__X:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(_x);
			
			break;
		}
		
		case STR_ID__Y:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(_y);
			
			break;
		}
		
		case STR_ID__XSCALE:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(_xscale);
			
			break;
		}
		
		case STR_ID__YSCALE:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(_yscale);
			
			break;
		}
		
		case STR_ID_TRANSFORM:
		{
			if (EXTDATA(transform) == NULL)
			{
				out_v->type = ACTION_STACK_VALUE_UNDEFINED;
				
				break;
			}
			
			out_v->type = ACTION_STACK_VALUE_OBJECT;
			out_v->object = EXTDATA(transform);
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}

bool MovieClip_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v)
{
	switch (string_id)
	{
		case STR_ID_PARENT:
		{
			EXTDATA(_parent) = v->object;
			
			break;
		}
		
		case STR_ID__ROTATION:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(_rotation) = v->f64;
			
			break;
		}
		
		case STR_ID__X:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(_x) = v->f64;
			
			break;
		}
		
		case STR_ID__Y:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(_y) = v->f64;
			
			break;
		}
		
		case STR_ID__XSCALE:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(_xscale) = v->f64;
			
			break;
		}
		
		case STR_ID__YSCALE:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(_yscale) = v->f64;
			
			break;
		}
		
		case STR_ID_TRANSFORM:
		{
			OBJ_LOCK_WRITE(v->object,
			{
				retainObject(v->object);
			});
			
			if (EXTDATA(transform) != NULL)
			{
				OBJ_LOCK_WRITE(EXTDATA(transform),
				{
					releaseObject(app_context, EXTDATA(transform));
				});
			}
			
			EXTDATA(transform) = v->object;
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}