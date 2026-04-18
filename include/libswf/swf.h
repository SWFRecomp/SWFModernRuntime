#pragma once

#include <stackvalue.h>
#include <utils_lock.h>

#define HEAP_SIZE 1024*1024*1024  // 1 GB

#define INITIAL_DICTIONARY_CAPACITY 1024
#define INITIAL_DISPLAYLIST_CAPACITY 1024

#define STACK (app_context->stack)
#define SP (app_context->sp)
#define OLDSP (app_context->oldSP)

typedef enum
{
	CHAR_TYPE_SHAPE,
	CHAR_TYPE_TEXT,
} CharacterType;

typedef struct Character
{
	CharacterType type;
	union
	{
		// DefineShape
		struct
		{
			u32 shape_offset;
			u32 size;
		};
		// DefineText
		struct
		{
			u32 text_start;
			u32 text_size;
			u32 transform_start;
			u32 cxform_id;
		};
	};
} Character;

typedef struct DisplayObject
{
	u32 char_id;
	u32 transform_id;
} DisplayObject;

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
	recomp_mutex_t heap_lock;
	char* heap;
	size_t heap_size;
	
	size_t max_string_id;
	
	void* object_prototype;
	
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

extern int quit_swf;
extern int bad_poll;
extern size_t next_frame;
extern int manual_next_frame;

extern Character* dictionary;

extern DisplayObject* display_list;
extern size_t max_depth;

void swfStart(SWFAppContext* app_context);