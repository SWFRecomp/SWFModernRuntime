#include <math.h>

#include <flashbang_context.h>
#include <heap.h>
#include <initial_strings_decls.h>
#include <BitmapData.h>

#define EXTDATA(member) (((BitmapData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((BitmapData*) o->extra_data)->member)

//~ ASObject* BitmapData_create(SWFAppContext* app_context)
//~ {
	//~ ASObject* this = allocObject(app_context);
	
	//~ ActionVar proto_v;
	//~ proto_v.type = ACTION_STACK_VALUE_OBJECT;
	//~ proto_v.object = app_context->BitmapData_prototype;
	//~ setProperty(app_context, this, STR_ID_PROTO, NULL, 0, &proto_v);
	
	//~ ActionVar ctor_v;
	//~ ctor_v.type = ACTION_STACK_VALUE_OBJECT;
	//~ ctor_v.object = app_context->MovieClip_constructor;
	//~ setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &ctor_v);
	
	//~ this->extra_data = HALLOC(sizeof(MovieClipData));
	
	//~ size_t capacity = 8;
	
	//~ EXTDATA(children) = HALLOC(capacity*sizeof(ASObject*));
	//~ EXTDATA(display_list_capacity) = capacity;
	
	//~ return this;
//~ }

void BitmapData_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	this->extra_data = HALLOC(sizeof(BitmapData));
	
	EXTDATA(width) = 0;
	EXTDATA(height) = 0;
	EXTDATA(char_id) = 0;
	
	RETURN_VOID();
}

void BitmapData_loadBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar bitmap_v;
	popVar(app_context, &bitmap_v);
	
	u32 bitmap_string_id = bitmap_v.string_id;
	
	releaseObjectVar(app_context, &bitmap_v);
	
	DISCARD_ARGS(num_args - 1);
	
	u16 char_id = swfGetExportedChar(app_context, bitmap_string_id);
	
	ASObject* bitmap = allocObject(app_context);
	bitmap->extra_data = HALLOC(sizeof(BitmapData));
	EXTDATA_OF(bitmap, char_id) = char_id;
	
	EXTDATA_OF(bitmap, width) = FBC->bitmap_sizes[0];
	EXTDATA_OF(bitmap, height) = FBC->bitmap_sizes[1];
	
	PUSH_OBJ(bitmap);
}