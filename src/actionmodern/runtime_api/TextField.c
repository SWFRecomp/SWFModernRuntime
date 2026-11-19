#include <heap.h>
#include <TextField.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((TextFieldData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((TextFieldData*) o->extra_data)->member)

void TextField_init(SWFAppContext* app_context, ASObject* this)
{
	this->extra_data = HALLOC(sizeof(TextFieldData));
	EXTDATA(mc_base.base.type) = NATIVE_TEXTFIELD;
	
	ActionVar proto_v;
	proto_v.type = ACTION_STACK_VALUE_OBJECT;
	proto_v.object = app_context->TextField_prototype;
	setProperty(app_context, this, STR_ID_PROTO, NULL, 0, &proto_v);
	
	ActionVar ctor_v;
	ctor_v.type = ACTION_STACK_VALUE_OBJECT;
	ctor_v.object = app_context->TextField_constructor;
	setProperty(app_context, this, STR_ID_CONSTRUCTOR, NULL, 0, &ctor_v);
	
	EXTDATA(mc_base._visible) = true;
	EXTDATA(embedFonts) = false;
	EXTDATA(multiline) = false;
	EXTDATA(selectable) = true;
	EXTDATA(wordWrap) = false;
}

ASObject* TextField_create(SWFAppContext* app_context)
{
	ASObject* this = allocObject(app_context);
	TextField_init(app_context, this);
	
	return this;
}

void recompTextField(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar tf_v;
	popVar(app_context, &tf_v);
	
	this = tf_v.object;
	
	TextField_init(app_context, this);
	
	releaseObjectVar(app_context, &tf_v);
	
	DISCARD_ARGS(num_args - 1);
	
	RETURN_VOID();
}

bool TextField_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v)
{
	switch (string_id)
	{
		case STR_ID__visible:
		{
			out_v->type = ACTION_STACK_VALUE_BOOLEAN;
			out_v->b = EXTDATA(mc_base._visible);
			
			break;
		}
		
		case STR_ID_embedFonts:
		{
			out_v->type = ACTION_STACK_VALUE_BOOLEAN;
			out_v->b = EXTDATA(embedFonts);
			
			break;
		}
		
		case STR_ID_recompFont:
		{
			setStringFromId(app_context, out_v, EXTDATA(recompFont));
			
			break;
		}
		
		case STR_ID_multiline:
		{
			out_v->type = ACTION_STACK_VALUE_BOOLEAN;
			out_v->b = EXTDATA(multiline);
			
			break;
		}
		
		case STR_ID_selectable:
		{
			out_v->type = ACTION_STACK_VALUE_BOOLEAN;
			out_v->b = EXTDATA(selectable);
			
			break;
		}
		
		case STR_ID_recompSize:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->u32 = EXTDATA(recompSize);
			
			break;
		}
		
		case STR_ID_textColor:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->u32 = EXTDATA(textColor);
			
			break;
		}
		
		case STR_ID_wordWrap:
		{
			out_v->type = ACTION_STACK_VALUE_BOOLEAN;
			out_v->b = EXTDATA(wordWrap);
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}

bool TextField_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v)
{
	switch (string_id)
	{
		case STR_ID__visible:
		{
			EXTDATA(mc_base._visible) = v->b;
			
			break;
		}
		
		case STR_ID_embedFonts:
		{
			EXTDATA(embedFonts) = v->b;
			
			break;
		}
		
		case STR_ID_recompFont:
		{
			EXTDATA(recompFont) = v->string_id;
			
			break;
		}
		
		case STR_ID_multiline:
		{
			EXTDATA(multiline) = v->b;
			
			break;
		}
		
		case STR_ID_selectable:
		{
			EXTDATA(selectable) = v->b;
			
			break;
		}
		
		case STR_ID_recompSize:
		{
			EXTDATA(recompSize) = v->u32;
			
			break;
		}
		
		case STR_ID_textColor:
		{
			EXTDATA(textColor) = v->u32;
			
			break;
		}
		
		case STR_ID_wordWrap:
		{
			EXTDATA(wordWrap) = v->b;
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}