#ifdef NO_GRAPHICS

#include <swf.h>
#include <tag.h>
#include <action.h>
#include <variables.h>
#include <utils.h>

// Core runtime state - exported
char* stack = NULL;
u32 sp = 0;
u32 oldSP = 0;

int quit_swf = 0;
int bad_poll = 0;
size_t next_frame = 0;
int manual_next_frame = 0;
ActionVar* temp_val = NULL;

// Console-only swfStart implementation
void swfStart(SWFAppContext* app_context)
{
	printf("=== SWF Execution Started (NO_GRAPHICS mode) ===\n");

	// Allocate stack
	stack = (char*) aligned_alloc(8, INITIAL_STACK_SIZE);
	if (!stack) {
		fprintf(stderr, "Failed to allocate stack\n");
		return;
	}
	sp = INITIAL_SP;

	// Initialize subsystems
	quit_swf = 0;
	bad_poll = 0;
	next_frame = 0;
	manual_next_frame = 0;

	initTime();
	initMap();
	tagInit();

	// Run frames in console mode
	frame_func* funcs = app_context->frame_funcs;
	size_t current_frame = 0;
	const size_t max_frames = 10000;

	while (!quit_swf && current_frame < max_frames)
	{
		printf("\n[Frame %zu]\n", current_frame);

		if (funcs[current_frame])
		{
			funcs[current_frame]();
		}
		else
		{
			printf("No function for frame %zu, stopping.\n", current_frame);
			break;
		}

		if (manual_next_frame)
		{
			current_frame = next_frame;
			manual_next_frame = 0;
		}
		else
		{
			current_frame++;
		}
	}

	printf("\n=== SWF Execution Completed ===\n");

	// Cleanup
	freeMap();
	aligned_free(stack);
}

#endif // NO_GRAPHICS
