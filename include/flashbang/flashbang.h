#pragma once

#include <SDL3/SDL.h>

#include <common.h>

struct FlashbangContext
{
	int width;
	int height;
	const float* stage_to_ndc;
    
	char* shape_data;
	size_t shape_data_size;
	char* transform_data;
	size_t transform_data_size;
	char* color_data;
	size_t color_data_size;
	
	SDL_Window* window;
	SDL_GPUDevice* device;
	
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* xform_buffer;
	SDL_GPUBuffer* color_buffer;
	
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
int flashbang_open_pass(FlashbangContext* context);
void flashbang_draw_shape(FlashbangContext* context, size_t offset, size_t num_verts, u32 transform_id);
void flashbang_close_pass(FlashbangContext* context);
void flashbang_free(FlashbangContext* context);