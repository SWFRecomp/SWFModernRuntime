#include <math.h>

#include <MovieClip.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((MovieClipData*) this->extra_data)->member)

ASObject* MovieClip_create(SWFAppContext* app_context)
{
	ASObject* this = allocObject(app_context);
	this->extra_data = HALLOC(sizeof(MovieClipData));
	
	return this;
}

void MovieClip_createTextField(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ASObject* tf = allocObject(app_context);
	
	ActionVar tf_v;
	tf_v.type = ACTION_STACK_VALUE_OBJECT;
	tf_v.object = tf;
	
	ActionVar name_v;
	popVar(app_context, &name_v);
	
	setProperty(app_context, this, name_v.string_id, NULL, 0, &tf_v);
	
	releaseObjectVar(app_context, &name_v);
}