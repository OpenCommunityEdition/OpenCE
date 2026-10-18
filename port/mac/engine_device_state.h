#ifndef HALO_MAC_ENGINE_DEVICE_STATE_H
#define HALO_MAC_ENGINE_DEVICE_STATE_H
#include <stddef.h>
struct mac_input_frame;
/* Native device-owned state. The CPU-only mode supports constructor probes;
   the bounded GPU path supports immediate draws and indexed stream subsets.
   Engine-thread only; bound external resources remain borrowed, device
   surfaces and compiled immediate-draw programs are owned. */
struct vertex_shader_object;
struct mac_device_state {
    D3DBaseTexture *textures[D3DTSS_MAXSTAGES];
    D3DPalette *palettes[D3DTSS_MAXSTAGES];
    float constants[192][4];
    struct { DWORD data; UINT stride; } streams[16];
    UINT base_vertex_index;
    DWORD index_data;
    D3DSurface back_buffer,depth_buffer;
    D3DSurface *render_target,*depth_stencil;
    D3DVIEWPORT8 viewport;
    D3DMATRIX transforms[D3DTS_MAX];
    D3DSHADERCONSTANTMODE shader_constant_mode;
    float viewport_scale[4],viewport_offset[4];
    struct vertex_shader_object *vertex_shader,*program_slots[136],*owned_shaders;
    unsigned long program_address,next_vertex_shader_id;
    float attributes[16][4];
    BOOL immediate_active;
    D3DPRIMITIVETYPE immediate_type;
    unsigned long immediate_written,immediate_count,immediate_capacity;
    float *immediate_vertices;
    unsigned long skipped_draws,skipped_clears;
    int headless;
    int gpu,gpu_frame_active;
    unsigned long gpu_default_framebuffer;
    unsigned long gpu_default_depth_renderbuffer;
    unsigned long gpu_width,gpu_height,gpu_present_count;
    unsigned long gpu_immediate_draw_count;
    unsigned long gpu_indexed_draw_count;
    int gpu_stream_draw_active;
    /* draws and clears into the back buffer so far (a copy of it is current while this holds still),
       and whether the other render targets are drawn top-down (HALO_MAC_FLIP_TARGETS) */
    unsigned long gpu_back_buffer_writes;
    int gpu_flip_targets;

};
int mac_device_state_start(void);
void mac_device_state_stop(void);
/* Scratch BSP decode-cache lifecycle hook; clear before certified world buffers
   are revoked or released. It is inert unless the opt-in cache was enabled. */
void mac_device_bsp_decode_cache_clear(void);
struct mac_device_bsp_decode_cache_stats {
    unsigned long long draw_hits,draw_misses,draw_bypasses;
    unsigned long long vertex_hits,vertex_misses,store_rejections;
};
void mac_device_bsp_decode_cache_get_stats(struct mac_device_bsp_decode_cache_stats *);
/* Explicit opt-in; allocates genuine CPU target storage, never opens a display. */
int mac_device_headless_start(unsigned long width,unsigned long height);
/* Bounded visible/offscreen OpenGL device owner. Supports real clear,
   readback, present, and strict immediate/indexed shader subsets. */
int mac_device_gpu_start(unsigned long width,unsigned long height,int visible,const char *title);
typedef int (*mac_device_gpu_frame_proc)(void *,const struct mac_input_frame *,unsigned,unsigned);
int mac_device_gpu_frame(mac_device_gpu_frame_proc,void *context);
/* Opaque identity backed by the live native state, matching d3d8_gl's
   device_pointer() contract. It conveys no GPU or presentation capability. */
D3DDevice *mac_device_headless_identity(void);
D3DDevice *mac_device_gpu_identity(void);
const struct mac_device_state *mac_device_state_get(void);
/* With HALO_MAC_LAZY_PRESENT=1, the next present copies its frame into the
   back buffer's memory (others do not); without it every present does. */
void mac_device_gpu_request_readback(void);
/* port: the pixels the back buffer's frame is drawn with (the picture's own at the Xbox's scale,
   more where mac.render_resolution asks), for the textures made of it */
void mac_device_render_region(unsigned *width,unsigned *height);
/* display.anti_aliasing "fxaa": the 3D view's bounds (the game's coordinates) smoothed in place; 0 if not done */
int mac_device_post_fxaa(int x0,int y0,int x1,int y1);
unsigned long long mac_device_state_constant_serial(void);
/* Source-equivalent NV2A NORMPACKED3 conversion used by the indexed stream
   expander; output is three normalized components. */
void mac_device_unpack_normpacked3(uint32_t packed,float out[3]);
#endif
