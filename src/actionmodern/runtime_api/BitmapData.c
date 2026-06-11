#include <math.h>

#include <flashbang_context.h>
#include <heap.h>
#include <initial_strings_decls.h>
#include <BitmapData.h>
#include <ColorTransform.h>

#define EXTDATA(member) (((BitmapData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((BitmapData*) o->extra_data)->member)

void BitmapData_init(SWFAppContext* app_context, ASObject* this)
{
	ActionVar proto_v;
	proto_v.type = ACTION_STACK_VALUE_OBJECT;
	proto_v.object = app_context->BitmapData_prototype;
	setProperty(app_context, this, STR_ID_PROTO, NULL, 0, &proto_v);
	
	ActionVar ctor_v;
	ctor_v.type = ACTION_STACK_VALUE_OBJECT;
	ctor_v.object = app_context->BitmapData_constructor;
	setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &ctor_v);
	
	this->extra_data = HALLOC(sizeof(BitmapData));
	
	EXTDATA(base.type) = NATIVE_BITMAP_DATA;
	
	EXTDATA(char_id) = 0;
	EXTDATA(_parent) = NULL;
	
	EXTDATA(width) = 0;
	EXTDATA(height) = 0;
}

ASObject* BitmapData_create(SWFAppContext* app_context)
{
	ASObject* this = allocObject(app_context);
	
	BitmapData_init(app_context, this);
	
	return this;
}

void BitmapData_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	BitmapData_init(app_context, this);
	
	RETURN_VOID();
}

void BitmapData_loadBitmap(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar bitmap_v;
	popVar(app_context, &bitmap_v);
	
	u32 bitmap_string_id;
	
	if (bitmap_v.string_id)
	{
		bitmap_string_id = bitmap_v.string_id;
	}
	
	else
	{
		bitmap_string_id = getStringId(app_context, bitmap_v.str, bitmap_v.str_size);
	}
	
	releaseObjectVar(app_context, &bitmap_v);
	
	DISCARD_ARGS(num_args - 1);
	
	u16 char_id = swfGetExportedChar(app_context, bitmap_string_id);
	
	ASObject* bitmap = BitmapData_create(app_context);
	bitmap->extra_data = HALLOC(sizeof(BitmapData));
	EXTDATA_OF(bitmap, char_id) = char_id;
	
	u16 bitmap_id = swfGetBitmapId(app_context, char_id);
	EXTDATA_OF(bitmap, bitmap_id) = bitmap_id;
	
	EXTDATA_OF(bitmap, width) = FBC->bitmap_sizes[2*bitmap_id];
	EXTDATA_OF(bitmap, height) = FBC->bitmap_sizes[2*bitmap_id + 1];
	
	PUSH_OBJ(bitmap);
}

bool BitmapData_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v)
{
	switch (string_id)
	{
		case STR_ID_WIDTH:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->s32 = EXTDATA(width);
			
			break;
		}
		
		case STR_ID_HEIGHT:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->s32 = EXTDATA(height);
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}