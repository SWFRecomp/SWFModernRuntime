#pragma once

#include <common.h>

typedef struct FlashbangContext FlashbangContext;

FlashbangContext* flashbang_new();
void flashbang_init(FlashbangContext* context, int width, int height);
int flashbang_poll();
void flashbang_set_window_background(FlashbangContext* context, u8 r, u8 g, u8 b);
char* flashbang_map_vertex_transfer_buffer(FlashbangContext* context);
char* flashbang_map_xform_transfer_buffer(FlashbangContext* context);
void flashbang_upload_vertices(FlashbangContext* context, char* buffer, char* tris, size_t tris_size);
void flashbang_upload_xform(FlashbangContext* context, char* buffer, char* xform);
void flashbang_unmap_vertex_transfer_buffer(FlashbangContext* context);
void flashbang_unmap_xform_transfer_buffer(FlashbangContext* context);
void flashbang_draw(FlashbangContext* context);
void flashbang_free(FlashbangContext* context);