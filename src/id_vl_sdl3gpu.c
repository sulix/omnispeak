/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2017 Omnispeak Project Authors
Commander Keen created by id Software

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "assert.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdlib.h>
#include <string.h>
#include "id_mm.h"
#include "id_vl.h"
#include "id_vl_private.h"
#include "id_us.h"
#include "ck_cross.h"

#define VL_SDL3GPU_NUM_BUFFERS 2

static uint32_t vert_spv[] =
#include "id_vl_vk_vert.h"
	;

static uint32_t frag_spv[] =
#include "id_vl_vk_frag.h"
	;

SDL_Window *vl_sdl3_window;
static int vl_sdl3gpu_framebufferWidth, vl_sdl2vk_framebufferHeight;
static int vl_sdl3gpu_screenWidth;
static int vl_sdl3gpu_screenHeight;
static struct
{
	int left, right, top, bottom;
} vl_sdl3gpu_scaledBorders;
static int vl_sdl3gpu_screenHorizScaleFactor;
static int vl_sdl3gpu_screenVertScaleFactor;

static SDL_GPUDevice *vl_sdl3gpu_device;

static SDL_GPUShader *vl_sdl3gpu_vertShader;
static SDL_GPUShader *vl_sdl3gpu_fragShader;

static SDL_GPUGraphicsPipeline *vl_sdl3gpu_pipeline;

static SDL_GPUSampler *vl_sdl3gpu_sampler;

static int vl_sdl3gpu_integerWidth;
static int vl_sdl3gpu_integerHeight;

static SDL_GPUTexture *vl_sdl3gpu_integerFramebuffer;

static SDL_GPUPresentMode vl_sdl3gpu_supportedNoVsyncMode;

static bool vl_sdl3gpu_flushSwapchain;


static void VL_SDL3GPU_CreateShaders()
{
	SDL_GPUShaderCreateInfo vertShaderCreateInfo = {};
	vertShaderCreateInfo.code = (const Uint8 *)vert_spv;
	vertShaderCreateInfo.code_size = sizeof(vert_spv);
	vertShaderCreateInfo.entrypoint = "main";
	vertShaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	vertShaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
	vertShaderCreateInfo.num_samplers = 0;

	vl_sdl3gpu_vertShader = SDL_CreateGPUShader(vl_sdl3gpu_device, &vertShaderCreateInfo);

	SDL_GPUShaderCreateInfo fragShaderCreateInfo = {};
	fragShaderCreateInfo.code = (const Uint8 *)frag_spv;
	fragShaderCreateInfo.code_size = sizeof(frag_spv);
	fragShaderCreateInfo.entrypoint = "main";
	fragShaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	fragShaderCreateInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	fragShaderCreateInfo.num_storage_textures = 1;
	fragShaderCreateInfo.num_uniform_buffers = 1;

	vl_sdl3gpu_fragShader = SDL_CreateGPUShader(vl_sdl3gpu_device, &fragShaderCreateInfo);
}

static void VL_SDL3GPU_CreatePipeline()
{
	SDL_GPUColorTargetDescription colorDesc = {};
	colorDesc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;//SDL_GPUGetSwapchainTextureFormat(vl_sdl3gpu_device, vl_sdl3_window);
	colorDesc.blend_state.enable_blend = true;
	colorDesc.blend_state.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B;
	colorDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_MAX;
	colorDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
	colorDesc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	colorDesc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	colorDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
	colorDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;


	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.target_info.num_color_targets = 1;
	pipelineCreateInfo.target_info.color_target_descriptions = &colorDesc;

	pipelineCreateInfo.vertex_input_state.num_vertex_attributes = 0;
	pipelineCreateInfo.vertex_input_state.num_vertex_buffers = 0;

	pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
	pipelineCreateInfo.vertex_shader = vl_sdl3gpu_vertShader;
	pipelineCreateInfo.fragment_shader = vl_sdl3gpu_fragShader;

	pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

	pipelineCreateInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
	pipelineCreateInfo.multisample_state.sample_mask = 0xFFFF;

	vl_sdl3gpu_pipeline = SDL_CreateGPUGraphicsPipeline(vl_sdl3gpu_device, &pipelineCreateInfo);
}

static void VL_SDL3GPU_CreateFramebuffers()
{
	SDL_GPUTextureCreateInfo createInfo = { };
	createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM; //SDL_GPUGetSwapchainTextureFormat(vl_sdl3gpu_device, vl_sdl3_window);
	createInfo.width = vl_integerWidth;
	createInfo.height = vl_integerHeight;
	createInfo.layer_count_or_depth= 1;
	createInfo.num_levels = 1;
	createInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Creating SDL GPU integer framebuffer of size (%d×%d)\n", vl_integerWidth, vl_integerHeight);

	vl_sdl3gpu_integerFramebuffer = SDL_CreateGPUTexture(vl_sdl3gpu_device, &createInfo);

	SDL_SetGPUTextureName(vl_sdl3gpu_device, vl_sdl3gpu_integerFramebuffer, "integerFramebuffer");
}

/* TODO (Overscan border):
 * - If a texture is used for offscreen rendering with scaling applied later,
 * it's better to have the borders within the texture itself.
 */

typedef struct VL_SDL3GPU_Surface
{
	VL_SurfaceUsage use;
	SDL_GPUTexture *texture;
	SDL_GPUTransferBuffer *transferBuffers[VL_SDL3GPU_NUM_BUFFERS];
	int w, h;
	int pitch;
	int activePage;
	void *data;
	void *bufferMaps[VL_SDL3GPU_NUM_BUFFERS];
} VL_SDL3GPU_Surface;

void VL_SDL2GL_SetIcon(SDL_Window *wnd);

void VL_SDL3GPU_FlushParams()
{
	SDL_SetWindowFullscreen(vl_sdl3_window, vl_isFullScreen ? SDL_WINDOW_FULLSCREEN : 0);
	SDL_SetGPUSwapchainParameters(vl_sdl3gpu_device, vl_sdl3_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, (vl_swapInterval == 0) ? vl_sdl3gpu_supportedNoVsyncMode : SDL_GPU_PRESENTMODE_VSYNC);
	vl_sdl3gpu_flushSwapchain = true;
}


static void VL_SDL3GPU_SetVideoMode(int mode)
{
	if (mode == 0xD)
	{
		SDL_Rect desktopBounds;
		int scale = 3;
		const SDL_DisplayID *displayIDs = SDL_GetDisplays(NULL);
		if (displayIDs && !SDL_GetDisplayUsableBounds(*displayIDs, &desktopBounds))
		{
			CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Couldn't get usable display bounds to calculate default scale: \"%s\"\n", SDL_GetError());
		}
		else
		{
			scale = VL_CalculateDefaultWindowScale(desktopBounds.w, desktopBounds.h);
		}
		vl_sdl3_window = SDL_CreateWindow(VL_WINDOW_TITLE,
			VL_DEFAULT_WINDOW_WIDTH(scale), VL_DEFAULT_WINDOW_HEIGHT(scale),
			SDL_WINDOW_RESIZABLE | (vl_isFullScreen ? SDL_WINDOW_FULLSCREEN : 0));

		SDL_SetWindowMinimumSize(vl_sdl3_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);

		VL_SDL2GL_SetIcon(vl_sdl3_window);

		SDL_PropertiesID props = SDL_CreateProperties();
		vl_sdl3gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

		if (!vl_sdl3gpu_device)
			Quit("Couldn't create SDL GPU device!");

		SDL_ClaimWindowForGPUDevice(vl_sdl3gpu_device, vl_sdl3_window);

		// Prefer MAILBOX to VSYNC if we can.
		vl_sdl3gpu_supportedNoVsyncMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
		if (SDL_WindowSupportsGPUPresentMode(vl_sdl3gpu_device, vl_sdl3_window, SDL_GPU_PRESENTMODE_MAILBOX))
			vl_sdl3gpu_supportedNoVsyncMode = SDL_GPU_PRESENTMODE_MAILBOX;

		VL_SDL3GPU_FlushParams();

		// Compile the shader we use to emulate EGA palettes.
		VL_SDL3GPU_CreateShaders();
		VL_SDL3GPU_CreatePipeline();

		VL_CalculateRenderRegions(VL_DEFAULT_WINDOW_WIDTH(scale), VL_DEFAULT_WINDOW_HEIGHT(scale));
		VL_SDL3GPU_CreateFramebuffers();


		// Hide mouse cursor
		SDL_HideCursor();
	}
	else
	{
		SDL_ShowCursor();
		SDL_ReleaseGPUTexture(vl_sdl3gpu_device, vl_sdl3gpu_integerFramebuffer);
		SDL_ReleaseGPUGraphicsPipeline(vl_sdl3gpu_device, vl_sdl3gpu_pipeline);
		SDL_ReleaseGPUShader(vl_sdl3gpu_device, vl_sdl3gpu_fragShader);
		SDL_ReleaseGPUShader(vl_sdl3gpu_device, vl_sdl3gpu_vertShader);
		SDL_DestroyGPUDevice(vl_sdl3gpu_device);

		SDL_DestroyWindow(vl_sdl3_window);
	}
}


static void VL_SDL3GPU_SetSurfacePage(VL_SDL3GPU_Surface *surf, int page)
{
	if (surf->use != VL_SurfaceUsage_FrontBuffer)
		Quit("Tried to set page for a single buffered surface!");

	surf->activePage = page;
	surf->data = surf->bufferMaps[page];
}

static void VL_SDL3GPU_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL3GPU_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)malloc(sizeof(VL_SDL3GPU_Surface));
	surf->w = w;
	surf->h = h;
	surf->use = usage;
	if (usage == VL_SurfaceUsage_FrontBuffer)
	{
		SDL_GPUTextureCreateInfo texCreateInfo = { };
		texCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
		texCreateInfo.width = w;
		texCreateInfo.height = h;
		texCreateInfo.layer_count_or_depth= 1;
		texCreateInfo.num_levels = 1;
		texCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ;

		surf->texture = SDL_CreateGPUTexture(vl_sdl3gpu_device, &texCreateInfo);

		SDL_GPUTransferBufferCreateInfo transferCreateInfo = { };
		transferCreateInfo.size = w * h;
		transferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

		for (int i = 0; i < VL_SDL3GPU_NUM_BUFFERS; ++i)
		{
			surf->transferBuffers[i] = SDL_CreateGPUTransferBuffer(vl_sdl3gpu_device, &transferCreateInfo);
			surf->bufferMaps[i] = SDL_MapGPUTransferBuffer(vl_sdl3gpu_device, surf->transferBuffers[i], true);
		}
		VL_SDL3GPU_SetSurfacePage(surf, 0);
		surf->pitch = w;
	}
	else
	{
		surf->data = malloc(w * h); // 8-bit pal for now
		surf->pitch = w;
	}
	return surf;
}

static void VL_SDL3GPU_UploadSurface(void *surface)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;

	SDL_GPUCommandBuffer *uploadCmdBuf = SDL_AcquireGPUCommandBuffer(vl_sdl3gpu_device);
	SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

	SDL_GPUTextureTransferInfo transferInfo = { 0 };
	transferInfo.pixels_per_row = surf->pitch;
	transferInfo.rows_per_layer = surf->h;
	transferInfo.offset = 0;
	transferInfo.transfer_buffer = surf->transferBuffers[surf->activePage];

	SDL_GPUTextureRegion transferRegion = { 0 };
	transferRegion.x = 0;
	transferRegion.y = 0;
	transferRegion.z = 0;
	transferRegion.w = surf->w;
	transferRegion.h = surf->h;
	transferRegion.texture = surf->texture;
	transferRegion.d = 1;

	SDL_UploadToGPUTexture(copyPass, &transferInfo, &transferRegion, true);

	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
}

static void VL_SDL3GPU_DestroySurface(void *surface)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		for (int i = 0; i < VL_SDL3GPU_NUM_BUFFERS; ++i)
		{
			SDL_UnmapGPUTransferBuffer(vl_sdl3gpu_device, surf->transferBuffers[i]);
			SDL_ReleaseGPUTransferBuffer(vl_sdl3gpu_device, surf->transferBuffers[i]);
			surf->bufferMaps[i] = NULL;
		}
		surf->data = 0;
		SDL_ReleaseGPUTexture(vl_sdl3gpu_device, surf->texture);
	}
	else
	{
		if (surf->data)
			free(surf->data);
	}
	free(surf);
}

static long VL_SDL3GPU_GetSurfaceMemUse(void *surface)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
		return surf->w * surf->h * VL_SDL3GPU_NUM_BUFFERS;
	return surf->w * surf->h;
}

static void VL_SDL3GPU_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_SDL3GPU_SetGLClearColorFromBorder(void)
{
	/*glClearColor((GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0]/255,
		     (GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1]/255,
		     (GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2]/255,
		     1.0f
	);*/
}

static void VL_SDL3GPU_RefreshPaletteAndBorderColor(void *screen)
{
	static uint8_t sdl3gpu_palette[16][3];

	for (int i = 0; i < 16; i++)
	{
		memcpy(sdl3gpu_palette[i], VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]], 3);
	}
	// Load the palette into a texture.

	VL_SDL3GPU_SetGLClearColorFromBorder();
}

static int VL_SDL3GPU_SurfacePGet(void *surface, int x, int y)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;
	return ((uint8_t *)surf->data)[y * surf->pitch + x];
}

static void VL_SDL3GPU_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		memset(((uint8_t *)surf->data) + _y * surf->pitch + x, colour, w);
	}
}

static void VL_SDL3GPU_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	mapmask &= 0xF;
	colour &= mapmask;

	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		for (int _x = x; _x < x + w; ++_x)
		{
			uint8_t *p = ((uint8_t *)surf->data) + _y * surf->pitch + _x;
			*p &= ~mapmask;
			*p |= colour;
		}
	}
}

static void VL_SDL3GPU_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)src_surface;
	VL_SDL3GPU_Surface *dest = (VL_SDL3GPU_Surface *)dst_surface;
	for (int _y = sy; _y < sy + sh; ++_y)
	{
		memcpy(((uint8_t *)dest->data) + (_y - sy + y) * dest->pitch + x, ((uint8_t *)surf->data) + _y * surf->pitch + sx, sw);
	}
}

static void VL_SDL3GPU_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL3GPU_Surface *srf = (VL_SDL3GPU_Surface *)surface;
	bool directionX = sx > x;
	bool directionY = sy > y;

	if (directionY)
	{
		for (int yi = 0; yi < sh; ++yi)
		{
			memmove(((uint8_t *)srf->data) + ((yi + y) * srf->pitch + x), ((uint8_t *)srf->data) + ((sy + yi) * srf->pitch + sx), sw);
		}
	}
	else
	{
		for (int yi = sh - 1; yi >= 0; --yi)
		{
			memmove(((uint8_t *)srf->data) + ((yi + y) * srf->pitch + x), ((uint8_t *)srf->data) + ((sy + yi) * srf->pitch + sx), sw);
		}
	}
}

static void VL_SDL3GPU_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_UnmaskedToPAL8(src, surf->data, x, y, surf->pitch, w, h);
}

static void VL_SDL3GPU_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_UnmaskedToPAL8_PM(src, surf->data, x, y, surf->pitch, w, h, mapmask);
}

static void VL_SDL3GPU_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_MaskedToPAL8(src, surf->data, x, y, surf->pitch, w, h);
}

static void VL_SDL3GPU_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_MaskedBlitClipToPAL8(src, surf->data, x, y, surf->pitch, w, h, surf->w, surf->h);
}

static void VL_SDL3GPU_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_1bppToPAL8(src, surf->data, x, y, surf->pitch, w, h, colour);
}

static void VL_SDL3GPU_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_1bppToPAL8_PM(src, surf->data, x, y, surf->pitch, w, h, colour, mapmask);
}

static void VL_SDL3GPU_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_1bppXorWithPAL8(src, surf->data, x, y, surf->pitch, w, h, colour);
}

static void VL_SDL3GPU_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_1bppBlitToPAL8(src, surf->data, x, y, surf->pitch, w, h, colour);
}

static void VL_SDL3GPU_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)dst_surface;
	VL_1bppInvBlitClipToPAL8(src, surf->data, x, y, surf->pitch, w, h, surf->w, surf->h, colour);
}

static void VL_SDL3GPU_ScrollSurface(void *surface, int x, int y)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;
	int dx = 0, dy = 0, sx = 0, sy = 0;
	int w = surf->w - CK_Cross_max(x, -x), h = surf->h - CK_Cross_max(y, -y);
	if (x > 0)
	{
		dx = 0;
		sx = x;
	}
	else
	{
		dx = -x;
		sx = 0;
	}
	if (y > 0)
	{
		dy = 0;
		sy = y;
	}
	else
	{
		dy = -y;
		sy = 0;
	}

	int oldPage = surf->activePage;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		for (int i = 0; i < VL_SDL3GPU_NUM_BUFFERS; ++i)
		{
			VL_SDL3GPU_SetSurfacePage(surf, i);
			VL_SDL3GPU_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
		}
		VL_SDL3GPU_SetSurfacePage(surf, oldPage);
	}
	else

	VL_SDL3GPU_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_SDL3GPU_BindTexture(VL_SDL3GPU_Surface *surf, SDL_GPURenderPass *renderPass)
{
	SDL_GPUTextureSamplerBinding binding = {};
	binding.texture = surf->texture;
	binding.sampler = vl_sdl3gpu_sampler;

	SDL_BindGPUFragmentStorageTextures(renderPass, 0, &surf->texture, 1);
	//SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);
}

static void VL_SDL3GPU_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	SDL_GPUTexture *swapchainTexture;
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;

	uint32_t newW, newH;
	VL_SDL3GPU_UploadSurface(surface);
	SDL_GPUCommandBuffer *renderBuffer = SDL_AcquireGPUCommandBuffer(vl_sdl3gpu_device);
	if (!SDL_AcquireGPUSwapchainTexture(renderBuffer, vl_sdl3_window, &swapchainTexture, &newW, &newH) || !swapchainTexture)
	{
		SDL_SubmitGPUCommandBuffer(renderBuffer);
		return;
	}

	if (newW != vl_sdl3gpu_screenWidth || newH != vl_sdl3gpu_screenHeight || vl_sdl3gpu_flushSwapchain)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Resizing swapchain: %d -> %d, %d -> %d\n", vl_sdl3gpu_screenWidth, vl_sdl3gpu_screenHeight, newW, newH);
		vl_sdl3gpu_screenWidth = newW;
		vl_sdl3gpu_screenHeight = newH;
		VL_CalculateRenderRegions(newW, newH);
		SDL_ReleaseGPUTexture(vl_sdl3gpu_device, vl_sdl3gpu_integerFramebuffer);
		VL_SDL3GPU_CreateFramebuffers();
		vl_sdl3gpu_flushSwapchain = false;
	}

	SDL_GPUColorTargetInfo innerAttachmentInfo = {};
	innerAttachmentInfo.clear_color.r = VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0] / 255.f;
	innerAttachmentInfo.clear_color.g = VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1] / 255.f;
	innerAttachmentInfo.clear_color.b = VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2] / 255.f;
	innerAttachmentInfo.clear_color.a = 1.f;
	innerAttachmentInfo.cycle = true;
	innerAttachmentInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	innerAttachmentInfo.store_op = SDL_GPU_STOREOP_STORE;
	innerAttachmentInfo.texture = vl_sdl3gpu_integerFramebuffer;

	SDL_GPURenderPass *innerPass = SDL_BeginGPURenderPass(renderBuffer, &innerAttachmentInfo, 1, NULL);

	SDL_BindGPUGraphicsPipeline(innerPass, vl_sdl3gpu_pipeline);
	VL_SDL3GPU_BindTexture((VL_SDL3GPU_Surface *)surface, innerPass);

	SDL_GPUViewport renderRgn;
	renderRgn.x = vl_renderRgn_x;
	renderRgn.y = vl_renderRgn_y;
	renderRgn.w = vl_renderRgn_w;
	renderRgn.h = vl_renderRgn_h;
	renderRgn.max_depth = 1.f;
	renderRgn.min_depth= 0.f;

	SDL_SetGPUViewport(innerPass, &renderRgn);
	//TODO: Do we need to set a scissor rect as well?

	float data[4 * 17];
	data[0] = scrlX;
	data[1] = scrlY;
	for (int i = 0; i < 16; ++i)
	{
		data[4 + 4 * i] = (float)VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][0] / 255.0;
		data[5 + 4 * i] = (float)VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][1] / 255.0;
		data[6 + 4 * i] = (float)VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][2] / 255.0;
		data[7 + 4 * i] = 255.0;
	}
	SDL_PushGPUFragmentUniformData(renderBuffer, 0, data, sizeof(float) * 4 * 17);

	SDL_DrawGPUPrimitives(innerPass, 4, 1, 0, 0);

	SDL_EndGPURenderPass(innerPass);

	SDL_GPUBlitInfo blitInfo = {};
	blitInfo.source.texture = vl_sdl3gpu_integerFramebuffer;
	blitInfo.source.x = 0;
	blitInfo.source.y = 0;
	blitInfo.source.w = vl_integerWidth;
	blitInfo.source.h = vl_integerHeight;

	blitInfo.destination.texture = swapchainTexture;
	blitInfo.destination.x = vl_fullRgn_x;
	blitInfo.destination.y = vl_fullRgn_y;
	blitInfo.destination.w = vl_fullRgn_w;
	blitInfo.destination.h = vl_fullRgn_h;
	blitInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	blitInfo.clear_color.r = 0.f;
	blitInfo.clear_color.g = 0.f;
	blitInfo.clear_color.b = 0.f;
	blitInfo.clear_color.a = 1.f;
	blitInfo.cycle = true;
	blitInfo.filter = SDL_GPU_FILTER_LINEAR;

	//TODO: We need a way to ensure the framebuffer is cleared first?
	SDL_BlitGPUTexture(renderBuffer, &blitInfo);

	SDL_SubmitGPUCommandBuffer(renderBuffer);

	if (!singleBuffered && surf->use == VL_SurfaceUsage_FrontBuffer)
		VL_SDL3GPU_SetSurfacePage(surf, (surf->activePage + 1) % VL_SDL3GPU_NUM_BUFFERS);

}

static int VL_SDL3GPU_GetActiveBufferId(void *surface)
{
	VL_SDL3GPU_Surface *srf = (VL_SDL3GPU_Surface*)surface;
	return srf->activePage;
}

static int VL_SDL3GPU_GetNumBuffers(void *surface)
{
	(void)surface;
	return VL_SDL3GPU_NUM_BUFFERS;
}

static void VL_SDL3GPU_SyncBuffers(void *surface)
{
	VL_SDL3GPU_Surface *srf = (VL_SDL3GPU_Surface*)surface;
	// Get the last fully-rendered page.
	int prevPage = (srf->activePage + VL_SDL3GPU_NUM_BUFFERS - 1) % VL_SDL3GPU_NUM_BUFFERS;

	for (int page = 0; page < VL_SDL3GPU_NUM_BUFFERS; ++page)
	{
		if (page == prevPage)
			continue;
		memcpy(srf->bufferMaps[page], srf->bufferMaps[prevPage], srf->w * srf->h);
	}
}

static void VL_SDL3GPU_UpdateRect(void *surface, int x, int y, int w, int h)
{
	VL_SDL3GPU_Surface *surf = (VL_SDL3GPU_Surface *)surface;
	for (int page = 0; page < VL_SDL3GPU_NUM_BUFFERS; ++page)
	{
		if (page == surf->activePage)
			continue;
		for (int _y = y; _y < y + h; ++_y)
		{
			memcpy(((uint8_t *)surf->bufferMaps[page]) + (_y) * surf->w + x, ((uint8_t *)surf->data) + _y * surf->w + x, w);
		}
	}
}

static void VL_SDL3GPU_WaitVBLs(int vbls)
{
	SDL_Delay(vbls * 1000 / 70);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl3gpu_backend =
	{
		/*.setVideoMode =*/&VL_SDL3GPU_SetVideoMode,
		/*.createSurface =*/&VL_SDL3GPU_CreateSurface,
		/*.destroySurface =*/&VL_SDL3GPU_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_SDL3GPU_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_SDL3GPU_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_SDL3GPU_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_SDL3GPU_SurfacePGet,
		/*.surfaceRect =*/&VL_SDL3GPU_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_SDL3GPU_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_SDL3GPU_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_SDL3GPU_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_SDL3GPU_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_SDL3GPU_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_SDL3GPU_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_SDL3GPU_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_SDL3GPU_BitToSurface,
		/*.bitToSurface_PM =*/&VL_SDL3GPU_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_SDL3GPU_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_SDL3GPU_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_SDL3GPU_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_SDL3GPU_ScrollSurface,
		/*.present =*/&VL_SDL3GPU_Present,
		/*.getActiveBufferId =*/&VL_SDL3GPU_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_SDL3GPU_GetNumBuffers,
		/*.syncBuffers =*/&VL_SDL3GPU_SyncBuffers,
		/*.updateRect =*/&VL_SDL3GPU_UpdateRect,
		/*.flushParams =*/&VL_SDL3GPU_FlushParams,
		/*.waitVBLs =*/&VL_SDL3GPU_WaitVBLs
	};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl3gpu_backend;
}
