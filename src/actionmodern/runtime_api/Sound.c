#include <math.h>

#define MINIMP3_IMPLEMENTATION
#include <minimp3_ex.h>

#include <flashbang.h>
#include <heap.h>
#include <initial_strings_decls.h>
#include <Sound.h>

#define EXTDATA(member) (((SoundData*) this->extra_data)->member)

#define MPC ((SoundData*) app_context->minimp3_ctx)

void Sound_new(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	DISCARD_ARGS(num_args);
	
	this->extra_data = HALLOC(sizeof(SoundData));
	
	EXTDATA(samples) = NULL;
	EXTDATA(loaded) = false;
	
	RETURN_VOID();
}

void Sound_loadSound(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	ActionVar file_v;
	popVar(app_context, &file_v);
	ActionVar is_stream_v;
	popVar(app_context, &is_stream_v);
	
	DISCARD_ARGS(num_args - 2);
	
	mp3dec_ex_t ctx;
	
	if (mp3dec_ex_open(&ctx, file_v.str, MP3D_SEEK_TO_SAMPLE))
	{
		goto return_void;
	}
	
	if (EXTDATA(samples) != NULL)
	{
		FREE(EXTDATA(samples));
	}
	
	size_t sample_count = ctx.samples;
	EXTDATA(byte_count) = sizeof(mp3d_sample_t)*sample_count;
	EXTDATA(samples) = HALLOC(EXTDATA(byte_count));
	size_t read = mp3dec_ex_read(&ctx, (mp3d_sample_t*) EXTDATA(samples), sample_count);
	
	if (read != sample_count)
	{
		FREE(EXTDATA(samples));
		goto return_void;
	}
	
	EXTDATA(loaded) = true;
	EXTDATA(stream_id) = flashbang_create_audio_stream(FBC, app_context);
	
	PUSH_BOOL(true);
	getAndCallMethodIfExists(app_context, this, STR_ID_ON_LOAD, 1);
	POP();
	
	return_void:
	
	releaseObjectVar(app_context, &is_stream_v);
	releaseObjectVar(app_context, &file_v);
	
	RETURN_VOID();
}

void Sound_start(SWFAppContext* app_context, ASObject* this, u32 num_args)
{
	f64 offset = 0.0;
	s32 loops = 0;
	
	u32 discards = 0;
	
	if (num_args >= 1)
	{
		ActionVar offset_v;
		popVar(app_context, &offset_v);
		
		if (IS_OBJ_T(offset_v.type))
		{
			UNIMPLEMENTED("Sound.start object parameters\n");
		}
		
		convertNumericToNumber(app_context, &offset_v);
		offset = offset_v.f64;
		
		discards += 1;
	}
	
	if (num_args >= 2)
	{
		ActionVar loop_v;
		popVar(app_context, &loop_v);
		
		if (IS_OBJ_T(loop_v.type))
		{
			UNIMPLEMENTED("Sound.start object parameters\n");
		}
		
		convertNumericToInteger(app_context, &loop_v);
		loops = loop_v.s32;
		
		discards += 1;
	}
	
	DISCARD_ARGS(num_args - discards);
	
	for (s32 i = 0; i < loops + 1; ++i)
	{
		flashbang_put_audio(FBC, EXTDATA(stream_id), EXTDATA(samples), EXTDATA(byte_count));
	}
	
	RETURN_VOID();
}

void Sound_destroy(SWFAppContext* app_context, ASObject* this)
{
	FREE(EXTDATA(samples));
	
	flashbang_stop_stream(FBC, EXTDATA(stream_id));
}