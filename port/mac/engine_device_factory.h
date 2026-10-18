#ifndef HALO_MAC_ENGINE_DEVICE_FACTORY_H
#define HALO_MAC_ENGINE_DEVICE_FACTORY_H
#include "engine_device_state.h"

/* Native Direct3D entry points used by the real Xbox rasterizer initializer.
   Hidden CGL is the default; an interactive frontend can explicitly request
   a visible Carbon/AGL device before initialization. */
int mac_engine_device_factory_set_visible(int visible);
int mac_engine_device_factory_get_caps(D3DDevice *device,D3DCAPS8 *caps);
int mac_engine_device_factory_set_palette(D3DDevice *device,DWORD stage,
    D3DPalette *palette);
D3DPalette *mac_engine_device_factory_palette_for_stage(unsigned stage);
unsigned mac_engine_device_factory_palette_count(void);

#endif
