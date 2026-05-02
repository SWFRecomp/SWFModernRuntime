#pragma once

#include <swap_vector.h>
#include <utils_lock.h>

typedef struct SWFAppContext SWFAppContext;

typedef void (*frame_func)(SWFAppContext* app_context);
typedef void (*action_func)(SWFAppContext* app_context);

extern frame_func frame_funcs[];

typedef struct O1HeapInstance O1HeapInstance;

typedef struct SWFAppContext
{
	char* stack;
	u32 sp;
	u32 oldSP;
	
	u8 version;
	
	frame_func* frame_funcs;
	
	char** str_table;
	u32* str_len_table;
	
	int width;
	int height;
	
	const float* stage_to_ndc;
	
	O1HeapInstance* heap_instance;
	recomp_rwlock_t heap_lock;
	char* heap;
	size_t heap_size;
	
	bool stop_free;
	bool global_free_override;
	
	SwapVector active_objects;
	SwapVector reachable;
	SwapVector objects_to_test;
	SwapVector cycles;
	SwapVector objects_subbed;
	
	size_t max_string_id;
	
	void* object_prototype;
	void* object_constructor;
	
	size_t bitmap_count;
	size_t bitmap_highest_w;
	size_t bitmap_highest_h;
	
	char* shape_data;
	size_t shape_data_size;
	char* transform_data;
	size_t transform_data_size;
	char* color_data;
	size_t color_data_size;
	char* uninv_mat_data;
	size_t uninv_mat_data_size;
	char* gradient_data;
	size_t gradient_data_size;
	char* bitmap_data;
	size_t bitmap_data_size;
	u32* glyph_data;
	size_t glyph_data_size;
	u32* text_data;
	size_t text_data_size;
	char* cxform_data;
	size_t cxform_data_size;
} SWFAppContext;