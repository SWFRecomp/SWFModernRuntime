#pragma once

#include <context.h>
#include <flashbang_context.h>

extern const float identity[16];
extern const float identity_cxform[20];

void flashbang_init(FlashbangContext* context, SWFAppContext* app_context);
int flashbang_poll(FlashbangContext* context, SWFAppContext* app_context);
size_t flashbang_create_audio_stream(FlashbangContext* context, SWFAppContext* app_context);
void flashbang_put_audio(FlashbangContext* context, size_t stream_id, char* buffer, size_t size);
void flashbang_stop_stream(FlashbangContext* context, size_t stream_id);
void flashbang_set_window_background(FlashbangContext* context, u8 r, u8 g, u8 b);
void flashbang_upload_bitmap(FlashbangContext* context, size_t offset, size_t size, u32 width, u32 height);
void flashbang_finalize_bitmaps(FlashbangContext* context);
void flashbang_open_pass(FlashbangContext* context, SWFAppContext* app_context);
u32 flashbang_allocate_vertices(FlashbangContext* context, u32 num_verts);
u32 flashbang_allocate_uninv(FlashbangContext* context);
void flashbang_open_vertex_transfer(FlashbangContext* context, size_t total_vertex_count, size_t total_uninv_count);
void flashbang_upload_vertices(FlashbangContext* context, u32* data, u32 upload_offset, u32 vertex_count);
void flashbang_upload_uninv(FlashbangContext* context, float* uninv, u32 offset);
void flashbang_close_vertex_transfer(FlashbangContext* context);
void flashbang_upload_extra_transform_id(FlashbangContext* context, u32 transform_id);
void flashbang_upload_extra_transform(FlashbangContext* context, float* transform);
void flashbang_upload_cxform_id(FlashbangContext* context, u32 cxform_id);
void flashbang_upload_cxform(FlashbangContext* context, float* cxform);
void flashbang_draw_shape(FlashbangContext* context, u32 offset, u32 num_verts, u32 transform_id);
void flashbang_close_pass(FlashbangContext* context, SWFAppContext* app_context);
void flashbang_release(FlashbangContext* context, SWFAppContext* app_context);