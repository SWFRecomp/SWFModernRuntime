#include <math.h>

#include <MovieClip.h>
#include <BitmapData.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((MovieClipData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((MovieClipData*) o->extra_data)->member)

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
	
	EXTDATA(_x) = 0.0;
	EXTDATA(_y) = 0.0;
	EXTDATA(_xscale) = 100.0;
	EXTDATA(_yscale) = 100.0;
	
	size_t capacity = 8;
	
	EXTDATA(children) = HALLOC(capacity*sizeof(ASObject*));
	EXTDATA(display_list_capacity) = capacity;
	
	for (size_t i = 0; i < capacity; ++i)
	{
		EXTDATA(children)[i] = NULL;
	}
	
	return this;
}

void MovieClip_setChild_internal(SWFAppContext* app_context, ASObject* this, u32 depth, ASObject* new_child)
{
	ASObject* old_child = EXTDATA(children)[depth];
	
	if (old_child != NULL)
	{
		OBJ_LOCK_WRITE(old_child,
		{
			releaseObject(app_context, old_child);
		});
	}
	
	EXTDATA(children)[depth] = new_child;
	
	OBJ_LOCK_WRITE(new_child,
	{
		retainObject(new_child);
	});
}

void MovieClip_placeObject2_internal(SWFAppContext* app_context, ASObject* this, u32 depth, u32 char_id, u32 transform_id)
{
	ENSURE_SIZE_FAR(EXTDATA(children), depth, EXTDATA(display_list_capacity), sizeof(ASObject*));
	
	MovieClip_setChild_internal(app_context, this, depth, MovieClip_create(app_context));
	
	EXTDATA_OF(EXTDATA(children)[depth], char_id) = char_id;
	EXTDATA_OF(EXTDATA(children)[depth], transform_id) = transform_id;
	
	if (depth > app_context->max_depth)
	{
		app_context->max_depth = depth;
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
	ENSURE_SIZE_FAR(EXTDATA(children), depth, EXTDATA(display_list_capacity), sizeof(ASObject*));
	MovieClip_setChild_internal(app_context, this, depth, bitmap);
	EXTDATA(bitmap_at) = depth;
	
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
	
	setProperty(app_context, this, name_v->string_id, NULL, 0, &mc_v);
	
	releaseObjectVar(app_context, depth_v);
	toNumber(app_context, depth_v);
	popVar(app_context, depth_v);
	
	u32 depth = (u32) depth_v->f64;
	
	if (app_context->max_depth < depth)
	{
		app_context->max_depth = depth;
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

bool MovieClip_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v)
{
	switch (string_id)
	{
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
		
		default:
		{
			return false;
		}
	}
	
	return true;
}