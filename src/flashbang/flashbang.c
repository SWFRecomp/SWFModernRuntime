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
	SDL_GPUTransferBuffer* gradmat_transfer_buffer;
	
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
	bufferInfo.size = (Uint32) context->gradmat_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
	context->gradmat_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a storage buffer for inverse gradient matrices
	bufferInfo.size = (Uint32) context->gradmat_data_size;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
	context->inv_gradmat_buffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a transfer buffer to upload to the vertex buffer
	SDL_GPUTransferBufferCreateInfo transferInfo = {0};
	transferInfo.size = (Uint32) context->shape_data_size;
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transferInfo);
	
	// create a transfer buffer to upload to the transform buffer
	transferInfo.size = (Uint32) context->transform_data_size;
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	xform_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transferInfo);
	
	// create a transfer buffer to upload to the color buffer
	transferInfo.size = (Uint32) context->color_data_size;
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	color_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transferInfo);
	
	// create a transfer buffer to upload to the gradient matrix buffer
	transferInfo.size = (Uint32) context->gradmat_data_size;
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	gradmat_transfer_buffer = SDL_CreateGPUTransferBuffer(context->device, &transferInfo);
	
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
	vertex_shader_info.num_storage_buffers = 2;
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
	fragment_shader_info.num_samplers = 1;
	fragment_shader_info.num_storage_buffers = 0;
	fragment_shader_info.num_storage_textures = 1;
	fragment_shader_info.num_uniform_buffers = 0;
	
	SDL_GPUShader* fragment_shader = SDL_CreateGPUShader(context->device, &fragment_shader_info);
	
	// free the file
	SDL_free(fragment_code);
	
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
	compute_pipeline_info.threadcount_y = 0;
	compute_pipeline_info.threadcount_z = 0;
	
	SDL_GPUComputePipeline* compute_pipeline = SDL_CreateGPUComputePipeline(context->device, &compute_pipeline_info);
	
	// free the file
	SDL_free(compute_code);
	
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
	
	// a_position
	vertex_attributes[0].buffer_slot = 0; // fetch data from the buffer at slot 0
	vertex_attributes[0].location = 0; // layout (location = 0) in shader
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; //vec2
	vertex_attributes[0].offset = 0; // start from the first byte from current buffer position
	
	// a_color
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
	
	pipeline_info.target_info.num_color_targets = 1;
	pipeline_info.target_info.color_target_descriptions = color_target_descriptions;
	
	// create the pipeline
	context->graphics_pipeline = SDL_CreateGPUGraphicsPipeline(context->device, &pipeline_info);
	
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
	
	// acquire the command buffer
	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(context->device);
	
	assert(command_buffer != NULL);
	
	// start a copy pass
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	
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
	
	// end the copy pass
	SDL_EndGPUCopyPass(copy_pass);
	
	// submit the command buffer
	SDL_SubmitGPUCommandBuffer(command_buffer);
	
	SDL_ReleaseGPUTransferBuffer(context->device, vertex_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, xform_transfer_buffer);
	SDL_ReleaseGPUTransferBuffer(context->device, color_transfer_buffer);
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

void flashbang_open_pass(FlashbangContext* context)
{
	// acquire the command buffer
	context->command_buffer = SDL_AcquireGPUCommandBuffer(context->device);
	
	assert(context->command_buffer != NULL);
	
	// get the swapchain texture
	SDL_GPUTexture* swapchainTexture;
	Uint32 width, height;
	SDL_WaitAndAcquireGPUSwapchainTexture(context->command_buffer, context->window, &swapchainTexture, &width, &height);
	
	assert(swapchainTexture != NULL);
	
	// create the color target
	SDL_GPUColorTargetInfo colorTargetInfo = {0};
	colorTargetInfo.clear_color.r = context->red/255.0f;
	colorTargetInfo.clear_color.g = context->green/255.0f;
	colorTargetInfo.clear_color.b = context->blue/255.0f;
	colorTargetInfo.clear_color.a = 255/255.0f;
	colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
	colorTargetInfo.texture = swapchainTexture;
	
	// begin a render pass
	context->render_pass = SDL_BeginGPURenderPass(context->command_buffer, &colorTargetInfo, 1, NULL);
	
	assert(context->render_pass != NULL);
	
	// bind the graphics pipeline
	SDL_BindGPUGraphicsPipeline(context->render_pass, context->graphics_pipeline);
	
	SDL_PushGPUVertexUniformData(context->command_buffer, 0, context->stage_to_ndc, 16*sizeof(float));
	SDL_BindGPUVertexStorageBuffers(context->render_pass, 0, &context->xform_buffer, 1);
	SDL_BindGPUVertexStorageBuffers(context->render_pass, 1, &context->color_buffer, 1);
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