#ifdef NO_GRAPHICS

#include <stdlib.h>
#include <swf.h>
#include <tag.h>
#include <action.h>
#include <variables.h>
#include <utils.h>
#include <heap.h>

// Core runtime state - exported
int quit_swf = 0;
int is_playing = 1;
int bad_poll = 0;
size_t current_frame = 0;
size_t next_frame = 0;
int manual_next_frame = 0;
ActionVar* temp_val = NULL;

// Global frame access for ActionCall opcode
frame_func* g_frame_funcs = NULL;
size_t g_frame_count = 0;

// Drag state tracking
int is_dragging = 0;
char* dragged_target = NULL;

// Console-only swfStart implementation
void swfStart(SWFAppContext* app_context)
{
	printf("=== SWF Execution Started (NO_GRAPHICS mode) ===\n");
	
	// Allocate stack into app_context (use system malloc, not heap - stack is allocated before heap_init)
	app_context->stack = (char*) malloc(INITIAL_STACK_SIZE);
	if (!app_context->stack) {
		fprintf(stderr, "Failed to allocate stack\n");
		return;
	}
	app_context->sp = INITIAL_SP;
	app_context->oldSP = 0;
	
	// Initialize subsystems
	quit_swf = 0;
	is_playing = 1;
	bad_poll = 0;
	current_frame = 0;
	next_frame = 0;
	manual_next_frame = 0;
	
	// Store frame info globally for ActionCall opcode
	g_frame_funcs = app_context->frame_funcs;
	g_frame_count = app_context->frame_count;
	
	initTime(app_context);
	initMap();
	
	// Initialize heap allocator
	if (!heap_init(app_context, 0)) {  // 0 = use default size (64 MB)
		fprintf(stderr, "Failed to initialize heap allocator\n");
		return;
	}
	
	tagInit();
	
	// Run frames in console mode
	frame_func* funcs = app_context->frame_funcs;
	current_frame = 0;
	const size_t max_frames = 10000;
	
	while (!quit_swf && current_frame < max_frames)
	{
		printf("\n[Frame %zu]\n", current_frame);
		
		if (funcs[current_frame])
		{
			funcs[current_frame](app_context);
		}
		else
		{
			printf("No function for frame %zu, stopping.\n", current_frame);
			break;
		}
		
		// Advance to next frame
		// IMPORTANT: Process manual_next_frame BEFORE checking is_playing
		// This ensures that gotoFrame/gotoAndStop commands execute the target frame
		// even when they stop playback
		if (manual_next_frame)
		{
			current_frame = next_frame;
			manual_next_frame = 0;
		}
		else if (is_playing)
		{
			// Only advance naturally if we're still playing
			current_frame++;
		}
		else
		{
			// Stopped and no manual jump - exit loop
			break;
		}
	}
	
	printf("\n=== SWF Execution Completed ===\n");
	
	// Cleanup
	heap_shutdown(app_context);
	freeMap();
	free(app_context->stack);
}

#endif // NO_GRAPHICS