#pragma once

#include <recomp.h>
#include <base.h>
#include <MovieClip.h>

#define TF_EXTDATA_OF(o, member) (((TextFieldData*) o->extra_data)->member)

typedef struct
{
	MovieClipData mc_base;
	
	bool embedFonts;
	u32 recompFont;
	bool multiline;
	bool selectable;
	u32 recompSize;
	u32 textColor;
	bool wordWrap;
} TextFieldData;

void TextField_init(SWFAppContext* app_context, ASObject* this);
ASObject* TextField_create(SWFAppContext* app_context);

void recompTextField(SWFAppContext* app_context, ASObject* this, u32 num_args);
bool TextField_getMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* out_v);
bool TextField_setMember(SWFAppContext* app_context, ASObject* this, u32 string_id, ActionVar* v);