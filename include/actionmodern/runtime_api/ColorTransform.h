#pragma once

#include <recomp.h>
#include <base.h>

#define CT_EXTDATA_OF(o, member) (((ColorTransformData*) o->extra_data)->member)

typedef struct
{
	BaseExtData base;
	
	f64 rm;
	f64 gm;
	f64 bm;
	f64 am;
	s16 ro;
	s16 go;
	s16 bo;
	s16 ao;
} ColorTransformData;

void recompColorTransform(SWFAppContext* app_context, ASObject* this, u32 num_args);
bool ColorTransform_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v);
bool ColorTransform_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v);