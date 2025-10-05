#pragma once

#include <SDL3/SDL.h>

#include <common.h>

struct FlashbangContext
{
	int width;
	int height;
	
	const float* stage_to_ndc;
	
	size_t bitmap_count;
	size_t bitmap_highest_w;
	size_t bitmap_highest_h;
	
	size_t current_bitmap;
	u32* bitmap_sizes;
	
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
	
	SDL_Window* window;
	SDL_GPUDevice* device;
	
	SDL_GPUTexture* dummy_tex;
	SDL_GPUSampler* dummy_sampler;
	
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* xform_buffer;
	SDL_GPUBuffer* color_buffer;
	SDL_GPUBuffer* uninv_mat_buffer;
	SDL_GPUBuffer* inv_mat_buffer;
	SDL_GPUBuffer* bitmap_sizes_buffer;
	
	SDL_GPUTexture* gradient_tex_array;
	SDL_GPUSampler* gradient_sampler;
	
	SDL_GPUTransferBuffer* bitmap_transfer;
	SDL_GPUTransferBuffer* bitmap_sizes_transfer;
	SDL_GPUTexture* bitmap_tex_array;
	SDL_GPUSampler* bitmap_sampler;
	
	SDL_GPUTexture* msaa_texture;
	SDL_GPUTexture* resolve_texture;
	
	SDL_GPUGraphicsPipeline* graphics_pipeline;
	
	SDL_GPUCommandBuffer* command_buffer;
	SDL_GPURenderPass* render_pass;
	
	// Window background color
	u8 red;
	u8 green;
	u8 blue;
};

typedef struct FlashbangContext FlashbangContext;

FlashbangContext* flashbang_new();
void flashbang_init(FlashbangContext* context);
int flashbang_poll();
void flashbang_set_window_background(FlashbangContext* context, u8 r, u8 g, u8 b);
void flashbang_upload_bitmap(FlashbangContext* context, size_t offset, size_t size, u32 width, u32 height);
void flashbang_finalize_bitmaps(FlashbangContext* context);
void flashbang_open_pass(FlashbangContext* context);
void flashbang_draw_shape(FlashbangContext* context, size_t offset, size_t num_verts, u32 transform_id);
void flashbang_close_pass(FlashbangContext* context);
void flashbang_free(FlashbangContext* context);