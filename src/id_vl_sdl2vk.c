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

// This is an UNFINISHED, BUGGY, Vulkan backend for Omnispeak.
// It only does nearest-neighbour sampling, does not support the overscan border
// and crashes rather a lot if you try to resize the window. It also only works
// on Linux with X11 (it uses XCB by default). Finally, the SPIR-V programs are
// hardcoded in and it leaks memory like a seive.
// This is because it is incomplete. Frankly, I'm surprised it works as well as
// it does. It's not compiled in by default for the above reasons, but you're
// welcome to try to fix it up and get it working if you want.
// -- David

#include "assert.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <stdlib.h>
#include "id_mm.h"
#include "id_us.h"
#include "id_vl.h"
#include "id_vl_private.h"
#include "ck_cross.h"

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

PFN_vkGetDeviceProcAddr id_vkGetDeviceProcAddr;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR id_vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR id_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR id_vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR id_vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetSwapchainImagesKHR id_vkGetSwapchainImagesKHR;
PFN_vkCreateDebugReportCallbackEXT id_vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT id_vkDestroyDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT id_vkDebugReportMessageEXT;

PFN_vkCreateSwapchainKHR id_vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR id_vkDestroySwapchainKHR;
PFN_vkAcquireNextImageKHR id_vkAcquireNextImageKHR;
PFN_vkQueuePresentKHR id_vkQueuePresentKHR;

VKAPI_ATTR VkBool32 VKAPI_CALL VL_SDL2VK_DebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char *pLayerPrefix,
	const char *pMessage,
	void *pUserData)
{
	CK_Cross_LogMessage(CK_LOG_MSG_WARNING, " [Vulkan|%s] %s\n", pLayerPrefix, pMessage);
	return VK_FALSE;
}

// Vertex shader
unsigned char vert_spv[] = {
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
	0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00,
	0x04, 0x00, 0x09, 0x00, 0x47, 0x4c, 0x5f, 0x41, 0x52, 0x42, 0x5f, 0x73,
	0x65, 0x70, 0x61, 0x72, 0x61, 0x74, 0x65, 0x5f, 0x73, 0x68, 0x61, 0x64,
	0x65, 0x72, 0x5f, 0x6f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x73, 0x00, 0x00,
	0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0c, 0x00, 0x00, 0x00,
	0x70, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x05, 0x00, 0x14, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x43,
	0x6f, 0x6f, 0x72, 0x64, 0x73, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x43, 0x6f, 0x6f, 0x72, 0x64,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x21, 0x00, 0x00, 0x00,
	0x67, 0x6c, 0x5f, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x49, 0x6e, 0x64,
	0x65, 0x78, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x2a, 0x00, 0x00, 0x00,
	0x67, 0x6c, 0x5f, 0x50, 0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78,
	0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x06, 0x00, 0x2a, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x73, 0x69, 0x74,
	0x69, 0x6f, 0x6e, 0x00, 0x05, 0x00, 0x03, 0x00, 0x2c, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x1e, 0x00, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0x21, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00,
	0x48, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00,
	0x2a, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x0b, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xbf, 0x2c, 0x00, 0x05, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2c, 0x00, 0x05, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
	0x2c, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
	0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x07, 0x00,
	0x0a, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x43, 0x2c, 0x00, 0x05, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x43, 0x2c, 0x00, 0x05, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
	0x19, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x1b, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00,
	0x2c, 0x00, 0x07, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00,
	0x16, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
	0x1b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
	0x1d, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x29, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x03, 0x00,
	0x2a, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x2b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00,
	0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x34, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00,
	0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00,
	0x05, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x0c, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x14, 0x00, 0x00, 0x00,
	0x1c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00,
	0x22, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
	0x23, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
	0x22, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x25, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x27, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x50, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
	0x26, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0x1f, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
	0x41, 0x00, 0x05, 0x00, 0x23, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
	0x0c, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
	0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00,
	0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00, 0x29, 0x00, 0x00, 0x00,
	0x33, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
	0x34, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,
	0x2d, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x35, 0x00, 0x00, 0x00,
	0x33, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00};
unsigned int vert_spv_len = 1332;

unsigned char frag_spv[] = {
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
	0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
	0x0b, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
	0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00,
	0x47, 0x4c, 0x5f, 0x41, 0x52, 0x42, 0x5f, 0x73, 0x65, 0x70, 0x61, 0x72,
	0x61, 0x74, 0x65, 0x5f, 0x73, 0x68, 0x61, 0x64, 0x65, 0x72, 0x5f, 0x6f,
	0x62, 0x6a, 0x65, 0x63, 0x74, 0x73, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x66, 0x69, 0x78, 0x65,
	0x64, 0x54, 0x65, 0x78, 0x43, 0x6f, 0x6f, 0x72, 0x64, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x43,
	0x6f, 0x6f, 0x72, 0x64, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x46, 0x72, 0x61, 0x67, 0x55, 0x6e, 0x69, 0x66,
	0x6f, 0x72, 0x6d, 0x73, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x05, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x4f,
	0x66, 0x66, 0x73, 0x00, 0x06, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x70, 0x61, 0x6c, 0x00, 0x05, 0x00, 0x03, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x75, 0x62, 0x6f, 0x00, 0x05, 0x00, 0x05, 0x00,
	0x1b, 0x00, 0x00, 0x00, 0x70, 0x61, 0x6c, 0x49, 0x6e, 0x64, 0x65, 0x78,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00,
	0x73, 0x63, 0x72, 0x65, 0x65, 0x6e, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
	0x27, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x43, 0x6f, 0x6c, 0x6f, 0x75,
	0x72, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x48, 0x00, 0x05, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
	0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x11, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x47, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00,
	0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0x27, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
	0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
	0x0e, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x1c, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x0f, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x12, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
	0x14, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x19, 0x00, 0x09, 0x00,
	0x1c, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x03, 0x00,
	0x1d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00,
	0x0e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
	0x0e, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x26, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x26, 0x00, 0x00, 0x00,
	0x27, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
	0x14, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x0c, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
	0x16, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00,
	0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
	0x57, 0x00, 0x05, 0x00, 0x22, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
	0x0e, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x1b, 0x00, 0x00, 0x00,
	0x25, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00,
	0x29, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00,
	0x2a, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x28, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00,
	0x3e, 0x00, 0x03, 0x00, 0x27, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,
	0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00};
unsigned int frag_spv_len = 1256;

static SDL_Window *vl_sdl2vk_window;
static int vl_sdl2vk_framebufferWidth, vl_sdl2vk_framebufferHeight;
static int vl_sdl2vk_screenWidth;
static int vl_sdl2vk_screenHeight;
static struct
{
	int left, right, top, bottom;
} vl_sdl2vk_scaledBorders;
static int vl_sdl2vk_screenHorizScaleFactor;
static int vl_sdl2vk_screenVertScaleFactor;

static ID_MM_Arena *vl_sdl2vk_arena;
static ID_MM_Arena *vl_sdl2vk_tempArena;

static VkApplicationInfo vl_sdl2vk_applicationInfo;
static VkInstanceCreateInfo vl_sdl2vk_instanceCreateInfo;
static VkInstance vl_sdl2vk_instance;
static VkDebugReportCallbackEXT vl_sdl2vk_debugCallback;
static VkPhysicalDevice vl_sdl2vk_physicalDevice;
static VkDevice vl_sdl2vk_device;
static int vl_sdl2vk_graphicsQueueIndex;
static int vl_sdl2vk_presentQueueIndex;
static VkQueue vl_sdl2vk_graphicsQueue;
static VkQueue vl_sdl2vk_presentQueue;
static VkSurfaceKHR vl_sdl2vk_windowSurface;
static VkSwapchainKHR vl_sdl2vk_swapchain = VK_NULL_HANDLE;
static uint32_t vl_sdl2vk_numSwapchainImages;
static VkImage *vl_sdl2vk_swapchainImages;
static VkImageView *vl_sdl2vk_swapchainImageViews;
static VkExtent2D vl_sdl2vk_swapchainSize;
static VkFormat vl_sdl2vk_swapchainFormat;
static VkImage vl_sdl2vk_integerImage;
static VkImageView vl_sdl2vk_integerImageView;
static VkDeviceMemory vl_sdl2vk_integerMemory;
static VkFramebuffer vl_sdl2vk_integerFramebuffer;
static VkShaderModule vl_sdl2vk_vertShaderModule;
static VkShaderModule vl_sdl2vk_fragShaderModule;
static VkPipelineLayout vl_sdl2vk_pipelineLayout;
static VkRenderPass vl_sdl2vk_renderPass;
static VkPipeline vl_sdl2vk_pipeline;
static VkFramebuffer *vl_sdl2vk_framebuffers;
static VkSemaphore vl_sdl2vk_imageAvailableSemaphore;
static VkSemaphore vl_sdl2vk_frameCompleteSemaphore;
static VkCommandPool vl_sdl2vk_commandPool;
static VkCommandBuffer *vl_sdl2vk_commandBuffers;
static VkBuffer vl_sdl2vk_uniformBuffer;
static VkDeviceMemory vl_sdl2vk_uniformBufferMemory;
static VkDescriptorSetLayout vl_sdl2vk_ubDescriptorSetLayout;
static VkDescriptorPool vl_sdl2vk_ubDescriptorPool;
static VkDescriptorSet vl_sdl2vk_ubDescriptorSet;
static VkDescriptorSetLayout vl_sdl2vk_samplerDescriptorSetLayout;
static VkDescriptorPool vl_sdl2vk_samplerDescriptorPool;
static VkDescriptorSet vl_sdl2vk_samplerDescriptorSet;

static int vl_sdl2vk_integerWidth;
static int vl_sdl2vk_integerHeight;

static bool vl_sdl2vk_flushSwapchain;

// Here is how the dimensions of the window are currently picked:
// 1. The emulated 320x200 sub-window is first zoomed
// by a factor of 3 (for each dimension) to 960x600.
// 2. The height is then multiplied by 1.2, so the internal contents
// (without the borders) have the aspect ratio of 4:3.
//
// There are a few more tricks in use to handle the overscan border
// and VGA line doubling.
#define VL_SDL2VK_DEFAULT_WINDOW_WIDTH (VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER * 3 / VL_VGA_GFX_WIDTH_SCALEFACTOR)
#define VL_SDL2VK_DEFAULT_WINDOW_HEIGHT (6 * VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER * 3 / (5 * VL_VGA_GFX_HEIGHT_SCALEFACTOR))

static void VL_SDL2VK_LoadVKInstanceProcs()
{
	id_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkGetDeviceProcAddr");
	if (!id_vkGetDeviceProcAddr)
		Quit("Couldn't get address of vkGetDeviceProcAddr");
	id_vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
	if (!id_vkGetPhysicalDeviceSurfaceSupportKHR)
		Quit("Couldn't get address of vkGetPhysicalDeviceSurfaceSupportKHR");
	id_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	if (!id_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
		Quit("Couldn't get address of vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	id_vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	if (!id_vkGetPhysicalDeviceSurfaceFormatsKHR)
		Quit("Couldn't get address of vkGetPhysicalDeviceSurfaceFormatsKHR");
	id_vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	if (!id_vkGetPhysicalDeviceSurfacePresentModesKHR)
		Quit("Couldn't get address of vkGetPhysicalDeviceSurfacePresentModesKHR");
	id_vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkGetSwapchainImagesKHR");
	if (!id_vkGetSwapchainImagesKHR)
		Quit("Couldn't get address of vkGetSwapchainImagesKHR");
	id_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkCreateDebugReportCallbackEXT");
	if (!id_vkCreateDebugReportCallbackEXT)
		Quit("Couldn't get address of vkCreateDebugReportCallbackEXT");
	id_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkDestroyDebugReportCallbackEXT");
	if (!id_vkDestroyDebugReportCallbackEXT)
		Quit("Couldn't get address of vkDestroyDebugReportCallbackEXT");
	id_vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(vl_sdl2vk_instance, "vkDebugReportMessageEXT");
	if (!id_vkDebugReportMessageEXT)
		Quit("Couldn't get address of vkDebugReportMessageEXT");
}

static void VL_SDL2VK_LoadVKDeviceProcs()
{
	id_vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)id_vkGetDeviceProcAddr(vl_sdl2vk_device, "vkCreateSwapchainKHR");
	if (!id_vkCreateSwapchainKHR)
		Quit("Couldn't get address of vkCreateSwapchainKHR");
	id_vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)id_vkGetDeviceProcAddr(vl_sdl2vk_device, "vkDestroySwapchainKHR");
	if (!id_vkDestroySwapchainKHR)
		Quit("Couldn't get address of vkDestroySwapchainKHR");
	id_vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)id_vkGetDeviceProcAddr(vl_sdl2vk_device, "vkAcquireNextImageKHR");
	if (!id_vkAcquireNextImageKHR)
		Quit("Couldn't get address of vkAcquireNextImageKHR");
	id_vkQueuePresentKHR = (PFN_vkQueuePresentKHR)id_vkGetDeviceProcAddr(vl_sdl2vk_device, "vkQueuePresentKHR");
	if (!id_vkQueuePresentKHR)
		Quit("Couldn't get address of vkQueuePresentKHR");
}

static VkCommandBuffer VL_SDL2VK_StartCommandBuffer(VkCommandBufferUsageFlagBits usageFlags)
{
	//VkResult result = VK_SUCCESS;
	VkCommandBufferAllocateInfo cmdbufAllocInfo = {};
	cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdbufAllocInfo.commandPool = vl_sdl2vk_commandPool;
	cmdbufAllocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuf;
	vkAllocateCommandBuffers(vl_sdl2vk_device, &cmdbufAllocInfo, &cmdBuf);
	VkCommandBufferBeginInfo cmdbufBeginInfo = {};
	cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdbufBeginInfo.flags = usageFlags;

	vkBeginCommandBuffer(cmdBuf, &cmdbufBeginInfo);

	return cmdBuf;
}

static void VL_SDL2VK_ImageLayoutBarrier(VkCommandBuffer buf, VkImage img, VkFormat imgFormat, VkImageLayout oldLayout, VkAccessFlags oldAccess, VkPipelineStageFlags oldStageFlags, VkImageLayout newLayout, VkAccessFlags newAccess, VkPipelineStageFlags newStageFlags)
{
	VkImageMemoryBarrier memBarrier = {};
	memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memBarrier.oldLayout = oldLayout;
	memBarrier.newLayout = newLayout;
	memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier.srcAccessMask = oldAccess;
	memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier.dstAccessMask = newAccess;
	memBarrier.image = img;
	memBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	memBarrier.subresourceRange.baseMipLevel = 0;
	memBarrier.subresourceRange.levelCount = 1;
	memBarrier.subresourceRange.baseArrayLayer = 0;
	memBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(buf, oldStageFlags, newStageFlags,
		0, 0, 0, 0, 0, 1, &memBarrier);
}

static void VL_SDL2VK_CreateVulkanInstance()
{
	const char *requiredInstanceExtensions[] = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

	unsigned int totalRequiredInstExtCount;
	if (!SDL_Vulkan_GetInstanceExtensions(vl_sdl2vk_window, &totalRequiredInstExtCount, 0))
		Quit("Couldn't get Vulkan instance extension list from SDL.");

	const char **totalRequiredInstanceExtensions = (const char **)MM_ArenaAlloc(vl_sdl2vk_tempArena, sizeof(const char *) * (totalRequiredInstExtCount + 2));
	SDL_Vulkan_GetInstanceExtensions(vl_sdl2vk_window, &totalRequiredInstExtCount, totalRequiredInstanceExtensions);
	memcpy(totalRequiredInstanceExtensions + totalRequiredInstExtCount, requiredInstanceExtensions, sizeof(const char *));
	totalRequiredInstExtCount++;

	const char *desiredDebugLayers[] = {"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_parameter_validation"};
	VkResult result = VK_SUCCESS;
	vl_sdl2vk_applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vl_sdl2vk_applicationInfo.pNext = 0;
	vl_sdl2vk_applicationInfo.pApplicationName = "Commander Keen";
	vl_sdl2vk_applicationInfo.pEngineName = "Omnispeak";
	vl_sdl2vk_applicationInfo.engineVersion = 1;
	vl_sdl2vk_applicationInfo.apiVersion = VK_API_VERSION_1_0;

	vl_sdl2vk_instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vl_sdl2vk_instanceCreateInfo.pNext = 0;
	vl_sdl2vk_instanceCreateInfo.flags = 0;
	vl_sdl2vk_instanceCreateInfo.pApplicationInfo = &vl_sdl2vk_applicationInfo;
	vl_sdl2vk_instanceCreateInfo.enabledLayerCount = 0;
	vl_sdl2vk_instanceCreateInfo.ppEnabledLayerNames = desiredDebugLayers;
	vl_sdl2vk_instanceCreateInfo.enabledExtensionCount = totalRequiredInstExtCount;
	vl_sdl2vk_instanceCreateInfo.ppEnabledExtensionNames = totalRequiredInstanceExtensions;

	// Search for the required surface extensions.
	uint32_t instanceExtensionCount = 0;
	result = vkEnumerateInstanceExtensionProperties(0, &instanceExtensionCount, 0);
	if (result != VK_SUCCESS || instanceExtensionCount < 2)
		Quit("No suitable Vulkan instance.");

	/*
	VkExtensionProperties *allInstanceExtensions = (VkExtensionProperties *)MM_ArenaAlloc(vl_sdl2vk_tempArena, sizeof(VkExtensionProperties) * instanceExtensionCount);
	result = vkEnumerateInstanceExtensionProperties(0, &instanceExtensionCount, allInstanceExtensions);
	if (result != VK_SUCCESS)
		Quit("Couldn't enumerate Vulkan instance extensions.");

	for (int i = 0; i < instanceExtensionCount; ++i)
	{
		if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, allInstanceExtensions[i].extensionName))
		{
			// We've found the KHR_surface extension.
			vl_sdl2vk_instanceCreateInfo.enabledExtensionCount++;
		}
		if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, allInstanceExtensions[i].extensionName))
		{
			// We've found the debug report extension.
			vl_sdl2vk_instanceCreateInfo.enabledExtensionCount++;
		}
	}
	if (vl_sdl2vk_instanceCreateInfo.enabledExtensionCount < 3)
		Quit("Vulkan Surface Extensions not available.");

	*/
	result = vkCreateInstance(&vl_sdl2vk_instanceCreateInfo, 0, &vl_sdl2vk_instance);
	if (result != VK_SUCCESS)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Couldn't create Vulkan instance: %d", result);
	}
}

static uint32_t VL_SDL2VK_SelectMemoryType(uint32_t memoryTypeMask, VkMemoryPropertyFlags memoryTypeFlags)
{
	VkPhysicalDeviceMemoryProperties physDeviceMemoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(vl_sdl2vk_physicalDevice, &physDeviceMemoryProperties);

	uint32_t memTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < physDeviceMemoryProperties.memoryTypeCount; ++i)
	{
		// Make sure the memory type is supported.
		if ((memoryTypeMask & (1 << i)) == 0)
			continue;

		if ((physDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryTypeFlags) == memoryTypeFlags)
		{
			memTypeIndex = i;
			break;
		}
	}
	return memTypeIndex;
}

static void VL_SDL2VK_InitPhysicalDevice()
{
	VkResult result = VK_SUCCESS;

	bool surfaceResult = SDL_Vulkan_CreateSurface(vl_sdl2vk_window, vl_sdl2vk_instance, &vl_sdl2vk_windowSurface);
	if (surfaceResult != true)
		Quit("Couldn't create window surface.");

	uint32_t physicalDeviceCount = 0;
	result = vkEnumeratePhysicalDevices(vl_sdl2vk_instance, &physicalDeviceCount, 0);

	if (result != VK_SUCCESS || !physicalDeviceCount)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Couldn't enumerate Vulkan devices: %d", result);
		return;
	}

	VkPhysicalDevice *allPhysicalDevices = (VkPhysicalDevice *)MM_ArenaAlloc(vl_sdl2vk_tempArena, sizeof(VkPhysicalDevice) * physicalDeviceCount);
	result = vkEnumeratePhysicalDevices(vl_sdl2vk_instance, &physicalDeviceCount, allPhysicalDevices);

	vl_sdl2vk_physicalDevice = allPhysicalDevices[0];

	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	vkGetPhysicalDeviceProperties(vl_sdl2vk_physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceMemoryProperties(vl_sdl2vk_physicalDevice, &memoryProperties);

	printf("Device id %x %s\n", deviceProperties.vendorID, deviceProperties.deviceName);

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vl_sdl2vk_physicalDevice, &queueFamilyCount, 0);

	VkQueueFamilyProperties *allQueueFamilyProperties = (VkQueueFamilyProperties *)MM_ArenaAlloc(vl_sdl2vk_tempArena, sizeof(VkQueueFamilyProperties) * queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(vl_sdl2vk_physicalDevice, &queueFamilyCount, allQueueFamilyProperties);
	vl_sdl2vk_graphicsQueueIndex = -1;
	vl_sdl2vk_presentQueueIndex = -1;

	for (int i = 0; i < queueFamilyCount; ++i)
	{
		VkBool32 hasPresentSupport = false;
		id_vkGetPhysicalDeviceSurfaceSupportKHR(vl_sdl2vk_physicalDevice, i, vl_sdl2vk_windowSurface, &hasPresentSupport);

		if (allQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			vl_sdl2vk_graphicsQueueIndex = i;
		}
		if (hasPresentSupport)
		{
			vl_sdl2vk_presentQueueIndex = i;
		}
	}

	if (vl_sdl2vk_graphicsQueueIndex == -1)
		Quit("Vulkan device had no graphics queue.");
	if (vl_sdl2vk_presentQueueIndex == -1)
		Quit("Vulkan device had no present queue.");

	static float queuePriorities = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo[2];
	queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[0].pNext = 0;
	queueCreateInfo[0].flags = 0;
	queueCreateInfo[0].queueFamilyIndex = vl_sdl2vk_graphicsQueueIndex;
	queueCreateInfo[0].queueCount = 1;
	queueCreateInfo[0].pQueuePriorities = &queuePriorities;
	queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[1].pNext = 0;
	queueCreateInfo[1].flags = 0;
	queueCreateInfo[1].queueFamilyIndex = vl_sdl2vk_presentQueueIndex;
	queueCreateInfo[1].queueCount = 1;
	queueCreateInfo[1].pQueuePriorities = &queuePriorities;

	const char *deviceExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = 0;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = 0;
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
	deviceCreateInfo.queueCreateInfoCount = (vl_sdl2vk_presentQueueIndex == vl_sdl2vk_graphicsQueueIndex) ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;

	result = vkCreateDevice(vl_sdl2vk_physicalDevice, &deviceCreateInfo, 0, &vl_sdl2vk_device);
	if (result != VK_SUCCESS)
		Quit("Couldn't create Vulkan device.");

	vkGetDeviceQueue(vl_sdl2vk_device, vl_sdl2vk_graphicsQueueIndex, 0, &vl_sdl2vk_graphicsQueue);
	vkGetDeviceQueue(vl_sdl2vk_device, vl_sdl2vk_presentQueueIndex, 0, &vl_sdl2vk_presentQueue);
}

static void VL_SDL2VK_SetupSwapchain(int width, int height)
{
	VkResult result = VK_SUCCESS;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	result = id_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vl_sdl2vk_physicalDevice, vl_sdl2vk_windowSurface, &surfaceCapabilities);
	if (result != VK_SUCCESS)
		Quit("Couldn't get window surface capabilities");

	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT == 0)
		Quit("Vulkan swapchain does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT");

	uint32_t surfaceFormatCount;
	result = id_vkGetPhysicalDeviceSurfaceFormatsKHR(vl_sdl2vk_physicalDevice, vl_sdl2vk_windowSurface, &surfaceFormatCount, 0);
	if (result != VK_SUCCESS)
		Quit("Couldn't enumerate physical device surface formats.");

	VkSurfaceFormatKHR *surfaceFormats = (VkSurfaceFormatKHR *)MM_ArenaAlloc(vl_sdl2vk_tempArena, sizeof(VkSurfaceFormatKHR) & surfaceFormatCount);
	result = id_vkGetPhysicalDeviceSurfaceFormatsKHR(vl_sdl2vk_physicalDevice, vl_sdl2vk_windowSurface, &surfaceFormatCount, surfaceFormats);
	if (result != VK_SUCCESS)
		Quit("Couldn't enumerate physical device surface formats.");

	vl_sdl2vk_swapchainSize = surfaceCapabilities.currentExtent;

	// On Wayland, the currentExtent is never set, so take it from the given w/h
	if ((vl_sdl2vk_swapchainSize.width == -1 || vl_sdl2vk_swapchainSize.height == -1))
	{
		vl_sdl2vk_swapchainSize.width = width;
		vl_sdl2vk_swapchainSize.height = height;
	}

	// Clamp the swapchainSize to make sure it's within surfaceCapabilities
	vl_sdl2vk_swapchainSize.width = CK_Cross_min(CK_Cross_max(vl_sdl2vk_swapchainSize.width, surfaceCapabilities.minImageExtent.width), surfaceCapabilities.maxImageExtent.width);
	vl_sdl2vk_swapchainSize.height = CK_Cross_min(CK_Cross_max(vl_sdl2vk_swapchainSize.height, surfaceCapabilities.minImageExtent.height), surfaceCapabilities.maxImageExtent.height);

	// We calculate the render regions here, as on X11,
	// SDL_Vulkan_GetDrawableSize() is not always valid, so we can end up
	// trying to blit to a fullRgn that exceeds the surface size.
	// See, for example: https://bugzilla.libsdl.org/show_bug.cgi?id=4671
	// By using the actual swapchain size here, it should always be correct
	VL_CalculateRenderRegions(vl_sdl2vk_swapchainSize.width, vl_sdl2vk_swapchainSize.height);

	int desiredFormat = 0;
	for (int i = 0; i < surfaceFormatCount; ++i)
	{
		if (surfaceFormats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			continue;
		if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
			desiredFormat = i;
	}

	vl_sdl2vk_swapchainFormat = surfaceFormats[desiredFormat].format;

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = 0;
	swapchainCreateInfo.surface = vl_sdl2vk_windowSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = vl_sdl2vk_swapchainFormat;
	swapchainCreateInfo.imageColorSpace = surfaceFormats[desiredFormat].colorSpace;
	swapchainCreateInfo.imageExtent = vl_sdl2vk_swapchainSize;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = 0;
	// TODO: Support extra present modes. Force FIFO for now.
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.oldSwapchain = vl_sdl2vk_swapchain;

	result = id_vkCreateSwapchainKHR(vl_sdl2vk_device, &swapchainCreateInfo, 0, &vl_sdl2vk_swapchain);
	if (result != VK_SUCCESS)
		Quit("Couldn't create swapchain.");

	if (swapchainCreateInfo.oldSwapchain != VK_NULL_HANDLE)
		id_vkDestroySwapchainKHR(vl_sdl2vk_device, swapchainCreateInfo.oldSwapchain, 0);

	result = id_vkGetSwapchainImagesKHR(vl_sdl2vk_device, vl_sdl2vk_swapchain, &vl_sdl2vk_numSwapchainImages, 0);
	if (result != VK_SUCCESS)
		Quit("Couldn't get swapchain images.");

	vl_sdl2vk_swapchainImages = (VkImage *)MM_ArenaAlloc(vl_sdl2vk_arena, vl_sdl2vk_numSwapchainImages * sizeof(VkImage));
	vl_sdl2vk_swapchainImageViews = (VkImageView *)MM_ArenaAlloc(vl_sdl2vk_arena, vl_sdl2vk_numSwapchainImages * sizeof(VkImageView));

	result = id_vkGetSwapchainImagesKHR(vl_sdl2vk_device, vl_sdl2vk_swapchain, &vl_sdl2vk_numSwapchainImages, vl_sdl2vk_swapchainImages);
	if (result != VK_SUCCESS)
		Quit("Couldn't get swapchain images.");

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = 0;
	imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.flags = 0;

	for (int i = 0; i < vl_sdl2vk_numSwapchainImages; ++i)
	{
		imageViewCreateInfo.image = vl_sdl2vk_swapchainImages[i];
		result = vkCreateImageView(vl_sdl2vk_device, &imageViewCreateInfo, 0, &vl_sdl2vk_swapchainImageViews[i]);
		if (result != VK_SUCCESS)
			Quit("Couldn't create swapchain image view.");
	}
}

static void VL_SDL2VK_CreateRenderPass()
{
	VkResult result = VK_SUCCESS;

	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = vl_sdl2vk_swapchainFormat;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colourAttachmentRef = {};
	colourAttachmentRef.attachment = 0;
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colourAttachmentRef;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colourAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDependency;

	result = vkCreateRenderPass(vl_sdl2vk_device, &renderPassInfo, 0, &vl_sdl2vk_renderPass);
	if (result != VK_SUCCESS)
		Quit("Couldn't create render pass.");
}

static void VL_SDL2VK_CreateShaders()
{
	VkResult result = VK_SUCCESS;

	VkShaderModuleCreateInfo vertShaderCreateInfo = {};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertShaderCreateInfo.pNext = 0;
	vertShaderCreateInfo.codeSize = vert_spv_len;
	vertShaderCreateInfo.pCode = (uint32_t *)vert_spv;
	vertShaderCreateInfo.flags = 0;

	result = vkCreateShaderModule(vl_sdl2vk_device, &vertShaderCreateInfo, 0, &vl_sdl2vk_vertShaderModule);
	if (result != VK_SUCCESS)
		Quit("Failed to create vertex shader");

	VkShaderModuleCreateInfo fragShaderCreateInfo = {};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragShaderCreateInfo.pNext = 0;
	fragShaderCreateInfo.codeSize = frag_spv_len;
	fragShaderCreateInfo.pCode = (uint32_t *)frag_spv;
	fragShaderCreateInfo.flags = 0;

	result = vkCreateShaderModule(vl_sdl2vk_device, &fragShaderCreateInfo, 0, &vl_sdl2vk_fragShaderModule);
	if (result != VK_SUCCESS)
		Quit("Failed to create fragment shader");
}

static void VL_SDL2VK_CreateDescriptorSetLayouts()
{
	VkResult result = VK_SUCCESS;
	VkDescriptorSetLayoutBinding ubLayoutBinding = {};
	ubLayoutBinding.binding = 0;
	ubLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubLayoutBinding.descriptorCount = 1;
	ubLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo ubDescriptorSetLayoutInfo = {};
	ubDescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ubDescriptorSetLayoutInfo.bindingCount = 1;
	ubDescriptorSetLayoutInfo.pBindings = &ubLayoutBinding;

	result = vkCreateDescriptorSetLayout(vl_sdl2vk_device, &ubDescriptorSetLayoutInfo, 0, &vl_sdl2vk_ubDescriptorSetLayout);
	if (result != VK_SUCCESS)
		Quit("Couldn't create uniform buffer descriptor set layout.");

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = 0;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo samplerDescriptorSetLayoutInfo = {};
	samplerDescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	samplerDescriptorSetLayoutInfo.bindingCount = 1;
	samplerDescriptorSetLayoutInfo.pBindings = &samplerLayoutBinding;

	result = vkCreateDescriptorSetLayout(vl_sdl2vk_device, &samplerDescriptorSetLayoutInfo, 0, &vl_sdl2vk_samplerDescriptorSetLayout);
	if (result != VK_SUCCESS)
		Quit("Couldn't create sampler descriptor set layout.");
}

static void VL_SDL2VK_CreatePipeline()
{
	VkResult result = VK_SUCCESS;
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = 0;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vl_sdl2vk_vertShaderModule;
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = 0;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = vl_sdl2vk_fragShaderModule;
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStage[2] = {vertShaderStageInfo, fragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexStage = {};
	vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexStage.pNext = 0;
	vertexStage.flags = 0;
	vertexStage.vertexAttributeDescriptionCount = 0;
	vertexStage.pVertexAttributeDescriptions = 0;
	vertexStage.vertexBindingDescriptionCount = 0;
	vertexStage.pVertexBindingDescriptions = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStage = {};
	inputAssemblyStage.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStage.pNext = 0;
	inputAssemblyStage.flags = 0;
	inputAssemblyStage.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssemblyStage.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = vl_sdl2vk_swapchainSize.width;
	viewport.height = vl_sdl2vk_swapchainSize.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = vl_sdl2vk_swapchainSize;

	VkPipelineViewportStateCreateInfo viewportStage = {};
	viewportStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStage.pNext = 0;
	viewportStage.flags = 0;
	viewportStage.viewportCount = 1;
	viewportStage.pViewports = &viewport;
	viewportStage.scissorCount = 1;
	viewportStage.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizeState = {};
	rasterizeState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizeState.pNext = 0;
	rasterizeState.flags = 0;
	rasterizeState.depthClampEnable = VK_FALSE;
	rasterizeState.rasterizerDiscardEnable = VK_FALSE;
	rasterizeState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizeState.lineWidth = 1.0f;
	rasterizeState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizeState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizeState.depthBiasEnable = VK_FALSE;
	rasterizeState.depthBiasConstantFactor = 0.0f;
	rasterizeState.depthBiasSlopeFactor = 0.0f;
	rasterizeState.depthBiasClamp = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pNext = 0;
	multisampleState.flags = 0;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 1.0f;
	multisampleState.pSampleMask = 0;
	multisampleState.alphaToOneEnable = VK_FALSE;
	multisampleState.alphaToCoverageEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState attachmentBlendState = {};
	attachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	attachmentBlendState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.logicOpEnable = VK_FALSE;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &attachmentBlendState;

	VkDescriptorSetLayout layouts[2] = {vl_sdl2vk_ubDescriptorSetLayout, vl_sdl2vk_samplerDescriptorSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = layouts;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	result = vkCreatePipelineLayout(vl_sdl2vk_device, &pipelineLayoutInfo, 0, &vl_sdl2vk_pipelineLayout);
	if (result != VK_SUCCESS)
		Quit("Couldn't create pipeline layout.");

	VkDynamicState dynamicStates[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStage;
	pipelineCreateInfo.pVertexInputState = &vertexStage;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStage;
	pipelineCreateInfo.pViewportState = &viewportStage;
	pipelineCreateInfo.pRasterizationState = &rasterizeState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = 0;
	pipelineCreateInfo.pColorBlendState = &blendState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = vl_sdl2vk_pipelineLayout;
	pipelineCreateInfo.renderPass = vl_sdl2vk_renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(vl_sdl2vk_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, 0, &vl_sdl2vk_pipeline);
	if (result != VK_SUCCESS)
		Quit("Couldn't create graphics pipeline.");
}

static void VL_SDL2VK_CreateFramebuffers(int integerScaleX, int integerScaleY)
{
	VkResult result = VK_SUCCESS;

	// Set all of the swapchain image layouts to PRESENT_SRC, which is what our command buffers expect them to
	// be going in.
	// We do this here, rather than in SetupSwapchain(), because we always have the command pool initialized by
	// this point.
	VkCommandBuffer layoutChange = VL_SDL2VK_StartCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	for (int i = 0; i < vl_sdl2vk_numSwapchainImages; ++i)
	{
		VL_SDL2VK_ImageLayoutBarrier(layoutChange, vl_sdl2vk_swapchainImages[i], vl_sdl2vk_swapchainFormat,
			VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}
	vkEndCommandBuffer(layoutChange);
	// And submit it...
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &layoutChange;

	vkQueueSubmit(vl_sdl2vk_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vl_sdl2vk_graphicsQueue);

	vkFreeCommandBuffers(vl_sdl2vk_device, vl_sdl2vk_commandPool, 1, &layoutChange);

	vl_sdl2vk_framebuffers = (VkFramebuffer *)MM_ArenaAlloc(vl_sdl2vk_arena, vl_sdl2vk_numSwapchainImages * sizeof(VkFramebuffer));

	for (int i = 0; i < vl_sdl2vk_numSwapchainImages; ++i)
	{
		VkImageView fbAttachments[] = {vl_sdl2vk_swapchainImageViews[i]};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = vl_sdl2vk_renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = fbAttachments;
		framebufferCreateInfo.width = vl_sdl2vk_swapchainSize.width;
		framebufferCreateInfo.height = vl_sdl2vk_swapchainSize.height;
		framebufferCreateInfo.layers = 1;

		result = vkCreateFramebuffer(vl_sdl2vk_device, &framebufferCreateInfo, 0, &vl_sdl2vk_framebuffers[i]);
		if (result != VK_SUCCESS)
			Quit("Couldn't create framebuffer.");
	}

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result = vkCreateSemaphore(vl_sdl2vk_device, &semaphoreInfo, 0, &vl_sdl2vk_imageAvailableSemaphore);
	if (result != VK_SUCCESS)
		Quit("Couldn't create semaphore");

	result = vkCreateSemaphore(vl_sdl2vk_device, &semaphoreInfo, 0, &vl_sdl2vk_frameCompleteSemaphore);
	if (result != VK_SUCCESS)
		Quit("Couldn't create semaphore");

	// Now create a framebuffer with the integer scaled size.
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = integerScaleX;
	imageInfo.extent.height = integerScaleY;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	result = vkCreateImage(vl_sdl2vk_device, &imageInfo, 0, &vl_sdl2vk_integerImage);
	if (result != VK_SUCCESS)
		Quit("Failed to create image for front buffer surface.");

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(vl_sdl2vk_device, vl_sdl2vk_integerImage, &memoryRequirements);

	VkMemoryAllocateInfo imageAllocInfo = {};
	imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocInfo.allocationSize = memoryRequirements.size;
	imageAllocInfo.memoryTypeIndex = VL_SDL2VK_SelectMemoryType(memoryRequirements.memoryTypeBits, 0);

	result = vkAllocateMemory(vl_sdl2vk_device, &imageAllocInfo, 0, &vl_sdl2vk_integerMemory);
	if (result != VK_SUCCESS)
		Quit("Couldn't allocate memory for image.");

	vkBindImageMemory(vl_sdl2vk_device, vl_sdl2vk_integerImage, vl_sdl2vk_integerMemory, 0);

	VkImageViewCreateInfo imageViewInfo = {};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = vl_sdl2vk_integerImage;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = 1;

	result = vkCreateImageView(vl_sdl2vk_device, &imageViewInfo, 0, &vl_sdl2vk_integerImageView);
	if (result != VK_SUCCESS)
		Quit("Couldn't create image view for surface.");

	VkFramebufferCreateInfo integerFramebufferCreateInfo = {};
	integerFramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	integerFramebufferCreateInfo.renderPass = vl_sdl2vk_renderPass;
	integerFramebufferCreateInfo.attachmentCount = 1;
	integerFramebufferCreateInfo.pAttachments = &vl_sdl2vk_integerImageView;
	integerFramebufferCreateInfo.width = integerScaleX;
	integerFramebufferCreateInfo.height = integerScaleY;
	integerFramebufferCreateInfo.layers = 1;

	result = vkCreateFramebuffer(vl_sdl2vk_device, &integerFramebufferCreateInfo, 0, &vl_sdl2vk_integerFramebuffer);
	if (result != VK_SUCCESS)
		Quit("Couldn't create framebuffer.");

	vl_sdl2vk_integerWidth = integerScaleX;
	vl_sdl2vk_integerHeight = integerScaleY;
}

static void VL_SDL2VK_CreateUniformBuffer()
{
	VkResult result = VK_SUCCESS;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = sizeof(float) * 4 * 17;
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(vl_sdl2vk_device, &createInfo, 0, &vl_sdl2vk_uniformBuffer);
	if (result != VK_SUCCESS)
		Quit("Couldn't create uniform buffer.");

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(vl_sdl2vk_device, vl_sdl2vk_uniformBuffer, &memoryRequirements);

	const VkMemoryPropertyFlags desiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t memTypeIndex = VL_SDL2VK_SelectMemoryType(memoryRequirements.memoryTypeBits, desiredFlags);
	if (memTypeIndex == UINT32_MAX)
		Quit("No suitable uniform buffer memory types.");

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memTypeIndex;

	result = vkAllocateMemory(vl_sdl2vk_device, &allocateInfo, 0, &vl_sdl2vk_uniformBufferMemory);
	if (result != VK_SUCCESS)
		Quit("Failed to allocate device memory for uniform buffer.");

	vkBindBufferMemory(vl_sdl2vk_device, vl_sdl2vk_uniformBuffer, vl_sdl2vk_uniformBufferMemory, 0);

	VkDescriptorPoolSize ubDescriptorPoolSize = {};
	ubDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubDescriptorPoolSize.descriptorCount = 1;
	VkDescriptorPoolSize samplerDescriptorPoolSize = {};
	samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerDescriptorPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize sizes[2] = {ubDescriptorPoolSize, samplerDescriptorPoolSize};

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = sizes;
	descriptorPoolInfo.maxSets = 2;

	result = vkCreateDescriptorPool(vl_sdl2vk_device, &descriptorPoolInfo, 0, &vl_sdl2vk_ubDescriptorPool);
	if (result != VK_SUCCESS)
		Quit("Couldn't create uniform buffer descriptor pool.");

	VkDescriptorSetAllocateInfo ubDescriptorSetAllocInfo = {};
	ubDescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ubDescriptorSetAllocInfo.descriptorPool = vl_sdl2vk_ubDescriptorPool;
	ubDescriptorSetAllocInfo.descriptorSetCount = 1;
	ubDescriptorSetAllocInfo.pSetLayouts = &vl_sdl2vk_ubDescriptorSetLayout;

	VkDescriptorSetAllocateInfo samplerDescriptorSetAllocInfo = {};
	samplerDescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	samplerDescriptorSetAllocInfo.descriptorPool = vl_sdl2vk_ubDescriptorPool;
	samplerDescriptorSetAllocInfo.descriptorSetCount = 1;
	samplerDescriptorSetAllocInfo.pSetLayouts = &vl_sdl2vk_samplerDescriptorSetLayout;

	result = vkAllocateDescriptorSets(vl_sdl2vk_device, &ubDescriptorSetAllocInfo, &vl_sdl2vk_ubDescriptorSet);
	if (result != VK_SUCCESS)
		Quit("Couldn't allocate uniform buffer descriptor sets.");

	result = vkAllocateDescriptorSets(vl_sdl2vk_device, &samplerDescriptorSetAllocInfo, &vl_sdl2vk_samplerDescriptorSet);
	if (result != VK_SUCCESS)
		Quit("Couldn't allocate sampler descriptor sets.");

	VkDescriptorBufferInfo ubDescriptorBufferInfo = {};
	ubDescriptorBufferInfo.buffer = vl_sdl2vk_uniformBuffer;
	ubDescriptorBufferInfo.offset = 0;
	ubDescriptorBufferInfo.range = 4 * 17 * sizeof(float);

	VkWriteDescriptorSet ubWriteDescriptorSet = {};
	ubWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ubWriteDescriptorSet.dstSet = vl_sdl2vk_ubDescriptorSet;
	ubWriteDescriptorSet.dstBinding = 0;
	ubWriteDescriptorSet.dstArrayElement = 0;
	ubWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubWriteDescriptorSet.descriptorCount = 1;
	ubWriteDescriptorSet.pBufferInfo = &ubDescriptorBufferInfo;

	vkUpdateDescriptorSets(vl_sdl2vk_device, 1, &ubWriteDescriptorSet, 0, 0);
}

static void VL_SDL2VK_CreateCommandBuffers()
{
	VkResult result = VK_SUCCESS;

	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = vl_sdl2vk_graphicsQueueIndex;

	result = vkCreateCommandPool(vl_sdl2vk_device, &commandPoolInfo, 0, &vl_sdl2vk_commandPool);
	if (result != VK_SUCCESS)
		Quit("Couldn't create command pool.");

	vl_sdl2vk_commandBuffers = (VkCommandBuffer *)MM_ArenaAlloc(vl_sdl2vk_arena, vl_sdl2vk_numSwapchainImages * sizeof(VkCommandBuffer));

	VkCommandBufferAllocateInfo cmdbufAllocInfo = {};
	cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbufAllocInfo.commandPool = vl_sdl2vk_commandPool;
	cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdbufAllocInfo.commandBufferCount = vl_sdl2vk_numSwapchainImages;

	result = vkAllocateCommandBuffers(vl_sdl2vk_device, &cmdbufAllocInfo, vl_sdl2vk_commandBuffers);
	if (result != VK_SUCCESS)
		Quit("Couldn't allocate command buffers.");
}

static void VL_SDL2VK_PopulateCommandBuffer(int i, VkRect2D fullRgn, VkRect2D renderRgn, VkClearColorValue borderColour)
{
	VkResult result = VK_SUCCESS;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = 0;

	result = vkBeginCommandBuffer(vl_sdl2vk_commandBuffers[i], &beginInfo);
	if (result != VK_SUCCESS)
		Quit("Couldn't begin command buffer");

	VkClearValue clearColour = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	VkClearValue borderValue = {};
	borderValue.color = borderColour;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vl_sdl2vk_renderPass;
	renderPassBeginInfo.framebuffer = vl_sdl2vk_integerFramebuffer;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	//renderPassBeginInfo.renderArea.extent = vl_sdl2vk_swapchainSize;
	renderPassBeginInfo.renderArea.extent.width = vl_sdl2vk_integerWidth;
	renderPassBeginInfo.renderArea.extent.height = vl_sdl2vk_integerHeight;

	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &borderValue;

	VkViewport viewport = {};
	viewport.x = renderRgn.offset.x;
	viewport.y = renderRgn.offset.y;
	viewport.width = renderRgn.extent.width;
	viewport.height = renderRgn.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = renderRgn;

	VkImageBlit blitRegion = {};
	blitRegion.dstOffsets[0].x = fullRgn.offset.x;
	blitRegion.dstOffsets[0].y = fullRgn.offset.y;
	blitRegion.dstOffsets[0].z = 0;
	blitRegion.dstOffsets[1].x = fullRgn.offset.x + fullRgn.extent.width;
	blitRegion.dstOffsets[1].y = fullRgn.offset.y + fullRgn.extent.height;
	blitRegion.dstOffsets[1].z = 1;
	blitRegion.dstSubresource.mipLevel = 0;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcOffsets[0].x = 0;
	blitRegion.srcOffsets[0].y = 0;
	blitRegion.srcOffsets[0].z = 0;
	blitRegion.srcOffsets[1].x = vl_sdl2vk_integerWidth;
	blitRegion.srcOffsets[1].y = vl_sdl2vk_integerHeight;
	blitRegion.srcOffsets[1].z = 1;
	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VL_SDL2VK_ImageLayoutBarrier(vl_sdl2vk_commandBuffers[i], vl_sdl2vk_swapchainImages[i], vl_sdl2vk_swapchainFormat,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkCmdBeginRenderPass(vl_sdl2vk_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(vl_sdl2vk_commandBuffers[i], 0, 1, &viewport);
	vkCmdSetScissor(vl_sdl2vk_commandBuffers[i], 0, 1, &scissor);
	vkCmdBindDescriptorSets(vl_sdl2vk_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vl_sdl2vk_pipelineLayout, 0, 1, &vl_sdl2vk_ubDescriptorSet, 0, 0);
	vkCmdBindDescriptorSets(vl_sdl2vk_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vl_sdl2vk_pipelineLayout, 1, 1, &vl_sdl2vk_samplerDescriptorSet, 0, 0);

	vkCmdBindPipeline(vl_sdl2vk_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vl_sdl2vk_pipeline);

	vkCmdDraw(vl_sdl2vk_commandBuffers[i], 4, 1, 0, 0);

	vkCmdEndRenderPass(vl_sdl2vk_commandBuffers[i]);

	VkImageSubresourceRange dstSubresourceRange = {};
	dstSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dstSubresourceRange.baseMipLevel = 0;
	dstSubresourceRange.levelCount = 1;
	dstSubresourceRange.baseArrayLayer = 0;
	dstSubresourceRange.layerCount = 1;

	vkCmdClearColorImage(vl_sdl2vk_commandBuffers[i], vl_sdl2vk_swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&clearColour.color,
		1, &dstSubresourceRange);

	VL_SDL2VK_ImageLayoutBarrier(vl_sdl2vk_commandBuffers[i], vl_sdl2vk_integerImage, vl_sdl2vk_swapchainFormat,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	vkCmdBlitImage(vl_sdl2vk_commandBuffers[i],
		vl_sdl2vk_integerImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vl_sdl2vk_swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &blitRegion, VK_FILTER_LINEAR);

	VL_SDL2VK_ImageLayoutBarrier(vl_sdl2vk_commandBuffers[i], vl_sdl2vk_swapchainImages[i], vl_sdl2vk_swapchainFormat,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	result = vkEndCommandBuffer(vl_sdl2vk_commandBuffers[i]);

	if (result != VK_SUCCESS)
		Quit("Couldn't record command buffer.");
}

void VL_SDL2VK_DestroySwapchain()
{
	vkDeviceWaitIdle(vl_sdl2vk_device);
	for (int i = 0; i < vl_sdl2vk_numSwapchainImages; ++i)
	{
		vkDestroyFramebuffer(vl_sdl2vk_device, vl_sdl2vk_framebuffers[i], 0);
		vkDestroyImageView(vl_sdl2vk_device, vl_sdl2vk_swapchainImageViews[i], 0);
	}
	vkDestroySemaphore(vl_sdl2vk_device, vl_sdl2vk_imageAvailableSemaphore, 0);
	vkDestroySemaphore(vl_sdl2vk_device, vl_sdl2vk_frameCompleteSemaphore, 0);

	vkDestroyFramebuffer(vl_sdl2vk_device, vl_sdl2vk_integerFramebuffer, 0);
	vkDestroyImageView(vl_sdl2vk_device, vl_sdl2vk_integerImageView, 0);
	vkDestroyImage(vl_sdl2vk_device, vl_sdl2vk_integerImage, 0);
	vkFreeMemory(vl_sdl2vk_device, vl_sdl2vk_integerMemory, 0);

	vkDestroyPipeline(vl_sdl2vk_device, vl_sdl2vk_pipeline, 0);
	vkDestroyPipelineLayout(vl_sdl2vk_device, vl_sdl2vk_pipelineLayout, 0);
}

/* TODO (Overscan border):
 * - If a texture is used for offscreen rendering with scaling applied later,
 * it's better to have the borders within the texture itself.
 */

typedef struct VL_SDL2VK_Surface
{
	VL_SurfaceUsage use;
	VkImage stagingImage;
	VkDeviceMemory stagingMemory;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
	int w, h;
	int pitch;
	void *data;
} VL_SDL2VK_Surface;

void VL_SDL2VK_SetIcon(SDL_Window *wnd);

// Here is how the dimensions of the window are currently picked:
// 1. The emulated 320x200 sub-window is first zoomed
// by a factor of 3 (for each dimension) to 960x600.
// 2. The height is then multiplied by 1.2, so the internal contents
// (without the borders) have the aspect ratio of 4:3.
//
// There are a few more tricks in use to handle the overscan border
// and VGA line doubling.
#define VL_SDL2VK_DEFAULT_WINDOW_WIDTH (VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER * 3 / VL_VGA_GFX_WIDTH_SCALEFACTOR)
#define VL_SDL2VK_DEFAULT_WINDOW_HEIGHT (6 * VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER * 3 / (5 * VL_VGA_GFX_HEIGHT_SCALEFACTOR))

static void VL_SDL2VK_SetVideoMode(int mode)
{
	if (mode == 0xD)
	{
		// Here is how the dimensions of the window are currently picked:
		// 1. The emulated 320x200 sub-window is first zoomed
		// by a factor of 3 (for each dimension) to 960x600.
		// 2. The height is then multiplied by 1.2, so the internal contents
		// (without the borders) have the aspect ratio of 4:3.
		//
		// There are a few more tricks in use to handle the overscan border
		// and VGA line doubling.
		vl_sdl2vk_window = SDL_CreateWindow(VL_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			VL_SDL2VK_DEFAULT_WINDOW_WIDTH, VL_SDL2VK_DEFAULT_WINDOW_HEIGHT,
			SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | (vl_isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
		vl_sdl2vk_screenWidth = VL_EGAVGA_GFX_WIDTH;
		vl_sdl2vk_screenHeight = VL_EGAVGA_GFX_HEIGHT;

		VL_CalculateRenderRegions(VL_SDL2VK_DEFAULT_WINDOW_WIDTH, VL_SDL2VK_DEFAULT_WINDOW_HEIGHT);

		SDL_SetWindowMinimumSize(vl_sdl2vk_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);

		//VL_SDL2VK_SetIcon(vl_sdl2vk_window);

		VL_SDL2VK_CreateVulkanInstance();
		VL_SDL2VK_LoadVKInstanceProcs();

		VkDebugReportCallbackCreateInfoEXT debugCallbackInfo = {};
		debugCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debugCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugCallbackInfo.pfnCallback = &VL_SDL2VK_DebugCallback;
		debugCallbackInfo.pUserData = 0;

		VkResult result = id_vkCreateDebugReportCallbackEXT(vl_sdl2vk_instance, &debugCallbackInfo, 0, &vl_sdl2vk_debugCallback);
		if (result != VK_SUCCESS)
			Quit("Couldn't create debug callback.");
		VL_SDL2VK_InitPhysicalDevice();
		VL_SDL2VK_LoadVKDeviceProcs();

		VL_SDL2VK_SetupSwapchain(VL_SDL2VK_DEFAULT_WINDOW_WIDTH, VL_SDL2VK_DEFAULT_WINDOW_HEIGHT);

		VL_SDL2VK_CreateRenderPass();

		VL_SDL2VK_CreateShaders();
		VL_SDL2VK_CreateDescriptorSetLayouts();
		VL_SDL2VK_CreatePipeline();

		VL_SDL2VK_CreateCommandBuffers();
		VL_SDL2VK_CreateFramebuffers(vl_integerWidth, vl_integerHeight);

		VL_SDL2VK_CreateUniformBuffer();

		// Compile the shader we use to emulate EGA palettes.

		// Generate palette texture

		// Setup framebuffer stuff, just in case.
		vl_sdl2vk_framebufferWidth = vl_sdl2vk_screenWidth;
		vl_sdl2vk_framebufferHeight = vl_sdl2vk_screenHeight;

		// Hide mouse cursor
		SDL_ShowCursor(0);
	}
	else
	{
		SDL_ShowCursor(1);
		vkDeviceWaitIdle(vl_sdl2vk_device);
		vkFreeCommandBuffers(vl_sdl2vk_device, vl_sdl2vk_commandPool, vl_sdl2vk_numSwapchainImages, vl_sdl2vk_commandBuffers);
		vkDestroyDescriptorPool(vl_sdl2vk_device, vl_sdl2vk_ubDescriptorPool, 0);
		vkDestroyDescriptorSetLayout(vl_sdl2vk_device, vl_sdl2vk_ubDescriptorSetLayout, 0);
		vkDestroyDescriptorSetLayout(vl_sdl2vk_device, vl_sdl2vk_samplerDescriptorSetLayout, 0);
		vkFreeMemory(vl_sdl2vk_device, vl_sdl2vk_uniformBufferMemory, 0);
		vkDestroyBuffer(vl_sdl2vk_device, vl_sdl2vk_uniformBuffer, 0);
		vkDestroyRenderPass(vl_sdl2vk_device, vl_sdl2vk_renderPass, 0);
		id_vkDestroySwapchainKHR(vl_sdl2vk_device, vl_sdl2vk_swapchain, 0);
		VL_SDL2VK_DestroySwapchain();
		vkDestroyShaderModule(vl_sdl2vk_device, vl_sdl2vk_vertShaderModule, 0);
		vkDestroyShaderModule(vl_sdl2vk_device, vl_sdl2vk_fragShaderModule, 0);
		vkDestroyCommandPool(vl_sdl2vk_device, vl_sdl2vk_commandPool, 0);
		vkDeviceWaitIdle(vl_sdl2vk_device);
		vkDestroyDevice(vl_sdl2vk_device, 0);
		vkDestroySurfaceKHR(vl_sdl2vk_instance, vl_sdl2vk_windowSurface, 0);
		id_vkDestroyDebugReportCallbackEXT(vl_sdl2vk_instance, vl_sdl2vk_debugCallback, 0);
		vkDestroyInstance(vl_sdl2vk_instance, 0);
		SDL_DestroyWindow(vl_sdl2vk_window);
	}
}

static void VL_SDL2VK_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL2VK_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)malloc(sizeof(VL_SDL2VK_Surface));
	surf->w = w;
	surf->h = h;
	surf->use = usage;
	if (usage == VL_SurfaceUsage_FrontBuffer)
	{
		VkResult result = VK_SUCCESS;
		VkImageCreateInfo stagingImageInfo = {};
		stagingImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		stagingImageInfo.imageType = VK_IMAGE_TYPE_2D;
		stagingImageInfo.extent.width = w;
		stagingImageInfo.extent.height = h;
		stagingImageInfo.extent.depth = 1;
		stagingImageInfo.mipLevels = 1;
		stagingImageInfo.arrayLayers = 1;
		stagingImageInfo.format = VK_FORMAT_R8_UINT;
		stagingImageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		stagingImageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		stagingImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		stagingImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stagingImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		result = vkCreateImage(vl_sdl2vk_device, &stagingImageInfo, 0, &surf->stagingImage);
		if (result != VK_SUCCESS)
			Quit("Failed to create staging image for front buffer surface.");

		VkMemoryRequirements stagingMemoryRequirements;
		vkGetImageMemoryRequirements(vl_sdl2vk_device, surf->stagingImage, &stagingMemoryRequirements);

		VkMemoryAllocateInfo stagingAllocInfo = {};
		stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		stagingAllocInfo.allocationSize = stagingMemoryRequirements.size;
		stagingAllocInfo.memoryTypeIndex = VL_SDL2VK_SelectMemoryType(stagingMemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		result = vkAllocateMemory(vl_sdl2vk_device, &stagingAllocInfo, 0, &surf->stagingMemory);
		if (result != VK_SUCCESS)
			Quit("Couldn't allocate memory for staging image.");

		vkBindImageMemory(vl_sdl2vk_device, surf->stagingImage, surf->stagingMemory, 0);

		VkImageSubresource stagingSubresource = {};
		stagingSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		stagingSubresource.mipLevel = 0;
		stagingSubresource.arrayLayer = 0;

		VkSubresourceLayout stagingSubresourceLayout;
		vkGetImageSubresourceLayout(vl_sdl2vk_device, surf->stagingImage, &stagingSubresource, &stagingSubresourceLayout);

		surf->pitch = stagingSubresourceLayout.rowPitch;

		vkMapMemory(vl_sdl2vk_device, surf->stagingMemory, 0, stagingSubresourceLayout.size, 0, &surf->data);

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = w;
		imageInfo.extent.height = h;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8_UINT;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		result = vkCreateImage(vl_sdl2vk_device, &imageInfo, 0, &surf->image);
		if (result != VK_SUCCESS)
			Quit("Failed to create image for front buffer surface.");

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(vl_sdl2vk_device, surf->image, &memoryRequirements);

		VkMemoryAllocateInfo imageAllocInfo = {};
		imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		imageAllocInfo.allocationSize = memoryRequirements.size;
		imageAllocInfo.memoryTypeIndex = VL_SDL2VK_SelectMemoryType(memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		result = vkAllocateMemory(vl_sdl2vk_device, &imageAllocInfo, 0, &surf->memory);
		if (result != VK_SUCCESS)
			Quit("Couldn't allocate memory for image.");

		vkBindImageMemory(vl_sdl2vk_device, surf->image, surf->memory, 0);

		VkImageViewCreateInfo imageViewInfo = {};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = surf->image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = VK_FORMAT_R8_UINT;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;

		result = vkCreateImageView(vl_sdl2vk_device, &imageViewInfo, 0, &surf->view);
		if (result != VK_SUCCESS)
			Quit("Couldn't create image view for surface.");

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.unnormalizedCoordinates = VK_TRUE;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

		result = vkCreateSampler(vl_sdl2vk_device, &samplerInfo, 0, &surf->sampler);
		if (result != VK_SUCCESS)
			Quit("Couldn't create sampler for surface.");

		VkCommandBuffer layoutChange = VL_SDL2VK_StartCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VL_SDL2VK_ImageLayoutBarrier(layoutChange, surf->stagingImage, VK_FORMAT_R8_UINT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		VL_SDL2VK_ImageLayoutBarrier(layoutChange, surf->image, VK_FORMAT_R8_UINT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		vkEndCommandBuffer(layoutChange);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &layoutChange;

		vkQueueSubmit(vl_sdl2vk_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(vl_sdl2vk_graphicsQueue);

		vkFreeCommandBuffers(vl_sdl2vk_device, vl_sdl2vk_commandPool, 1, &layoutChange);
	}
	else
	{
		surf->data = malloc(w * h); // 8-bit pal for now
		surf->pitch = w;
	}
	return surf;
}

static void VL_SDL2VK_UploadSurface(void *surface)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;

	VkResult result = VK_SUCCESS;

	VkCommandBuffer copyImageCmdBuf = VL_SDL2VK_StartCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VL_SDL2VK_ImageLayoutBarrier(copyImageCmdBuf, surf->image, VK_FORMAT_R8_UINT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	// Copy
	VkImageSubresourceLayers subresourceLayers = {};
	subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceLayers.baseArrayLayer = 0;
	subresourceLayers.mipLevel = 0;
	subresourceLayers.layerCount = 1;

	VkImageCopy imageCopy = {};
	imageCopy.srcSubresource = subresourceLayers;
	imageCopy.srcOffset = (VkOffset3D){0, 0, 0};
	imageCopy.dstSubresource = subresourceLayers;
	imageCopy.dstOffset = (VkOffset3D){0, 0, 0};
	imageCopy.extent.width = surf->w;
	imageCopy.extent.height = surf->h;
	imageCopy.extent.depth = 1;

	vkCmdCopyImage(copyImageCmdBuf, surf->stagingImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, surf->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

	VL_SDL2VK_ImageLayoutBarrier(copyImageCmdBuf, surf->image, VK_FORMAT_R8_UINT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

	vkEndCommandBuffer(copyImageCmdBuf);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyImageCmdBuf;

	vkQueueSubmit(vl_sdl2vk_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vl_sdl2vk_graphicsQueue);

	vkFreeCommandBuffers(vl_sdl2vk_device, vl_sdl2vk_commandPool, 1, &copyImageCmdBuf);
}

static void VL_SDL2VK_DestroySurface(void *surface)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		vkUnmapMemory(vl_sdl2vk_device, surf->stagingMemory);
		vkFreeMemory(vl_sdl2vk_device, surf->stagingMemory, 0);
		vkDestroySampler(vl_sdl2vk_device, surf->sampler, 0);
		vkDestroyImage(vl_sdl2vk_device, surf->stagingImage, 0);
		vkDestroyImageView(vl_sdl2vk_device, surf->view, 0);
		vkFreeMemory(vl_sdl2vk_device, surf->memory, 0);
		vkDestroyImage(vl_sdl2vk_device, surf->image, 0);
	}
	else
	{
		if (surf->data)
			free(surf->data);
	}
	free(surf);
}

static long VL_SDL2VK_GetSurfaceMemUse(void *surface)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;
	return surf->w * surf->h;
}

static void VL_SDL2VK_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_SDL2VK_SetGLClearColorFromBorder(void)
{
	/*glClearColor((GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0]/255,
	             (GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1]/255,
	             (GLclampf)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2]/255,
	             1.0f
	);*/
}

static void VL_SDL2VK_RefreshPaletteAndBorderColor(void *screen)
{
	static uint8_t sdl2vk_palette[16][3];

	for (int i = 0; i < 16; i++)
	{
		memcpy(sdl2vk_palette[i], VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]], 3);
	}
	// Load the palette into a texture.

	VL_SDL2VK_SetGLClearColorFromBorder();
}

static int VL_SDL2VK_SurfacePGet(void *surface, int x, int y)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;
	return ((uint8_t *)surf->data)[y * surf->pitch + x];
}

static void VL_SDL2VK_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		memset(((uint8_t *)surf->data) + _y * surf->pitch + x, colour, w);
	}
}

static void VL_SDL2VK_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	mapmask &= 0xF;
	colour &= mapmask;

	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
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

static void VL_SDL2VK_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)src_surface;
	VL_SDL2VK_Surface *dest = (VL_SDL2VK_Surface *)dst_surface;
	for (int _y = sy; _y < sy + sh; ++_y)
	{
		memcpy(((uint8_t *)dest->data) + (_y - sy + y) * dest->pitch + x, ((uint8_t *)surf->data) + _y * surf->pitch + sx, sw);
	}
}

static void VL_SDL2VK_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SDL2VK_Surface *srf = (VL_SDL2VK_Surface *)surface;
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

static void VL_SDL2VK_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_UnmaskedToPAL8(src, surf->data, x, y, surf->pitch, w, h);
}

static void VL_SDL2VK_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_UnmaskedToPAL8_PM(src, surf->data, x, y, surf->pitch, w, h, mapmask);
}

static void VL_SDL2VK_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_MaskedToPAL8(src, surf->data, x, y, surf->pitch, w, h);
}

static void VL_SDL2VK_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_MaskedBlitClipToPAL8(src, surf->data, x, y, surf->pitch, w, h, surf->w, surf->h);
}

static void VL_SDL2VK_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_1bppToPAL8(src, surf->data, x, y, surf->pitch, w, h, colour);
}

static void VL_SDL2VK_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_1bppToPAL8_PM(src, surf->data, x, y, surf->pitch, w, h, colour, mapmask);
}

static void VL_SDL2VK_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_1bppXorWithPAL8(src, surf->data, x, y, surf->pitch, w, h, colour);
}

static void VL_SDL2VK_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_1bppBlitToPAL8(src, surf->data, x, y, surf->pitch, w, h, colour);
}

static void VL_SDL2VK_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)dst_surface;
	VL_1bppInvBlitClipToPAL8(src, surf->data, x, y, surf->pitch, w, h, surf->w, surf->h, colour);
}

static void VL_SDL2VK_BindTexture(void *surface)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;
	VkResult result = VK_SUCCESS;

	VkDescriptorImageInfo descriptorInfo = {};
	descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptorInfo.imageView = surf->view;
	descriptorInfo.sampler = surf->sampler;

	VkWriteDescriptorSet writeSet = {};
	writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.dstSet = vl_sdl2vk_samplerDescriptorSet;
	writeSet.dstBinding = 1;
	writeSet.dstArrayElement = 0;
	writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeSet.descriptorCount = 1;
	writeSet.pImageInfo = &descriptorInfo;

	vkUpdateDescriptorSets(vl_sdl2vk_device, 1, &writeSet, 0, 0);
}

static void VL_SDL2VK_ScrollSurface(void *surface, int x, int y)
{
	VL_SDL2VK_Surface *surf = (VL_SDL2VK_Surface *)surface;
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
	VL_SDL2VK_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_SDL2VK_Present(void *surface, int scrlX, int scrlY)
{
	VkResult result = VK_SUCCESS;
	uint32_t framebufferIndex;

	int newW, newH;
	SDL_Vulkan_GetDrawableSize(vl_sdl2vk_window, &newW, &newH);
	if (vl_sdl2vk_flushSwapchain || vl_sdl2vk_swapchainSize.width != newW || vl_sdl2vk_swapchainSize.height != newH)
	{
		VL_SDL2VK_DestroySwapchain();
		VL_SDL2VK_SetupSwapchain(newW, newH);
		VL_SDL2VK_CreateFramebuffers(vl_integerWidth, vl_integerHeight);
		VL_SDL2VK_CreatePipeline();
		vl_sdl2vk_flushSwapchain = false;
	}

	result = vkAcquireNextImageKHR(vl_sdl2vk_device, vl_sdl2vk_swapchain, UINT64_MAX, vl_sdl2vk_imageAvailableSemaphore, VK_NULL_HANDLE, &framebufferIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		VL_SDL2VK_DestroySwapchain();
		VL_SDL2VK_SetupSwapchain(newW, newH);
		VL_SDL2VK_CreateFramebuffers(vl_integerWidth, vl_integerHeight);
		VL_SDL2VK_CreatePipeline();
		result = vkAcquireNextImageKHR(vl_sdl2vk_device, vl_sdl2vk_swapchain, UINT64_MAX, vl_sdl2vk_imageAvailableSemaphore, VK_NULL_HANDLE, &framebufferIndex);
	}

	VkSemaphore waitSemaphores[] = {vl_sdl2vk_imageAvailableSemaphore};
	VkPipelineStageFlags waitPipelineStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore doneSemaphores[] = {vl_sdl2vk_frameCompleteSemaphore};

	VL_SDL2VK_UploadSurface(surface);

	VL_SDL2VK_BindTexture(surface);

	VkRect2D fullRgn = {{vl_fullRgn_x, vl_fullRgn_y}, {(uint32_t)vl_fullRgn_w, (uint32_t)vl_fullRgn_h}};
	VkRect2D renderRgn = {{vl_renderRgn_x, vl_renderRgn_y}, {(uint32_t)vl_renderRgn_w, (uint32_t)vl_renderRgn_h}};
	VkClearColorValue borderColour = {{(float)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0] / 255,
		(float)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1] / 255,
		(float)VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2] / 255,
		1.0f}};
	VL_SDL2VK_PopulateCommandBuffer(framebufferIndex, fullRgn, renderRgn, borderColour);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitPipelineStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vl_sdl2vk_commandBuffers[framebufferIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = doneSemaphores;

	float *data;
	vkMapMemory(vl_sdl2vk_device, vl_sdl2vk_uniformBufferMemory, 0, sizeof(float) * 4 * 17, 0, (void **)&data);
	data[0] = scrlX;
	data[1] = scrlY;
	for (int i = 0; i < 16; ++i)
	{
		data[4 + 4 * i] = (float)VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][0] / 255.0;
		data[5 + 4 * i] = (float)VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][1] / 255.0;
		data[6 + 4 * i] = (float)VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][2] / 255.0;
		data[7 + 4 * i] = 255.0;
	}
	vkUnmapMemory(vl_sdl2vk_device, vl_sdl2vk_uniformBufferMemory);

	result = vkQueueSubmit(vl_sdl2vk_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		Quit("Couldn't submit command buffer.");

	vkQueueWaitIdle(vl_sdl2vk_graphicsQueue);

	VkSwapchainKHR swapchains[] = {vl_sdl2vk_swapchain};

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = doneSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &framebufferIndex;
	presentInfo.pResults = 0;

	vkQueuePresentKHR(vl_sdl2vk_presentQueue, &presentInfo);
}

static int VL_SDL2VK_GetActiveBufferId(void *surface)
{
	(void *)surface;
	return 0;
}

static int VL_SDL2VK_GetNumBuffers(void *surface)
{
	(void *)surface;
	return 1;
}

void VL_SDL2VK_FlushParams()
{
	SDL_SetWindowFullscreen(vl_sdl2vk_window, vl_isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	vl_sdl2vk_flushSwapchain = true;
}

static void VL_SDL2VK_WaitVBLs(int vbls)
{
	SDL_Delay(vbls * 1000 / 70);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl2vk_backend =
	{
		/*.setVideoMode =*/&VL_SDL2VK_SetVideoMode,
		/*.createSurface =*/&VL_SDL2VK_CreateSurface,
		/*.destroySurface =*/&VL_SDL2VK_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_SDL2VK_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_SDL2VK_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_SDL2VK_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_SDL2VK_SurfacePGet,
		/*.surfaceRect =*/&VL_SDL2VK_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_SDL2VK_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_SDL2VK_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_SDL2VK_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_SDL2VK_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_SDL2VK_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_SDL2VK_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_SDL2VK_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_SDL2VK_BitToSurface,
		/*.bitToSurface_PM =*/&VL_SDL2VK_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_SDL2VK_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_SDL2VK_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_SDL2VK_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_SDL2VK_ScrollSurface,
		/*.present =*/&VL_SDL2VK_Present,
		/*.getActiveBufferId =*/&VL_SDL2VK_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_SDL2VK_GetNumBuffers,
		/*.flushParams =*/&VL_SDL2VK_FlushParams,
		/*.waitVBLs =*/&VL_SDL2VK_WaitVBLs};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	vl_sdl2vk_arena = MM_ArenaCreate(5 * 1024 * 1024);
	vl_sdl2vk_tempArena = MM_ArenaCreate(2 * 1024 * 1024);
	return &vl_sdl2vk_backend;
}
