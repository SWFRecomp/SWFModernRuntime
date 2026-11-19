#pragma once

#include <recomp.h>
#include <base.h>

#define TFO_EXTDATA_OF(o, member) (((TextFormatData*) o->extra_data)->member)

typedef struct
{
	BaseExtData base;
	
	u32 align;
	u32 color;
	u32 font;
	u32 size;
} TextFormatData;

void TextFormat_init(SWFAppContext* app_context, ASObject* this);
ASObject* TextFormat_create(SWFAppContext* app_context);

void recompTextFormat(SWFAppContext* app_context, ASObject* this, u32 num_args);
bool TextFormat_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v);
bool TextFormat_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v);