#pragma once

#include <recomp.h>
#include <base.h>

#define TO_EXTDATA_OF(o, member) (((TomlData*) o->extra_data)->member)

typedef struct
{
	BaseExtData base;
} TomlData;

void recompToml(SWFAppContext* app_context, ASObject* this, u32 num_args);

void Toml_destroy(SWFAppContext* app_context, ASObject* this);