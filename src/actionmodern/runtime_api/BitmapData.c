#include <math.h>

#include <BitmapData.h>

#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((BitmapData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((BitmapData*) o->extra_data)->member)

void BitmapData_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	this->extra_data = HALLOC(sizeof(BitmapData));
	
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
	
	PUSH_OBJ(bitmap);
}