#ifndef HALO_MAC_ENGINE_DEVICE_RESOURCES_H
#define HALO_MAC_ENGINE_DEVICE_RESOURCES_H

#include "engine_device_state.h"

/* Native-only resource constructors called by the clean XDK inline methods.
   They preserve the shared Xbox texture memory layout and reject calls that
   do not target the live native device or an implemented format/pool. */
HRESULT WINAPI mac_engine_device_create_texture(UINT width,UINT height,UINT levels,
    DWORD usage,D3DFORMAT format,D3DPOOL pool,D3DTexture **texture);
HRESULT WINAPI mac_engine_device_create_cube_texture(UINT edge_length,UINT levels,
    DWORD usage,D3DFORMAT format,D3DPOOL pool,D3DCubeTexture **texture);
HRESULT WINAPI mac_engine_device_create_volume_texture(UINT width,UINT height,
    UINT depth,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,
    D3DVolumeTexture **texture);
/* OpenGL backing for the rasterizer's color render-target textures. The
   caller may bind the returned framebuffer when the active surface changes. */
int mac_engine_device_resources_target_info(D3DSurface *surface,
    unsigned *framebuffer,unsigned *texture,unsigned *width,unsigned *height);
int mac_engine_device_resources_texture_target(const D3DTexture *resource,unsigned *texture);
int mac_engine_device_resources_target_depth(D3DSurface *surface,unsigned *depth_buffer);
int mac_engine_device_resources_bind_target(D3DSurface *surface,D3DSurface *depth);
void mac_engine_device_resources_stop(void);

#endif
