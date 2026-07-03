#include <heap.h>
#include <ColorTransform.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((ColorTransformData*) this->extra_data)->member)
#define EXTDATA_OF(o, member) (((ColorTransformData*) o->extra_data)->member)

void recompColorTransform(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar ct_v;
	popVar(app_context, &ct_v);
	
	this = ct_v.object;
	
	ct_v.object->extra_data = HALLOC(sizeof(ColorTransformData));
	
	EXTDATA(base.type) = NATIVE_COLOR_TRANSFORM;
	
	releaseObjectVar(app_context, &ct_v);
	
	DISCARD_ARGS(num_args - 1);
	
	RETURN_VOID();
}

bool ColorTransform_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v)
{
	switch (string_id)
	{
		case STR_ID_RED_MULTIPLIER:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(rm);
			
			break;
		}
		
		case STR_ID_GREEN_MULTIPLIER:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(gm);
			
			break;
		}
		
		case STR_ID_BLUE_MULTIPLIER:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(bm);
			
			break;
		}
		
		case STR_ID_ALPHA_MULTIPLIER:
		{
			out_v->type = ACTION_STACK_VALUE_F64;
			out_v->f64 = EXTDATA(am);
			
			break;
		}
		
		case STR_ID_RED_OFFSET:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->s32 = EXTDATA(ro);
			
			break;
		}
		
		case STR_ID_GREEN_OFFSET:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->s32 = EXTDATA(go);
			
			break;
		}
		
		case STR_ID_BLUE_OFFSET:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->s32 = EXTDATA(bo);
			
			break;
		}
		
		case STR_ID_ALPHA_OFFSET:
		{
			out_v->type = ACTION_STACK_VALUE_INT;
			out_v->s32 = EXTDATA(ao);
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}

bool ColorTransform_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v)
{
	switch (string_id)
	{
		case STR_ID_RED_MULTIPLIER:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(rm) = v->f64;
			
			break;
		}
		
		case STR_ID_GREEN_MULTIPLIER:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(gm) = v->f64;
			
			break;
		}
		
		case STR_ID_BLUE_MULTIPLIER:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(bm) = v->f64;
			
			break;
		}
		
		case STR_ID_ALPHA_MULTIPLIER:
		{
			convertNumericToNumber(app_context, v);
			EXTDATA(am) = v->f64;
			
			break;
		}
		
		case STR_ID_RED_OFFSET:
		{
			convertNumericToInteger(app_context, v);
			EXTDATA(ro) = v->s32;
			
			break;
		}
		
		case STR_ID_GREEN_OFFSET:
		{
			convertNumericToInteger(app_context, v);
			EXTDATA(go) = v->s32;
			
			break;
		}
		
		case STR_ID_BLUE_OFFSET:
		{
			convertNumericToInteger(app_context, v);
			EXTDATA(bo) = v->s32;
			
			break;
		}
		
		case STR_ID_ALPHA_OFFSET:
		{
			convertNumericToInteger(app_context, v);
			EXTDATA(ao) = v->s32;
			
			break;
		}
		
		default:
		{
			return false;
		}
	}
	
	return true;
}