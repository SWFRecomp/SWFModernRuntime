#pragma once

#include <stackvalue.h>

// Forward declaration for o1heap
typedef struct O1HeapInstance O1HeapInstance;

#define HEAP_SIZE 1024*1024*1024  // 1 GB

#ifndef NO_GRAPHICS
#define INITIAL_DICTIONARY_CAPACITY 1024
#define INITIAL_DISPLAYLIST_CAPACITY 1024

// Character type enum for shapes and text
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
			size_t shape_offset;
			size_t size;
		};
		// DefineText
		struct
		{
			size_t text_start;
			size_t text_size;
			u32 transform_start;
			u32 cxform_id;
		};
	};
} Character;

typedef struct DisplayObject
{
	size_t char_id;
	u32 transform_id;
} DisplayObject;
#endif

// Forward declaration for SWFAppContext (needed for frame_func typedef)
typedef struct SWFAppContext SWFAppContext;

// Frame function now takes app_context parameter
typedef void (*frame_func)(SWFAppContext* app_context);

extern frame_func frame_funcs[];

// Macros for stack access via app_context
#define STACK (app_context->stack)
#define SP (app_context->sp)
#define OLDSP (app_context->oldSP)

typedef struct SWFAppContext
{
	// Stack management (moved from globals)
	char* stack;
	u32 sp;
	u32 oldSP;

	frame_func* frame_funcs;
	size_t frame_count;  // Local addition - kept for compatibility

#ifndef NO_GRAPHICS
	int width;
	int height;

	const float* stage_to_ndc;

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

	// Font/Text data (from upstream)
	u32* glyph_data;
	size_t glyph_data_size;
	u32* text_data;
	size_t text_data_size;
	char* cxform_data;
	size_t cxform_data_size;
#endif

	// Heap management fields
	O1HeapInstance* heap_instance;
	char* heap;
	size_t heap_size;
	size_t heap_full_size;
	size_t heap_current_size;
	int heap_inited;

	// String ID support (from upstream)
	size_t max_string_id;
} SWFAppContext;

extern int quit_swf;
extern int is_playing;
extern size_t current_frame;
extern size_t next_frame;
extern int manual_next_frame;

// Global frame access for ActionCall opcode
extern frame_func* g_frame_funcs;
extern size_t g_frame_count;

// Drag state tracking (works in both graphics and NO_GRAPHICS modes)
extern int is_dragging;         // 1 if a sprite is being dragged, 0 otherwise
extern char* dragged_target;    // Name of the target being dragged (or NULL)

#ifndef NO_GRAPHICS
extern Character* dictionary;

extern DisplayObject* display_list;
extern size_t max_depth;
#endif

void swfStart(SWFAppContext* app_context);