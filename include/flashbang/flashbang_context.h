#pragma once

#include <common.h>

#define FBC ((FlashbangContext*) app_context->fbc)

typedef struct
{
	int width;
	int height;
	
	const float* stage_to_ndc;
	
	size_t bitmap_count;
	size_t bitmap_highest_w;
	size_t bitmap_highest_h;
	
	size_t current_bitmap;
	u32* bitmap_sizes;
	
	bool shape_data_exists;
	
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
	char* cxform_data;
	size_t cxform_data_size;
	
	void* copy_pass;
	
	size_t allocated_vertex_size;
	size_t allocated_vertex_transfer_size;
	size_t current_vertex_offset;
	size_t vertices_uploading_count;
	void* vertex_transfer_buffer;
	char* vertex_buffer_mapped;
	
	void* vertex_buffer_to_free;
	void* transfer_buffer_to_free;
	
	size_t allocated_uninv_size;
	size_t allocated_uninv_transfer_size;
	size_t current_uninv_offset;
	size_t uninvs_uploading_count;
	void* uninv_transfer_buffer;
	char* uninv_buffer_mapped;
	
	void* uninv_buffer_to_free;
	void* uninv_transfer_buffer_to_free;
	void* inv_buffer_to_free;
	
	size_t allocated_transform_size;
	size_t allocated_transform_transfer_size;
	size_t current_transform_offset;
	size_t transform_uploading_count;
	void* transform_transfer_buffer;
	char* transform_transfer_mapped;
	
	void* transform_buffer_to_free;
	void* transform_transfer_buffer_to_free;
	
	void* window;
	void* device;
	
	void* dummy_tex;
	void* dummy_sampler;
	
	void* vertex_buffer;
	void* xform_buffer;
	void* color_buffer;
	void* uninv_mat_buffer;
	void* inv_mat_buffer;
	void* bitmap_sizes_buffer;
	void* cxform_buffer;
	
	void* gradient_tex_array;
	void* gradient_sampler;
	
	void* bitmap_transfer;
	void* bitmap_sizes_transfer;
	void* bitmap_tex_array;
	void* bitmap_sampler;
	
	void* graphics_pipeline;
	void* inv_pipeline;
	void* mult_pipeline;
	
	void* command_buffer;
	void* render_pass;
	
	// Window background color
	u8 red;
	u8 green;
	u8 blue;
} FlashbangContext;