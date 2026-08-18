#include <math.h>

#include <yyjson.h>

#include <objects.h>
#include <flashbang.h>
#include <toplevel.h>

#include <initial_strings_decls.h>

void recompJSON(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	FILE* docf = fopen("test.json", "rb");
	fseek(docf, 0, SEEK_END);
	size_t len = ftell(docf);
	fseek(docf, 0, SEEK_SET);
	
	char text[1024];
	fread(text, 1, len, docf);
	text[len] = '\0';
	fclose(docf);
	
	printf("text: %s\n", text);
	
	yyjson_doc* doc = yyjson_read(text, len, 0);
	yyjson_val* root = yyjson_doc_get_root(doc);
	yyjson_val* test = yyjson_obj_get(root, "test");
	printf("test: %s\n", yyjson_get_str(test));
	
	RETURN_VOID();
}