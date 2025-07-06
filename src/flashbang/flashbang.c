#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <common.h>

SDL_Window* window;
SDL_GPUDevice* device;

// Window background color
u8 red;
u8 green;
u8 blue;

void flashbang_init()
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	// create a window
	window = SDL_CreateWindow("TestSWFRecompiled", 800, 600, SDL_WINDOW_RESIZABLE);
	
	// create the device
	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
	if (device == NULL)
	{
		SDL_Log("Failed to create GPU device: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	// Claim the window for the GPU device
	if (!SDL_ClaimWindowForGPUDevice(device, window))
	{
		SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
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

void flashbang_set_window_background(u8 r, u8 g, u8 b)
{
	red = r;
	green = g;
	blue = b;
}

void flashbang_draw()
{
	// acquire the command buffer
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	
	assert(commandBuffer != NULL);
	
	// get the swapchain texture
	SDL_GPUTexture* swapchainTexture;
	Uint32 width, height;
	SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height);
	
	// skip rendering the frame if a swapchain texture is not available
	if (swapchainTexture == NULL)
	{
		SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
	}
	
	else
	{
		// create the color target
		SDL_GPUColorTargetInfo colorTargetInfo = {0};
		colorTargetInfo.clear_color.r = red/255.0f;
		colorTargetInfo.clear_color.g = green/255.0f;
		colorTargetInfo.clear_color.b = blue/255.0f;
		colorTargetInfo.clear_color.a = 255/255.0f;
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
		colorTargetInfo.texture = swapchainTexture;
		
		// begin a render pass
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
		
		assert(renderPass != NULL);
		
		// end the render pass
		SDL_EndGPURenderPass(renderPass);
	}
	
	// submit the command buffer
	SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void flashbang_quit()
{
	// destroy the GPU device
	SDL_DestroyGPUDevice(device);
	
	// destroy the window
	SDL_DestroyWindow(window);
}