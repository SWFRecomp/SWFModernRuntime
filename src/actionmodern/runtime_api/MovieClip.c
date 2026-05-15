#include <math.h>

#include <MovieClip.h>

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
	
	size_t capacity = 8;
	
	EXTDATA(children) = HALLOC(capacity*sizeof(ASObject*));
	EXTDATA(display_list_capacity) = capacity;
	
	return this;
}

void MovieClip_placeObject2_internal(SWFAppContext* app_context, ASObject* this, u32 depth, u32 char_id, u32 transform_id)
{
	ENSURE_SIZE_FAR(EXTDATA(children), depth, EXTDATA(display_list_capacity), sizeof(ASObject*));
	
	EXTDATA(children)[depth] = MovieClip_create(app_context);
	
	EXTDATA_OF(EXTDATA(children)[depth], char_id) = char_id;
	EXTDATA_OF(EXTDATA(children)[depth], transform_id) = transform_id;
	
	if (depth > app_context->max_depth)
	{
		app_context->max_depth = depth;
	}
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