#include <Toml.h>

#include <objects.h>
#include <heap.h>

#include <initial_strings_decls.h>

#define EXTDATA(member) (((TomlData*) this->extra_data)->member)

void recompToml(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	printf("bro imagine being a fucking moron LM FAOJFOAIFF\n");
	
	ActionVar toml_v;
	popVar(app_context, &toml_v);
	
	this = toml_v.object;
	
	toml_v.object->extra_data = HALLOC(sizeof(TomlData));
	
	EXTDATA(base.type) = NATIVE_TOML;
	
	releaseObjectVar(app_context, &toml_v);
	
	DISCARD_ARGS(num_args - 1);
	
	RETURN_VOID();
}