/*
VK_DEVICE.H

What the Vulkan renderer's guest files share with each other (d3d8_vk.c and xbox_textures_vk.c): the command stream the device
keeps, and the texture cache that writes into it. Guest only: vk_commands.h is the one both halves include.
*/

#ifndef __VK_DEVICE_H
#define __VK_DEVICE_H

#include "xgpu.h"
#include "vk_commands.h"

/* d3d8_vk.c: room for a command in the stream, its header filled in. It may hand the stream over, so a pointer it gave earlier
is not good after the next call */
void *vk_stream_command(uint32_t type, unsigned long size);
/* d3d8_vk.c: bytes copied into the stream now (the game's memory as it is at this call); the id commands name them by (a
vk_data_ref's id) */
uint32_t vk_data_put(const void *bytes, unsigned long size);

/* xbox_textures_vk.c: the image number of the Xbox texture whose header is resource (and the palette its stage's palettized
texture uses), sending its texels to the host if it is new or has been written since; 0 if there is none to draw. *kind
receives VK_IMAGE_2D, _3D or _CUBE, and description what xbox_textures knows of it. */
uint32_t vk_texture_get(const DWORD *resource, const D3DCOLOR *palette, int *kind, struct xgpu_texture_description *description);
/* once a frame (Present): ages the cache */
void vk_texture_cache_begin_frame(void);

#endif
