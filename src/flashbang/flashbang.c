#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <common.h>
#include <flashbang.h>
#include <utils.h>

int once = 0;

FlashbangContext* flashbang_new()
{
	return malloc(sizeof(FlashbangContext));
}

void flashbang_init(FlashbangContext* context)
{
	if (!once && !SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	once = 1;
	
	context->current_bitmap = 0;
	context->bitmap_sizes = (u32*) aligned_alloc(8, 2*sizeof(u32)*context->bitmap_count);
	
	// create a window
	context->window = SDL_CreateWindow("TestSWFRecompiled", context->width, context->height, SDL_WINDOW_RESIZABLE);
	
	// create the device
	context->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
	if (context->device == NULL)
	{
		SDL_Log("Failed to create GPU device: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	// Claim the window for the GPU device
	if (!SDL_ClaimWindowForGPUDevice(context->device, context->window))
	{
		SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	SDL_GPUTransferBuffer* vertex_transfer_buffer;
	SDL_GPUTransferBuffer* xform_transfer_buffer;
	SDL_GPUTransferBuffer* color_transfer_buffer;
	SDL_GPUTransferBuffer* uninv_mat_transfer_buffer;
	SDL_GPUTransferBuffer* gradient_transfer_buffer;
	SDL_GPUTransferBuffer* dummy_transfer_buffer;
	
	// create the vertex buffer
	SDL_GPUBufferCreateInfo bufferInfo = {0};
	bufferInfo.size = (Uint32) context->shape_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	context->vertex_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a storage buffer for transform matrices
	bufferInfo.size = (Uint32) context->transform_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
	context->xform_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a storage buffer for colors
	bufferInfo.size = (Uint32) context->color_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
	context->color_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a storage buffer for gradient matrices
	bufferInfo.size = (Uint32) context->uninv_mat_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
	context->uninv_mat_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a storage buffer for inverse gradient matrices
	bufferInfo.size = (Uint32) context->uninv_mat_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
	context->inv_mat_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a storage buffer for bitmap sizes
	bufferInfo.size = (Uint32) (2*sizeof(u32)*context->bitmap_count);
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
	context->bitmap_sizes_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a transfer buffer to upload to the vertex buffer
	SDL_GPUTransferBufferCreateInfo transfer_info = {0};
	transfer_info.size = (Uint32) context->shape_data_size;
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload to the transform buffer
	transfer_info.size = (Uint32) context->transform_data_size;
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	xform_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload to the color buffer
	transfer_info.size = (Uint32) context->color_data_size;
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	color_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload to the gradient matrix buffer
	transfer_info.size = (Uint32) context->uninv_mat_data_size;
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	uninv_mat_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload to the gradient texture
	transfer_info.size = (Uint32) context->gradient_data_size;
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	gradient_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload to the bitmap texture
	transfer_info.size = (Uint32) (context->bitmap_count*(4*(context->bitmap_highest_w + 1)*(context->bitmap_highest_h + 1)));
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	context->bitmap_transfer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload bitmap sizes
	transfer_info.size = (Uint32) (2*sizeof(u32)*context->bitmap_count);
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	context->bitmap_sizes_transfer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	// create a transfer buffer to upload a dummy texture
	transfer_info.size = 4;
	transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	dummy_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transfer_info);
	
	size_t sizeof_gradient = 256*4*sizeof(float);
	size_t num_gradient_textures = context->gradient_data_size/sizeof_gradient;
	
	SDL_GPUTextureCreateInfo texture_info = {0};
	
	if (num_gradient_textures)
	{
		texture_info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
		texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		texture_info.width = 256;
		texture_info.height = 1;
		texture_info.layer_count_or_depth = (Uint32) num_gradient_textures;
		texture_info.num_levels = 1;
		texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
		
		context->gradient_tex_array = SDL_CreateGPUTexture(context->device, &texture_info);
	}
	
	texture_info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
	texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	texture_info.width = 1;
	texture_info.height = 1;
	texture_info.layer_count_or_depth = 1;
	texture_info.num_levels = 1;
	texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
	context->dummy_tex = SDL_CreateGPUTexture(context->device, &texture_info);
	
	// load the compute shader code
	size_t compute_code_size;
	void* compute_code = SDL_LoadFile("shaders/compute.spv", &compute_code_size);
	
	// create the compute pipeline
	SDL_GPUComputePipelineCreateInfo compute_pipeline_info = {0};
	compute_pipeline_info.code = (Uint8*) compute_code; //convert to an array of bytes
	compute_pipeline_info.code_size = compute_code_size;
	compute_pipeline_info.entrypoint = "main";
	compute_pipeline_info.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading .spv shaders
	compute_pipeline_info.num_samplers = 0;
	compute_pipeline_info.num_readonly_storage_buffers = 1;
	compute_pipeline_info.num_readonly_storage_textures = 0;
	compute_pipeline_info.num_readwrite_storage_buffers = 1;
	compute_pipeline_info.num_readwrite_storage_textures = 0;
	compute_pipeline_info.num_uniform_buffers = 0;
	compute_pipeline_info.threadcount_x = 64;
	compute_pipeline_info.threadcount_y = 1;
	compute_pipeline_info.threadcount_z = 1;
	
	SDL_GPUComputePipeline* compute_pipeline = SDL_CreateGPUComputePipeline(context->device, &compute_pipeline_info);
	
	// free the file
	SDL_free(compute_code);
	
	// load the vertex shader code
	size_t vertex_code_size;
	void* vertex_code = SDL_LoadFile("shaders/vertex.spv", &vertex_code_size);
	
	// create the vertex shader
	SDL_GPUShaderCreateInfo vertex_shader_info = {0};
	vertex_shader_info.code = (Uint8*) vertex_code; //convert to an array of bytes
	vertex_shader_info.code_size = vertex_code_size;
	vertex_shader_info.entrypoint = "main";
	vertex_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading .spv shaders
	vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX; // vertex shader
	vertex_shader_info.num_samplers = 0;
	vertex_shader_info.num_storage_buffers = 4;
	vertex_shader_info.num_storage_textures = 0;
	vertex_shader_info.num_uniform_buffers = 2;
	
	SDL_GPUShader* vertex_shader = SDL_CreateGPUShader(context->device, &vertex_shader_info);
	
	// free the file
	SDL_free(vertex_code);
	
	// create the fragment shader
	size_t fragment_code_size;
	void* fragment_code = SDL_LoadFile("shaders/fragment.spv", &fragment_code_size);
	
	// create the fragment shader
	SDL_GPUShaderCreateInfo fragment_shader_info = {0};
	fragment_shader_info.code = (Uint8*) fragment_code;
	fragment_shader_info.code_size = fragment_code_size;
	fragment_shader_info.entrypoint = "main";
	fragment_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
	fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT; // fragment shader
	fragment_shader_info.num_samplers = 2;
	fragment_shader_info.num_storage_buffers = 0;
	fragment_shader_info.num_storage_textures = 0;
	fragment_shader_info.num_uniform_buffers = 0;
	
	SDL_GPUShader* fragment_shader = SDL_CreateGPUShader(context->device, &fragment_shader_info);
	
	// free the file
	SDL_free(fragment_code);
	
	// define the pipeline
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {0};
	
	// bind shaders
	pipeline_info.vertex_shader = vertex_shader;
	pipeline_info.fragment_shader = fragment_shader;
	
	// we want to draw triangles
	pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	
	// describe the vertex buffers
	SDL_GPUVertexBufferDescription vertex_buffer_descriptions[1] = {0};
	vertex_buffer_descriptions[0].slot = 0;
	vertex_buffer_descriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_descriptions[0].instance_step_rate = 0;
	vertex_buffer_descriptions[0].pitch = sizeof(u32) * 4;
	
	pipeline_info.vertex_input_state.num_vertex_buffers = 1;
	pipeline_info.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
	
	// describe the vertex attribute
	SDL_GPUVertexAttribute vertex_attributes[2] = {0};
	
	// position
	vertex_attributes[0].buffer_slot = 0; // fetch data from the buffer at slot 0
	vertex_attributes[0].location = 0; // layout (location = 0) in shader
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; //vec2
	vertex_attributes[0].offset = 0; // start from the first byte from current buffer position
	
	// style
	vertex_attributes[1].buffer_slot = 0; // use buffer at slot 0
	vertex_attributes[1].location = 1; // layout (location = 1) in shader
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT2; //uvec2
	vertex_attributes[1].offset = sizeof(u32) * 2; // 3rd float from current buffer position
	
	pipeline_info.vertex_input_state.num_vertex_attributes = 2;
	pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
	
	// describe the color target
	SDL_GPUColorTargetDescription color_target_descriptions[1] = {0};
	color_target_descriptions[0].blend_state.enable_blend = true;
	color_target_descriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	color_target_descriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
	color_target_descriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	color_target_descriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_target_descriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
	color_target_descriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	color_target_descriptions[0].format = SDL_GetGPUSwapchainTextureFormat(context->device, context->window);
	
	SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_8;
	
	pipeline_info.multisample_state.sample_count = sample_count;
	
	pipeline_info.target_info.num_color_targets = 1;
	pipeline_info.target_info.color_target_descriptions = color_target_descriptions;
	
	// create the pipeline
	context->graphics_pipeline = SDL_CreateGPUGraphicsPipeline(context->device, &pipeline_info);
	
	texture_info.type = SDL_GPU_TEXTURETYPE_2D;
	texture_info.format = SDL_GetGPUSwapchainTextureFormat(context->device, context->window);
	texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	texture_info.sample_count = sample_count;
	texture_info.width = context->width;
	texture_info.height = context->height;
	texture_info.layer_count_or_depth = 1;
	texture_info.num_levels = 1;
	context->msaa_texture = SDL_CreateGPUTexture(context->device, &texture_info);
	
	texture_info.type = SDL_GPU_TEXTURETYPE_2D;
	texture_info.format = SDL_GetGPUSwapchainTextureFormat(context->device, context->window);
	texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
	texture_info.width = context->width;
	texture_info.height = context->height;
	texture_info.layer_count_or_depth = 1;
	texture_info.num_levels = 1;
	context->resolve_texture = SDL_CreateGPUTexture(context->device, &texture_info);
	
	// we don't need to store the shaders after creating the pipeline
	SDL_ReleaseGPUShader(context->device, vertex_shader);
	SDL_ReleaseGPUShader(context->device, fragment_shader);
	
	// upload all DefineShape vertex data once on init
	char* buffer = (char*) SDL_MapGPUTransferBuffer(context->device, vertex_transfer_buffer, 0);
	
	for (size_t i = 0; i < context->shape_data_size; ++i)
	{
		buffer[i] = context->shape_data[i];
	}
	
	SDL_UnmapGPUTransferBuffer(context->device, vertex_transfer_buffer);
	
	// upload all PlaceObject transform data once on init
	buffer = (char*) SDL_MapGPUTransferBuffer(context->device, xform_transfer_buffer, 0);
	
	for (size_t i = 0; i < context->transform_data_size; ++i)
	{
		buffer[i] = context->transform_data[i];
	}
	
	SDL_UnmapGPUTransferBuffer(context->device, xform_transfer_buffer);
	
	// upload all DefineShape color data once on init
	buffer = (char*) SDL_MapGPUTransferBuffer(context->device, color_transfer_buffer, 0);
	
	for (size_t i = 0; i < context->color_data_size; ++i)
	{
		buffer[i] = context->color_data[i];
	}
	
	SDL_UnmapGPUTransferBuffer(context->device, color_transfer_buffer);
	
	// clear all bitmap pixels on init
	buffer = (char*) SDL_MapGPUTransferBuffer(context->device, context->bitmap_transfer, 0);
	
	for (size_t i = 0; i < 4*(context->bitmap_highest_w + 1)*(context->bitmap_highest_h + 1)*context->bitmap_count; ++i)
	{
		buffer[i] = 0;
	}
	
	SDL_UnmapGPUTransferBuffer(context->device, context->bitmap_transfer);
	
	if (num_gradient_textures || context->bitmap_count)
	{
		// upload all DefineShape gradient matrix data once on init
		buffer = (char*) SDL_MapGPUTransferBuffer(context->device, uninv_mat_transfer_buffer, 0);
		
		for (size_t i = 0; i < context->uninv_mat_data_size; ++i)
		{
			buffer[i] = context->uninv_mat_data[i];
		}
		
		SDL_UnmapGPUTransferBuffer(context->device, uninv_mat_transfer_buffer);
	}
	
	SDL_GPUSamplerCreateInfo sampler_create_info = {0};
	
	if (num_gradient_textures)
	{
		// upload all DefineShape gradient data once on init
		buffer = (char*) SDL_MapGPUTransferBuffer(context->device, gradient_transfer_buffer, 0);
		
		for (size_t i = 0; i < context->gradient_data_size; ++i)
		{
			buffer[i] = context->gradient_data[i];
		}
		
		SDL_UnmapGPUTransferBuffer(context->device, gradient_transfer_buffer);
		
		// TODO: use different sampler address modes for different gradient spreads
		sampler_create_info.min_filter = SDL_GPU_FILTER_LINEAR;
		sampler_create_info.mag_filter = SDL_GPU_FILTER_LINEAR;
		sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
		sampler_create_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
		sampler_create_info.mip_lod_bias = 0.0f;
		sampler_create_info.max_anisotropy = 0.0f;
		sampler_create_info.compare_op = SDL_GPU_COMPAREOP_NEVER;
		sampler_create_info.min_lod = 0.0f;
		sampler_create_info.max_lod = 0.0f;
		sampler_create_info.enable_anisotropy = false;
		sampler_create_info.enable_compare = false;
		
		context->gradient_sampler = SDL_CreateGPUSampler(context->device, &sampler_create_info);
		
		assert(context->gradient_sampler != NULL);
	}
	
	buffer = (char*) SDL_MapGPUTransferBuffer(context->device, dummy_transfer_buffer, 0);
	
	for (size_t i = 0; i < 4; ++i)
	{
		buffer[i] = 0xFF;
	}
	
	SDL_UnmapGPUTransferBuffer(context->device, dummy_transfer_buffer);
	
	// acquire the command buffer
	context->command_buffer = SDL_AcquireGPUCommandBuffer(context->device);
	
	assert(context->command_buffer != NULL);
	
	// start a copy pass
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(context->command_buffer);
	
	// where is the data
	SDL_GPUTransferBufferLocation location = {0};
	location.transfer_buffer = vertex_transfer_buffer;
	location.offset = 0; // start from the beginning
	
	// where to upload the data
	SDL_GPUBufferRegion region = {0};
	region.buffer = context->vertex_buffer;
	region.size = (Uint32) context->shape_data_size; // size of the data in bytes
	region.offset = 0; // begin writing from the first byte
	
	// upload vertices
	SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
	
	// where is the data
	location.transfer_buffer = xform_transfer_buffer;
	location.offset = 0;
	
	// where to upload the data
	region.buffer = context->xform_buffer;
	region.size = (Uint32) context->transform_data_size; // size of the data in bytes
	region.offset = 0; // begin writing from the first byte
	
	// upload transforms
	SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
	
	// where is the data
	location.transfer_buffer = color_transfer_buffer;
	location.offset = 0;
	
	// where to upload the data
	region.buffer = context->color_buffer;
	region.size = (Uint32) context->color_data_size; // size of the data in bytes
	region.offset = 0; // begin writing from the first byte
	
	// upload colors
	SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
	
	// where is the texture
	SDL_GPUTextureTransferInfo texture_transfer_info = {0};
	texture_transfer_info.transfer_buffer = dummy_transfer_buffer;
	texture_transfer_info.offset = 0;
	texture_transfer_info.pixels_per_row = 0; // set as 0 to use the width
	texture_transfer_info.rows_per_layer = 0; // set as 0 to use the height
	
	// where to upload the data
	SDL_GPUTextureRegion texture_region = {0};
	texture_region.texture = context->dummy_tex;
	texture_region.mip_level = 0;
	texture_region.layer = 0;
	texture_region.x = 0;
	texture_region.y = 0;
	texture_region.z = 0;
	texture_region.w = 1;
	texture_region.h = 1;
	texture_region.d = 1;
	
	// upload dummy texture
	SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
	
	sampler_create_info.min_filter = SDL_GPU_FILTER_LINEAR;
	sampler_create_info.mag_filter = SDL_GPU_FILTER_LINEAR;
	sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	sampler_create_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.mip_lod_bias = 0.0f;
	sampler_create_info.max_anisotropy = 0.0f;
	sampler_create_info.compare_op = SDL_GPU_COMPAREOP_NEVER;
	sampler_create_info.min_lod = 0.0f;
	sampler_create_info.max_lod = 0.0f;
	sampler_create_info.enable_anisotropy = false;
	sampler_create_info.enable_compare = false;
	
	context->dummy_sampler = SDL_CreateGPUSampler(context->device, &sampler_create_info);
	
	assert(context->dummy_sampler != NULL);
	
	if (num_gradient_textures || context->bitmap_count)
	{
		// where is the data
		location.transfer_buffer = uninv_mat_transfer_buffer;
		location.offset = 0;
		
		// where to upload the data
		region.buffer = context->uninv_mat_buffer;
		region.size = (Uint32) context->uninv_mat_data_size; // size of the data in bytes
		region.offset = 0; // begin writing from the first byte
		
		// upload uninv matrices
		SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
	}
	
	for (size_t i = 0; i < num_gradient_textures; ++i)
	{
		// where is the texture
		texture_transfer_info.transfer_buffer = gradient_transfer_buffer;
		texture_transfer_info.offset = (Uint32) (i*sizeof_gradient);
		texture_transfer_info.pixels_per_row = 0; // set as 0 to use the width
		texture_transfer_info.rows_per_layer = 0; // set as 0 to use the height
		
		// where to upload the data
		texture_region.texture = context->gradient_tex_array;
		texture_region.mip_level = 0;
		texture_region.layer = (Uint32) i;
		texture_region.x = 0;
		texture_region.y = 0;
		texture_region.z = 0;
		texture_region.w = 256;
		texture_region.h = 1;
		texture_region.d = 1;
		
		// upload a gradient
		SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
	}
	
	// end the copy pass
	SDL_EndGPUCopyPass(copy_pass);
	
	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(context->command_buffer);
	SDL_WaitForGPUFences(context->device, true, &fence, 1);
	SDL_ReleaseGPUFence(context->device, fence);
	
	if (num_gradient_textures || context->bitmap_count)
	{
		context->command_buffer = SDL_AcquireGPUCommandBuffer(context->device);
		
		SDL_GPUStorageBufferReadWriteBinding compute_buffer_bindings[1] = {0};
		
		compute_buffer_bindings[0].buffer = context->inv_mat_buffer;
		compute_buffer_bindings[0].cycle = false;
		
		SDL_GPUComputePass* compute_pass = SDL_BeginGPUComputePass(context->command_buffer, NULL, 0, compute_buffer_bindings, 1);
		SDL_BindGPUComputePipeline(compute_pass, compute_pipeline);
		SDL_BindGPUComputeStorageBuffers(compute_pass, 0, &context->uninv_mat_buffer, 1);
		SDL_DispatchGPUCompute(compute_pass, (Uint32) (num_gradient_textures/64 + 1), 1, 1); // "we have ceil at home"
		SDL_EndGPUComputePass(compute_pass);
		
		// submit the command buffer
		SDL_SubmitGPUCommandBuffer(context->command_buffer);
	}
	
	SDL_ReleaseGPUTransferBuffer(context->device, vertex_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, xform_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, color_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, uninv_mat_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, gradient_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, dummy_transfer_buffer);
}

int flashbang_poll()
{
	SDL_Event evt;
	
	if (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			{
				return 1;
			}
		}
	}
	
	return 0;
}

void flashbang_set_window_background(FlashbangContext* context, u8 r, u8 g, u8 b)
{
	context->red = r;
	context->green = g;
	context->blue = b;
}

void flashbang_upload_bitmap(FlashbangContext* context, size_t offset, size_t size, u32 width, u32 height)
{
	u32* buffer = (u32*) SDL_MapGPUTransferBuffer(context->device, context->bitmap_transfer, 0);
	
	size_t bitmap_global_size = (context->bitmap_highest_w + 1)*(context->bitmap_highest_h + 1);
	size_t current_buffer_offset = bitmap_global_size*context->current_bitmap;
	
	for (size_t y = 0; y < height + 1; ++y)
	{
		for (size_t x = 0; x < width + 1; ++x)
		{
			size_t buffer_pixel = current_buffer_offset + y*(context->bitmap_highest_w + 1) + x;
			size_t bitmap_pixel = y*width + x;
			
			if (x == width)
			{
				if (y == height)
				{
					break;
				}
				
				buffer[buffer_pixel] = ((u32*) context->bitmap_data)[bitmap_pixel - 1];
				break;
			}
			
			else if (y == height)
			{
				buffer[buffer_pixel - context->bitmap_highest_w - 1] = ((u32*) context->bitmap_data)[bitmap_pixel - width];
				continue;
			}
			
			buffer[buffer_pixel] = ((u32*) context->bitmap_data)[bitmap_pixel];
		}
	}
	
	context->bitmap_sizes[2*context->current_bitmap] = width;
	context->bitmap_sizes[2*context->current_bitmap + 1] = height;
	
	context->current_bitmap += 1;
	
	SDL_UnmapGPUTransferBuffer(context->device, context->bitmap_transfer);
}

void flashbang_finalize_bitmaps(FlashbangContext* context)
{
	// upload all bitmap sizes
	u64* buffer = (u64*) SDL_MapGPUTransferBuffer(context->device, context->bitmap_sizes_transfer, 0);
	
	for (size_t i = 0; i < context->bitmap_count; ++i)
	{
		buffer[i] = ((u64*) context->bitmap_sizes)[i];
	}
	
	SDL_UnmapGPUTransferBuffer(context->device, context->bitmap_sizes_transfer);
	
	SDL_GPUTextureCreateInfo texture_info = {0};
	
	texture_info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
	texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	texture_info.width = (Uint32) (context->bitmap_highest_w + 1);
	texture_info.height = (Uint32) (context->bitmap_highest_h + 1);
	texture_info.layer_count_or_depth = (Uint32) context->bitmap_count;
	texture_info.num_levels = 1;
	texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
	
	context->bitmap_tex_array = SDL_CreateGPUTexture(context->device, &texture_info);
	
	SDL_GPUSamplerCreateInfo sampler_create_info = {0};
	
	sampler_create_info.min_filter = SDL_GPU_FILTER_LINEAR;
	sampler_create_info.mag_filter = SDL_GPU_FILTER_LINEAR;
	sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	sampler_create_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.mip_lod_bias = 0.0f;
	sampler_create_info.max_anisotropy = 0.0f;
	sampler_create_info.compare_op = SDL_GPU_COMPAREOP_NEVER;
	sampler_create_info.min_lod = 0.0f;
	sampler_create_info.max_lod = 0.0f;
	sampler_create_info.enable_anisotropy = false;
	sampler_create_info.enable_compare = false;
	
	context->bitmap_sampler = SDL_CreateGPUSampler(context->device, &sampler_create_info);
	
	assert(context->bitmap_sampler != NULL);
	
	// acquire the command buffer
	context->command_buffer = SDL_AcquireGPUCommandBuffer(context->device);
	
	assert(context->command_buffer != NULL);
	
	// start a copy pass
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(context->command_buffer);
	
	size_t bitmap_size = 4*(context->bitmap_highest_w + 1)*(context->bitmap_highest_h + 1);
	
	for (size_t i = 0; i < context->bitmap_count; ++i)
	{
		// where is the texture
		SDL_GPUTextureTransferInfo texture_transfer_info = {0};
		texture_transfer_info.transfer_buffer = context->bitmap_transfer;
		texture_transfer_info.offset = (Uint32) (i*bitmap_size);
		texture_transfer_info.pixels_per_row = 0; // set as 0 to use the width
		texture_transfer_info.rows_per_layer = 0; // set as 0 to use the height
		
		// where to upload the data
		SDL_GPUTextureRegion texture_region = {0};
		texture_region.texture = context->bitmap_tex_array;
		texture_region.mip_level = 0;
		texture_region.layer = (Uint32) i;
		texture_region.x = 0;
		texture_region.y = 0;
		texture_region.z = 0;
		texture_region.w = (Uint32) (context->bitmap_highest_w + 1);
		texture_region.h = (Uint32) (context->bitmap_highest_h + 1);
		texture_region.d = 1;
		
		// upload a bitmap
		SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
	}
	
	// where is the data
	SDL_GPUTransferBufferLocation location = {0};
	location.transfer_buffer = context->bitmap_sizes_transfer;
	location.offset = 0; // start from the beginning
	
	// where to upload the data
	SDL_GPUBufferRegion region = {0};
	region.buffer = context->bitmap_sizes_buffer;
	region.size = (Uint32) (2*sizeof(u32)*context->bitmap_count); // size of the data in bytes
	region.offset = 0; // begin writing from the first byte
	
	// upload bitmap sizes
	SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);
	
	// end the copy pass
	SDL_EndGPUCopyPass(copy_pass);
	
	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(context->command_buffer);
	SDL_WaitForGPUFences(context->device, true, &fence, 1);
	SDL_ReleaseGPUFence(context->device, fence);
}

void flashbang_open_pass(FlashbangContext* context)
{
	// acquire the command buffer
	context->command_buffer = SDL_AcquireGPUCommandBuffer(context->device);
	
	assert(context->command_buffer != NULL);
	
	// create the color target
	SDL_GPUColorTargetInfo colorTargetInfo = {0};
	colorTargetInfo.clear_color.r = context->red/255.0f;
	colorTargetInfo.clear_color.g = context->green/255.0f;
	colorTargetInfo.clear_color.b = context->blue/255.0f;
	colorTargetInfo.clear_color.a = 255/255.0f;
	colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	colorTargetInfo.store_op = SDL_GPU_STOREOP_RESOLVE;
	colorTargetInfo.texture = context->msaa_texture;
	colorTargetInfo.resolve_texture = context->resolve_texture;
	
	// begin a render pass
	context->render_pass = SDL_BeginGPURenderPass(context->command_buffer, &colorTargetInfo, 1, NULL);
	
	assert(context->render_pass != NULL);
	
	// bind the graphics pipeline
	SDL_BindGPUGraphicsPipeline(context->render_pass, context->graphics_pipeline);
	
	SDL_PushGPUVertexUniformData(context->command_buffer, 0, context->stage_to_ndc, 16*sizeof(float));
	SDL_BindGPUVertexStorageBuffers(context->render_pass, 0, &context->xform_buffer, 1);
	SDL_BindGPUVertexStorageBuffers(context->render_pass, 1, &context->color_buffer, 1);
	SDL_BindGPUVertexStorageBuffers(context->render_pass, 2, &context->inv_mat_buffer, 1);
	SDL_BindGPUVertexStorageBuffers(context->render_pass, 3, &context->bitmap_sizes_buffer, 1);
	
	size_t sizeof_gradient = 256*4*sizeof(float);
	size_t num_gradient_textures = context->gradient_data_size/sizeof_gradient;
	
	SDL_GPUTextureSamplerBinding sampler_bindings[2] = {0};
	
	if (num_gradient_textures)
	{
		sampler_bindings[0].texture = context->gradient_tex_array;
		sampler_bindings[0].sampler = context->gradient_sampler;
	}
	
	else
	{
		sampler_bindings[0].texture = context->dummy_tex;
		sampler_bindings[0].sampler = context->dummy_sampler;
	}
	
	if (context->bitmap_count)
	{
		sampler_bindings[1].texture = context->bitmap_tex_array;
		sampler_bindings[1].sampler = context->bitmap_sampler;
	}
	
	else
	{
		sampler_bindings[1].texture = context->dummy_tex;
		sampler_bindings[1].sampler = context->dummy_sampler;
	}
	
	SDL_BindGPUFragmentSamplers(context->render_pass, 0, sampler_bindings, 2);
}

void flashbang_draw_shape(FlashbangContext* context, size_t offset, size_t num_verts, u32 transform_id)
{
	// bind the vertex buffer
	SDL_GPUBufferBinding buffer_bindings[1];
	buffer_bindings[0].buffer = context->vertex_buffer;
	buffer_bindings[0].offset = (Uint32) offset*4*sizeof(u32);
	
	SDL_BindGPUVertexBuffers(context->render_pass, 0, buffer_bindings, 1);
	
	SDL_PushGPUVertexUniformData(context->command_buffer, 1, &transform_id, sizeof(u32));
	
	// issue a draw call
	SDL_DrawGPUPrimitives(context->render_pass, (Uint32) num_verts, 1, 0, 0);
}

void flashbang_close_pass(FlashbangContext* context)
{
	// end the render pass
	SDL_EndGPURenderPass(context->render_pass);
	
	// get the swapchain texture
	SDL_GPUTexture* swapchain_texture;
	Uint32 width, height;
	SDL_WaitAndAcquireGPUSwapchainTexture(context->command_buffer, context->window, &swapchain_texture, &width, &height);
	
	assert(swapchain_texture != NULL);
	
	SDL_GPUBlitInfo blit_info = {0};
	blit_info.source.texture = context->resolve_texture;
	blit_info.source.mip_level = 0;
	blit_info.source.layer_or_depth_plane = 0;
	blit_info.source.x = 0;
	blit_info.source.y = 0;
	blit_info.source.w = context->width;
	blit_info.source.h = context->height;
	
	blit_info.destination.texture = swapchain_texture;
	blit_info.destination.mip_level = 0;
	blit_info.destination.layer_or_depth_plane = 0;
	blit_info.destination.x = 0;
	blit_info.destination.y = 0;
	blit_info.destination.w = width;
	blit_info.destination.h = height;
	
	blit_info.load_op = SDL_GPU_LOADOP_DONT_CARE;
	blit_info.flip_mode = SDL_FLIP_NONE;
	blit_info.filter = SDL_GPU_FILTER_LINEAR;
	blit_info.cycle = false;
	
	SDL_BlitGPUTexture(context->command_buffer, &blit_info);
	
	// submit the command buffer
	SDL_SubmitGPUCommandBuffer(context->command_buffer);
}

void flashbang_free(FlashbangContext* context)
{
	// release the pipeline
	SDL_ReleaseGPUGraphicsPipeline(context->device, context->graphics_pipeline);
	
	// destroy the buffers
	SDL_ReleaseGPUBuffer(context->device, context->vertex_buffer);
	
	// destroy the GPU device
	SDL_DestroyGPUDevice(context->device);
	
	// destroy the window
	SDL_DestroyWindow(context->window);
	
	free(context);
}