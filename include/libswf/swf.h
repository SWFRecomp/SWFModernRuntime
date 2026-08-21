#pragma once

#include <stackvalue.h>
#include <context.h>
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

extern int quit_swf;
extern int bad_poll;
extern int signaled;
extern int signaled_quit;
extern size_t next_frame;
extern int manual_next_frame;

extern Character* dictionary;

extern DisplayObject* display_list;
extern size_t max_depth;

u16 swfGetExportedChar(SWFAppContext* app_context, u32 string_id);
u16 swfGetBitmapId(SWFAppContext* app_context, u32 char_id);

void swfStart(SWFAppContext* app_context);