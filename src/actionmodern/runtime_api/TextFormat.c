#include <heap.h>
#include <TextFormat.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((TextFormatData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((TextFormatData*) o->extra_data)->member)

void TextFormat_init(SWFAppContext* app_context, ASObject* this)
{
	this->extra_data = HALLOC(sizeof(TextFormatData));
	EXTDATA(base.type) = NATIVE_TEXTFORMAT;
	
	EXTDATA(color) = 0x000000;
}

ASObject* TextFormat_create(SWFAppContext* app_context)
{
	ASObject* this = allocObject(app_context);
	TextFormat_init(app_context, this);
	
	return this;
}

void recompTextFormat(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar tf_v;
	popVar(app_context, &tf_v);
	
	this = tf_v.object;
	
	TextFormat_init(app_context, this);
	
	releaseObjectVar(app_context, &tf_v);
	
	DISCARD_ARGS(num_args - 1);
	
	RETURN_VOID();
}

bool TextFormat_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v)
{
	switch (string_id)
	{
		case STR_ID_align:
		{
			out_v->type = ACTION_STACK_VALUE_STRING;
			out_v->string_id = EXTDATA(align);
			out_v->str = app_context->str_table[EXTDATA(align)];
			out_v->str_size = app_context->str_len_table[EXTDATA(align)];
			
			break;
		}
		
		case STR_ID_color:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->u32 = EXTDATA(color);
			
			break;
		}
		
		case STR_ID_font:
		{
			out_v->type = ACTION_STACK_VALUE_STRING;
			out_v->string_id = EXTDATA(font);
			out_v->str = app_context->str_table[EXTDATA(font)];
			out_v->str_size = app_context->str_len_table[EXTDATA(font)];
			
			break;
		}
		
		case STR_ID_size:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->u32 = EXTDATA(size);
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}

bool TextFormat_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v)
{
	switch (string_id)
	{
		case STR_ID_align:
		{
			EXTDATA(align) = v->string_id;
			
			break;
		}
		
		case STR_ID_color:
		{
			convertNumericToInteger(app_context, v);
			EXTDATA(color) = v->u32;
			
			break;
		}
		
		case STR_ID_font:
		{
			EXTDATA(font) = v->string_id;
			
			break;
		}
		
		case STR_ID_size:
		{
			convertNumericToInteger(app_context, v);
			EXTDATA(size) = v->u32;
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}