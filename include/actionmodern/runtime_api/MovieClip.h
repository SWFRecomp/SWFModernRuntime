#pragma once

#include <recomp.h>

#define MC_EXTDATA(member) (((MovieClipData*) this->extra_data)->member)

typedef struct
{
	ASObject* children;
	
	bool _multiline;
	bool _visible;
} MovieClipData;

ASObject* MovieClip_create(SWFAppContext* app_context);