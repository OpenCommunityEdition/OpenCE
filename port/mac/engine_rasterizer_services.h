#ifndef HALO_MAC_ENGINE_RASTERIZER_SERVICES_H
#define HALO_MAC_ENGINE_RASTERIZER_SERVICES_H
#include "xdk_d3d8.h"

struct mac_rasterizer_display_options {
    DWORD push_buffer_size;
    DWORD kick_off_size;
    DWORD flicker_filter;
    BOOL soft_display_filter;
};

extern D3DCAPS8 global_d3d_caps;

int mac_engine_rasterizer_services_get_display_options(
    struct mac_rasterizer_display_options *options);
void mac_engine_rasterizer_services_reset(void);

#endif
