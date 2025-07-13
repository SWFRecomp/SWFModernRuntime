#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <common.h>
#include <flashbang.h>

int once = 0;

struct FlashbangContext
{
	SDL_Window* window;
	SDL_GPUDevice* device;
	
	SDL_GPUBuffer* vertexBuffer;
	SDL_GPUTransferBuffer* transferBuffer;
	
	size_t current_data_offset;
	
	SDL_GPUGraphicsPipeline* graphicsPipeline;
	
	// Window background color
	u8 red;
	u8 green;
	u8 blue;
};

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
	
	context->current_data_offset = 0;
	
	// create a window
	context->window = SDL_CreateWindow("TestSWFRecompiled", 800, 600, SDL_WINDOW_RESIZABLE);
	
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
	
	// create the vertex buffer
	SDL_GPUBufferCreateInfo bufferInfo = {0};
	bufferInfo.size = 7*1024;
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	context->vertexBuffer = SDL_CreateGPUBuffer(context->device, &bufferInfo);
	
	// create a transfer buffer to upload to the vertex buffer
	SDL_GPUTransferBufferCreateInfo transferInfo = {0};
	transferInfo.size = 7*1024;
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	context->transferBuffer = SDL_CreateGPUTransferBuffer(context->device, &transferInfo);
	
	// load the vertex shader code
	size_t vertexCodeSize;
	void* vertexCode = SDL_LoadFile("shaders/vertex.spv", &vertexCodeSize);
	
	// create the vertex shader
	SDL_GPUShaderCreateInfo vertexInfo = {0};
	vertexInfo.code = (Uint8*) vertexCode; //convert to an array of bytes
	vertexInfo.code_size = vertexCodeSize;
	vertexInfo.entrypoint = "main";
	vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading .spv shaders
	vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX; // vertex shader
	vertexInfo.num_samplers = 0;
	vertexInfo.num_storage_buffers = 0;
	vertexInfo.num_storage_textures = 0;
	vertexInfo.num_uniform_buffers = 0;
	SDL_GPUShader* vertexShader = SDL_CreateGPUShader(context->device, &vertexInfo);
	
	// free the file
	SDL_free(vertexCode);
	
	// create the fragment shader
	size_t fragmentCodeSize;
	void* fragmentCode = SDL_LoadFile("shaders/fragment.spv", &fragmentCodeSize);
	
	// create the fragment shader
	SDL_GPUShaderCreateInfo fragmentInfo = {0};
	fragmentInfo.code = (Uint8*) fragmentCode;
	fragmentInfo.code_size = fragmentCodeSize;
	fragmentInfo.entrypoint = "main";
	fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT; // fragment shader
	fragmentInfo.num_samplers = 0;
	fragmentInfo.num_storage_buffers = 0;
	fragmentInfo.num_storage_textures = 0;
	fragmentInfo.num_uniform_buffers = 0;
	
	SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(context->device, &fragmentInfo);
	
	// free the file
	SDL_free(fragmentCode);
	
	// define the pipeline
	SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};
	
	// bind shaders
	pipelineInfo.vertex_shader = vertexShader;
	pipelineInfo.fragment_shader = fragmentShader;
	
	// we want to draw triangles
	pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	
	// describe the vertex buffers
	SDL_GPUVertexBufferDescription vertexBufferDescriptions[1] = {0};
	vertexBufferDescriptions[0].slot = 0;
	vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertexBufferDescriptions[0].instance_step_rate = 0;
	vertexBufferDescriptions[0].pitch = sizeof(float) * 7;
	
	pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
	pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescriptions;
	
	// describe the vertex attribute
	SDL_GPUVertexAttribute vertexAttributes[2] = {0};
	
	// a_position
	vertexAttributes[0].buffer_slot = 0; // fetch data from the buffer at slot 0
	vertexAttributes[0].location = 0; // layout (location = 0) in shader
	vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; //vec3
	vertexAttributes[0].offset = 0; // start from the first byte from current buffer position
	
	// a_color
	vertexAttributes[1].buffer_slot = 0; // use buffer at slot 0
	vertexAttributes[1].location = 1; // layout (location = 1) in shader
	vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; //vec4
	vertexAttributes[1].offset = sizeof(float) * 3; // 4th float from current buffer position
	
	pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
	pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;
	
	// describe the color target
	SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {0};
    colorTargetDescriptions[0].blend_state.enable_blend = true;
    colorTargetDescriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDescriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTargetDescriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTargetDescriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(context->device, context->window);
	
	pipelineInfo.target_info.num_color_targets = 1;
	pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;
	
	// create the pipeline
	context->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(context->device, &pipelineInfo);
	
	// we don't need to store the shaders after creating the pipeline
	SDL_ReleaseGPUShader(context->device, vertexShader);
	SDL_ReleaseGPUShader(context->device, fragmentShader);
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

void flashbang_upload_tris(FlashbangContext* context, char* tris, size_t tris_size)
{
	// map the transfer buffer to a pointer
	char* data = (char*) SDL_MapGPUTransferBuffer(context->device, context->transferBuffer, false);
	
	SDL_memcpy(data + context->current_data_offset, tris, tris_size);
	
	// unmap the pointer when you are done updating the transfer buffer
	SDL_UnmapGPUTransferBuffer(context->device, context->transferBuffer);
	
	context->current_data_offset += tris_size;
}

void flashbang_draw(FlashbangContext* context)
{
	// acquire the command buffer
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(context->device);
	
	assert(commandBuffer != NULL);
	
	// get the swapchain texture
	SDL_GPUTexture* swapchainTexture;
	Uint32 width, height;
	SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, context->window, &swapchainTexture, &width, &height);
	
	// skip rendering the frame if a swapchain texture is not available
	if (swapchainTexture == NULL)
	{
		SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
	}
	
	else
	{
		// start a copy pass
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
		
		// where is the data
		SDL_GPUTransferBufferLocation location = {0};
		location.transfer_buffer = context->transferBuffer;
		location.offset = 0; // start from the beginning
		
		// where to upload the data
		SDL_GPUBufferRegion region = {0};
		region.buffer = context->vertexBuffer;
		region.size = (Uint32) context->current_data_offset; // size of the data in bytes
		
		region.offset = 0; // begin writing from the first vertex
		
		// upload the data
		SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
		
		// end the copy pass
		SDL_EndGPUCopyPass(copyPass);
		
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
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
		
		assert(renderPass != NULL);
		
		// bind the graphics pipeline
		SDL_BindGPUGraphicsPipeline(renderPass, context->graphicsPipeline);
		
		// bind the vertex buffer
		SDL_GPUBufferBinding bufferBindings[1];
		bufferBindings[0].buffer = context->vertexBuffer; // index 0 is slot 0 in this example
		bufferBindings[0].offset = 0; // start from the first byte
		
		SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1); // bind one buffer starting from slot 0
		
		Uint32 num_verts = (Uint32) context->current_data_offset/(7*sizeof(float));
		
		// issue a draw call
		SDL_DrawGPUPrimitives(renderPass, num_verts, num_verts/3, 0, 0);
		
		// end the render pass
		SDL_EndGPURenderPass(renderPass);
		
		context->current_data_offset = 0;
	}
	
	// submit the command buffer
	SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void flashbang_free(FlashbangContext* context)
{
	// release the pipeline
	SDL_ReleaseGPUGraphicsPipeline(context->device, context->graphicsPipeline);
	
	// destroy the buffers
	SDL_ReleaseGPUBuffer(context->device, context->vertexBuffer);
	SDL_ReleaseGPUTransferBuffer(context->device, context->transferBuffer);
	
	// destroy the GPU device
	SDL_DestroyGPUDevice(context->device);
	
	// destroy the window
	SDL_DestroyWindow(context->window);
	
	free(context);
}