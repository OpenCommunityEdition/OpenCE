/* The native GL device has no Xbox push buffer or interlaced display filters.
   Keep the real rasterizer's configuration calls bounded and observable;
   never claim that those Xbox-only effects are active. */
#include "engine_rasterizer_services.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char rasterizer_render_state_extent[(D3DRS_MAX==144)?1:-1];
typedef char rasterizer_texture_state_extent[
    (D3DTSS_MAXSTAGES==4&&D3DTSS_MAX==32)?1:-1];

/* These exact types and array extents are declared by rasterizer_xbox.c.
   GetDeviceCaps fills the caps record from the active GL implementation;
   SetupSmartStates fills the shadow tables before they are consumed. */
D3DCAPS8 global_d3d_caps;
unsigned long renderstate_table[D3DRS_MAX];
unsigned long texturestagestate_table[D3DTSS_MAXSTAGES][D3DTSS_MAX];
D3DBaseTexture *texture_table[D3DTSS_MAXSTAGES];

static struct mac_rasterizer_display_options display_options;

void WINAPI Direct3D_SetPushBufferSize(DWORD size,DWORD kick_off_size)
{
    /* GL submits through its own command queue. Retain only bounded values
       for lifecycle diagnostics; do not allocate a pretend Xbox push buffer. */
    if(!size||size>8UL*1024UL*1024UL||!kick_off_size||kick_off_size>size) {
        fprintf(stderr,"native D3D push-buffer request is unsupported\n");
        abort();
    }
    display_options.push_buffer_size=size;
    display_options.kick_off_size=kick_off_size;
}

void WINAPI D3DDevice_SetFlickerFilter(DWORD filter)
{
    /* Xbox flicker filtering targets interlaced TV output. Native macOS output
       is progressive, so preserve the requested setting without resampling. */
    if(filter>5) {
        fprintf(stderr,"native D3D flicker-filter value is unsupported\n");
        abort();
    }
    display_options.flicker_filter=filter;
}

void WINAPI D3DDevice_SetSoftDisplayFilter(BOOL enabled)
{
    /* The source initializer disables this Xbox display filter. Enabling it
       has no native equivalent and fails closed. */
    if(enabled) {
        fprintf(stderr,"native D3D soft-display filter is unsupported\n");
        abort();
    }
    display_options.soft_display_filter=FALSE;
}

int mac_engine_rasterizer_services_get_display_options(
    struct mac_rasterizer_display_options *options)
{
    if(!options)return 0;
    *options=display_options;
    return 1;
}

void mac_engine_rasterizer_services_reset(void)
{
    display_options.push_buffer_size=0;
    display_options.kick_off_size=0;
    display_options.flicker_filter=0;
    display_options.soft_display_filter=FALSE;
    memset(&global_d3d_caps,0,sizeof(global_d3d_caps));
    memset(renderstate_table,0,sizeof(renderstate_table));
    memset(texturestagestate_table,0,sizeof(texturestagestate_table));
    memset(texture_table,0,sizeof(texture_table));
}
