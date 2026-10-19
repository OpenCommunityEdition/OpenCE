/* Native renderer device state. The explicit headless mode owns CPU targets;
   GPU mode translates supported NV2A programs and draw state to legacy GL. */
#include "mac_flags.h"
#include "engine_device_state.h"
#include "engine_device_resources.h"
#define MAC_INPUT_NO_CARBON
#include "mac_window.h"
#undef MAC_INPUT_NO_CARBON
/* Platform boundary uses native libc, outside engine memory instrumentation. */
#undef memset
#undef memcpy
#undef memcmp
#undef realloc
#undef calloc
#undef malloc
#undef free
#include "native_resource_memory.h"
#include "mac_resource_declarations.h"
#include <setjmp.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "mac_nv2a_draw_api.h"
#include "mac_glsl120.h"
#include "mac_gpu_textures.h"
#include "mac_gl_visibility.h"
#include "mac_gl_program_cache.h"
#include "mac_shader_source_cache.h"
#include "mac_render_timing.h"
#include "mac_load_timing.h" /* port: load timing */
#include "mac_vertex_upload.h"
#include "mac_index_decode_cache.h"
#include "bsp_decode_cache.h"
#include "mac_nv2a_arb.h"
#include "mac_nv2a_arbfp.h"
#include <OpenGL/OpenGL.h>
#include <mach/mach_time.h>
#include "engine_screen.h"
#include "mac_render_scale.h" /* port: the render resolution */
#include <stdlib.h>
#include <dlfcn.h>
void mac_gpu_textures_meter_draw(int meter); /* mac_gpu_textures.c */
const unsigned char *mac_native_bitmap_cache_rgba(const void *,uint32_t,uint32_t *,uint32_t *,size_t *);
#define VERTEX_PROGRAM_SLOTS 136
#define VERTEX_SHADER_SIGNATURE 0x76736864UL
#define MAC_D3DSHADE_GOURAUD 2UL
struct vertex_element { unsigned char reg,stream,type,bytes; unsigned short offset; };
struct vertex_shader_object {
 unsigned long signature,id; DWORD *instructions; unsigned long instruction_count;
 struct nv2a_vertex_lighting lighting;
 struct vertex_element elements[16]; unsigned long element_count,packed_mask;
 struct vertex_shader_object *next;
};
void d3d8_surface_initialize(D3DSurface *,D3DFORMAT,unsigned long,unsigned long);
static struct mac_device_state device;
static int active;
static struct mac_window gpu_window;
static int gpu_visibility_started;
static int gpu_visibility_active;
static struct mac_gl_program_cache gpu_program_cache;
static struct mac_vertex_upload gpu_vertex_upload;
static int gpu_compact_vertex_upload;
static struct mac_shader_source_cache gpu_source_cache;
static struct mac_index_decode_cache gpu_index_decode_cache;
static unsigned long long gpu_stream_decoded, gpu_stream_reused;
static int gpu_indexed_vertex_reuse;
static struct bsp_decode_cache gpu_bsp_decode_cache;
static int gpu_bsp_decode_cache_enabled;
/* Opt-in (HALO_MAC_WORLD_VBO=1): certified world vertex buffers, decoded once
   into GL buffers holding only their declared attributes, then drawn with the
   game's own indices. */
#define WORLD_VBO_MAX_ENTRIES 512
#define WORLD_VBO_BUDGET (32UL*1024UL*1024UL)
#define WORLD_VBO_MAX_VERTICES 262144UL
struct world_vbo_stream_key {
    unsigned long slot,generation,base_offset,stride,capacity;unsigned long long owner;
};
struct world_vbo_entry {
    int live,rejected,gather;unsigned char declaration[BSP_DECODE_MAX_DECLARATION_BYTES];
    size_t declaration_bytes;struct world_vbo_stream_key keys[16];unsigned long key_count;
    GLuint buffer;unsigned long vertex_count,stride,bytes,mask,offsets[XGPU_VERTEX_ATTRIBUTE_COUNT];
    unsigned char sizes[XGPU_VERTEX_ATTRIBUTE_COUNT]; /* floats stored per attribute */
    unsigned long long last_use;
    /* the decoded bone address (attribute 5 x, y) of every vertex, when
       declared: a gathering draw computes its rows from these alone */
    float *addresses;
    /* the largest bone address (attribute 5 x or y) of any vertex, before the program's scale */
    float bone_max;
};
static int gpu_world_vbo_enabled;
/* Opt-in (HALO_MAC_LEAN_DRAW=1): no per-draw GL state queries or restores
   (every draw sets what it uses, and nothing else draws), the program kept
   bound, and each program's uniforms resolved once and uploaded only when
   changed; vertex constants by the shared renderer's changed-range scheme. */
#define GPU_UNIFORM_BLOCKS 256
struct gpu_uniform_block {
    GLuint program;
    GLint texture_scale,constants,viewport_scale,viewport_offset;
    GLint ps_c0,ps_c1,ps_final_c0,ps_final_c1,fog_color,fog_parameters,alpha_reference,screen_offset;
    unsigned long constant_count;int constants_consecutive;
    unsigned long long constants_serial;int constants_valid;
    float last_texture_scale[16],last_viewport_scale[4],last_viewport_offset[4];
    float last_ps_c0[32],last_ps_c1[32],last_ps_final_c0[4],last_ps_final_c1[4];
    float last_fog_color[4],last_fog_parameters[4],last_alpha_reference,last_screen_offset;
    unsigned long valid;
};
static int gpu_lean_draw;
static GLuint gpu_current_program;
static struct gpu_uniform_block gpu_uniform_blocks[GPU_UNIFORM_BLOCKS];
static unsigned long gpu_uniform_block_count,gpu_uniform_evictions_seen;
static struct world_vbo_entry gpu_world_vbos[WORLD_VBO_MAX_ENTRIES];
/* the entry last found or built for each hash of a draw's first stream key
   and declaration: a hint checked before the full search */
#define WORLD_VBO_HINTS 1024
static unsigned short gpu_world_vbo_hints[WORLD_VBO_HINTS];
static unsigned long gpu_world_vbo_bytes,gpu_world_vbo_peak;
static unsigned long long gpu_world_vbo_serial;
static unsigned long long gpu_world_vbo_draws,gpu_world_vbo_builds,gpu_world_vbo_rejections;
static unsigned long long gpu_world_vbo_fallbacks,gpu_world_vbo_evictions,gpu_world_vbo_bypasses;
static unsigned long long gpu_world_vbo_gather_skips,gpu_world_vbo_gather_draws;
static float *gpu_gather_rows;static unsigned long gpu_gather_rows_capacity;
/* (HALO_MAC_LAZY_PRESENT) Present copies the frame into the back buffer's
   memory only when a reader asked for that frame; otherwise it only flushes,
   so the CPU does not wait each frame for the GPU to finish. */
static int gpu_lazy_present,gpu_readback_requested;
/* (HALO_MAC_LEAN_DRAW) The GL state the draws last set, so that a draw calls
   GL only for what differs: every call marks state the driver revalidates.
   Forgotten at each frame's start, by clears (which set masks and the
   scissor themselves) and before every draw that is not lean. */
enum {
    SHADOW_VIEWPORT=1,SHADOW_SCISSOR=2,SHADOW_DEPTH_RANGE=4,SHADOW_COLOR_MASK=8,
    SHADOW_DEPTH_FUNC=16,SHADOW_DEPTH_MASK=32,SHADOW_CULL=64,SHADOW_BLEND_FUNC=128,
    SHADOW_BLEND_EQUATION=256,SHADOW_BLEND_COLOR=512,SHADOW_STENCIL_FUNC=1024,
    SHADOW_STENCIL_MASK=2048,SHADOW_STENCIL_OP=4096,SHADOW_ARRAY_BUFFER=8192,SHADOW_ARB_PROGRAM=16384,
    SHADOW_ARB_FRAGMENT=32768
};
enum {
    SHADOW_CAP_SCISSOR,SHADOW_CAP_DEPTH,SHADOW_CAP_STENCIL,SHADOW_CAP_CULL,SHADOW_CAP_BLEND,
    SHADOW_CAP_DITHER,SHADOW_CAP_VERTEX_PROGRAM,SHADOW_CAP_POINT_SIZE,SHADOW_CAP_FRAGMENT_PROGRAM,SHADOW_CAPS
};
struct gl_attribute_shadow {
    unsigned char enabled; /* 0 unknown, 1 off, 2 on */
    unsigned char pointer_valid,value_valid;GLint size;GLsizei stride;GLuint buffer;const void *pointer;
    float value[4];
};
static struct {
    unsigned long valid;unsigned char caps[SHADOW_CAPS]; /* 0 unknown, 1 off, 2 on */
    GLint viewport[4],scissor[4];GLclampd depth_range[2];GLboolean color_mask[4],depth_mask;
    GLenum depth_func,cull_face,front_face,blend_source,blend_destination,blend_equation;
    GLfloat blend_color[4];GLenum stencil_func,stencil_ops[3];GLint stencil_ref;
    GLuint stencil_value_mask,stencil_write_mask,array_buffer,arb_program,arb_fragment;
    struct gl_attribute_shadow attributes[XGPU_VERTEX_ATTRIBUTE_COUNT];
} gl_shadow;
static void mac_gl_shadow_forget(void){memset(&gl_shadow,0,sizeof(gl_shadow));}
static void mac_gl_shadow_forget_attributes(void)
{memset(gl_shadow.attributes,0,sizeof(gl_shadow.attributes));gl_shadow.valid&=~(unsigned long)SHADOW_ARRAY_BUFFER;}
static void mac_gl_shadow_cap(unsigned slot,GLenum cap,int on)
{
    unsigned char want=(unsigned char)(on?2:1);
    if(gl_shadow.caps[slot]==want)return;
    if(on)glEnable(cap);else glDisable(cap);
    gl_shadow.caps[slot]=want;
}
#define SHADOW_SAME(bit,condition) ((gl_shadow.valid&(bit))&&(condition))
static void mac_gl_shadow_viewport(GLint x,GLint y,GLsizei w,GLsizei h)
{
    if(SHADOW_SAME(SHADOW_VIEWPORT,gl_shadow.viewport[0]==x&&gl_shadow.viewport[1]==y&&
       gl_shadow.viewport[2]==w&&gl_shadow.viewport[3]==h))return;
    glViewport(x,y,w,h);gl_shadow.viewport[0]=x;gl_shadow.viewport[1]=y;
    gl_shadow.viewport[2]=w;gl_shadow.viewport[3]=h;gl_shadow.valid|=SHADOW_VIEWPORT;
}
static void mac_gl_shadow_scissor(GLint x,GLint y,GLsizei w,GLsizei h)
{
    if(SHADOW_SAME(SHADOW_SCISSOR,gl_shadow.scissor[0]==x&&gl_shadow.scissor[1]==y&&
       gl_shadow.scissor[2]==w&&gl_shadow.scissor[3]==h))return;
    glScissor(x,y,w,h);gl_shadow.scissor[0]=x;gl_shadow.scissor[1]=y;
    gl_shadow.scissor[2]=w;gl_shadow.scissor[3]=h;gl_shadow.valid|=SHADOW_SCISSOR;
}
static void mac_gl_shadow_depth_range(GLclampd n,GLclampd f)
{
    if(SHADOW_SAME(SHADOW_DEPTH_RANGE,gl_shadow.depth_range[0]==n&&gl_shadow.depth_range[1]==f))return;
    glDepthRange(n,f);gl_shadow.depth_range[0]=n;gl_shadow.depth_range[1]=f;gl_shadow.valid|=SHADOW_DEPTH_RANGE;
}
static void mac_gl_shadow_color_mask(GLboolean r,GLboolean g,GLboolean b,GLboolean a)
{
    if(SHADOW_SAME(SHADOW_COLOR_MASK,gl_shadow.color_mask[0]==r&&gl_shadow.color_mask[1]==g&&
       gl_shadow.color_mask[2]==b&&gl_shadow.color_mask[3]==a))return;
    glColorMask(r,g,b,a);gl_shadow.color_mask[0]=r;gl_shadow.color_mask[1]=g;
    gl_shadow.color_mask[2]=b;gl_shadow.color_mask[3]=a;gl_shadow.valid|=SHADOW_COLOR_MASK;
}
static void mac_gl_shadow_depth_func(GLenum f)
{if(SHADOW_SAME(SHADOW_DEPTH_FUNC,gl_shadow.depth_func==f))return;glDepthFunc(f);gl_shadow.depth_func=f;gl_shadow.valid|=SHADOW_DEPTH_FUNC;}
static void mac_gl_shadow_depth_mask(GLboolean m)
{if(SHADOW_SAME(SHADOW_DEPTH_MASK,gl_shadow.depth_mask==m))return;glDepthMask(m);gl_shadow.depth_mask=m;gl_shadow.valid|=SHADOW_DEPTH_MASK;}
static void mac_gl_shadow_cull(GLenum face,GLenum front)
{
    if(SHADOW_SAME(SHADOW_CULL,gl_shadow.cull_face==face&&gl_shadow.front_face==front))return;
    glCullFace(face);glFrontFace(front);gl_shadow.cull_face=face;gl_shadow.front_face=front;gl_shadow.valid|=SHADOW_CULL;
}
static void mac_gl_shadow_blend(GLenum source,GLenum destination,GLenum equation,const GLfloat color[4])
{
    if(!SHADOW_SAME(SHADOW_BLEND_FUNC,gl_shadow.blend_source==source&&gl_shadow.blend_destination==destination)) {
        glBlendFunc(source,destination);gl_shadow.blend_source=source;gl_shadow.blend_destination=destination;
        gl_shadow.valid|=SHADOW_BLEND_FUNC;
    }
    if(!SHADOW_SAME(SHADOW_BLEND_EQUATION,gl_shadow.blend_equation==equation)) {
        glBlendEquation(equation);gl_shadow.blend_equation=equation;gl_shadow.valid|=SHADOW_BLEND_EQUATION;
    }
    if(!SHADOW_SAME(SHADOW_BLEND_COLOR,!memcmp(gl_shadow.blend_color,color,sizeof(gl_shadow.blend_color)))) {
        glBlendColor(color[0],color[1],color[2],color[3]);
        memcpy(gl_shadow.blend_color,color,sizeof(gl_shadow.blend_color));gl_shadow.valid|=SHADOW_BLEND_COLOR;
    }
}
static void mac_gl_shadow_stencil(GLenum func,GLint ref,GLuint mask,GLuint write,GLenum fail,GLenum zfail,GLenum pass)
{
    if(!SHADOW_SAME(SHADOW_STENCIL_FUNC,gl_shadow.stencil_func==func&&gl_shadow.stencil_ref==ref&&
       gl_shadow.stencil_value_mask==mask)) {
        glStencilFunc(func,ref,mask);gl_shadow.stencil_func=func;gl_shadow.stencil_ref=ref;
        gl_shadow.stencil_value_mask=mask;gl_shadow.valid|=SHADOW_STENCIL_FUNC;
    }
    if(!SHADOW_SAME(SHADOW_STENCIL_MASK,gl_shadow.stencil_write_mask==write)) {
        glStencilMask(write);gl_shadow.stencil_write_mask=write;gl_shadow.valid|=SHADOW_STENCIL_MASK;
    }
    if(!SHADOW_SAME(SHADOW_STENCIL_OP,gl_shadow.stencil_ops[0]==fail&&gl_shadow.stencil_ops[1]==zfail&&
       gl_shadow.stencil_ops[2]==pass)) {
        glStencilOp(fail,zfail,pass);gl_shadow.stencil_ops[0]=fail;gl_shadow.stencil_ops[1]=zfail;
        gl_shadow.stencil_ops[2]=pass;gl_shadow.valid|=SHADOW_STENCIL_OP;
    }
}
static void mac_gl_shadow_array_buffer(GLuint buffer)
{
    if(SHADOW_SAME(SHADOW_ARRAY_BUFFER,gl_shadow.array_buffer==buffer))return;
    glBindBuffer(GL_ARRAY_BUFFER,buffer);gl_shadow.array_buffer=buffer;gl_shadow.valid|=SHADOW_ARRAY_BUFFER;
}
static void mac_gl_shadow_attribute_array(GLuint index,GLint size,GLsizei stride,const void *pointer)
{
    struct gl_attribute_shadow *a=&gl_shadow.attributes[index];
    if(a->enabled!=2){glEnableVertexAttribArray(index);a->enabled=2;}
    /* (the pointer is captured with the array buffer bound now) */
    if(a->pointer_valid&&(gl_shadow.valid&SHADOW_ARRAY_BUFFER)&&a->buffer==gl_shadow.array_buffer&&
       a->size==size&&a->stride==stride&&a->pointer==pointer)return;
    glVertexAttribPointer(index,size,GL_FLOAT,GL_FALSE,stride,pointer);
    a->pointer_valid=(gl_shadow.valid&SHADOW_ARRAY_BUFFER)!=0;a->buffer=gl_shadow.array_buffer;
    a->size=size;a->stride=stride;a->pointer=pointer;
}
static void mac_gl_shadow_attribute_off(GLuint index)
{
    struct gl_attribute_shadow *a=&gl_shadow.attributes[index];
    if(a->enabled!=1){glDisableVertexAttribArray(index);a->enabled=1;}
}
static void mac_gl_shadow_attribute_value(GLuint index,const float value[4])
{
    struct gl_attribute_shadow *a=&gl_shadow.attributes[index];
    mac_gl_shadow_attribute_off(index);
    if(a->value_valid&&!memcmp(a->value,value,sizeof(a->value)))return;
    glVertexAttrib4fv(index,value);memcpy(a->value,value,sizeof(a->value));a->value_valid=1;
}
#undef SHADOW_SAME
/* port: the render resolution. The game still sees its 640x480 (or wider,
   mac_window.c: 480 lines and as many columns as the window's shape gives);
   the GL viewport, scissor and clears of the back buffer are scaled, so that
   the frame is drawn with more pixels (mac.render_resolution: the window's
   own, or a number of lines) into the lower left of the window's framebuffer,
   and scaled to the window when it is presented. HALO_MAC_RENDER_SCALE=0.4..4
   (diagnostic) takes the place of the setting; below 1 the frame is drawn
   smaller and stretched back to the game's size at the present. */
static float gpu_render_scale=1.0f;
static unsigned gpu_region_width,gpu_region_height;
static GLuint gpu_scale_texture;static unsigned gpu_scale_texture_width,gpu_scale_texture_height;
/* installed by engine_render_services.c, which reads config.toml: the
   setting's words ("auto", "native", "original", lines), and the card's video
   memory and whether the machine is a PowerPC */
const char *(*mac_device_render_setting_hook)(int *vram_mb,int *is_ppc);
static int mac_render_scaled(void)
{
    return gpu_render_scale!=1.0f&&device.render_target==&device.back_buffer;
}
static void mac_render_scale_rect(GLint *x,GLint *y,GLsizei *w,GLsizei *h)
{
    GLint x0=(GLint)((float)*x*gpu_render_scale+0.5f),y0=(GLint)((float)*y*gpu_render_scale+0.5f);
    GLint x1=(GLint)((float)(*x+*w)*gpu_render_scale+0.5f),y1=(GLint)((float)(*y+*h)*gpu_render_scale+0.5f);
    *x=x0;*y=y0;*w=x1-x0;*h=y1-y0;
}
/* Rows of the render targets the game samples (shadow maps, the radar's
   blips, the water's ripples, the screen effects' sources) are top-down on the
   Xbox, and its texture coordinates assume it. OpenGL's framebuffer objects
   have row 0 at the bottom, so a draw into one is made upside down: the
   clip y is negated (the viewport scale's sign, which every vertex program's
   position divides by), the viewport and scissor count rows from the top,
   and the front face turns over. The back buffer is the window's, drawn
   upright; a texture of it is copied the other way up (mac_gpu_textures.c).
   HALO_MAC_FLIP_TARGETS=0 draws every target as the window is. */
static int mac_target_flipped(void)
{
    return device.gpu_flip_targets&&device.render_target&&device.render_target!=&device.back_buffer;
}
/* (HALO_MAC_FLIP_LOG) the draws made into each render target other than the
   back buffer, by its data address and size, reported when the device stops */
static int gpu_flip_logging;
static struct { unsigned long data,size,draws; } gpu_flip_log[24];
static unsigned gpu_flip_log_count;
static void mac_flip_log_draw(void)
{
    unsigned i;
    if(!device.render_target)return;
    for(i=0;i<gpu_flip_log_count;i++)
        if(gpu_flip_log[i].data==device.render_target->Data&&gpu_flip_log[i].size==device.render_target->Size){++gpu_flip_log[i].draws;return;}
    if(gpu_flip_log_count<24) {
        gpu_flip_log[gpu_flip_log_count].data=device.render_target->Data;gpu_flip_log[gpu_flip_log_count].size=device.render_target->Size;
        gpu_flip_log[gpu_flip_log_count++].draws=1;
    }
}
static const float *mac_gpu_viewport_scale(float copy[4])
{
    if(!mac_target_flipped())return device.viewport_scale;
    copy[0]=device.viewport_scale[0];copy[1]=-device.viewport_scale[1];
    copy[2]=device.viewport_scale[2];copy[3]=device.viewport_scale[3];
    return copy;
}
/* The ARB environment parameters last sent (valid when gpu_arb_env_valid
   has the parameter's bit), so that each draw sends only changes. */
static float gpu_arb_env[MAC_NV2A_ARB_ENV_COUNT][4];
static unsigned char gpu_arb_env_valid[MAC_NV2A_ARB_ENV_COUNT];
static unsigned long long gpu_arb_env_writes;
/* constants_serial when every changed constant was last sent */
static unsigned long long gpu_arb_env_constants_serial;
/* (HALO_MAC_ARB_FRAGMENT=1, with HALO_MAC_ARB_VERTEX) Pixel shaders as ARB
   fragment programs (mac_nv2a_arbfp.c): cheaper to switch to and to give
   constants than GLSL programs on the Radeons, and folded per state. Keyed
   by the state they read, the vertex program's fog slot and the projective
   stages; the source cache's identity of the pixel source finds them
   without comparing states. */
#define ARBFP_PROGRAMS 160
/* (HALO_MAC_FF_COMBINERS=1, default off, with HALO_MAC_ARB_FRAGMENT) Pixel
   shaders that GL_ARB_texture_env_combine expresses exactly (see
   mac_nv2a_arbfp_lower_fixed) draw with the texture units' combiner state and
   no fragment program, as Halo for the Mac did on the ATI cards (the Mac
   binary imports ATI_texture_env_combine3). Their vertex program is the
   variant that writes the coordinates and the diffuse colour where fixed
   function reads them (MAC_NV2A_ARB_RELATIVE_FF). Needs lean drawing. */
/* (HALO_MAC_VAR=1, default off) the vertex-array-range ring, below */
#define VAR_SEGMENTS 8
static unsigned long var_segment_bytes=1024UL*1024UL;
#define VAR_SEGMENT_BYTES var_segment_bytes
static int gpu_var_enabled=-1;
static unsigned char *gpu_var_base;
static GLuint gpu_var_fence[VAR_SEGMENTS];static unsigned char gpu_var_fence_set[VAR_SEGMENTS];
static unsigned gpu_var_segment;static size_t gpu_var_used;
static unsigned long long gpu_var_draws,gpu_var_bytes,gpu_var_fallbacks,gpu_var_waits,gpu_var_stalls;
static int gpu_ff_enabled=-1;
static int gpu_ff_combine3;
static void mac_ff_leave(void);
static unsigned long long gpu_ff_draws,gpu_ff_texenv_writes,gpu_ff_const_writes,gpu_ff_unit_toggles,gpu_ff_alpha_writes;
struct arbfp_program {
    struct mac_nv2a_arbfp_state state;int fog_texcoord,fog_component;unsigned long projective;
    GLuint program;int failed;struct mac_nv2a_arbfp_info info;struct mac_shader_source_identity identity;
    unsigned long long draws;char *dump;
    struct mac_nv2a_ff_program ff;char ff_reason[120];
};
static struct arbfp_program gpu_arbfp[ARBFP_PROGRAMS];
static unsigned long gpu_arbfp_count,gpu_arbfp_next;
static unsigned long gpu_prog_epoch=1;
static int gpu_arbfp_enabled=-1;
static float gpu_arbfp_env[MAC_NV2A_ARBFP_ENV_COUNT][4];
static unsigned char gpu_arbfp_env_valid[MAC_NV2A_ARBFP_ENV_COUNT];
static unsigned long long gpu_arbfp_draws,gpu_arbfp_failures,gpu_arbfp_env_writes;
static unsigned long long gpu_vertex_program_binds,gpu_fragment_program_binds;
static int gpu_diag_fixed_programs;
/* the render target set is one the native renderer cannot draw into */
static int gpu_target_skipped;static unsigned long long gpu_target_skipped_draws;
/* Diagnostic only (HALO_MAC_DIAG_SEGMENTS=1): time spent in the parts of
   an indexed draw's preparation, printed with the statistics */
static int gpu_diag_segments=-1;
/* (HALO_MAC_PERF_ENV_LOG) the vertex constants a draw sends, found from the
   log of writes rather than a scan of all of them */
static int gpu_env_log;
static int gpu_fast_minmax;
static int gpu_fast_jmp;
static uint64_t gpu_segment_time[12];static unsigned long gpu_segment_calls;
/* port: four floats equal bit for bit (memcmp's answer, without the call: each constant a draw sends is compared) */
typedef unsigned int __attribute__((__may_alias__)) mac_alias_word;
static __inline__ int mac_vec4_same(const float *a,const float *b)
{
    const mac_alias_word *x=(const mac_alias_word *)a,*y=(const mac_alias_word *)b;
    return x[0]==y[0]&&x[1]==y[1]&&x[2]==y[2]&&x[3]==y[3];
}
static void mac_arbfp_env_set(unsigned index,const float value[4])
{
    if(index>=MAC_NV2A_ARBFP_ENV_COUNT)return;
    if(gpu_arbfp_env_valid[index]&&mac_vec4_same(gpu_arbfp_env[index],value))return;
    glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,index,value);
    memcpy(gpu_arbfp_env[index],value,sizeof(gpu_arbfp_env[index]));
    gpu_arbfp_env_valid[index]=1;++gpu_arbfp_env_writes;
}

static void mac_arb_env_set(GLuint index,const float value[4])
{
    if(index>=MAC_NV2A_ARB_ENV_COUNT)return;
    if(gpu_arb_env_valid[index]&&mac_vec4_same(gpu_arb_env[index],value))return;
    glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,index,value);
    memcpy(gpu_arb_env[index],value,sizeof(gpu_arb_env[index]));
    gpu_arb_env_valid[index]=1;++gpu_arb_env_writes;
}
/* (HALO_MAC_INDEX_CACHE) World draws' indices kept in one GL element buffer,
   filled from the front, so the driver does not copy client indices (and
   wire and unwire their pages) on every draw. Index memory is not write
   tracked: a cached span is used only while it still equals the game's
   indices, which a copy of the buffer's contents is compared with. A full
   buffer is orphaned and its spans forgotten. */
#define INDEX_CACHE_BYTES (2UL*1024UL*1024UL)
#define INDEX_CACHE_SLOTS 1024
struct index_cache_entry {const WORD *source;unsigned long count,offset;};
static int gpu_index_cache_enabled;
static GLuint gpu_index_arena;
static unsigned long gpu_index_arena_used;
static WORD *gpu_index_shadow;
static struct index_cache_entry gpu_index_entries[INDEX_CACHE_SLOTS];
static unsigned long long gpu_index_hits,gpu_index_uploads,gpu_index_resets,gpu_index_bypasses;
/* port: the range of indices of draws seen before (see the draw function) */
#define INDEX_RANGE_SLOTS 512
struct index_range_entry {const WORD *indices;unsigned long count,base,generation;WORD minimum,maximum;};
static struct index_range_entry gpu_index_ranges[INDEX_RANGE_SLOTS];
static int gpu_index_range_enabled=-1;
static unsigned long long gpu_index_range_hits;
void mac_device_gpu_request_readback(void){gpu_readback_requested=1;}
static long mac_index_cache_span(const WORD *indices,unsigned long count);
static struct {
    int active;const struct world_vbo_entry *entry;const WORD *indices;unsigned long base_vertex;
    const unsigned char *streams[16];unsigned long strides[16];
    unsigned long minimum,maximum;int gather_active;
} gpu_world_draw;
static void mac_world_vbo_clear(void);
static void mac_world_draw_decode_records(void);
/* Opt-in (HALO_MAC_ARB_VERTEX="31,50"): the listed vertex shader table entries,
   which Apple's GLSL compiler sends to its software vertex path on R300-class
   GPUs, run as ARB vertex programs (port/mac/mac_nv2a_arb.c) with a
   fragment-only GLSL program. Any refusal keeps the GLSL path. */
#define ARB_VERTEX_PROGRAMS 128
#define ARB_FRAGMENT_PROGRAMS 128
#define GPU_WINDOW_SIZES 4
struct arb_vertex_program {
    unsigned long id;int gather,failed;GLuint program;struct mac_nv2a_arb_layout layout;
    /* port: the same program with its relative reads confined to a window of the constants
       (mac_arb_vertex_window), one for each size of gpu_window_size; NULL until asked for */
    const struct arb_vertex_program *windows[GPU_WINDOW_SIZES];unsigned char windows_tried[GPU_WINDOW_SIZES];
};
struct arb_fragment_program {
    const struct arb_vertex_program *vertex;char *source;unsigned long hash,projective;GLuint program;
    struct mac_shader_source_identity identity; /* of the source it was made from */
};
/* (HALO_MAC_PROJ_FETCH=1) 2D stages whose coordinates need no scale fetch
   with texture2DProj straight from the varying: the generated division by w
   (1 where w is 0) and the scale multiply make every fetch a dependent read
   on the R300. Both divide by the interpolated w, so only a primitive whose
   w is 0 throughout differs. */
static int gpu_projective_fetch=-1;
static struct arb_vertex_program gpu_arb_vertex[ARB_VERTEX_PROGRAMS];
/* port: (HALO_MAC_DM_PROGRAMS=0 turns it off) the programs a draw found, kept for a draw with the same inputs:
   the vertex shader object, the pixel state key (the textures' kinds among it), the textures' scales (they
   choose the projective fetches) and whether the stream is decoded. The lookups are functions of these alone,
   so a draw that repeats them takes what they gave; gpu_prog_epoch counts what could change that
   (a fragment program evicted, a vertex shader created or deleted). The instances of a model, drawn in turn,
   differ only in constants. */
#define PROG_MEMOS 4
static struct {
    const struct vertex_shader_object *vertex;unsigned long epoch;int stream;
    struct nv2a_pixel_shader_key key;float scale[4][4];
    const struct arb_vertex_program *arb_vertex;const struct arbfp_program *arbfp;unsigned long input_mask;
} gpu_prog_memo[PROG_MEMOS];
static unsigned gpu_prog_memo_next;
static int gpu_prog_memo_enabled=-1;
static unsigned long long gpu_prog_memo_hits,gpu_prog_memo_misses;

static struct arb_fragment_program gpu_arb_fragment[ARB_FRAGMENT_PROGRAMS];
static unsigned long gpu_arb_vertex_count,gpu_arb_fragment_next;
static unsigned long long gpu_arb_draws,gpu_arb_failures;
static int mac_arb_vertex_wanted(unsigned long id)
{
    static int parsed=0,all=0;static unsigned long ids[16];static unsigned count=0;
    unsigned k;
    if(!parsed) {
        const char *list=getenv("HALO_MAC_ARB_VERTEX");
        parsed=1;
        /* "all": every vertex shader the translator accepts */
        /* unset: "all", the default; "0" or "none": none */
        if(!list||!strcmp(list,"all")){all=1;list=NULL;}
        else if(!strcmp(list,"0")||!strcmp(list,"none"))list=NULL;
        while(list&&*list&&count<16) {
            char *end=NULL;unsigned long value=strtoul(list,&end,10);
            if(end==list)break;
            ids[count++]=value;list=*end==','?end+1:end;
        }
    }
    if(all)return 1;
    for(k=0;k<count;k++)if(ids[k]==id)return 1;
    return 0;
}
/* The ARB program of a vertex shader (with or without its bone gather), or
   NULL to keep GLSL: refused by the translator, rejected by GL, or beyond the
   GPU's native limits. A failure is remembered and logged once. */
static const struct arb_vertex_program *mac_arb_vertex_program(const struct vertex_shader_object *vertex,int gather)
{
    unsigned long i;struct arb_vertex_program *entry;
    char *source=NULL,error[160];unsigned env_count=0;GLint position=-1,native=0,max_env=0;double load_begin;
    for(i=0;i<gpu_arb_vertex_count;i++)
        if(gpu_arb_vertex[i].id==vertex->id&&gpu_arb_vertex[i].gather==gather)
            return gpu_arb_vertex[i].failed?NULL:&gpu_arb_vertex[i];
    if(gpu_arb_vertex_count==ARB_VERTEX_PROGRAMS)return NULL;
    entry=&gpu_arb_vertex[gpu_arb_vertex_count++];
    memset(entry,0,sizeof(*entry));entry->id=vertex->id;entry->gather=gather;entry->failed=1;
    if(mac_nv2a_arb_translate((const unsigned long *)vertex->instructions,vertex->instruction_count,
           gather,&source,&env_count,&entry->layout,error,sizeof(error))!=1) {
        fprintf(stderr,"native ARB vertex %lu refused: %s\n",vertex->id,error);
        free(source);++gpu_arb_failures;return NULL;
    }
    /* Diagnostic only (HALO_MAC_ARB_DUMP=1): each program's text, once */
    if(getenv("HALO_MAC_ARB_DUMP"))fprintf(stderr,"native ARB vertex %lu gather=%d source:\n%s\n",vertex->id,gather,source);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_ENV_PARAMETERS_ARB,&max_env);
    if(max_env<(GLint)env_count) {
        fprintf(stderr,"native ARB vertex %lu: %u env parameters, GPU allows %d\n",vertex->id,env_count,(int)max_env);
        free(source);++gpu_arb_failures;return NULL;
    }
    while(glGetError()!=GL_NO_ERROR){}
    load_begin=mac_load_begin(); /* port: load timing */
    glGenProgramsARB(1,&entry->program);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB,entry->program);
    glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(source),source);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&position);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&native);
    mac_load_end(MAC_LOAD_SLOT_PROGRAMS,load_begin);
    glBindProgramARB(GL_VERTEX_PROGRAM_ARB,0);
    gl_shadow.valid&=~(unsigned long)SHADOW_ARB_PROGRAM;
    if(position!=-1||!native||glGetError()!=GL_NO_ERROR) {
        fprintf(stderr,"native ARB vertex %lu rejected: error_position=%d native=%d %s\n",vertex->id,
            (int)position,(int)native,(const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        glDeleteProgramsARB(1,&entry->program);entry->program=0;
        free(source);++gpu_arb_failures;return NULL;
    }
    free(source);entry->failed=0;
    fprintf(stderr,"native ARB vertex %lu: GPU program%s\n",vertex->id,
        entry->layout.relative_used?" with relative addressing":entry->layout.bone_gather_used?" with bone gather":"");
    return entry;
}
/* port: (HALO_MAC_DM_ARB_WINDOW=1 turns it on, default off: the G4 ran it far slower; HALO_MAC_DM_ARB_WINDOWS="12,24" the sizes, in constants)
   A skinning program with the smallest window of the node matrices that holds every node the
   draw's vertex buffer names (its bone addresses times the program's scale c[7].w, plus the last
   offset the program reads): the driver is charged for the constants a program reads, and these read
   a dozen rather than 192. Every vertex reads the same constants as in the plain program, so the picture
   is the same. NULL: the plain program. */
static unsigned gpu_window_size[GPU_WINDOW_SIZES],gpu_window_count;
static int gpu_window_enabled=-1;
static unsigned long long gpu_window_draws[GPU_WINDOW_SIZES+1],gpu_window_checked,gpu_window_violations;
static void mac_window_configure(void)
{
    const char *list=getenv("HALO_MAC_DM_ARB_WINDOWS");
    gpu_window_enabled=mac_flag("HALO_MAC_DM_ARB_WINDOW",0);gpu_window_count=0;
    if(!list||!*list)list="12,24";
    while(*list&&gpu_window_count<GPU_WINDOW_SIZES) {
        char *end=NULL;unsigned long size=strtoul(list,&end,10);
        if(end==list)break;
        if(size>=3&&size<=192&&(!gpu_window_count||size>gpu_window_size[gpu_window_count-1]))gpu_window_size[gpu_window_count++]=(unsigned)size;
        list=*end==','?end+1:end;
    }
    if(!gpu_window_count)gpu_window_enabled=0;
}
static const struct arb_vertex_program *mac_arb_vertex_window(const struct vertex_shader_object *vertex,
    const struct arb_vertex_program *plain,int mode,const struct world_vbo_entry *entry)
{
    struct arb_vertex_program *base=(struct arb_vertex_program *)plain;
    float address;long top,needed;unsigned k;
    if(!plain->layout.skin_base||!entry->addresses||plain->gather!=mode)return plain;
    address=entry->bone_max*device.constants[7][3];
    if(!(address>=0.0f&&address<4096.0f))return plain; /* (and NaN) */
    top=(long)(address+1.5f)+(long)plain->layout.skin_top;
    needed=top-(long)plain->layout.skin_base+1;
    for(k=0;k<gpu_window_count;k++) {
        if((long)gpu_window_size[k]<needed)continue;
        if(!base->windows_tried[k]) {
            base->windows_tried[k]=1;
            base->windows[k]=mac_arb_vertex_program(vertex,mode|(int)(gpu_window_size[k]<<8));
        }
        if(base->windows[k]){++gpu_window_draws[k];return base->windows[k];}
        break;
    }
    ++gpu_window_draws[GPU_WINDOW_SIZES];
    return plain;
}
static unsigned long mac_arb_hash(const char *text)
{
    unsigned long hash=2166136261UL;
    while(*text)hash=((hash^(unsigned char)*text++)*16777619UL)&0xffffffffUL;
    return hash;
}
/* The fragment-only program of a pixel source for an ARB vertex program. */
/* Replace each stage's scaled 2D fetch in the mask with a projective one. */
static void mac_arb_projective_fetches(char *source,unsigned long mask)
{
    unsigned long stage;
    for(stage=0;stage<4;stage++) {
        char pattern[160],replacement[48];char *at;size_t length,replacement_length;
        if(!(mask&(1UL<<stage)))continue;
        sprintf(pattern,"texture2D(tex%lu, (vec4(xT%lu.xyz / (xT%lu.w != 0.0 ? xT%lu.w : 1.0), 1.0)).xy * texture_scale[%lu].xy)",
            stage,stage,stage,stage,stage);
        sprintf(replacement,"texture2DProj(tex%lu, xT%lu.xyw)",stage,stage);
        length=strlen(pattern);replacement_length=strlen(replacement);
        while((at=strstr(source,pattern))!=NULL) {
            /* (the replacement is shorter: shift the rest down) */
            memcpy(at,replacement,replacement_length);
            memmove(at+replacement_length,at+length,strlen(at+length)+1);
        }
    }
}
static GLuint mac_arb_fragment_program(const struct arb_vertex_program *vertex,const char *pixel_source,
    unsigned long projective,const struct mac_shader_source_identity *identity)
{
    unsigned long i,hash;struct arb_fragment_program *entry;
    char *rewritten=NULL,error[160],log[512];GLuint shader,program;GLint ok=0;double load_begin;
    /* (the source cache's identity of the pixel source: no text compare) */
    for(i=0;identity&&identity->cache_epoch&&i<ARB_FRAGMENT_PROGRAMS;i++)
        if(gpu_arb_fragment[i].program&&gpu_arb_fragment[i].vertex==vertex&&
           gpu_arb_fragment[i].projective==projective&&
           gpu_arb_fragment[i].identity.cache_epoch==identity->cache_epoch&&
           gpu_arb_fragment[i].identity.pixel_entry==identity->pixel_entry)return gpu_arb_fragment[i].program;
    hash=mac_arb_hash(pixel_source);
    for(i=0;i<ARB_FRAGMENT_PROGRAMS;i++)
        if(gpu_arb_fragment[i].program&&gpu_arb_fragment[i].vertex==vertex&&gpu_arb_fragment[i].hash==hash&&
           gpu_arb_fragment[i].projective==projective&&
           !strcmp(gpu_arb_fragment[i].source,pixel_source)) {
            if(identity)gpu_arb_fragment[i].identity=*identity;
            return gpu_arb_fragment[i].program;
        }
    if(mac_nv2a_arb_fragment_rewrite(pixel_source,&vertex->layout,&rewritten,error,sizeof(error))!=1) {
        fprintf(stderr,"native ARB fragment refused: %s\n",error);free(rewritten);return 0;
    }
    mac_arb_projective_fetches(rewritten,projective);
    mac_glsl_dump("arbfragment",rewritten);
    load_begin=mac_load_begin(); /* port: load timing */
    shader=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader,1,(const GLchar **)&rewritten,NULL);glCompileShader(shader);
    glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok) {
        glGetShaderInfoLog(shader,(GLsizei)sizeof(log),NULL,log);
        fprintf(stderr,"native ARB fragment compile failed: %s\n",log);
        glDeleteShader(shader);free(rewritten);return 0;
    }
    program=glCreateProgram();glAttachShader(program,shader);glLinkProgram(program);
    glDeleteShader(shader);free(rewritten);
    glGetProgramiv(program,GL_LINK_STATUS,&ok);
    mac_load_end(MAC_LOAD_SLOT_PROGRAMS,load_begin);
    if(!ok) {
        glGetProgramInfoLog(program,(GLsizei)sizeof(log),NULL,log);
        fprintf(stderr,"native ARB fragment link failed: %s\n",log);
        glDeleteProgram(program);return 0;
    }
    entry=&gpu_arb_fragment[gpu_arb_fragment_next++%ARB_FRAGMENT_PROGRAMS];
    if(entry->program) {
        glDeleteProgram(entry->program);free(entry->source);
        /* (its name may come back as another program: forget its uniforms) */
        gpu_uniform_block_count=0;gpu_current_program=0;
    }
    entry->vertex=vertex;entry->hash=hash;entry->projective=projective;entry->program=program;
    if(identity)entry->identity=*identity;else memset(&entry->identity,0,sizeof(entry->identity));
    entry->source=(char *)malloc(strlen(pixel_source)+1);
    if(!entry->source){glDeleteProgram(program);memset(entry,0,sizeof(*entry));return 0;}
    strcpy(entry->source,pixel_source);
    return program;
}
/* The ARB fragment program of a draw's pixel state, or NULL to keep GLSL
   (refused by the translator, rejected by GL, or over the GPU's limits; a
   failure is remembered). */
static const struct arbfp_program *mac_arbfp_program(const struct nv2a_pixel_shader_key *key,
    const struct arb_vertex_program *vertex,unsigned long projective,
    const struct mac_shader_source_identity *identity)
{
    unsigned long i;struct arbfp_program *entry;struct mac_nv2a_arbfp_state state;
    char *source=NULL,error[160];GLint position=-1,native=0;int result;double load_begin;
    int fog_texcoord=vertex->layout.fog_texcoord,fog_component=vertex->layout.fog_component;
    for(i=0;identity&&identity->cache_epoch&&i<gpu_arbfp_count;i++) {
        entry=&gpu_arbfp[i];
        if(entry->identity.cache_epoch==identity->cache_epoch&&entry->identity.pixel_entry==identity->pixel_entry&&
           entry->projective==projective&&entry->fog_texcoord==fog_texcoord&&entry->fog_component==fog_component)
            return entry->failed?NULL:entry;
    }
    memset(&state,0,sizeof(state));
    mac_nv2a_arbfp_state_from_key(key,&state);
    for(i=0;i<gpu_arbfp_count;i++) {
        entry=&gpu_arbfp[i];
        if(entry->projective==projective&&entry->fog_texcoord==fog_texcoord&&entry->fog_component==fog_component&&
           !memcmp(&entry->state,&state,sizeof(state))) {
            if(identity)entry->identity=*identity;
            return entry->failed?NULL:entry;
        }
    }
    if(gpu_arbfp_count<ARBFP_PROGRAMS)entry=&gpu_arbfp[gpu_arbfp_count++];
    else {
        entry=&gpu_arbfp[gpu_arbfp_next++%ARBFP_PROGRAMS];
        ++gpu_prog_epoch;
        if(entry->program)glDeleteProgramsARB(1,&entry->program);
    }
    memset(entry,0,sizeof(*entry));
    entry->state=state;entry->projective=projective;entry->fog_texcoord=fog_texcoord;
    entry->fog_component=fog_component;entry->failed=1;
    if(identity)entry->identity=*identity;
    result=mac_nv2a_arbfp_translate_state(&state,&vertex->layout,projective,&source,&entry->info,error,sizeof(error));
    if(result!=1) {
        if(result<0)fprintf(stderr,"native ARB fragment translation failed: %s\n",error);
        free(source);++gpu_arbfp_failures;return NULL;
    }
    while(glGetError()!=GL_NO_ERROR){}
    load_begin=mac_load_begin(); /* port: load timing */
    glGenProgramsARB(1,&entry->program);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,entry->program);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(source),source);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&position);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&native);
    mac_load_end(MAC_LOAD_SLOT_PROGRAMS,load_begin);
    gl_shadow.valid&=~(unsigned long)SHADOW_ARB_FRAGMENT;
    if(position!=-1||!native||glGetError()!=GL_NO_ERROR) {
        fprintf(stderr,"native ARB fragment program rejected: error_position=%d native=%d %s\n",
            (int)position,(int)native,(const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        glDeleteProgramsARB(1,&entry->program);entry->program=0;
        free(source);++gpu_arbfp_failures;return NULL;
    }
    if(gpu_ff_enabled>0||getenv("HALO_MAC_ARBFP_DUMP")) {
        /* (HALO_MAC_FF_COMBINERS) whether the combiners can express it exactly */
        if(mac_nv2a_arbfp_lower_fixed(&state,&vertex->layout,projective,&entry->ff,entry->ff_reason,
               sizeof(entry->ff_reason))!=1)entry->ff.ok=0;
    }
    if(getenv("HALO_MAC_ARBFP_DUMP")){size_t n=strlen(source);entry->dump=(char *)malloc(n+1);if(entry->dump)memcpy(entry->dump,source,n+1);}
    free(source);entry->failed=0;
    return entry;
}
static unsigned long long gpu_bsp_draw_hits,gpu_bsp_draw_misses,gpu_bsp_draw_bypasses;
static unsigned long long gpu_bsp_vertex_hits,gpu_bsp_vertex_misses,gpu_bsp_store_rejections;
static WORD *gpu_stream_remap;
static unsigned long gpu_stream_remap_capacity, gpu_stream_remap_count;
static unsigned long long gpu_remapped_draws;
static float *gpu_bone_vertices;
static size_t gpu_bone_vertex_bytes;
/* Diagnostic-only renderer trace. It is deliberately bounded and is inert
   unless HALO_MAC_RENDER_TRACE is present in the process environment. */
static int mac_render_trace_enabled=-1;
static unsigned long mac_render_trace_draws;
static unsigned long mac_render_trace_shader_ids[64];
typedef char mac_d3d_matrix_abi[(sizeof(D3DMATRIX)==64)?1:-1];
typedef char mac_transform_count_abi[(sizeof(device.transforms)/sizeof(device.transforms[0])==D3DTS_MAX)?1:-1];
#include "mac_device_state_body.inc"
void WINAPI D3DDevice_SetShaderConstantMode(D3DSHADERCONSTANTMODE mode)
{
    if(!active||mode!=D3DSCM_192CONSTANTS) {
        fprintf(stderr,"native D3D shader constant mode is unsupported\n");
        abort();
    }
    device.shader_constant_mode=mode;
    viewport_update_constants();
}
int mac_device_state_start(void)
{
    unsigned long index;
    if(active)return 0;
    memset(&device,0,sizeof(device));
    {
        const char *compact_upload=getenv("HALO_MAC_COMPACT_VERTEX_UPLOAD");
        const char *indexed_reuse=getenv("HALO_MAC_INDEXED_VERTEX_REUSE");
        const char *bsp_decode=getenv("HALO_MAC_BSP_DECODE_CACHE");
        const char *world_vbo=getenv("HALO_MAC_WORLD_VBO");
        const char *lean_draw=getenv("HALO_MAC_LEAN_DRAW");
        const char *lazy_present=getenv("HALO_MAC_LAZY_PRESENT");
        const char *index_cache=getenv("HALO_MAC_INDEX_CACHE");
        gpu_compact_vertex_upload=compact_upload&&compact_upload[0]=='1'&&compact_upload[1]=='\0';
        /* (the measured wins default to on: mac_flags.h) */
        (void)indexed_reuse;(void)world_vbo;(void)lean_draw;(void)lazy_present;
        gpu_indexed_vertex_reuse=mac_flag("HALO_MAC_INDEXED_VERTEX_REUSE",1);
        gpu_world_vbo_enabled=mac_flag("HALO_MAC_WORLD_VBO",1);
        gpu_lean_draw=mac_flag("HALO_MAC_LEAN_DRAW",1);
        gpu_lazy_present=mac_flag("HALO_MAC_LAZY_PRESENT",1);
        gpu_readback_requested=0;
        gpu_fast_jmp=mac_perf_flag("HALO_MAC_PERF_JMP");
        gpu_fast_minmax=mac_perf_flag("HALO_MAC_PERF_MINMAX");
        gpu_env_log=mac_perf_flag("HALO_MAC_PERF_ENV_LOG");
        device.gpu_flip_targets=mac_flag("HALO_MAC_FLIP_TARGETS",1);
        gpu_flip_logging=getenv("HALO_MAC_FLIP_LOG")!=NULL;gpu_flip_log_count=0;
        memset(gpu_arb_env_valid,0,sizeof(gpu_arb_env_valid));gpu_arb_env_writes=0;
        gpu_arb_env_constants_serial=0;
        memset(gpu_arbfp_env_valid,0,sizeof(gpu_arbfp_env_valid));
        gpu_arbfp_draws=gpu_arbfp_failures=gpu_arbfp_env_writes=0;
        gpu_index_cache_enabled=index_cache&&index_cache[0]=='1'&&index_cache[1]=='\0';
        gpu_index_arena=0;gpu_index_arena_used=0;gpu_index_shadow=NULL;
        memset(gpu_index_entries,0,sizeof(gpu_index_entries));
        gpu_index_hits=gpu_index_uploads=gpu_index_resets=gpu_index_bypasses=0;
        gpu_current_program=0;gpu_uniform_block_count=0;gpu_uniform_evictions_seen=0;
        memset(gpu_prog_memo,0,sizeof(gpu_prog_memo));++gpu_prog_epoch;
        mac_gpu_textures_set_lean(gpu_lean_draw);
        memset(gpu_world_vbos,0,sizeof(gpu_world_vbos));gpu_world_vbo_bytes=0;gpu_world_vbo_peak=0;gpu_world_vbo_serial=0;
        gpu_world_vbo_draws=gpu_world_vbo_builds=gpu_world_vbo_rejections=0;
        gpu_world_vbo_fallbacks=gpu_world_vbo_evictions=gpu_world_vbo_bypasses=0;
        gpu_world_vbo_gather_skips=gpu_world_vbo_gather_draws=0;
        memset(&gpu_world_draw,0,sizeof(gpu_world_draw));
        gpu_bsp_decode_cache_enabled=bsp_decode&&bsp_decode[0]=='1'&&bsp_decode[1]=='\0'&&
            bsp_decode_cache_init(&gpu_bsp_decode_cache,16U*1024U*1024U);
    }
    gpu_bsp_draw_hits=gpu_bsp_draw_misses=gpu_bsp_draw_bypasses=0;
    gpu_bsp_vertex_hits=gpu_bsp_vertex_misses=gpu_bsp_store_rejections=0;
    mac_vertex_upload_init(&gpu_vertex_upload);
    gpu_stream_decoded=gpu_stream_reused=gpu_remapped_draws=0;
    gpu_stream_remap_count=0;
    mac_render_trace_draws=0;
    memset(mac_render_trace_shader_ids,0,sizeof(mac_render_trace_shader_ids));
    memset(D3D__RenderState,0,sizeof(D3D__RenderState));
    memset(D3D__TextureState,0,sizeof(D3D__TextureState));
    memset(D3D__StateBlockDirty,0,sizeof(D3D__StateBlockDirty));
    memset(constant_serials,0,sizeof(constant_serials));
    memset(constant_log,0,sizeof(constant_log));
    constants_serial=constants_checkpoint_serial=0;
    constants_checkpoint_first=XGPU_VERTEX_CONSTANT_COUNT;
    constants_checkpoint_last=0;
    D3D__IndexData=NULL;
#include "mac_device_state_defaults.inc"
    mac_shader_source_cache_init(&gpu_source_cache);
    {
        mac_shader_source_cache_set_bone_gather(&gpu_source_cache,
            mac_flag("HALO_MAC_BONE_GATHER",1));
    }
    mac_render_timing_init();
    active=1;return 1;
}
void mac_device_bsp_decode_cache_clear(void)
{
    /* (world buffers are keyed by the same certificates: drop them too) */
    mac_world_vbo_clear();
    if(!gpu_bsp_decode_cache_enabled)return;
    bsp_decode_cache_dispose(&gpu_bsp_decode_cache);
    if(!bsp_decode_cache_init(&gpu_bsp_decode_cache,16U*1024U*1024U))
        gpu_bsp_decode_cache_enabled=0;
}
void mac_device_bsp_decode_cache_get_stats(struct mac_device_bsp_decode_cache_stats *stats)
{
    if(!stats)return;
    stats->draw_hits=gpu_bsp_draw_hits;stats->draw_misses=gpu_bsp_draw_misses;
    stats->draw_bypasses=gpu_bsp_draw_bypasses;stats->vertex_hits=gpu_bsp_vertex_hits;
    stats->vertex_misses=gpu_bsp_vertex_misses;stats->store_rejections=gpu_bsp_store_rejections;
}
void mac_device_state_stop(void)
{
    struct vertex_shader_object *shader=device.owned_shaders,*next;
    if(gpu_flip_logging) {
        unsigned i;
        for(i=0;i<gpu_flip_log_count;i++)
            fprintf(stderr,"flip log: target data 0x%lx size 0x%lx draws %lu\n",gpu_flip_log[i].data,gpu_flip_log[i].size,gpu_flip_log[i].draws);
    }
    if(gpu_lean_draw&&gpu_current_program){glUseProgram(0);gpu_current_program=0;}
    if(gpu_arbfp_draws||gpu_arbfp_failures)
        fprintf(stderr,"native ARB fragment programs: draws=%llu failures=%llu programs=%lu env_writes=%llu vertex_binds=%llu fragment_binds=%llu\n",
            gpu_arbfp_draws,gpu_arbfp_failures,gpu_arbfp_count,gpu_arbfp_env_writes,gpu_vertex_program_binds,gpu_fragment_program_binds);
    if(getenv("HALO_MAC_ARBFP_DUMP")) {
        unsigned long k;
        for(k=0;k<gpu_arbfp_count;k++)
            if(gpu_arbfp[k].dump)fprintf(stderr,"ARBFP %lu draws=%llu alu=%u tex=%u kil=%u comb=0x%lx modes=0x%lx fog=%u/%u ff=%s%s\n%s\n",k,
                gpu_arbfp[k].draws,gpu_arbfp[k].info.alu_instructions,gpu_arbfp[k].info.tex_instructions,
                (unsigned)gpu_arbfp[k].info.uses_kil,gpu_arbfp[k].state.combiner_count,gpu_arbfp[k].state.texture_modes,
                (unsigned)gpu_arbfp[k].state.fog_enable,(unsigned)gpu_arbfp[k].state.fog_table_mode,
                gpu_arbfp[k].ff.ok?"yes":"no",gpu_arbfp[k].ff.ok?"":gpu_arbfp[k].ff_reason,gpu_arbfp[k].dump);
    }
    if(gpu_arbfp_draws) {
        /* how many of the fragment-program draws the combiners can express (and, with the
           switch on, drew that way) */
        unsigned long k,expressible=0;unsigned long long expressible_draws=0,total_draws=0;
        for(k=0;k<gpu_arbfp_count;k++) {
            total_draws+=gpu_arbfp[k].draws;
            if(gpu_arbfp[k].ff.ok){expressible++;expressible_draws+=gpu_arbfp[k].draws;}
        }
        if(gpu_ff_enabled>0||getenv("HALO_MAC_ARBFP_DUMP"))
            fprintf(stderr,"native fixed-function combiners: enabled=%d expressible programs=%lu of %lu, draws %llu of %llu (%.1f%%), drawn that way=%llu texenv_writes=%llu color_writes=%llu unit_toggles=%llu alpha_test_writes=%llu\n",
                gpu_ff_enabled>0,expressible,gpu_arbfp_count,expressible_draws,total_draws,
                total_draws?100.0*(double)expressible_draws/(double)total_draws:0.0,
                gpu_ff_draws,gpu_ff_texenv_writes,gpu_ff_const_writes,gpu_ff_unit_toggles,gpu_ff_alpha_writes);
    }
    {
        /* per presented frame, to set beside the G4's timings */
        unsigned long long frames=device.gpu_present_count?device.gpu_present_count:1;
        unsigned long long tb=0,ta=0,tbias=0;double f=(double)frames;
        mac_gpu_textures_call_counts(&tb,&ta,&tbias);
        fprintf(stderr,"native frame counts per frame (%llu frames): draws=%.1f vertex_program_binds=%.1f fragment_program_binds=%.1f vertex_env_writes=%.1f fragment_env_writes=%.1f texture_binds=%.1f active_texture=%.1f lod_bias_writes=%.1f ff_draws=%.1f ff_texenv_writes=%.1f ff_color_writes=%.1f ff_unit_toggles=%.1f\n",
            frames,(double)(gpu_arbfp_draws+gpu_ff_draws)/f,(double)gpu_vertex_program_binds/f,(double)gpu_fragment_program_binds/f,
            (double)gpu_arb_env_writes/f,(double)gpu_arbfp_env_writes/f,(double)tb/f,(double)ta/f,(double)tbias/f,
            (double)gpu_ff_draws/f,(double)gpu_ff_texenv_writes/f,(double)gpu_ff_const_writes/f,(double)gpu_ff_unit_toggles/f);
    }
    if(gpu_var_enabled>0||gpu_var_draws)
        fprintf(stderr,"native vertex array range: enabled=%d draws=%llu bytes=%llu fallbacks=%llu fence_waits=%llu fence_stalls=%llu\n",
            gpu_var_enabled>0,gpu_var_draws,gpu_var_bytes,gpu_var_fallbacks,gpu_var_waits,gpu_var_stalls);
    if(gpu_ff_draws)mac_ff_leave();
    /* (the programs stay with the context, as the ARB vertex programs do) */
    if(gpu_arb_draws||gpu_arb_failures)
        fprintf(stderr,"native ARB vertex programs: draws=%llu failures=%llu env_writes=%llu\n",gpu_arb_draws,gpu_arb_failures,gpu_arb_env_writes);
    if(gpu_prog_memo_hits||gpu_prog_memo_misses)
        fprintf(stderr,"native program memo: hits=%llu misses=%llu\n",gpu_prog_memo_hits,gpu_prog_memo_misses);
    if(gpu_window_enabled>0)
        fprintf(stderr,"native ARB skinning windows: sizes=%u,%u,%u,%u draws=%llu,%llu,%llu,%llu full=%llu\n",
            gpu_window_size[0],gpu_window_size[1],gpu_window_size[2],gpu_window_size[3],
            gpu_window_draws[0],gpu_window_draws[1],gpu_window_draws[2],gpu_window_draws[3],gpu_window_draws[GPU_WINDOW_SIZES]);
    if(gpu_window_checked)fprintf(stderr,"native ARB skinning windows checked: draws=%llu violations=%llu\n",gpu_window_checked,gpu_window_violations);
    if(gpu_world_vbo_enabled) {
        if(gpu_diag_segments>0) {
            mach_timebase_info_data_t timebase;unsigned k;
            mach_timebase_info(&timebase);
            fprintf(stderr,"native indexed draw segments (us total over %lu calls):",gpu_segment_calls);
            for(k=0;k<11;k++)fprintf(stderr," %u=%.0f",k,(double)gpu_segment_time[k]*timebase.numer/timebase.denom/1000.0);
            fprintf(stderr,"\n");
        }
        fprintf(stderr,"native world vertex buffers: draws=%llu builds=%llu rejections=%llu bypasses=%llu fallbacks=%llu gather_skips=%llu gather_draws=%llu evictions=%llu bytes=%lu peak=%lu\n",
            gpu_world_vbo_draws,gpu_world_vbo_builds,gpu_world_vbo_rejections,gpu_world_vbo_bypasses,
            gpu_world_vbo_fallbacks,gpu_world_vbo_gather_skips,gpu_world_vbo_gather_draws,gpu_world_vbo_evictions,gpu_world_vbo_bytes,
            gpu_world_vbo_peak);
        if(gpu_index_range_hits)fprintf(stderr,"native index range cache: hits=%llu\n",gpu_index_range_hits);
        if(gpu_index_cache_enabled||gpu_index_hits||gpu_index_uploads)
            fprintf(stderr,"native index cache: hits=%llu uploads=%llu resets=%llu bypasses=%llu used=%lu\n",
                gpu_index_hits,gpu_index_uploads,gpu_index_resets,gpu_index_bypasses,gpu_index_arena_used);
        mac_world_vbo_clear();
    }
    if(gpu_bsp_decode_cache_enabled) {
        fprintf(stderr,"native BSP decoded cache: draws_hit=%llu draws_miss=%llu draws_bypass=%llu vertex_hits=%llu vertex_misses=%llu store_rejections=%llu entries=%lu bytes=%lu\n",
            gpu_bsp_draw_hits,gpu_bsp_draw_misses,gpu_bsp_draw_bypasses,
            gpu_bsp_vertex_hits,gpu_bsp_vertex_misses,gpu_bsp_store_rejections,
            (unsigned long)gpu_bsp_decode_cache.entry_count,
            (unsigned long)gpu_bsp_decode_cache.bytes_used);
        bsp_decode_cache_dispose(&gpu_bsp_decode_cache);
        gpu_bsp_decode_cache_enabled=0;
    }
    mac_engine_vblank_stop();
    if(gpu_visibility_started)mac_gl_visibility_stop();
    gpu_visibility_started=0;
    gpu_visibility_active=0;
    if(gpu_window.state) {
        struct mac_gl_program_cache_stats stats;
        if(device.gpu&&gpu_compact_vertex_upload&&
           !mac_vertex_upload_dispose(&gpu_vertex_upload)) {
            fprintf(stderr,"native compact vertex upload still has live GL state at teardown\n");
            abort();
        }
        mac_gl_program_cache_get_stats(&gpu_program_cache,&stats);
        fprintf(stderr,"native GL program cache: entries=%lu hits=%lu misses=%lu evictions=%lu uniform_lookups=%lu uniform_hits=%lu identity_hits=%lu\n",
            stats.entries,stats.hits,stats.misses,stats.evictions,
            stats.uniform_lookups,stats.uniform_hits,stats.identity_hits);
        if(!mac_gpu_textures_dispose()) {
            fprintf(stderr,"native texture cache still has an active draw at teardown\n");
            abort();
        }
        mac_gl_program_cache_dispose(&gpu_program_cache);
        mac_window_close(&gpu_window);
    }
    fprintf(stderr,"native shader source cache: bytes=%lu vertex_hits=%lu vertex_misses=%lu pixel_hits=%lu pixel_misses=%lu\n",
        (unsigned long)gpu_source_cache.bytes,gpu_source_cache.vertex_hits,
        gpu_source_cache.vertex_misses,gpu_source_cache.pixel_hits,gpu_source_cache.pixel_misses);
    mac_shader_source_cache_dispose(&gpu_source_cache);
    free(gpu_bone_vertices);gpu_bone_vertices=NULL;gpu_bone_vertex_bytes=0;
    fprintf(stderr,"native stream decode: decoded=%llu reused=%llu\n",
        gpu_stream_decoded,gpu_stream_reused);
    mac_index_decode_cache_dispose(&gpu_index_decode_cache);
    fprintf(stderr,"native indexed vertex reuse: enabled=%d draws=%llu\n",
        gpu_indexed_vertex_reuse,gpu_remapped_draws);
    free(gpu_stream_remap);gpu_stream_remap=NULL;
    gpu_stream_remap_capacity=gpu_stream_remap_count=0;
    mac_render_timing_report();
    if(device.back_buffer.Data)platform_contiguous_free(PLATFORM_PHYSICAL_TO_VIRTUAL(device.back_buffer.Data));
    if(device.depth_buffer.Data)platform_contiguous_free(PLATFORM_PHYSICAL_TO_VIRTUAL(device.depth_buffer.Data));
    while(shader){next=shader->next;free(shader->instructions);free(shader);shader=next;}
    free(device.immediate_vertices);
    memset(&device,0,sizeof(device));
    D3D__IndexData=NULL;active=0;
}
const struct mac_device_state *mac_device_state_get(void)
{return active?&device:NULL;}
D3DDevice *mac_device_headless_identity(void)
{return active&&device.headless?(D3DDevice *)&device:NULL;}
D3DDevice *mac_device_gpu_identity(void)
{return active&&device.gpu?(D3DDevice *)&device:NULL;}
unsigned long long mac_device_state_constant_serial(void)
{return constants_serial;}

/* CPU operations below specialize d3d8_gl.c's explicit null_renderer path.
   Drawing calls require explicit headless opt-in; they cannot silently stand
   in for a working graphics device. */
static void require_headless(void)
{ if(!active||!device.headless){fprintf(stderr,"D3D operation requires explicit native headless device\n");abort();} }
/* Native headless mode has no GPU queue: after checking that the caller opted
   into that CPU-only device, idle and push-buffer completion are immediate. */
BOOL WINAPI D3DDevice_IsBusy(void)
{ if(active&&device.gpu)return FALSE; /* port: GL draws are queued by the driver; an Xbox map's buffers ask when they are let go */ require_headless(); return FALSE; }
void WINAPI D3DDevice_KickPushBuffer(void)
{ if(active&&device.gpu)return; require_headless(); }
int mac_device_headless_start(unsigned long width,unsigned long height)
{
    if(!width||!height||width>2048||height>2048||!mac_device_state_start())return 0;
    device.headless=1;
    d3d8_surface_initialize(&device.back_buffer,D3DFMT_LIN_A8R8G8B8,width,height);
    d3d8_surface_initialize(&device.depth_buffer,D3DFMT_LIN_D24S8,width,height);
    if(!device.back_buffer.Data||!device.depth_buffer.Data){mac_device_state_stop();return 0;}
    D3DDevice_SetRenderTarget(&device.back_buffer,&device.depth_buffer);
    return 1;
}
static void mac_render_scale_update(void);
struct gpu_frame_call { mac_device_gpu_frame_proc callback;void *context; };
static int mac_device_gpu_frame_call(void *context,const struct mac_input_frame *input,
    unsigned width,unsigned height)
{
    struct gpu_frame_call *call=(struct gpu_frame_call *)context;int result;
    GLint default_framebuffer=0,depth_type=0,depth_object=0;
    if(!device.gpu||device.gpu_frame_active||width!=device.gpu_width||height!=device.gpu_height)return 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&default_framebuffer);
    if(default_framebuffer==0) {
        GLint depth_bits=0,stencil_bits=0;
        /* AGL drawable attachments have no framebuffer-object names. */
        if(!mac_window_is_visible(&gpu_window))return 0;
        glGetIntegerv(GL_DEPTH_BITS,&depth_bits);glGetIntegerv(GL_STENCIL_BITS,&stencil_bits);
        if(glGetError()!=GL_NO_ERROR||depth_bits<24||stencil_bits<8)return 0;
    } else {
        glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT,&depth_type);
        if(depth_type==GL_RENDERBUFFER_EXT)
            glGetFramebufferAttachmentParameterivEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&depth_object);
        if(glGetError()!=GL_NO_ERROR||depth_type!=GL_RENDERBUFFER_EXT||depth_object<=0)return 0;
    }
    device.gpu_default_framebuffer=(unsigned long)default_framebuffer;
    device.gpu_default_depth_renderbuffer=(unsigned long)depth_object;
    device.gpu_frame_active=1;
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glDepthMask(GL_TRUE);glStencilMask((GLuint)-1);
    mac_gl_shadow_forget();mac_gpu_textures_shadow_reset();
    mac_render_scale_update(); /* port: the render resolution, for this frame */
    if(device.render_target&&device.render_target!=&device.back_buffer&&
       !mac_engine_device_resources_bind_target(device.render_target,device.depth_stencil))
        result=0;
    else result=call->callback(call->context,input,width,height);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)device.gpu_default_framebuffer);
    device.gpu_frame_active=0;
    return result;
}
/* port: the picture's width changed (mac_window.c, between frames): the back
   and depth buffers describe the new width. Their memory was made for the
   widest (mac_device_gpu_start), and is kept. */
#define GPU_WIDE_MAXIMUM_WIDTH 1280UL
static void mac_device_surface_size(D3DSurface *surface,unsigned long width,unsigned long height)
{
    unsigned long pitch=(width*4+D3DTEXTURE_PITCH_ALIGNMENT-1)&~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT-1);
    surface->Size=((pitch/D3DTEXTURE_PITCH_ALIGNMENT-1)<<D3DSIZE_PITCH_SHIFT)|((height-1)<<D3DSIZE_HEIGHT_SHIFT)|(width-1);
}
/* The frame's scale, for the setting and the window as they are now: read
   between frames (and when the picture's width changes), so that a frame is
   drawn with one. Logged when it changes, with what held it back. */
static void mac_render_scale_update(void)
{
    static int env_read;static float env_scale;
    static GLint max_dimension;
    static long logged_lines=-2;static int logged_clamp=-1;
    int window_width=0,window_height=0,vram_mb=32,is_ppc=0,clamp=0;
    const char *setting="original";
    float scale;long lines;unsigned region_width,region_height;
    if(!device.gpu)return;
    if(!env_read) {
        const char *value=getenv("HALO_MAC_RENDER_SCALE");
        env_read=1;env_scale=value?(float)atof(value):0.0f;
        if(env_scale<0.4f||env_scale>4.0f)env_scale=0.0f;
    }
    if(env_scale>0.0f)scale=env_scale;
    else {
        if(mac_device_render_setting_hook)setting=mac_device_render_setting_hook(&vram_mb,&is_ppc);
        lines=mac_render_parse(setting);
        if(lines==MAC_RENDER_LINES_AUTO)lines=mac_gpu_pick_render_lines(vram_mb,is_ppc);
        if(!max_dimension) {
            GLint texture=0,viewport[2]={0,0};
            glGetIntegerv(GL_MAX_TEXTURE_SIZE,&texture);glGetIntegerv(GL_MAX_VIEWPORT_DIMS,viewport);
            while(glGetError()!=GL_NO_ERROR){}
            max_dimension=texture>0?texture:2048;
            if(viewport[0]>0&&viewport[0]<max_dimension)max_dimension=viewport[0];
        }
        platform_video_window_size(&window_width,&window_height);
        if(window_width<=0||window_height<=0){window_width=(int)device.gpu_width;window_height=(int)device.gpu_height;}
        scale=mac_render_scale_choose(lines,(unsigned)device.gpu_width,(unsigned)window_width,(unsigned)window_height,
            (unsigned)max_dimension,mac_render_pixel_budget(vram_mb),&clamp);
        if(lines!=logged_lines||clamp!=logged_clamp) {
            logged_lines=lines;logged_clamp=clamp;
            if(clamp)fprintf(stderr,"render resolution: %s asked for %s lines, held to scale %.3f (%s%s%s)\n",
                setting,lines?"more":"the window's",(double)scale,
                (clamp&MAC_RENDER_CLAMP_WINDOW)?"the window's pixels ":"",
                (clamp&MAC_RENDER_CLAMP_SIZE)?"the card's largest texture ":"",
                (clamp&MAC_RENDER_CLAMP_MEMORY)?"the card's memory":"");
        }
    }
    region_width=mac_render_pixels((unsigned)device.gpu_width,scale);region_height=mac_render_pixels((unsigned)device.gpu_height,scale);
    if(scale!=gpu_render_scale||region_width!=gpu_region_width||region_height!=gpu_region_height) {
        gpu_render_scale=scale;gpu_region_width=region_width;gpu_region_height=region_height;
        fprintf(stderr,"render resolution: %ux%u pixels for the %lux%lu picture (scale %.3f, setting \"%s\", window %dx%d, %d MB, %s)\n",
            region_width,region_height,(unsigned long)device.gpu_width,(unsigned long)device.gpu_height,(double)scale,
            env_scale>0.0f?"HALO_MAC_RENDER_SCALE":setting,window_width,window_height,vram_mb,is_ppc?"PowerPC":"Intel");
    }
}
/* the pixels the frame is drawn with: the picture's own, or scaled (also
   for the textures made of the back buffer, mac_gpu_textures.c) */
void mac_device_render_region(unsigned *width,unsigned *height)
{
    if(gpu_render_scale>1.0f&&gpu_region_width&&gpu_region_height) {
        *width=gpu_region_width;*height=gpu_region_height;
    } else {
        *width=(unsigned)device.gpu_width;*height=(unsigned)device.gpu_height;
    }
}
static void mac_device_screen_width_changed(unsigned width)
{
    if(!device.gpu||width<640||width>GPU_WIDE_MAXIMUM_WIDTH)return;
    device.gpu_width=width;
    mac_device_surface_size(&device.back_buffer,width,device.gpu_height);
    mac_device_surface_size(&device.depth_buffer,width,device.gpu_height);
    if(device.render_target==&device.back_buffer) {
        device.viewport.X=device.viewport.Y=0;device.viewport.Width=width;device.viewport.Height=device.gpu_height;
        viewport_update_constants();
    }
    mac_render_scale_update(); /* (the picture's shape is the scale's) */
}
int mac_device_gpu_start(unsigned long width,unsigned long height,int visible,const char *title)
{
    if(!width||!height||width>2048||height>2048||!mac_device_state_start())return 0;
    if(!mac_window_open_fixed(&gpu_window,(unsigned)width,(unsigned)height,visible,title)) {
        mac_device_state_stop();return 0;
    }
    device.gpu=1;device.gpu_width=width;device.gpu_height=height;
    if(mac_render_timing_enabled())
        fprintf(stderr,"native renderer configuration: bone_gather=%d compact_vertex_upload=%d packed_readback=1 visible=%d size=%lux%lu\n",
            gpu_source_cache.bone_gather_enabled,gpu_compact_vertex_upload,
            visible,width,height);
    /* (memory for the widest picture, and the size of this one) */
    d3d8_surface_initialize(&device.back_buffer,D3DFMT_LIN_A8R8G8B8,GPU_WIDE_MAXIMUM_WIDTH,height);
    d3d8_surface_initialize(&device.depth_buffer,D3DFMT_LIN_D24S8,GPU_WIDE_MAXIMUM_WIDTH,height);
    if(!device.back_buffer.Data||!device.depth_buffer.Data){mac_device_state_stop();return 0;}
    mac_device_surface_size(&device.back_buffer,width,height);
    mac_device_surface_size(&device.depth_buffer,width,height);
    mac_window_screen_width_hook(mac_device_screen_width_changed);
    D3DDevice_SetRenderTarget(&device.back_buffer,&device.depth_buffer);
    return 1;
}
int mac_device_gpu_frame(mac_device_gpu_frame_proc callback,void *context)
{
    struct gpu_frame_call call;
    if(!device.gpu||device.gpu_frame_active||!callback)return 0;
    call.callback=callback;call.context=context;
    return mac_window_frame(&gpu_window,mac_device_gpu_frame_call,&call);
}
void WINAPI D3DDevice_SetRenderTarget(D3DSurface *target,D3DSurface *depth)
{
    struct xgpu_texture_description description;
    if(target)device.render_target=target;
    device.depth_stencil=depth;
    if(device.gpu&&device.gpu_frame_active) {
        if(device.render_target==&device.back_buffer)
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)device.gpu_default_framebuffer);
        else if(!mac_engine_device_resources_bind_target(device.render_target,depth)) {
            /* A target the native renderer cannot draw into (a mip level of
               the water's ripple texture): its draws and clears are skipped
               until another is set, so that the texture keeps what it had. */
            static int reported;
            if(!reported) {
                fprintf(stderr,"D3D render target is not a supported native color/depth surface pair: its draws are skipped\n");
                reported=1;
            }
            gpu_target_skipped=1;
        } else gpu_target_skipped=0;
        if(device.render_target==&device.back_buffer)gpu_target_skipped=0;
    }
    if(device.render_target){
        xgpu_texture_describe(device.render_target->Format,device.render_target->Size,&description);
        device.viewport.X=device.viewport.Y=0;
        device.viewport.Width=description.width;device.viewport.Height=description.height;
        device.viewport.MinZ=0.0f;device.viewport.MaxZ=1.0f;
    }
    viewport_update_constants();
}
HRESULT WINAPI D3DDevice_CreateVertexShader(CONST DWORD *declaration,CONST DWORD *function,DWORD *handle,DWORD usage)
{
    struct vertex_shader_object *shader;
    (void)usage;
    if(!active||!handle)return E_INVALIDARG;
    shader=calloc(1,sizeof(*shader));if(!shader)return E_OUTOFMEMORY;
    shader->signature=VERTEX_SHADER_SIGNATURE;shader->id=device.next_vertex_shader_id++;
    if(function){
        shader->instruction_count=function[0]>>16;
        if(shader->instruction_count){
            shader->instructions=malloc(shader->instruction_count*4*sizeof(DWORD));
            if(!shader->instructions){free(shader);return E_OUTOFMEMORY;}
            memcpy(shader->instructions,function+1,shader->instruction_count*4*sizeof(DWORD));
        }
    }
    parse_declaration(shader,declaration);
    shader->next=device.owned_shaders;device.owned_shaders=shader;
    *handle=(DWORD)shader;return S_OK;
}
void halo_vertex_shader_lighting(unsigned long handle)
{
    struct vertex_shader_object *shader=(struct vertex_shader_object *)handle;
    if(!active||!shader||shader->signature!=VERTEX_SHADER_SIGNATURE||
       !shader->instructions||!shader->instruction_count) {
        fprintf(stderr,"native D3D vertex-lighting setup received an invalid shader\n");
        abort();
    }
    memset(&shader->lighting,0,sizeof(shader->lighting));
    if(!nv2a_vertex_shader_lighting(shader->instructions,shader->instruction_count,
        &shader->lighting))
        memset(&shader->lighting,0,sizeof(shader->lighting));
}
void WINAPI D3DDevice_DeleteVertexShader(DWORD handle)
{ /* Shared renderer keeps programs cached; native lifecycle frees them at stop. */ (void)handle; }
static GLenum mac_gpu_primitive(D3DPRIMITIVETYPE type)
{
    switch(type){case D3DPT_POINTLIST:return GL_POINTS;case D3DPT_LINELIST:return GL_LINES;
    case D3DPT_LINESTRIP:return GL_LINE_STRIP;case D3DPT_TRIANGLELIST:case D3DPT_QUADLIST:return GL_TRIANGLES;
    case D3DPT_TRIANGLESTRIP:return GL_TRIANGLE_STRIP;case D3DPT_TRIANGLEFAN:return GL_TRIANGLE_FAN;default:return (GLenum)-1;}
}
/* A draw the native renderer cannot make. In play it is skipped, its
   reason logged a few times, and the game goes on (one missing effect
   rather than the game stopping); runs that check the renderer (a frame
   count or seed, or HALO_MAC_STRICT_DRAWS=1) stop on it, so it is found.
   HALO_MAC_STRICT_DRAWS=0 skips there too. */
static jmp_buf *gpu_draw_escape;
/* (HALO_MAC_PERF_JMP) _setjmp/_longjmp for the draw escape: setjmp saves
   the signal mask with two system calls a draw, which the escape does not
   need (the mask is never changed here) */
#define GPU_GUARDED(escape,call) \
    do{ if(gpu_fast_jmp){if(!_setjmp(escape)){gpu_draw_escape=&(escape);call;}} \
        else {if(!setjmp(escape)){gpu_draw_escape=&(escape);call;}} }while(0)
static int gpu_draws_strict=-1;
static unsigned long gpu_draws_skipped;
static int mac_gpu_draws_strict(void)
{
    if(gpu_draws_strict<0) {
        const char *strict=getenv("HALO_MAC_STRICT_DRAWS");
        gpu_draws_strict=strict?strict[0]=='1':
            getenv("HALO_MAC_RENDER_FRAMES")!=NULL||getenv("HALO_MAC_RENDER_SEED")!=NULL;
    }
    return gpu_draws_strict;
}
static void mac_gpu_shader_failure(const char *message)
{
    if(!gpu_draw_escape||mac_gpu_draws_strict()) {
        fprintf(stderr,"native D3D immediate draw unavailable: %s\n",message);abort();
    }
    if(gpu_draws_skipped<16||!(gpu_draws_skipped&(gpu_draws_skipped-1)))
        fprintf(stderr,"native D3D draw skipped (%lu so far): %s\n",gpu_draws_skipped+1,message);
    ++gpu_draws_skipped;
    if(gpu_fast_jmp)_longjmp(*gpu_draw_escape,1);
    longjmp(*gpu_draw_escape,1);
}
static void mac_gpu_upload_vec4(GLuint program,const char *name,const float *values,GLsizei count)
{ GLint location=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,name);if(location>=0)glUniform4fv(location,count,values); }
static void mac_gpu_upload_float(GLuint program,const char *name,float value)
{ GLint location=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,name);if(location>=0)glUniform1f(location,value); }
static float mac_gpu_state_float(DWORD value)
{
    float result;
    memcpy(&result,&value,sizeof(result));
    return result;
}
static GLenum mac_gpu_blend_equation(DWORD operation)
{
    /* Xbox operations use the same mapping as the shared GL renderer. */
    switch(operation) {
    case D3DBLENDOP_SUBTRACT:return GL_FUNC_SUBTRACT;
    case D3DBLENDOP_REVSUBTRACT:case D3DBLENDOP_REVSUBTRACTSIGNED:return GL_FUNC_REVERSE_SUBTRACT;
    case D3DBLENDOP_MIN:return GL_MIN;
    case D3DBLENDOP_MAX:return GL_MAX;
    default:return GL_FUNC_ADD;
    }
}
static void mac_gpu_upload_pixel_constants(GLuint program)
{
    float c0[8][4],c1[8][4],final0[4],final1[4],fog[4],parameters[4];unsigned long i;
    for(i=0;i<8;i++){
        color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0+i],c0[i]);
        color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0+i],c1[i]);
    }
    color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0],final0);
    color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1],final1);
    mac_gpu_upload_vec4(program,"ps_c0",&c0[0][0],8);
    mac_gpu_upload_vec4(program,"ps_c1",&c1[0][0],8);
    mac_gpu_upload_vec4(program,"ps_final_c0",final0,1);
    mac_gpu_upload_vec4(program,"ps_final_c1",final1,1);
    color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR],fog);
    parameters[0]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGSTART]);
    parameters[1]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGEND]);
    parameters[2]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGDENSITY]);parameters[3]=0.0f;
    mac_gpu_upload_vec4(program,"fog_color",fog,1);
    mac_gpu_upload_vec4(program,"fog_parameters",parameters,1);
    mac_gpu_upload_float(program,"alpha_reference",(float)(D3D__RenderState[D3DRS_ALPHAREF]&255));
}
/* one cached uniform: 1 when uploaded */
static int mac_gpu_cached_vec4(GLint location,float *last,const float *values,
    GLsizei count,unsigned long *valid,unsigned long bit)
{
    size_t bytes=(size_t)count*4*sizeof(float);
    if(location<0)return 0;
    if((*valid&bit)&&!memcmp(last,values,bytes))return 0;
    memcpy(last,values,bytes);*valid|=bit;
    glUniform4fv(location,count,values);return 1;
}
/* The block of the bound program, made on its first use. Every block is
   dropped if the program cache has evicted (a GL name may be reused). */
static struct gpu_uniform_block *mac_gpu_uniform_block(GLuint program)
{
    unsigned long i;struct gpu_uniform_block *block;
    GLint location,count=0,size=0,j;GLenum type;char name[64];
    if(gpu_program_cache.evictions!=gpu_uniform_evictions_seen) {
        gpu_uniform_evictions_seen=gpu_program_cache.evictions;
        gpu_uniform_block_count=0;
    }
    for(i=0;i<gpu_uniform_block_count;i++)
        if(gpu_uniform_blocks[i].program==program)return &gpu_uniform_blocks[i];
    if(gpu_uniform_block_count==GPU_UNIFORM_BLOCKS)gpu_uniform_block_count=0;
    block=&gpu_uniform_blocks[gpu_uniform_block_count++];
    memset(block,0,sizeof(*block));block->program=program;
    for(i=0;i<4;++i) {
        sprintf(name,"tex%lu",i);location=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,name);
        if(location>=0)glUniform1i(location,(GLint)i);
    }
    location=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,"point_size");
    if(location>=0)glUniform1f(location,1.0f);
    location=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,"screen_offset");
    block->screen_offset=location;
    if(location>=0)glUniform1f(location,0.0f);
#define BLOCK_LOCATION(field,uniform) block->field=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,uniform)
    BLOCK_LOCATION(texture_scale,"texture_scale");BLOCK_LOCATION(constants,"c");
    BLOCK_LOCATION(viewport_scale,"viewport_scale");BLOCK_LOCATION(viewport_offset,"viewport_offset");
    BLOCK_LOCATION(ps_c0,"ps_c0");BLOCK_LOCATION(ps_c1,"ps_c1");
    BLOCK_LOCATION(ps_final_c0,"ps_final_c0");BLOCK_LOCATION(ps_final_c1,"ps_final_c1");
    BLOCK_LOCATION(fog_color,"fog_color");BLOCK_LOCATION(fog_parameters,"fog_parameters");
    BLOCK_LOCATION(alpha_reference,"alpha_reference");
#undef BLOCK_LOCATION
    /* the active length of c (a compiler may shorten it) and whether its
       elements' locations run on, which a partial upload needs */
    block->constant_count=XGPU_VERTEX_CONSTANT_COUNT;
    glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&count);
    for(j=0;j<count;j++) {
        GLsizei length=0;
        glGetActiveUniform(program,(GLuint)j,(GLsizei)sizeof(name),&length,&size,&type,name);
        if(!strcmp(name,"c")||!strcmp(name,"c[0]")) {
            if(size>0&&size<XGPU_VERTEX_CONSTANT_COUNT)block->constant_count=(unsigned long)size;
            break;
        }
    }
    if(block->constants>=0&&block->constant_count>1) {
        sprintf(name,"c[%lu]",block->constant_count-1);
        block->constants_consecutive=glGetUniformLocation(program,"c[1]")==block->constants+1&&
            glGetUniformLocation(program,name)==block->constants+(GLint)(block->constant_count-1);
    }
    return block;
}
/* The vertex constants that changed since this program last had them
   (port/linux/src/d3d8_gl.c, the draw's constant upload). */
static void mac_gpu_upload_constants(struct gpu_uniform_block *block)
{
    unsigned long first=block->constant_count,last=0,index;
    if(block->constants<0)return;
    if(block->constants_valid&&block->constants_serial==constants_serial)return;
    if(!block->constants_valid) {
        first=0;last=block->constant_count-1;
    } else if(block->constants_serial==constants_checkpoint_serial&&
              constants_checkpoint_last<block->constant_count) {
        if(constants_checkpoint_first<=constants_checkpoint_last) {
            first=constants_checkpoint_first;last=constants_checkpoint_last;
        }
    } else if(constants_serial-block->constants_serial<=XGPU_VERTEX_CONSTANT_COUNT) {
        unsigned long long serial;
        for(serial=block->constants_serial+1;serial<=constants_serial;serial++) {
            index=constant_log[serial%CONSTANT_LOG_SIZE];
            if(index>=block->constant_count)continue;
            if(first>index)first=index;
            if(last<index)last=index;
        }
    } else {
        for(index=0;index<block->constant_count;index++)
            if(constant_serials[index]>block->constants_serial) {
                if(first>index)first=index;
                last=index;
            }
    }
    if(first<block->constant_count) {
        if(block->constants_consecutive)
            glUniform4fv(block->constants+(GLint)first,(GLsizei)(last-first+1),device.constants[first]);
        else glUniform4fv(block->constants,(GLsizei)block->constant_count,&device.constants[0][0]);
    }
    block->constants_serial=constants_serial;block->constants_valid=1;
    constants_checkpoint_serial=constants_serial;
    constants_checkpoint_first=XGPU_VERTEX_CONSTANT_COUNT;
    constants_checkpoint_last=0;
}
static void mac_gpu_upload_lean_uniforms(struct gpu_uniform_block *block)
{
    float c0[8][4],c1[8][4],final0[4],final1[4],fog[4],parameters[4],reference,flipped_scale[4];unsigned long i;
    mac_gpu_upload_constants(block);
    mac_gpu_cached_vec4(block->viewport_scale,block->last_viewport_scale,mac_gpu_viewport_scale(flipped_scale),1,&block->valid,1UL);
    mac_gpu_cached_vec4(block->viewport_offset,block->last_viewport_offset,device.viewport_offset,1,&block->valid,2UL);
    for(i=0;i<8;i++){
        color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0+i],c0[i]);
        color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0+i],c1[i]);
    }
    color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0],final0);
    color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1],final1);
    mac_gpu_cached_vec4(block->ps_c0,block->last_ps_c0,&c0[0][0],8,&block->valid,4UL);
    mac_gpu_cached_vec4(block->ps_c1,block->last_ps_c1,&c1[0][0],8,&block->valid,8UL);
    mac_gpu_cached_vec4(block->ps_final_c0,block->last_ps_final_c0,final0,1,&block->valid,16UL);
    mac_gpu_cached_vec4(block->ps_final_c1,block->last_ps_final_c1,final1,1,&block->valid,32UL);
    color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR],fog);
    parameters[0]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGSTART]);
    parameters[1]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGEND]);
    parameters[2]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGDENSITY]);parameters[3]=0.0f;
    mac_gpu_cached_vec4(block->fog_color,block->last_fog_color,fog,1,&block->valid,64UL);
    mac_gpu_cached_vec4(block->fog_parameters,block->last_fog_parameters,parameters,1,&block->valid,128UL);
    /* port: the menus' centring while they draw (halo_screen_ui_offset) */
    {
        float offset=(float)mac_engine_ui_offset();
        if(block->screen_offset>=0&&offset!=block->last_screen_offset) {
            glUniform1f(block->screen_offset,offset);block->last_screen_offset=offset;
        }
    }
    reference=(float)(D3D__RenderState[D3DRS_ALPHAREF]&255);
    if(block->alpha_reference>=0&&(!(block->valid&256UL)||block->last_alpha_reference!=reference)) {
        glUniform1f(block->alpha_reference,reference);
        block->last_alpha_reference=reference;block->valid|=256UL;
    }
}
static int mac_gpu_render_trace(void)
{
    const char *value;
    if(mac_render_trace_enabled<0) {
        value=getenv("HALO_MAC_RENDER_TRACE");
        mac_render_trace_enabled=value&&value[0]&&value[0]!='0';
    }
    return mac_render_trace_enabled;
}
static void mac_gpu_trace_draw(struct vertex_shader_object *vertex,
    unsigned long target_width,unsigned long target_height)
{
    unsigned long i,j;
    float minimum[4],maximum[4];
    if(!mac_gpu_render_trace()||!device.gpu_stream_draw_active||mac_render_trace_draws>=64)return;
    for(i=0;i<mac_render_trace_draws;i++)
        if(mac_render_trace_shader_ids[i]==vertex->id)return;
    mac_render_trace_shader_ids[mac_render_trace_draws]=vertex->id;
    ++mac_render_trace_draws;
    for(j=0;j<4;j++)minimum[j]=maximum[j]=device.immediate_vertices[j];
    for(i=1;i<device.immediate_count;i++)for(j=0;j<4;j++) {
        float value=device.immediate_vertices[i*(XGPU_VERTEX_ATTRIBUTE_COUNT*4)+j];
        if(value<minimum[j])minimum[j]=value;
        if(value>maximum[j])maximum[j]=value;
    }
    fprintf(stderr,"HALO_MAC_RENDER_TRACE draw=%lu shader=%lu instructions=%lu primitive=%lu count=%lu target=%lux%lu viewport=%lu,%lu %lux%lu depthrange=%.6g..%.6g depth=%d zenable=%lu zwrite=%lu zfunc=%lu stencil=%d/%lu ref=%lu v0=[%.6g..%.6g,%.6g..%.6g,%.6g..%.6g,%.6g..%.6g]",
        mac_render_trace_draws,vertex?vertex->id:0,vertex?vertex->instruction_count:0,
        (unsigned long)device.immediate_type,device.immediate_count,
        target_width,target_height,
        (unsigned long)device.viewport.X,(unsigned long)device.viewport.Y,
        (unsigned long)device.viewport.Width,(unsigned long)device.viewport.Height,
        (double)device.viewport.MinZ,(double)device.viewport.MaxZ,
        device.depth_stencil!=NULL,(unsigned long)D3D__RenderState[D3DRS_ZENABLE],
        (unsigned long)D3D__RenderState[D3DRS_ZWRITEENABLE],
        (unsigned long)D3D__RenderState[D3DRS_ZFUNC],
        device.depth_stencil!=NULL&&D3D__RenderState[D3DRS_STENCILENABLE],
        (unsigned long)D3D__RenderState[D3DRS_STENCILFUNC],
        (unsigned long)D3D__RenderState[D3DRS_STENCILREF],
        minimum[0],maximum[0],minimum[1],maximum[1],minimum[2],maximum[2],minimum[3],maximum[3]);
    fprintf(stderr,"HALO_MAC_RENDER_TRACE c[-96..-89]=");
    for(i=0;i<8;i++)fprintf(stderr,"%s(%.6g,%.6g,%.6g,%.6g)",i?" ":"",
        device.constants[i][0],device.constants[i][1],device.constants[i][2],device.constants[i][3]);
    fputc('\n',stderr);
    if(device.depth_stencil&&D3D__RenderState[D3DRS_STENCILENABLE]&&
       D3D__RenderState[D3DRS_STENCILFUNC]==D3DCMP_ALWAYS&&
       D3D__RenderState[D3DRS_STENCILREF]==1) {
        fprintf(stderr,"HALO_MAC_RENDER_TRACE first_person_node_constants c[-36..-31]=");
        for(i=60;i<66;i++)fprintf(stderr,"%s(%.6g,%.6g,%.6g,%.6g)",i==60?"":" ",
            device.constants[i][0],device.constants[i][1],device.constants[i][2],device.constants[i][3]);
        fputc('\n',stderr);
    }
    if(vertex) {
        fprintf(stderr,"HALO_MAC_RENDER_TRACE declarations=%lu",vertex->element_count);
        for(i=0;i<vertex->element_count&&i<12;i++)
            fprintf(stderr," %u:%u/%u@%u+%u",(unsigned)vertex->elements[i].reg,
                (unsigned)vertex->elements[i].stream,(unsigned)vertex->elements[i].type,
                (unsigned)vertex->elements[i].offset,(unsigned)vertex->elements[i].bytes);
        fputc('\n',stderr);
    }
}
/* Diagnostic only (HALO_MAC_DIAG_DRAW_TIMES=1): each draw of the sixth
   presented frame, in order, with the time inside its draw call and whether
   it bound programs or sent parameters. */
static int gpu_diag_draw_times=-1;
#define SEGMENT_MARK(k) do{if(gpu_diag_segments>0){uint64_t now_=mach_absolute_time();gpu_segment_time[k]+=now_-segment_last;segment_last=now_;}}while(0)
static void mac_diag_draw_time(unsigned long id,unsigned long count,uint64_t start,
    unsigned long long vertex_binds,unsigned long long fragment_binds,unsigned long long env_writes)
{
    static mach_timebase_info_data_t timebase;
    uint64_t elapsed=mach_absolute_time()-start;
    if(!timebase.denom)mach_timebase_info(&timebase);
    fprintf(stderr,"HALO_MAC_DRAW_TIME vertex=%lu count=%lu us=%.1f vbind=%llu fbind=%llu env=%llu\n",
        id,count,(double)elapsed*timebase.numer/timebase.denom/1000.0,
        gpu_vertex_program_binds-vertex_binds,gpu_fragment_program_binds-fragment_binds,
        gpu_arb_env_writes+gpu_arbfp_env_writes-env_writes);
}
/* ---- fixed-function combiner draws (HALO_MAC_FF_COMBINERS) */
#ifndef GL_COMBINE_ARB
#define GL_COMBINE_ARB 0x8570
#define GL_COMBINE_RGB_ARB 0x8571
#define GL_COMBINE_ALPHA_ARB 0x8572
#define GL_RGB_SCALE_ARB 0x8573
#define GL_ADD_SIGNED_ARB 0x8574
#define GL_INTERPOLATE_ARB 0x8575
#define GL_CONSTANT_ARB 0x8576
#define GL_PRIMARY_COLOR_ARB 0x8577
#define GL_PREVIOUS_ARB 0x8578
#define GL_SOURCE0_RGB_ARB 0x8580
#define GL_SOURCE0_ALPHA_ARB 0x8588
#define GL_OPERAND0_RGB_ARB 0x8590
#define GL_OPERAND0_ALPHA_ARB 0x8598
#endif
#ifndef GL_SUBTRACT_ARB
#define GL_SUBTRACT_ARB 0x84E7
#endif
#ifndef GL_MODULATE_ADD_ATI
#define GL_MODULATE_ADD_ATI 0x8744
#endif
/* the texture environment fields kept per unit (indices into the cache) */
#define FF_FIELDS 18
static unsigned char gpu_ff_unit_on[4];
static int gpu_ff_field[4][FF_FIELDS];static unsigned long gpu_ff_field_valid[4];
static float gpu_ff_color[4][4];static unsigned char gpu_ff_color_valid[4];
static int gpu_ff_alpha_on;static float gpu_ff_alpha_reference;static int gpu_ff_alpha_valid;
/* Whether the GL has what the combiner draws need; `wanted` is the switch. */
static int mac_ff_probe(int wanted)
{
    const char *extensions;GLint units=0;
    if(!wanted)return 0;
    extensions=(const char *)glGetString(GL_EXTENSIONS);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS,&units);
    if(!extensions||!strstr(extensions,"GL_ARB_texture_env_combine")||!strstr(extensions,"GL_ARB_texture_env_crossbar")||units<4) {
        fprintf(stderr,"native fixed-function combiners: unavailable (units=%d)\n",(int)units);return 0;
    }
    gpu_ff_combine3=strstr(extensions,"GL_ATI_texture_env_combine3")!=NULL;
    /* (HALO_MAC_FF_FORWARD=1: a stage reads textures of later units too, fewer stages) */
    mac_nv2a_arbfp_set_ff_forward(mac_flag("HALO_MAC_FF_FORWARD",0));
    fprintf(stderr,"native fixed-function combiners: on (ATI_texture_env_combine3 %s)\n",gpu_ff_combine3?"yes":"no");
    return 1;
}
static void mac_ff_field(unsigned unit,unsigned field,GLenum name,GLint value)
{
    if((gpu_ff_field_valid[unit]&(1UL<<field))&&gpu_ff_field[unit][field]==(int)value)return;
    glTexEnvi(GL_TEXTURE_ENV,name,value);++gpu_ff_texenv_writes;
    gpu_ff_field[unit][field]=(int)value;gpu_ff_field_valid[unit]|=1UL<<field;
}
static GLenum mac_ff_source(unsigned source,unsigned unit)
{
    if(source<4)return source==unit?GL_TEXTURE:(GLenum)(GL_TEXTURE0_ARB+source);
    if(source==MAC_FF_SRC_PRIMARY)return GL_PRIMARY_COLOR_ARB;
    if(source==MAC_FF_SRC_CONSTANT)return GL_CONSTANT_ARB;
    return GL_PREVIOUS_ARB;
}
static GLenum mac_ff_operand(unsigned operand)
{
    static const GLenum operands[4]={GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA};
    return operands[operand&3];
}
static GLenum mac_ff_function(unsigned function)
{
    switch(function) {
    case MAC_FF_MODULATE:return GL_MODULATE;
    case MAC_FF_ADD:return GL_ADD;
    case MAC_FF_SUBTRACT:return GL_SUBTRACT_ARB;
    case MAC_FF_INTERPOLATE:return GL_INTERPOLATE_ARB;
    case MAC_FF_MODULATE_ADD:return GL_MODULATE_ADD_ATI;
    default:return GL_REPLACE;
    }
}
/* the value of a combiner colour constant (program.env[0..17] of the fragment programs) */
static void mac_ff_env_value(unsigned index,float value[4])
{
    if(index<8)color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0+index],value);
    else if(index<16)color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0+(index-8)],value);
    else if(index==16)color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0],value);
    else color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1],value);
}
static void mac_ff_alpha_off(void)
{
    if(gpu_ff_alpha_on){glDisable(GL_ALPHA_TEST);gpu_ff_alpha_on=0;++gpu_ff_alpha_writes;}
}
/* Put a draw's pixel state in the texture units: stage n on unit n; units
   beyond the last are switched off so that they do not continue the chain. */
static void mac_ff_apply(const struct mac_nv2a_ff_program *ff,const unsigned char *sampler)
{
    unsigned unit;
    for(unit=0;unit<4;unit++) {
        const struct mac_nv2a_ff_stage *stage=&ff->stage[unit];
        float color[4],value[4];
        if(unit>=ff->stage_count) {
            if(gpu_ff_unit_on[unit]){
                mac_gpu_textures_ff_select(unit);glDisable(GL_TEXTURE_2D);
                gpu_ff_unit_on[unit]=0;++gpu_ff_unit_toggles;
            }
            continue;
        }
        if(sampler[unit]!=MAC_GPU_TEXTURE_SAMPLER_2D)mac_gpu_textures_ff_dummy(unit);
        mac_gpu_textures_ff_select(unit);
        if(!gpu_ff_unit_on[unit]){glEnable(GL_TEXTURE_2D);gpu_ff_unit_on[unit]=1;++gpu_ff_unit_toggles;}
        mac_ff_field(unit,0,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
        mac_ff_field(unit,1,GL_COMBINE_RGB_ARB,(GLint)mac_ff_function(stage->rgb_function));
        mac_ff_field(unit,2,GL_COMBINE_ALPHA_ARB,(GLint)mac_ff_function(stage->a_function));
        {
            unsigned k,rgb_args=stage->rgb_function==MAC_FF_REPLACE?1:stage->rgb_function>=MAC_FF_INTERPOLATE?3:2;
            unsigned a_args=stage->a_function==MAC_FF_REPLACE?1:stage->a_function>=MAC_FF_INTERPOLATE?3:2;
            for(k=0;k<3;k++) {
                if(k<rgb_args) {
                    mac_ff_field(unit,3+k,GL_SOURCE0_RGB_ARB+k,(GLint)mac_ff_source(stage->rgb_source[k],unit));
                    mac_ff_field(unit,6+k,GL_OPERAND0_RGB_ARB+k,(GLint)mac_ff_operand(stage->rgb_operand[k]));
                }
                if(k<a_args) {
                    mac_ff_field(unit,9+k,GL_SOURCE0_ALPHA_ARB+k,(GLint)mac_ff_source(stage->a_source[k],unit));
                    mac_ff_field(unit,12+k,GL_OPERAND0_ALPHA_ARB+k,(GLint)mac_ff_operand(stage->a_operand[k]));
                }
            }
        }
        mac_ff_field(unit,15,GL_RGB_SCALE_ARB,(GLint)stage->rgb_scale);
        mac_ff_field(unit,16,GL_ALPHA_SCALE,(GLint)stage->a_scale);
        if(stage->rgb_const_kind||stage->a_const_kind) {
            color[0]=color[1]=color[2]=color[3]=0.0f;
            if(stage->rgb_const_kind==MAC_FF_CONST_LITERAL)color[0]=color[1]=color[2]=stage->rgb_literal;
            else if(stage->rgb_const_kind==MAC_FF_CONST_ENV){
                mac_ff_env_value(stage->rgb_env,value);color[0]=value[0];color[1]=value[1];color[2]=value[2];
            }
            if(stage->a_const_kind==MAC_FF_CONST_LITERAL)color[3]=stage->a_literal;
            else if(stage->a_const_kind==MAC_FF_CONST_ENV){mac_ff_env_value(stage->a_env,value);color[3]=value[3];}
            if(!gpu_ff_color_valid[unit]||memcmp(gpu_ff_color[unit],color,sizeof(color))) {
                glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,color);++gpu_ff_const_writes;
                memcpy(gpu_ff_color[unit],color,sizeof(color));gpu_ff_color_valid[unit]=1;
            }
        }
    }
    if(ff->alpha_test) {
        /* kept where floor(alpha * 255 + offset) >= reference */
        float reference=((float)(D3D__RenderState[D3DRS_ALPHAREF]&255)-ff->alpha_offset)/255.0f;
        if(!gpu_ff_alpha_on){glEnable(GL_ALPHA_TEST);gpu_ff_alpha_on=1;++gpu_ff_alpha_writes;}
        if(!gpu_ff_alpha_valid||gpu_ff_alpha_reference!=reference){
            glAlphaFunc(GL_GEQUAL,reference);gpu_ff_alpha_reference=reference;gpu_ff_alpha_valid=1;++gpu_ff_alpha_writes;
        }
    } else mac_ff_alpha_off();
}
/* Before anything else uses the fixed-function pipeline (the scaled frame's
   copy to the window): the units off, the state forgotten. */
static void mac_ff_leave(void)
{
    unsigned unit;
    for(unit=0;unit<4;unit++) {
        if(gpu_ff_unit_on[unit]){mac_gpu_textures_ff_select(unit);glDisable(GL_TEXTURE_2D);gpu_ff_unit_on[unit]=0;}
        gpu_ff_field_valid[unit]=0;gpu_ff_color_valid[unit]=0;
    }
    mac_ff_alpha_off();gpu_ff_alpha_valid=0;
    if(gpu_ff_draws)mac_gpu_textures_ff_select(0);
}
/* ---- APPLE_vertex_array_range ring (HALO_MAC_VAR=1, default off)
   The per-draw vertex data that is drawn from client arrays (immediate and
   stream-decoded draws) is packed (only the attributes the program reads)
   into a ring in a vertex-array-range region, as Halo for the Mac did, so that
   the driver can read it by DMA instead of copying it at every draw. The ring
   is VAR_SEGMENTS segments; a segment is fenced (APPLE_fence) when the ring
   leaves it and waited for before it is written again. Needs
   GL_APPLE_vertex_array_range and GL_APPLE_fence; otherwise (or for a draw
   larger than a segment) the client arrays are used as before. */
#ifndef GL_VERTEX_ARRAY_RANGE_APPLE
#define GL_VERTEX_ARRAY_RANGE_APPLE 0x851D
#endif
static int mac_var_mode(void)
{
    const char *value=getenv("HALO_MAC_VAR");
    return value&&value[0]=='1'&&!value[1]?1:value&&value[0]=='2'&&!value[1]?2:0;
}
static int mac_var_probe(int wanted)
{
    const char *extensions;void *region=NULL;
    if(!wanted)return 0;
    extensions=(const char *)glGetString(GL_EXTENSIONS);
    if(wanted==1&&(!extensions||!strstr(extensions,"GL_APPLE_vertex_array_range")||!strstr(extensions,"GL_APPLE_fence"))) {
        fprintf(stderr,"native vertex array range: unavailable\n");return 0;
    }
    {const char *kb=getenv("HALO_MAC_VAR_KB");if(kb&&atol(kb)>=16)var_segment_bytes=(unsigned long)atol(kb)*1024UL;}
    region=valloc(VAR_SEGMENTS*VAR_SEGMENT_BYTES);/* (page aligned; Tiger has no posix_memalign) */
    if(!region)return 0;
    gpu_var_base=(unsigned char *)region;
    {
        /* The GeForce 9400M driver of Mac OS X 10.6 crashes in glDrawArrays on the first draw whose
           generic attribute arrays lie in the range (found on the MacBook; the Radeons were not
           available to test): not used there unless HALO_MAC_VAR_FORCE is set. */
        const char *vendor=(const char *)glGetString(GL_VENDOR);
        if(wanted==1&&vendor&&strstr(vendor,"NVIDIA")&&!getenv("HALO_MAC_VAR_FORCE")) {
            fprintf(stderr,"native vertex array range: not used on this driver (%s)\n",vendor);free(region);gpu_var_base=NULL;return 0;
        }
    }
    /* (HALO_MAC_VAR=2: the ring and its packing without the range, to check
       the packing where the driver does not take the range) */
    if(wanted==2){fprintf(stderr,"native vertex array range: ring only, range not enabled\n");return 1;}
    glVertexArrayRangeAPPLE((GLsizei)(VAR_SEGMENTS*VAR_SEGMENT_BYTES),gpu_var_base);
    /* (HALO_MAC_VAR_HINT=cached|client: another storage hint; shared is for data written every frame) */
    {
        const char *hint=getenv("HALO_MAC_VAR_HINT");
        glVertexArrayParameteriAPPLE(GL_VERTEX_ARRAY_STORAGE_HINT_APPLE,
            hint&&!strcmp(hint,"cached")?GL_STORAGE_CACHED_APPLE:hint&&!strcmp(hint,"client")?GL_STORAGE_CLIENT_APPLE:GL_STORAGE_SHARED_APPLE);
    }
    glEnableClientState(GL_VERTEX_ARRAY_RANGE_APPLE);
    glGenFencesAPPLE(VAR_SEGMENTS,gpu_var_fence);
    fprintf(stderr,"native vertex array range: on (%lu segments of %lu bytes)\n",(unsigned long)VAR_SEGMENTS,(unsigned long)VAR_SEGMENT_BYTES);
    return 1;
}
static unsigned char *mac_var_alloc(size_t bytes)
{
    size_t aligned=(bytes+31U)&~(size_t)31U;
    unsigned char *chunk;
    if(aligned>VAR_SEGMENT_BYTES)return NULL;
    if(gpu_var_used+aligned>VAR_SEGMENT_BYTES) {
        /* leaving a segment: everything drawn from it is already issued */
        if(gpu_var_enabled==1){glSetFenceAPPLE(gpu_var_fence[gpu_var_segment]);gpu_var_fence_set[gpu_var_segment]=1;}
        gpu_var_segment=(gpu_var_segment+1)%VAR_SEGMENTS;gpu_var_used=0;
        if(gpu_var_fence_set[gpu_var_segment]) {
            ++gpu_var_waits;
            if(!glTestFenceAPPLE(gpu_var_fence[gpu_var_segment])) {
                ++gpu_var_stalls;glFinishFenceAPPLE(gpu_var_fence[gpu_var_segment]);
            }
            gpu_var_fence_set[gpu_var_segment]=0;
        }
    }
    chunk=gpu_var_base+gpu_var_segment*VAR_SEGMENT_BYTES+gpu_var_used;
    gpu_var_used+=aligned;
    return chunk;
}
/* The vertices' attributes in `mask`, packed, in the ring; slots[i] is where
   attribute i is (in vec4s), *stride the bytes between vertices. NULL: draw
   from the client arrays. */
static unsigned char *mac_var_pack(const float *vertices,unsigned long count,unsigned long mask,
    unsigned *stride,unsigned *slots)
{
    unsigned used=0,i,k;size_t bytes;unsigned char *chunk;unsigned long v;
    for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++)if(mask&(1UL<<i))slots[i]=used++;
    if(!used)return NULL;
    *stride=used*16U;bytes=(size_t)count*(size_t)*stride;
    chunk=mac_var_alloc(bytes);
    if(!chunk){++gpu_var_fallbacks;return NULL;}
    for(v=0;v<count;v++) {
        const float *source=vertices+v*(XGPU_VERTEX_ATTRIBUTE_COUNT*4);
        float *target=(float *)(chunk+v*(size_t)*stride);
        for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++)
            if(mask&(1UL<<i)){for(k=0;k<4;k++)target[k]=source[i*4+k];target+=4;}
    }
    if(gpu_var_enabled==1)glFlushVertexArrayRangeAPPLE((GLsizei)bytes,chunk);
    ++gpu_var_draws;gpu_var_bytes+=bytes;
    return chunk;
}
static void mac_gpu_draw_immediate(void)
{
    struct vertex_shader_object *vertex=device.program_address<VERTEX_PROGRAM_SLOTS?
        device.program_slots[device.program_address]:device.vertex_shader;
    unsigned long long diag_vertex_binds=gpu_vertex_program_binds,diag_fragment_binds=gpu_fragment_program_binds;
    unsigned long long diag_env_writes=gpu_arb_env_writes+gpu_arbfp_env_writes;
    uint64_t diag_start=0;
    struct nv2a_pixel_shader_key key;
    const char *legacy_vertex=NULL,*legacy_pixel=NULL;
    struct mac_shader_source_result source_result;
    mac_render_timing_stamp phase_stamp, detail_stamp, sub_stamp;
    char error[160];unsigned long input_mask=0,i;
    unsigned long active_attribute_mask=0;
    GLuint program=0;GLint old_program=0,old_array_buffer=0;
    struct mac_vertex_upload_layout upload_layout;
    const float *draw_vertices=device.immediate_vertices;
    char upload_error[160];
    int compact_upload_active=0;
    struct mac_gpu_texture_scope texture_scope;float texture_scale[4][4];
    const void *stage_textures[4];
    GLboolean old_depth=GL_FALSE,old_blend=GL_FALSE,old_cull=GL_FALSE;
    GLboolean old_scissor=GL_FALSE,old_dither=GL_FALSE,old_color[4],old_depth_mask=GL_TRUE;
    GLint old_depth_function=GL_LESS,old_cull_mode=GL_BACK,old_front_face=GL_CCW;
    GLint old_blend_source,old_blend_destination,old_blend_equation;
    GLfloat old_blend_color[4];
    GLboolean old_stencil=GL_FALSE;
    GLint old_stencil_func,old_stencil_ref,old_stencil_mask,old_stencil_write;
    GLint old_stencil_fail,old_stencil_zfail,old_stencil_pass;
    GLenum primitive;const char *failure=NULL;int texture_state_saved=0;
    unsigned target_width=device.gpu_width,target_height=device.gpu_height;
    struct gpu_uniform_block *uniform_block=NULL;
    const struct arb_vertex_program *arb_vertex=NULL;
    const struct arbfp_program *arbfp=NULL;
    const struct mac_nv2a_ff_program *ff=NULL;
    GLuint diag_samples_query=0;
    memset(&source_result,0,sizeof(source_result));
    if(gpu_target_skipped){++gpu_target_skipped_draws;return;}
    if(device.render_target==&device.back_buffer)++device.gpu_back_buffer_writes;
    else if(gpu_flip_logging)mac_flip_log_draw();
    if(!gpu_lean_draw) {
        old_depth=glIsEnabled(GL_DEPTH_TEST);old_blend=glIsEnabled(GL_BLEND);old_cull=glIsEnabled(GL_CULL_FACE);
        old_scissor=glIsEnabled(GL_SCISSOR_TEST);old_dither=glIsEnabled(GL_DITHER);
        old_stencil=glIsEnabled(GL_STENCIL_TEST);
    }
    if(!active||!device.gpu||!device.gpu_frame_active||!device.render_target)
        mac_gpu_shader_failure("draw is outside an active GPU frame");
    if(device.render_target!=&device.back_buffer&&
       !mac_engine_device_resources_target_info(device.render_target,NULL,NULL,&target_width,&target_height))
        mac_gpu_shader_failure("draw target has no owned framebuffer backing");
    if(device.viewport.X>target_width||device.viewport.Y>target_height||
       device.viewport.Width>target_width-device.viewport.X||
       device.viewport.Height>target_height-device.viewport.Y)
        mac_gpu_shader_failure("draw viewport exceeds its render target");
    if(!vertex||!device.vertex_shader||!vertex->instructions||!vertex->instruction_count)
        mac_gpu_shader_failure("no live NV2A vertex program is selected");
    if(vertex->packed_mask&&!device.gpu_stream_draw_active)
        mac_gpu_shader_failure("packed attributes require the bounded indexed stream decoder");
    primitive=mac_gpu_primitive(device.immediate_type);
    if(primitive==(GLenum)-1){fprintf(stderr,"native D3D immediate primitive unsupported: type=%lu\n",(unsigned long)device.immediate_type);mac_gpu_shader_failure("primitive kind is outside the immediate subset");}
    if(!device.immediate_count||device.immediate_count>65535)
        mac_gpu_shader_failure("empty or oversized immediate vertex batch");
    if((device.immediate_type==D3DPT_TRIANGLELIST&&device.immediate_count%3) ||
       (device.immediate_type==D3DPT_LINELIST&&device.immediate_count%2) ||
       (device.immediate_type==D3DPT_QUADLIST&&device.immediate_count%4))
        mac_gpu_shader_failure("incomplete list primitive");
    mac_gpu_trace_draw(vertex,target_width,target_height);
    {
        /* Diagnostic only (HALO_MAC_DIAG_DRAW_LOG=<vertex shader id>): that
           shader's draws in the sixth presented frame, with their textures,
           blending and callers (for atos), to find a frame's costly draws. */
        static int parsed=0;static long id=-1;static unsigned logged=0;
        unsigned stage;
        if(!parsed){const char *value=getenv("HALO_MAC_DIAG_DRAW_LOG");parsed=1;if(value&&*value)id=strcmp(value,"all")?atol(value):-3;}
        if((id==-3||(id>=0&&(unsigned long)id==vertex->id))&&device.gpu_present_count==5&&logged<600) {
            ++logged;
            fprintf(stderr,"HALO_MAC_DRAW_LOG vertex=%lu count=%lu type=%lu stream=%d target=%lux%lu viewport=%lu,%lu %lux%lu combiners=%lu modes=0x%lx blend=%lu src=%lu dst=%lu alphatest=%lu z=%lu/%lu callers=%p,%p,%p textures=",
                vertex->id,device.immediate_count,(unsigned long)device.immediate_type,device.gpu_stream_draw_active,
                target_width,target_height,(unsigned long)device.viewport.X,(unsigned long)device.viewport.Y,
                (unsigned long)device.viewport.Width,(unsigned long)device.viewport.Height,
                (unsigned long)(D3D__RenderState[D3DRS_PSCOMBINERCOUNT]&0xff),(unsigned long)D3D__RenderState[D3DRS_PSTEXTUREMODES],
                (unsigned long)D3D__RenderState[D3DRS_ALPHABLENDENABLE],(unsigned long)D3D__RenderState[D3DRS_SRCBLEND],
                (unsigned long)D3D__RenderState[D3DRS_DESTBLEND],(unsigned long)D3D__RenderState[D3DRS_ALPHATESTENABLE],
                (unsigned long)D3D__RenderState[D3DRS_ZENABLE],(unsigned long)D3D__RenderState[D3DRS_ZWRITEENABLE],
                __builtin_return_address(0),__builtin_return_address(1),__builtin_return_address(2));
            for(stage=0;stage<4;stage++) {
                const D3DBaseTexture *texture=(const D3DBaseTexture *)device.textures[stage];
                if(!texture){fprintf(stderr,"-,");continue;}
                fprintf(stderr,"%lux%lu/f%lx/%s,",
                    texture->Size?(unsigned long)((texture->Size&D3DSIZE_WIDTH_MASK)+1):1UL<<((texture->Format>>20)&15),
                    texture->Size?(unsigned long)(((texture->Size&D3DSIZE_HEIGHT_MASK)>>D3DSIZE_HEIGHT_SHIFT)+1):1UL<<((texture->Format>>24)&15),
                    (unsigned long)((texture->Format&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT),
                    texture==(const D3DBaseTexture *)device.render_target?"target":"tex");
            }
            fprintf(stderr,"\n");
        }
    }
    {
        /* Diagnostic only (HALO_MAC_DIAG_SKIP_VERTEX="31,50"): skip draws of
           the listed vertex shader table entries, to measure their cost. */
        static int parsed=0;static unsigned long skip_ids[16];static unsigned skip_count=0;
        unsigned k;
        if(!parsed) {
            const char *list=getenv("HALO_MAC_DIAG_SKIP_VERTEX");
            parsed=1;
            while(list&&*list&&skip_count<16) {
                char *end=NULL;unsigned long id=strtoul(list,&end,10);
                if(end==list)break;
                skip_ids[skip_count++]=id;list=*end==','?end+1:end;
            }
            if(skip_count)fprintf(stderr,"native diagnostic: skipping %u vertex shader ids\n",skip_count);
        }
        for(k=0;k<skip_count;k++)if(vertex->id==skip_ids[k]) {
            if(gpu_world_draw.active)gpu_world_draw.active=0;
            return;
        }
    }
    if(D3D__RenderState[D3DRS_SHADEMODE]&&D3D__RenderState[D3DRS_SHADEMODE]!=MAC_D3DSHADE_GOURAUD)
        mac_gpu_shader_failure("flat-shaded primitives are unsupported");
    memset(&key,0,sizeof(key));
    memcpy(key.combiner_state,D3D__RenderState,sizeof(key.combiner_state));
    key.texture_modes=D3D__RenderState[D3DRS_PSTEXTUREMODES];
    for(i=0;i<4;++i)stage_textures[i]=device.textures[i];
    /* A HUD meter (the only draw with this blend) samples Halo PC's meter
       bitmap, whose fill order is in alpha and shape in red: its texture is
       made with the channels where the Xbox's meter shader reads them (a
       static drawing the same bitmap has the bitmap as it is). */
    mac_gpu_textures_meter_draw(stage_textures[0]&&D3D__RenderState[D3DRS_ALPHABLENDENABLE]&&
       D3D__RenderState[D3DRS_SRCBLEND]==D3DBLEND_CONSTANTCOLOR&&D3D__RenderState[D3DRS_DESTBLEND]==D3DBLEND_SRCALPHA);
    phase_stamp=mac_render_timing_begin(mac_render_timing_texture_bind);
    {
        /* Diagnostic only (HALO_MAC_DIAG_NO_TEXTURES=1): draws keep the
           textures last bound, to measure what binding them costs. */
        static int no_textures=-1;
        if(no_textures<0){const char *v=getenv("HALO_MAC_DIAG_NO_TEXTURES");no_textures=v&&v[0]=='1';}
        if(no_textures&&device.gpu_present_count>1) {
            unsigned stage;
            for(stage=0;stage<4;stage++) {
                unsigned long mode=(key.texture_modes>>(5*stage))&0x1f;
                const D3DBaseTexture *native=(const D3DBaseTexture *)stage_textures[stage];
                key.sampler_type[stage]=0;texture_scale[stage][0]=texture_scale[stage][1]=texture_scale[stage][2]=texture_scale[stage][3]=1.0f;
                if(mode&&native)key.sampler_type[stage]=(native->Format&D3DFORMAT_CUBEMAP)?MAC_GPU_TEXTURE_SAMPLER_CUBE:MAC_GPU_TEXTURE_SAMPLER_2D;
            }
            mac_render_timing_end(mac_render_timing_texture_bind,phase_stamp);
            goto textures_bound;
        }
    }
    if(!mac_gpu_textures_bind(&texture_scope,stage_textures,key.texture_modes,
       key.sampler_type,texture_scale,&failure)) {
        if(!failure)failure="native texture stage binding failed";goto done;
    }
    mac_gpu_textures_meter_draw(0);
    mac_render_timing_end(mac_render_timing_texture_bind,phase_stamp);
    texture_state_saved=1;
textures_bound:
    for(i=0;i<4;++i) {
        /* the texture stages' alpha kill and signed channels, as d3d8_gl.c
           keys them: a HUD meter's shape is its bitmap's alpha, killed where 0 */
        key.alpha_kill[i]=D3D__TextureState[i][D3DTSS_ALPHAKILL]==D3DTALPHAKILL_ENABLE;
        key.color_sign[i]=(unsigned char)((D3D__TextureState[i][D3DTSS_COLORSIGN]>>28)&0xf);
    }
    key.alpha_test_function=D3D__RenderState[D3DRS_ALPHATESTENABLE]?D3D__RenderState[D3DRS_ALPHAFUNC]:0;
    key.fog_enable=D3D__RenderState[D3DRS_FOGENABLE]!=0;
    key.fog_table_mode=(unsigned char)D3D__RenderState[D3DRS_FOGTABLEMODE];
    if((key.combiner_state[D3DRS_PSCOMBINERCOUNT]&0xff)>8)
        mac_gpu_shader_failure("pixel combiner count exceeds the NV2A limit");
    /* Indexed attributes have already been expanded into float vectors by
       the native decoder; asking GLSL to unpack them again changes their ABI. */
    if(gpu_prog_memo_enabled<0)gpu_prog_memo_enabled=mac_flag("HALO_MAC_DM_PROGRAMS",1);
    if(gpu_prog_memo_enabled) {
        unsigned m;
        for(m=0;m<PROG_MEMOS;m++) {
            if(gpu_prog_memo[m].vertex==vertex&&gpu_prog_memo[m].epoch==gpu_prog_epoch&&
               gpu_prog_memo[m].stream==(device.gpu_stream_draw_active!=0)&&
               !memcmp(&gpu_prog_memo[m].key,&key,sizeof(key))&&
               !memcmp(gpu_prog_memo[m].scale,texture_scale,sizeof(texture_scale))) {
                arb_vertex=gpu_prog_memo[m].arb_vertex;arbfp=gpu_prog_memo[m].arbfp;
                input_mask=gpu_prog_memo[m].input_mask;++gpu_prog_memo_hits;
                goto programs_done;
            }
        }
        ++gpu_prog_memo_misses;
    }
    phase_stamp=mac_render_timing_begin(mac_render_timing_shader_sources);
    if(!mac_shader_source_cache_get(&gpu_source_cache,vertex->instructions,
        vertex->instruction_count,device.gpu_stream_draw_active?0:vertex->packed_mask,
        /* Match the shared renderer: extra world-space lighting outputs are
           needed only when the fragment shader actually consumes them. */
        key.per_pixel_lighting?&vertex->lighting:NULL,&key,&source_result)) {
        if(source_result.failure==MAC_SHADER_SOURCE_VERTEX_COMPAT_FAILED) {
            fprintf(stderr,"NV2A vertex shader %lu: %s\n%s\n",vertex->id,
                source_result.error,source_result.modern_vertex_source);
            failure="vertex shader is outside GLSL 1.20 subset";
        } else if(source_result.failure==MAC_SHADER_SOURCE_PIXEL_COMPAT_FAILED) {
            fprintf(stderr,"NV2A pixel shader: %s\n",source_result.error);
            failure="pixel shader is outside GLSL 1.20 subset";
        } else failure="NV2A shader generation failed";
        goto done;
    }
    mac_render_timing_end(mac_render_timing_shader_sources,phase_stamp);
    legacy_vertex=source_result.vertex_source;
    legacy_pixel=source_result.pixel_source;
    input_mask=source_result.input_mask;
    phase_stamp=mac_render_timing_begin(mac_render_timing_program_lookup);
    if(mac_arb_vertex_wanted(vertex->id)&&!key.per_pixel_lighting&&!gpu_compact_vertex_upload) {
        /* (HALO_MAC_ARB_VERTEX) The ARB program addresses the constants
           itself, so the draw needs no bone gather. */
        unsigned long projective=0,stage;
        arb_vertex=mac_arb_vertex_program(vertex,MAC_NV2A_ARB_RELATIVE);
        if(gpu_projective_fetch<0) {
            gpu_projective_fetch=mac_flag("HALO_MAC_PROJ_FETCH",1);
        }
        for(stage=0;gpu_projective_fetch&&stage<4;stage++)
            if(texture_scale[stage][0]==1.0f&&texture_scale[stage][1]==1.0f)projective|=1UL<<stage;
        if(gpu_arbfp_enabled<0) {
            gpu_arbfp_enabled=mac_flag("HALO_MAC_ARB_FRAGMENT",1);
        }
        if(gpu_var_enabled<0)gpu_var_enabled=mac_var_probe(mac_var_mode());
        if(gpu_ff_enabled<0)gpu_ff_enabled=gpu_arbfp_enabled&&gpu_lean_draw&&mac_ff_probe(mac_flag("HALO_MAC_FF_COMBINERS",0));
        if(arb_vertex&&gpu_arbfp_enabled)
            arbfp=mac_arbfp_program(&key,arb_vertex,projective,&source_result.identity);
        if(arbfp&&gpu_ff_enabled>0&&arbfp->ff.ok&&(gpu_ff_combine3||!arbfp->ff.needs_combine3)) {
            const struct arb_vertex_program *ff_vertex=mac_arb_vertex_program(vertex,MAC_NV2A_ARB_RELATIVE_FF);
            if(ff_vertex){arb_vertex=ff_vertex;ff=&arbfp->ff;}
        }
        if(arbfp)program=0;
        else program=arb_vertex?mac_arb_fragment_program(arb_vertex,legacy_pixel,projective,&source_result.identity):0;
        if(!program&&!arbfp)arb_vertex=NULL;
    }
    mac_render_timing_end(mac_render_timing_program_lookup,phase_stamp);
    if(arb_vertex) {
        /* It reads what the GLSL reads, the declared inputs and the bone
           address (v5) the GLSL's gather replaced; not the gathered rows. */
        unsigned long declared=0,e;
        for(e=0;e<vertex->element_count;e++)declared|=1UL<<vertex->elements[e].reg;
        input_mask|=declared|(1UL<<5);
        if(source_result.bone_gather.version)input_mask&=~(0x3fUL<<10)|declared;
    }
    if(gpu_prog_memo_enabled&&arb_vertex&&arbfp&&!ff&&!program) {
        unsigned m=gpu_prog_memo_next++%PROG_MEMOS;
        gpu_prog_memo[m].vertex=vertex;gpu_prog_memo[m].epoch=gpu_prog_epoch;gpu_prog_memo[m].stream=device.gpu_stream_draw_active!=0;
        memcpy(&gpu_prog_memo[m].key,&key,sizeof(key));memcpy(gpu_prog_memo[m].scale,texture_scale,sizeof(texture_scale));
        gpu_prog_memo[m].arb_vertex=arb_vertex;gpu_prog_memo[m].arbfp=arbfp;gpu_prog_memo[m].input_mask=input_mask;
    }
programs_done:
    /* port: the skinning program with the window its draw needs (mac_arb_vertex_window) */
    if(gpu_window_enabled<0)mac_window_configure();
    if(arb_vertex&&!ff&&gpu_window_enabled&&gpu_world_draw.active)
        arb_vertex=mac_arb_vertex_window(vertex,arb_vertex,MAC_NV2A_ARB_RELATIVE,gpu_world_draw.entry);
    if(arb_vertex&&(arb_vertex->gather>>8)&&gpu_world_draw.active) {
        /* Diagnostic only (HALO_MAC_DM_ARB_CHECK=1): every vertex the draw reaches reads inside the window */
        static int check=-1;
        if(check<0)check=getenv("HALO_MAC_DM_ARB_CHECK")!=NULL;
        if(check) {
            const struct world_vbo_entry *world=gpu_world_draw.entry;
            unsigned long v,first=gpu_world_draw.base_vertex;
            long limit=(long)arb_vertex->layout.skin_base+(arb_vertex->gather>>8)-1;
            ++gpu_window_checked;
            for(v=gpu_world_draw.minimum;v<=gpu_world_draw.maximum;v++) {
                unsigned c;
                if(first+v>=world->vertex_count){++gpu_window_violations;break;}
                for(c=0;c<2;c++) {
                    float a=world->addresses[(first+v)*2+c]*device.constants[7][3]+0.001f;
                    long reg=(long)floorf(a)+(long)arb_vertex->layout.skin_top;
                    if(a<0.0f||reg>limit){++gpu_window_violations;}
                }
            }
        }
    }
    /* A gathering world draw computes its rows from the buffer's decoded
       addresses for the vertices it reaches; without them (or if any of those
       vertices has an unusable address) it decodes per-draw records after all. */
    if(gpu_world_draw.active&&source_result.bone_gather.version&&!arb_vertex) {
        const struct world_vbo_entry *world=gpu_world_draw.entry;
        unsigned long first=gpu_world_draw.base_vertex+gpu_world_draw.minimum;
        unsigned long count=gpu_world_draw.maximum-gpu_world_draw.minimum+1;
        unsigned long needed=(gpu_world_draw.maximum+1)*24UL;
        int gathered=0;
        phase_stamp=mac_render_timing_begin(mac_render_timing_bone_gather);
        if(world->addresses&&first+count<=world->vertex_count) {
            if(needed>gpu_gather_rows_capacity) {
                float *grown=(float *)realloc(gpu_gather_rows,needed*sizeof(float));
                if(grown){gpu_gather_rows=grown;gpu_gather_rows_capacity=needed;}
            }
            /* rows for indices minimum..maximum, kept at their index position */
            if(needed<=gpu_gather_rows_capacity&&
               mac_bone_gather_rows(&source_result.bone_gather,world->addresses+first*2,count,
                   (const float (*)[4])device.constants,gpu_gather_rows+gpu_world_draw.minimum*24UL,
                   error,sizeof(error)))gathered=1;
        }
        mac_render_timing_end(mac_render_timing_bone_gather,phase_stamp);
        if(gathered){gpu_world_draw.gather_active=1;++gpu_world_vbo_gather_draws;}
        else mac_world_draw_decode_records();
    }
    if(source_result.bone_gather.version&&!gpu_world_draw.gather_active&&!arb_vertex) {
        size_t bytes=(size_t)device.immediate_count*XGPU_VERTEX_ATTRIBUTE_COUNT*4U*sizeof(float);
        phase_stamp=mac_render_timing_begin(mac_render_timing_bone_gather);
        if(bytes>gpu_bone_vertex_bytes) {
            float *grown=(float *)realloc(gpu_bone_vertices,bytes);
            if(!grown) {failure="bone matrix stream allocation failed";goto done;}
            gpu_bone_vertices=grown;gpu_bone_vertex_bytes=bytes;
        }
        /* Another draw may use different attributes from the same decoded
           stream. Keep its original values intact. Count is bounded above. */
        memcpy(gpu_bone_vertices,device.immediate_vertices,bytes);
        if(!mac_bone_gather_apply(&source_result.bone_gather,
            gpu_bone_vertices,device.immediate_count,XGPU_VERTEX_ATTRIBUTE_COUNT,
            device.constants,error,sizeof(error))) {
            fprintf(stderr,"native bone matrix gather failed: %s\n",error);
            failure="validated bone matrix stream preparation failed";goto done;
        }
        draw_vertices=gpu_bone_vertices;
        mac_render_timing_end(mac_render_timing_bone_gather,phase_stamp);
    }
    phase_stamp=mac_render_timing_begin(mac_render_timing_program_lookup);
    /* The source result stays pinned until this draw finishes. Its identity
       permits reuse without re-reading immutable cached GLSL on every draw. */
    if(!arb_vertex&&!mac_gl_program_cache_get_identified(&gpu_program_cache,legacy_vertex,legacy_pixel,
        input_mask,&source_result.identity,&program,error,sizeof(error))) {
        fprintf(stderr,"native D3D program rejected: %s\nvertex:\n%s\npixel:\n%s\n",
            error,legacy_vertex,legacy_pixel);
        failure="GLSL 1.20 compile/link failed";goto done;
    }
    mac_render_timing_end(mac_render_timing_program_lookup,phase_stamp);
    if(gpu_compact_vertex_upload&&
       !mac_gl_program_cache_active_attributes(&gpu_program_cache,program,
           &active_attribute_mask)) {
        failure="linked program has no cached effective vertex attribute mask";goto done;
    }
    phase_stamp=mac_render_timing_begin(mac_render_timing_draw_submit_restore);
    detail_stamp=mac_render_timing_begin(mac_render_timing_draw_state_setup);
    if(gpu_lean_draw) {
        /* (an evicted program's name may come back as another program) */
        if(gpu_program_cache.evictions!=gpu_uniform_evictions_seen)gpu_current_program=0;
        if(gpu_current_program!=program){glUseProgram(program);gpu_current_program=program;}
        /* (an ARB fragment program has no GLSL program: env parameters) */
        if(program) {
            uniform_block=mac_gpu_uniform_block(program);
            mac_gpu_cached_vec4(uniform_block->texture_scale,uniform_block->last_texture_scale,
                &texture_scale[0][0],4,&uniform_block->valid,512UL);
        }
    } else {
    glGetIntegerv(GL_CURRENT_PROGRAM,&old_program);
    glUseProgram(program);
    for(i=0;i<4;++i) {
        char name[8];GLint location;
        sprintf(name,"tex%lu",i);location=mac_gl_program_cache_uniform_location(&gpu_program_cache,program,name);
        if(location>=0)glUniform1i(location,(GLint)i);
    }
    mac_gpu_upload_vec4(program,"texture_scale",&texture_scale[0][0],4);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&old_array_buffer);
    glGetIntegerv(GL_BLEND_SRC,&old_blend_source);glGetIntegerv(GL_BLEND_DST,&old_blend_destination);
    glGetIntegerv(GL_BLEND_EQUATION,&old_blend_equation);glGetFloatv(GL_BLEND_COLOR,old_blend_color);
    glGetBooleanv(GL_COLOR_WRITEMASK,old_color);glGetBooleanv(GL_DEPTH_WRITEMASK,&old_depth_mask);
    glGetIntegerv(GL_DEPTH_FUNC,&old_depth_function);glGetIntegerv(GL_CULL_FACE_MODE,&old_cull_mode);
    glGetIntegerv(GL_FRONT_FACE,&old_front_face);
    glGetIntegerv(GL_STENCIL_FUNC,&old_stencil_func);glGetIntegerv(GL_STENCIL_REF,&old_stencil_ref);
    glGetIntegerv(GL_STENCIL_VALUE_MASK,&old_stencil_mask);glGetIntegerv(GL_STENCIL_WRITEMASK,&old_stencil_write);
    glGetIntegerv(GL_STENCIL_FAIL,&old_stencil_fail);glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL,&old_stencil_zfail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS,&old_stencil_pass);
    }
    if(!gpu_lean_draw)mac_gl_shadow_forget();
    mac_gl_shadow_array_buffer(0);
    {
        GLint x=device.viewport.X,y=mac_target_flipped()?(GLint)device.viewport.Y:(GLint)(target_height-(device.viewport.Y+device.viewport.Height));
        GLsizei w=(GLsizei)device.viewport.Width,h=(GLsizei)device.viewport.Height;
        if(mac_render_scaled())mac_render_scale_rect(&x,&y,&w,&h);
        mac_gl_shadow_viewport(x,y,w,h);
        mac_gl_shadow_depth_range(device.viewport.MinZ,device.viewport.MaxZ);
        mac_gl_shadow_scissor(x,y,w,h);
    }
    {
        /* Diagnostic only (HALO_MAC_DIAG_TINY_SCISSOR=1, or a list of vertex
           shader ids such as "9,27"): those draws cover one pixel, to
           measure how much of a frame is their fragment work. */
        static int parsed=0,all=0;static unsigned long ids[32];static unsigned count=0;
        unsigned k;int tiny=0;
        if(!parsed) {
            const char *list=getenv("HALO_MAC_DIAG_TINY_SCISSOR");
            parsed=1;
            if(list&&list[0]=='1'&&!list[1]){all=1;list=NULL;}
            while(list&&*list&&count<32) {
                char *end=NULL;unsigned long id=strtoul(list,&end,10);
                if(end==list)break;
                ids[count++]=id;list=*end==','?end+1:end;
            }
        }
        tiny=all;
        for(k=0;k<count&&!tiny;k++)if(vertex->id==ids[k])tiny=1;
        if(tiny)mac_gl_shadow_scissor(0,0,1,1);
    }
    mac_gl_shadow_cap(SHADOW_CAP_SCISSOR,GL_SCISSOR_TEST,1);
    mac_gl_shadow_color_mask((D3D__RenderState[D3DRS_COLORWRITEENABLE]&D3DCOLORWRITEENABLE_RED)!=0,
        (D3D__RenderState[D3DRS_COLORWRITEENABLE]&D3DCOLORWRITEENABLE_GREEN)!=0,
        (D3D__RenderState[D3DRS_COLORWRITEENABLE]&D3DCOLORWRITEENABLE_BLUE)!=0,
        (D3D__RenderState[D3DRS_COLORWRITEENABLE]&D3DCOLORWRITEENABLE_ALPHA)!=0);
    if(device.depth_stencil&&D3D__RenderState[D3DRS_ZENABLE]){
        mac_gl_shadow_cap(SHADOW_CAP_DEPTH,GL_DEPTH_TEST,1);
        mac_gl_shadow_depth_func(D3D__RenderState[D3DRS_ZFUNC]?(GLenum)D3D__RenderState[D3DRS_ZFUNC]:GL_LESS);
        mac_gl_shadow_depth_mask(D3D__RenderState[D3DRS_ZWRITEENABLE]?GL_TRUE:GL_FALSE);
    }else if(gpu_visibility_active){
        /* Radeon R300 queries need the depth unit running to count samples.
           Native targets retain private depth storage even when D3D has no
           logical depth surface. ALWAYS with writes off preserves that case. */
        mac_gl_shadow_cap(SHADOW_CAP_DEPTH,GL_DEPTH_TEST,1);
        mac_gl_shadow_depth_func(GL_ALWAYS);mac_gl_shadow_depth_mask(GL_FALSE);
    }else{mac_gl_shadow_cap(SHADOW_CAP_DEPTH,GL_DEPTH_TEST,0);mac_gl_shadow_depth_mask(GL_FALSE);}
    if(device.depth_stencil&&D3D__RenderState[D3DRS_STENCILENABLE]) {
        GLint bits=0;glGetIntegerv(GL_STENCIL_BITS,&bits);
        if(bits<8)mac_gpu_shader_failure("draw target lacks an eight-bit stencil attachment");
        mac_gl_shadow_cap(SHADOW_CAP_STENCIL,GL_STENCIL_TEST,1);
        /* Xbox stencil operations use GL enumerants, including zero. */
        mac_gl_shadow_stencil((GLenum)D3D__RenderState[D3DRS_STENCILFUNC],
            (GLint)D3D__RenderState[D3DRS_STENCILREF],D3D__RenderState[D3DRS_STENCILMASK],
            D3D__RenderState[D3DRS_STENCILWRITEMASK],(GLenum)D3D__RenderState[D3DRS_STENCILFAIL],
            (GLenum)D3D__RenderState[D3DRS_STENCILZFAIL],(GLenum)D3D__RenderState[D3DRS_STENCILPASS]);
    }else mac_gl_shadow_cap(SHADOW_CAP_STENCIL,GL_STENCIL_TEST,0);
    if(D3D__RenderState[D3DRS_CULLMODE]==D3DCULL_NONE)mac_gl_shadow_cap(SHADOW_CAP_CULL,GL_CULL_FACE,0);
    else if(D3D__RenderState[D3DRS_CULLMODE]==D3DCULL_CW||D3D__RenderState[D3DRS_CULLMODE]==D3DCULL_CCW){
        mac_gl_shadow_cap(SHADOW_CAP_CULL,GL_CULL_FACE,1);
        mac_gl_shadow_cull(GL_BACK,(D3D__RenderState[D3DRS_CULLMODE]==D3DCULL_CW)!=(mac_target_flipped()!=0)?GL_CCW:GL_CW);
    }else mac_gpu_shader_failure("cull mode is unsupported");
    if(D3D__RenderState[D3DRS_ALPHABLENDENABLE]) {
        float blend_color[4];
        color_to_vec4(D3D__RenderState[D3DRS_BLENDCOLOR],blend_color);
        mac_gl_shadow_cap(SHADOW_CAP_BLEND,GL_BLEND,1);
        mac_gl_shadow_blend((GLenum)D3D__RenderState[D3DRS_SRCBLEND],(GLenum)D3D__RenderState[D3DRS_DESTBLEND],
            mac_gpu_blend_equation(D3D__RenderState[D3DRS_BLENDOP]),blend_color);
    }else mac_gl_shadow_cap(SHADOW_CAP_BLEND,GL_BLEND,0);
    mac_gl_shadow_cap(SHADOW_CAP_DITHER,GL_DITHER,0);
    {
        /* Diagnostic only (HALO_MAC_DIAG_VERTEX_MODE="<vertex shader id>:<mode>",
           mode colormask, noblend, nodepth or tiny): change one property of
           that shader's draws, to find what makes them costly. */
        static int parsed=0;static long id=-1;static char mode[16];
        if(!parsed) {
            const char *v=getenv("HALO_MAC_DIAG_VERTEX_MODE");const char *colon=v?strchr(v,':'):NULL;
            parsed=1;
            if(v&&colon&&strlen(colon+1)<sizeof(mode)){id=atol(v);strcpy(mode,colon+1);}
        }
        if(id>=0&&(unsigned long)id==vertex->id) {
            if(!strcmp(mode,"colormask"))mac_gl_shadow_color_mask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
            else if(!strcmp(mode,"noblend"))mac_gl_shadow_cap(SHADOW_CAP_BLEND,GL_BLEND,0);
            else if(!strcmp(mode,"nodepth"))mac_gl_shadow_cap(SHADOW_CAP_DEPTH,GL_DEPTH_TEST,0);
            else if(!strcmp(mode,"tiny"))mac_gl_shadow_scissor(0,0,1,1);
            else if(!strcmp(mode,"nearest")||!strcmp(mode,"nomip")) {
                /* (changes the bound texture objects' filters for good) */
                unsigned stage;GLint filter=!strcmp(mode,"nearest")?GL_NEAREST:GL_LINEAR;
                for(stage=0;stage<4;stage++)if(key.sampler_type[stage]==1) {
                    glActiveTextureARB(GL_TEXTURE0_ARB+stage);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
                }
                glActiveTextureARB(GL_TEXTURE0_ARB);
            }
        }
    }
    mac_render_timing_end(mac_render_timing_draw_state_setup,detail_stamp);
    detail_stamp=mac_render_timing_begin(mac_render_timing_draw_uniforms);
    if(uniform_block)mac_gpu_upload_lean_uniforms(uniform_block);
    else if(program) {
    mac_gpu_upload_vec4(program,"c",&device.constants[0][0],XGPU_VERTEX_CONSTANT_COUNT);
    {float flipped_scale[4];mac_gpu_upload_vec4(program,"viewport_scale",mac_gpu_viewport_scale(flipped_scale),1);}
    mac_gpu_upload_vec4(program,"viewport_offset",device.viewport_offset,1);
    mac_gpu_upload_float(program,"point_size",1.0f);mac_gpu_upload_float(program,"screen_offset",(float)mac_engine_ui_offset());
    mac_gpu_upload_pixel_constants(program);
    }
    mac_render_timing_end(mac_render_timing_draw_uniforms,detail_stamp);
    detail_stamp=mac_render_timing_begin(mac_render_timing_draw_call);
    sub_stamp=mac_render_timing_begin(mac_render_timing_dc_attributes);
    if(gpu_world_draw.active) {
        const struct world_vbo_entry *world=gpu_world_draw.entry;
        unsigned long first=gpu_world_draw.base_vertex*world->stride;
        mac_gl_shadow_array_buffer(world->buffer);
        for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++){
            if(gpu_world_draw.gather_active&&i>=10&&i<16)continue;
            if((input_mask&(1UL<<i))&&(world->mask&(1UL<<i))) {
                mac_gl_shadow_attribute_array((GLuint)i,(GLint)world->sizes[i],(GLsizei)world->stride,
                    (const void *)(first+world->offsets[i]));
            } else if(input_mask&(1UL<<i)) {
                /* An input the declaration lacks takes the default the
                   decoder would have copied into every record. */
                mac_gl_shadow_attribute_value((GLuint)i,device.attributes[i]);
            } else mac_gl_shadow_attribute_off((GLuint)i);
        }
        if(gpu_world_draw.gather_active) {
            /* the gathered rows, indexed as the draw's indices: streamed into
               a span reserved from index 0, written from minimum to maximum */
            unsigned long row_bytes=24UL*sizeof(float);
            const char *rows_base=(const char *)gpu_gather_rows;
            mac_gl_shadow_array_buffer(0);
            for(i=10;i<16;i++) {
                if(input_mask&(1UL<<i))
                    mac_gl_shadow_attribute_array((GLuint)i,4,(GLsizei)row_bytes,rows_base+(i-10)*4*sizeof(float));
                else mac_gl_shadow_attribute_off((GLuint)i);
            }
        }
    } else if(gpu_compact_vertex_upload&&active_attribute_mask) {
        mac_gl_shadow_forget_attributes();
        if(!mac_vertex_upload_begin(&gpu_vertex_upload,draw_vertices,
            (size_t)device.immediate_count*XGPU_VERTEX_ATTRIBUTE_COUNT*4U*sizeof(float),
            device.immediate_count,active_attribute_mask,&upload_layout,
            upload_error,sizeof(upload_error))) {
            fprintf(stderr,"native compact vertex upload failed: %s\n",upload_error);
            failure="opt-in compact vertex upload failed";goto done;
        }
        compact_upload_active=1;
        for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++)
            if(active_attribute_mask&(1UL<<i)) {
                glEnableVertexAttribArray((GLuint)i);
                glVertexAttribPointer((GLuint)i,4,GL_FLOAT,GL_FALSE,
                    (GLsizei)upload_layout.stride_bytes,
                    (const void *)(unsigned long)upload_layout.offsets[i]);
            }
    } else {
        /* The compatibility source may omit unused generated inputs. Do not
           submit client arrays for attributes absent from that source. */
        unsigned char *var_chunk=NULL;unsigned var_stride=0,var_slot[XGPU_VERTEX_ATTRIBUTE_COUNT];
        mac_gl_shadow_array_buffer(0);
        if(gpu_var_enabled>0&&!getenv("HALO_MAC_DIAG_VAR_NOPACK"))var_chunk=mac_var_pack(draw_vertices,device.immediate_count,input_mask,&var_stride,var_slot);
        for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++){
            if(input_mask&(1UL<<i)) {
                if(var_chunk)mac_gl_shadow_attribute_array((GLuint)i,4,(GLsizei)var_stride,var_chunk+var_slot[i]*16U);
                else mac_gl_shadow_attribute_array((GLuint)i,4,
                    (GLsizei)(XGPU_VERTEX_ATTRIBUTE_COUNT*4*sizeof(float)),draw_vertices+i*4);
            } else mac_gl_shadow_attribute_off((GLuint)i);
        }
    }
    mac_render_timing_end(mac_render_timing_dc_attributes,sub_stamp);
    sub_stamp=mac_render_timing_begin(mac_render_timing_dc_vertex_program);
    if(arb_vertex) {
        float screen_point[4]={0.0f,1.0f,0.0f,0.0f};
        screen_point[0]=(float)mac_engine_ui_offset(); /* port: the menus' centring (halo_screen_ui_offset) */
        mac_gl_shadow_cap(SHADOW_CAP_VERTEX_PROGRAM,GL_VERTEX_PROGRAM_ARB,1);
        mac_gl_shadow_cap(SHADOW_CAP_POINT_SIZE,GL_VERTEX_PROGRAM_POINT_SIZE_ARB,1);
        {
            /* Diagnostic only (HALO_MAC_DIAG_NO_PROGRAM_SWITCH=1, with
               HALO_MAC_DIAG_TINY_SCISSOR=1): after the second frame the
               programs bound stay bound (the picture is wrong), to measure
               what switching programs costs the driver. */
            static int fixed=-1;
            if(fixed<0){const char *v=getenv("HALO_MAC_DIAG_NO_PROGRAM_SWITCH");fixed=v&&v[0]=='1';}
            if(fixed&&device.gpu_present_count>1&&(gl_shadow.valid&SHADOW_ARB_PROGRAM)&&
               (gl_shadow.valid&SHADOW_ARB_FRAGMENT)) {
                gpu_diag_fixed_programs=1;
            }
        }
        if(gpu_diag_fixed_programs);
        else if(!(gl_shadow.valid&SHADOW_ARB_PROGRAM)||gl_shadow.arb_program!=arb_vertex->program) {
            glBindProgramARB(GL_VERTEX_PROGRAM_ARB,arb_vertex->program);
            gl_shadow.arb_program=arb_vertex->program;gl_shadow.valid|=SHADOW_ARB_PROGRAM;
            ++gpu_vertex_program_binds;
        }
        /* Environment parameters are shared by every ARB program and set by
           nothing else: send only those that differ from what was sent. */
        {
            /* Diagnostic only (HALO_MAC_DIAG_FROZEN_ENV=1): after the second
               frame no vertex constants are sent (the picture is wrong), to
               measure what the driver's per-draw constant handling costs. */
            static int frozen=-1;
            if(frozen<0){const char *v=getenv("HALO_MAC_DIAG_FROZEN_ENV");frozen=v&&v[0]=='1';}
            if(frozen&&device.gpu_present_count>1)gpu_arb_env_constants_serial=constants_serial;
        }
        if(!gpu_arb_env_constants_serial||!gpu_arb_env_valid[0]) {
            for(i=0;i<XGPU_VERTEX_CONSTANT_COUNT;i++)
                mac_arb_env_set((GLuint)i,device.constants[i]);
        } else if(gpu_arb_env_constants_serial!=constants_serial) {
            /* only those written since: the game rewrites many unchanged */
            if(gpu_env_log&&constants_serial-gpu_arb_env_constants_serial<=XGPU_VERTEX_CONSTANT_COUNT) {
                /* (the log holds the register each serial wrote; one written twice is sent
                   twice, the second a repeat) */
                unsigned long long serial;
                for(serial=gpu_arb_env_constants_serial+1;serial<=constants_serial;serial++) {
                    i=constant_log[serial%CONSTANT_LOG_SIZE];
                    mac_arb_env_set((GLuint)i,device.constants[i]);
                }
            } else
            for(i=0;i<XGPU_VERTEX_CONSTANT_COUNT;i++)
                if(constant_serials[i]>gpu_arb_env_constants_serial)
                    mac_arb_env_set((GLuint)i,device.constants[i]);
        }
        {float flipped_scale[4];mac_arb_env_set(MAC_NV2A_ARB_ENV_VIEWPORT_SCALE,mac_gpu_viewport_scale(flipped_scale));}
        mac_arb_env_set(MAC_NV2A_ARB_ENV_VIEWPORT_OFFSET,device.viewport_offset);
        mac_arb_env_set(MAC_NV2A_ARB_ENV_SCREEN_POINT,screen_point);
        gpu_arb_env_constants_serial=constants_serial;
        ++gpu_arb_draws;
    } else {
        /* (a GLSL draw: no ARB vertex program, as without lean drawing) */
        mac_gl_shadow_cap(SHADOW_CAP_VERTEX_PROGRAM,GL_VERTEX_PROGRAM_ARB,0);
        mac_gl_shadow_cap(SHADOW_CAP_POINT_SIZE,GL_VERTEX_PROGRAM_POINT_SIZE_ARB,0);
    }
    mac_render_timing_end(mac_render_timing_dc_vertex_program,sub_stamp);
    sub_stamp=mac_render_timing_begin(mac_render_timing_dc_fragment_program);
    if(ff) {
        mac_gl_shadow_cap(SHADOW_CAP_FRAGMENT_PROGRAM,GL_FRAGMENT_PROGRAM_ARB,0);
        mac_ff_apply(ff,key.sampler_type);
        ++gpu_ff_draws;((struct arbfp_program *)arbfp)->draws++;
    } else if(arbfp) {
        /* the constants it reads, as the GLSL uniforms would have them */
        float value[4];unsigned long stage;
        mac_gl_shadow_cap(SHADOW_CAP_FRAGMENT_PROGRAM,GL_FRAGMENT_PROGRAM_ARB,1);
        if(gpu_diag_fixed_programs);
        else if(!(gl_shadow.valid&SHADOW_ARB_FRAGMENT)||gl_shadow.arb_fragment!=arbfp->program) {
            glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,arbfp->program);
            gl_shadow.arb_fragment=arbfp->program;gl_shadow.valid|=SHADOW_ARB_FRAGMENT;
            ++gpu_fragment_program_binds;
        }
        for(stage=0;stage<8;stage++) {
            if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_C0+stage]) {
                color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0+stage],value);
                mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_C0+stage,value);
            }
            if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_C1+stage]) {
                color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0+stage],value);
                mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_C1+stage,value);
            }
        }
        if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_FINAL_C0]) {
            color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0],value);
            mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_FINAL_C0,value);
        }
        if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_FINAL_C1]) {
            color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1],value);
            mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_FINAL_C1,value);
        }
        if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_FOG_COLOR]) {
            color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR],value);
            mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_FOG_COLOR,value);
        }
        if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS]) {
            value[0]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGSTART]);
            value[1]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGEND]);
            value[2]=mac_gpu_state_float(D3D__RenderState[D3DRS_FOGDENSITY]);value[3]=0.0f;
            mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_FOG_PARAMETERS,value);
        }
        if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE]) {
            value[0]=(float)(D3D__RenderState[D3DRS_ALPHAREF]&255);value[1]=value[2]=value[3]=0.0f;
            mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE,value);
        }
        for(stage=0;stage<4;stage++) {
            if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_TEXTURE_SCALE+stage])
                mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_TEXTURE_SCALE+stage,texture_scale[stage]);
            /* (the GLSL path never sets the bump uniforms: zero there too) */
            value[0]=value[1]=value[2]=value[3]=0.0f;
            if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_BUMP_MATRIX+stage])
                mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_BUMP_MATRIX+stage,value);
            if(arbfp->info.env_used[MAC_NV2A_ARBFP_ENV_BUMP_LUMINANCE+stage])
                mac_arbfp_env_set(MAC_NV2A_ARBFP_ENV_BUMP_LUMINANCE+stage,value);
        }
        ++gpu_arbfp_draws;((struct arbfp_program *)arbfp)->draws++;
        {
            /* Diagnostic only (HALO_MAC_DIAG_ALPHAREF0=<vertex shader id>):
               that shader's ARB alpha test with a reference of 0. */
            static long id=-2;
            if(id==-2){const char *v=getenv("HALO_MAC_DIAG_ALPHAREF0");id=v?atol(v):-1;}
            if(id>=0&&(unsigned long)id==vertex->id) {
                float zero[4]={0.0f,0.0f,0.0f,0.0f};
                glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB,MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE,zero);
                gpu_arbfp_env_valid[MAC_NV2A_ARBFP_ENV_ALPHA_REFERENCE]=0;
            }
        }
    } else mac_gl_shadow_cap(SHADOW_CAP_FRAGMENT_PROGRAM,GL_FRAGMENT_PROGRAM_ARB,0);
    if(!ff)mac_ff_alpha_off();
    mac_render_timing_end(mac_render_timing_dc_fragment_program,sub_stamp);
    {
        /* Diagnostic only (HALO_MAC_DIAG_DRAW_KEYS=<frame>): that frame's
           draws, one line each: vertex shader, programs, textures. */
        static long frame=-2;
        if(frame==-2){const char *v=getenv("HALO_MAC_DIAG_DRAW_KEYS");frame=v&&*v?atol(v):-1;}
        if(frame>=0&&(long)device.gpu_present_count==frame)
            fprintf(stderr,"drawkey v=%lu va=%u fp=%u t=%p,%p,%p,%p w=%d ff=%d\n",(unsigned long)vertex->id,
                arb_vertex?(unsigned)arb_vertex->program:0U,arbfp?(unsigned)arbfp->program:0U,
                stage_textures[0],stage_textures[1],stage_textures[2],stage_textures[3],gpu_world_draw.active?1:0,ff!=NULL);
    }
    {
        /* Diagnostic only (HALO_MAC_DIAG_DRAW_SAMPLES=<vertex shader id, or
           "all">): the samples each such draw of the sixth frame passes. */
        static int parsed=0;static long id=-2;
        if(!parsed){const char *v=getenv("HALO_MAC_DIAG_DRAW_SAMPLES");parsed=1;
            if(v&&*v)id=strcmp(v,"all")?atol(v):-1;}
        if(id!=-2&&(id<0||(unsigned long)id==vertex->id)&&device.gpu_present_count==5&&!gpu_visibility_active) {
            glGenQueriesARB(1,&diag_samples_query);
            if(diag_samples_query)glBeginQueryARB(GL_SAMPLES_PASSED_ARB,diag_samples_query);
            /* (HALO_MAC_DIAG_DRAW_SAMPLES_AREA=1: no depth test, so the
               count is the area the draw rasterizes) */
            if(getenv("HALO_MAC_DIAG_DRAW_SAMPLES_AREA"))mac_gl_shadow_cap(SHADOW_CAP_DEPTH,GL_DEPTH_TEST,0);
        }
    }
    if(gpu_diag_draw_times<0){const char *v=getenv("HALO_MAC_DIAG_DRAW_TIMES");gpu_diag_draw_times=v&&v[0]=='1';}
    if(gpu_diag_draw_times&&device.gpu_present_count==5)diag_start=mach_absolute_time();
    sub_stamp=mac_render_timing_begin(mac_render_timing_dc_gl_draw);
    if(gpu_world_draw.active) {
        GLint old_elements=0;
        long span;
        if(!gpu_lean_draw)glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&old_elements);
        span=mac_index_cache_span(gpu_world_draw.indices,(unsigned long)device.immediate_count);
        if(span>=0) {
            glDrawRangeElements(primitive,(GLuint)gpu_world_draw.minimum,(GLuint)gpu_world_draw.maximum,
                (GLsizei)device.immediate_count,GL_UNSIGNED_SHORT,(const char *)0+span);
        } else {
            static int twice=-1;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
            glDrawRangeElements(primitive,(GLuint)gpu_world_draw.minimum,(GLuint)gpu_world_draw.maximum,
                (GLsizei)device.immediate_count,GL_UNSIGNED_SHORT,gpu_world_draw.indices);
            /* Diagnostic only (HALO_MAC_DIAG_DRAW_TWICE=1, with
               HALO_MAC_DIAG_TINY_SCISSOR=1): each world draw is made again
               with nothing changed, to measure a draw's own cost. */
            if(twice<0){const char *v=getenv("HALO_MAC_DIAG_DRAW_TWICE");twice=v&&v[0]=='1';}
            if(twice)glDrawRangeElements(primitive,(GLuint)gpu_world_draw.minimum,(GLuint)gpu_world_draw.maximum,
                (GLsizei)device.immediate_count,GL_UNSIGNED_SHORT,gpu_world_draw.indices);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,(GLuint)old_elements);
    } else if(device.immediate_type==D3DPT_QUADLIST) {
        unsigned long quad,index_count=(device.immediate_count/4)*6;
        WORD *indices=(WORD *)malloc(index_count*sizeof(WORD));
        GLint old_elements=0;
        if(!indices)mac_gpu_shader_failure("quad index allocation failed");
        /* Match the shared renderer's two-triangle diagonal and winding. */
        for(quad=0;quad<device.immediate_count/4;++quad) {
            WORD first=(WORD)(quad*4);unsigned long out=quad*6;
            indices[out]=first;indices[out+1]=(WORD)(first+1);indices[out+2]=(WORD)(first+2);
            indices[out+3]=first;indices[out+4]=(WORD)(first+2);indices[out+5]=(WORD)(first+3);
        }
        if(device.gpu_stream_draw_active&&gpu_stream_remap_count==device.immediate_count) {
            for(quad=0;quad<index_count;++quad)indices[quad]=gpu_stream_remap[indices[quad]];
            gpu_remapped_draws++;
        }
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&old_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
        glDrawElements(GL_TRIANGLES,(GLsizei)index_count,GL_UNSIGNED_SHORT,indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,(GLuint)old_elements);
        free(indices);
    } else if(device.gpu_stream_draw_active&&gpu_stream_remap_count==device.immediate_count) {
        GLint old_elements=0;
        /* Client indices must not be interpreted as offsets into a caller's
           element buffer. Preserve the external binding just as for quads. */
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&old_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
        glDrawElements(primitive,(GLsizei)device.immediate_count,GL_UNSIGNED_SHORT,gpu_stream_remap);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,(GLuint)old_elements);
        gpu_remapped_draws++;
    } else glDrawArrays(primitive,0,(GLsizei)device.immediate_count);
    mac_render_timing_end(mac_render_timing_dc_gl_draw,sub_stamp);
    mac_render_timing_draw_end(vertex->id,detail_stamp);
    if(diag_start)mac_diag_draw_time(vertex->id,(unsigned long)device.immediate_count,diag_start,
        diag_vertex_binds,diag_fragment_binds,diag_env_writes);
    if(diag_samples_query) {
        GLuint samples=0;GLint vertex_gpu=-1,fragment_gpu=-1;
        glEndQueryARB(GL_SAMPLES_PASSED_ARB);
        glGetQueryObjectuivARB(diag_samples_query,GL_QUERY_RESULT_ARB,&samples);
        if(CGLGetCurrentContext()) {
            CGLGetParameter(CGLGetCurrentContext(),kCGLCPGPUVertexProcessing,&vertex_gpu);
            CGLGetParameter(CGLGetCurrentContext(),kCGLCPGPUFragmentProcessing,&fragment_gpu);
        }
        fprintf(stderr,"HALO_MAC_DRAW_SAMPLES vertex=%lu type=%lu count=%lu samples=%lu blend=%lu/%lu ztest=%lu vertex_gpu=%d fragment_gpu=%d arbfp=%d zbias=%lu zfunc=%lu fp=%u alu=%u tex=%u kil=%u combiners=%lu modes=0x%lx samplers=%u%u%u%u filters=%lu/%lu/%lu\n",
            vertex->id,(unsigned long)device.immediate_type,device.immediate_count,(unsigned long)samples,
            (unsigned long)D3D__RenderState[D3DRS_SRCBLEND],(unsigned long)D3D__RenderState[D3DRS_DESTBLEND],
            (unsigned long)D3D__RenderState[D3DRS_ZENABLE],(int)vertex_gpu,(int)fragment_gpu,arbfp!=NULL,
            (unsigned long)D3D__RenderState[D3DRS_ZBIAS],(unsigned long)D3D__RenderState[D3DRS_ZFUNC],
            arbfp?(unsigned)arbfp->program:0u,arbfp?arbfp->info.alu_instructions:0u,arbfp?arbfp->info.tex_instructions:0u,
            arbfp?(unsigned)arbfp->info.uses_kil:0u,(unsigned long)(D3D__RenderState[D3DRS_PSCOMBINERCOUNT]&0xff),
            (unsigned long)D3D__RenderState[D3DRS_PSTEXTUREMODES],
            (unsigned)key.sampler_type[0],(unsigned)key.sampler_type[1],(unsigned)key.sampler_type[2],(unsigned)key.sampler_type[3],
            (unsigned long)D3D__TextureState[0][D3DTSS_MINFILTER],(unsigned long)D3D__TextureState[0][D3DTSS_MAGFILTER],
            (unsigned long)D3D__TextureState[0][D3DTSS_MIPFILTER]);
        glDeleteQueriesARB(1,&diag_samples_query);diag_samples_query=0;
    }
    mac_gl_program_cache_diagnostics_draw(&gpu_program_cache,vertex->id,program,
        device.immediate_count,input_mask);
    if(arbfp&&!gpu_lean_draw){glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,0);glDisable(GL_FRAGMENT_PROGRAM_ARB);}
    if(arb_vertex&&!gpu_lean_draw) {
        glBindProgramARB(GL_VERTEX_PROGRAM_ARB,0);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);glDisable(GL_VERTEX_PROGRAM_ARB);
        gl_shadow.valid&=~(unsigned long)SHADOW_ARB_PROGRAM;
        gl_shadow.caps[SHADOW_CAP_VERTEX_PROGRAM]=gl_shadow.caps[SHADOW_CAP_POINT_SIZE]=1;
    }
    detail_stamp=mac_render_timing_begin(mac_render_timing_draw_restore);
    /* (lean: the next draw enables or disables every attribute itself) */
    if(!gpu_lean_draw||compact_upload_active) {
        for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++)glDisableVertexAttribArray((GLuint)i);
        mac_gl_shadow_forget_attributes();
    }
    if(compact_upload_active) {
        glBindBuffer(GL_ARRAY_BUFFER,0);
        for(i=0;i<XGPU_VERTEX_ATTRIBUTE_COUNT;i++)
            if(active_attribute_mask&(1UL<<i))
                glVertexAttribPointer((GLuint)i,4,GL_FLOAT,GL_FALSE,
                    (GLsizei)(XGPU_VERTEX_ATTRIBUTE_COUNT*4*sizeof(float)),
                    device.immediate_vertices+i*4);
        if(!mac_vertex_upload_end(&gpu_vertex_upload)) {
            fprintf(stderr,"native compact vertex upload state restore failed\n");
            failure="compact vertex upload state restore failed";
        }
        compact_upload_active=0;
    }
    if(!device.gpu_stream_draw_active)device.gpu_immediate_draw_count++;
    if(gpu_lean_draw)mac_gl_shadow_array_buffer(0);
    else {
    glUseProgram(old_program>0&&glIsProgram((GLuint)old_program)?(GLuint)old_program:0);
    glBindBuffer(GL_ARRAY_BUFFER,(GLuint)old_array_buffer);
    glColorMask(old_color[0],old_color[1],old_color[2],old_color[3]);glDepthMask(old_depth_mask);
    glDepthFunc((GLenum)old_depth_function);glCullFace((GLenum)old_cull_mode);glFrontFace((GLenum)old_front_face);
    glStencilFunc((GLenum)old_stencil_func,old_stencil_ref,(GLuint)old_stencil_mask);
    glStencilMask((GLuint)old_stencil_write);
    glStencilOp((GLenum)old_stencil_fail,(GLenum)old_stencil_zfail,(GLenum)old_stencil_pass);
    if(old_stencil)glEnable(GL_STENCIL_TEST);else glDisable(GL_STENCIL_TEST);
    if(old_depth)glEnable(GL_DEPTH_TEST);else glDisable(GL_DEPTH_TEST);
    glBlendFunc((GLenum)old_blend_source,(GLenum)old_blend_destination);
    glBlendEquation((GLenum)old_blend_equation);
    glBlendColor(old_blend_color[0],old_blend_color[1],old_blend_color[2],old_blend_color[3]);
    if(old_blend)glEnable(GL_BLEND);else glDisable(GL_BLEND);
    if(old_cull)glEnable(GL_CULL_FACE);else glDisable(GL_CULL_FACE);
    if(old_scissor)glEnable(GL_SCISSOR_TEST);else glDisable(GL_SCISSOR_TEST);
    if(old_dither)glEnable(GL_DITHER);else glDisable(GL_DITHER);
    }
    /* (each draw's errors are read where draws are checked: a glGetError
       may wait for the GPU, Apple's OpenGL guide says, so play skips it) */
    if(mac_gpu_draws_strict()) {GLenum error_code=glGetError();if(error_code!=GL_NO_ERROR){fprintf(stderr,"native D3D draw OpenGL error 0x%lx\n",(unsigned long)error_code);failure="OpenGL error during bounded immediate draw";}}
    mac_render_timing_end(mac_render_timing_draw_restore,detail_stamp);
    mac_render_timing_end(mac_render_timing_draw_submit_restore,phase_stamp);
done:
    if(texture_state_saved)mac_gpu_textures_end(&texture_scope);
    mac_shader_source_result_dispose(&source_result);
    if(failure)mac_gpu_shader_failure(failure);
}
void WINAPI D3DDevice_End(void)
{
    device.immediate_active=FALSE;
    if(device.gpu){
        if(device.immediate_count) {
            jmp_buf escape;jmp_buf *previous=gpu_draw_escape;
            GPU_GUARDED(escape,mac_gpu_draw_immediate());
            gpu_draw_escape=previous;device.gpu_stream_draw_active=0;
        }
        return;
    }
    require_headless();if(device.immediate_count)device.skipped_draws++;
}
static unsigned mac_float_components(unsigned type)
{
    switch(type){case D3DVSDT_FLOAT1:return 1;case D3DVSDT_FLOAT2:return 2;
    case D3DVSDT_FLOAT3:return 3;case D3DVSDT_FLOAT4:return 4;default:return 0;}
}
/* The components the decoder takes from a vertex element; it writes the
   others as GL's defaults (0, 0, 1 for y, z, w), so a buffer need hold only
   these. */
static unsigned mac_decoded_components(unsigned type)
{
    switch(type) {
    case D3DVSDT_FLOAT1:case D3DVSDT_SHORT1:case D3DVSDT_NORMSHORT1:return 1;
    case D3DVSDT_FLOAT2:case D3DVSDT_SHORT2:case D3DVSDT_NORMSHORT2:case D3DVSDT_PBYTE2:return 2;
    case D3DVSDT_FLOAT3:case D3DVSDT_NORMPACKED3:case D3DVSDT_PBYTE3:return 3;
    default:return 4;
    }
}
static unsigned mac_indexed_type_bytes(unsigned type)
{
    switch(type) {
    case D3DVSDT_FLOAT1:return 4;case D3DVSDT_FLOAT2:return 8;
    case D3DVSDT_FLOAT3:return 12;case D3DVSDT_FLOAT4:return 16;
    case D3DVSDT_D3DCOLOR:return 4;
    case D3DVSDT_NORMSHORT1:return 2;case D3DVSDT_NORMSHORT2:return 4;
    case D3DVSDT_NORMPACKED3:return 4;case D3DVSDT_SHORT1:return 2;
    case D3DVSDT_SHORT2:return 4;case D3DVSDT_PBYTE2:return 2;case D3DVSDT_PBYTE3:return 3;
    default:return 0;
    }
}
void mac_device_unpack_normpacked3(uint32_t packed,float out[3])
{
    int32_t i=(int32_t)(packed<<21),j=(int32_t)((packed>>11)<<21),k=(int32_t)((packed>>22)<<22);
    out[0]=((float)i*(1.0f/1048576.0f)+1.0f)*(1.0f/2047.0f);
    out[1]=((float)j*(1.0f/1048576.0f)+1.0f)*(1.0f/2047.0f);
    out[2]=((float)k*(1.0f/2097152.0f)+1.0f)*(1.0f/1023.0f);
}
static size_t mac_bsp_cache_declaration(const struct vertex_shader_object *vertex,
    unsigned char bytes[BSP_DECODE_MAX_DECLARATION_BYTES],unsigned long *mask)
{
    size_t used=0;unsigned long result=0;unsigned i;
    if(!vertex||!bytes||!mask||!vertex->element_count||vertex->element_count>16)return 0;
    /* A stable byte stream, not sizeof(vertex_element): no ABI padding. */
    bytes[used++]=0x42;bytes[used++]=1;bytes[used++]=(unsigned char)vertex->element_count;
    for(i=0;i<vertex->element_count;i++) {
        const struct vertex_element *element=&vertex->elements[i];
        if(element->reg>=XGPU_VERTEX_ATTRIBUTE_COUNT||element->stream>=16||
           (result&(1UL<<element->reg))||used>BSP_DECODE_MAX_DECLARATION_BYTES-6)return 0;
        bytes[used++]=element->reg;
        bytes[used++]=element->stream;
        bytes[used++]=element->type;
        bytes[used++]=element->bytes;
        bytes[used++]=(unsigned char)(element->offset&0xffU);
        bytes[used++]=(unsigned char)((element->offset>>8)&0xffU);
        result|=1UL<<element->reg;
    }
    *mask=result;return used;
}
static int mac_bsp_cache_begin_draw(const struct vertex_shader_object *vertex,
    const unsigned char used_streams[16],const unsigned char *const streams[16],
    const unsigned long strides[16],unsigned long vertex_count,
    struct bsp_decode_cache_view *view)
{
    unsigned char declaration[BSP_DECODE_MAX_DECLARATION_BYTES];
    struct bsp_decode_stream_key keys[BSP_DECODE_MAX_STREAMS];
    unsigned long mask=0;size_t declaration_bytes=0,key_count=0;unsigned i;
    if(view){view->cache=NULL;view->entry=NULL;}
    if(!gpu_bsp_decode_cache_enabled||!view)return BSP_DECODE_CACHE_BYPASS;
    declaration_bytes=mac_bsp_cache_declaration(vertex,declaration,&mask);
    if(!declaration_bytes)return BSP_DECODE_CACHE_BYPASS;
    memset(keys,0,sizeof(keys));
    for(i=0;i<16;i++)if(used_streams[i]) {
        unsigned long long owner=0;unsigned long generation=0,offset=0,remaining=0;
        unsigned long needed,capacity;
        if(!strides[i]||vertex_count>0xffffffffUL/strides[i])return BSP_DECODE_CACHE_BYPASS;
        needed=vertex_count*strides[i];
        if(!mac_resource_certificate_query(streams[i],needed,&owner,&generation,&offset,&remaining) ||
           !owner||!generation||!remaining||remaining%strides[i])return BSP_DECODE_CACHE_BYPASS;
        capacity=remaining/strides[i];
        if(!capacity||capacity>BSP_DECODE_MAX_VERTICES)return BSP_DECODE_CACHE_BYPASS;
        keys[key_count].slot=(unsigned char)i;
        keys[key_count].owner_token=owner;
        keys[key_count].write_generation=generation;
        keys[key_count].base_offset=offset;
        keys[key_count].stride=strides[i];
        keys[key_count].vertex_capacity=capacity;
        ++key_count;
    }
    if(!key_count)return BSP_DECODE_CACHE_BYPASS;
    return bsp_decode_cache_begin_draw(&gpu_bsp_decode_cache,declaration,
        declaration_bytes,mask,keys,key_count,view);
}
void WINAPI D3DDevice_SetIndices(D3DIndexBuffer *indices,UINT base_vertex_index)
{
    device.base_vertex_index=base_vertex_index;
    device.index_data=indices?indices->Data:0;
    D3D__IndexData=(WORD *)device.index_data;
}
/* One vertex's declared elements, decoded into its 16-attribute record (the
   caller fills the defaults first). 0: a float element is not finite. */
static int mac_device_decode_vertex_elements(const struct vertex_shader_object *vertex,
    const unsigned char *const streams[16],const unsigned long strides[16],
    unsigned long source_index,float *destination)
{
    unsigned long j,k;const unsigned char *source;
        for(j=0;j<vertex->element_count;j++){
            const struct vertex_element *element=&vertex->elements[j];unsigned components=mac_float_components(element->type);
            source=streams[element->stream]+source_index*strides[element->stream];
            if(element->type==D3DVSDT_NORMPACKED3){uint32_t packed;float decoded[3];
                memcpy(&packed,source+element->offset,sizeof(packed));mac_device_unpack_normpacked3(packed,decoded);
                for(k=0;k<3;k++)destination[element->reg*4+k]=decoded[k];
                destination[element->reg*4+3]=1.0f;continue;
            }
            if(element->type==D3DVSDT_SHORT2) {
                short value[2];memcpy(value,source+element->offset,sizeof(value));
                destination[element->reg*4]=(float)value[0];
                destination[element->reg*4+1]=(float)value[1];
                destination[element->reg*4+2]=0.0f;destination[element->reg*4+3]=1.0f;continue;
            }
            if(element->type==D3DVSDT_NORMSHORT2) {
                short value[2];float x,y;memcpy(value,source+element->offset,sizeof(value));
                x=(float)value[0]*(1.0f/32767.0f);y=(float)value[1]*(1.0f/32767.0f);
                destination[element->reg*4]=x<-1.0f?-1.0f:x;
                destination[element->reg*4+1]=y<-1.0f?-1.0f:y;
                destination[element->reg*4+2]=0.0f;destination[element->reg*4+3]=1.0f;continue;
            }
            if(element->type==D3DVSDT_PBYTE2) {
                destination[element->reg*4]=source[element->offset]*(1.0f/255.0f);
                destination[element->reg*4+1]=source[element->offset+1]*(1.0f/255.0f);
                destination[element->reg*4+2]=0.0f;destination[element->reg*4+3]=1.0f;continue;
            }
            if(element->type==D3DVSDT_SHORT1) {
                short value;memcpy(&value,source+element->offset,sizeof(value));
                destination[element->reg*4]=(float)value;
                destination[element->reg*4+1]=0.0f;destination[element->reg*4+2]=0.0f;
                destination[element->reg*4+3]=1.0f;continue;
            }
            if(element->type==D3DVSDT_NORMSHORT1) {
                short value;float x;memcpy(&value,source+element->offset,sizeof(value));
                x=(float)value*(1.0f/32767.0f);destination[element->reg*4]=x<-1.0f?-1.0f:x;
                destination[element->reg*4+1]=0.0f;destination[element->reg*4+2]=0.0f;
                destination[element->reg*4+3]=1.0f;continue;
            }
            if(element->type==D3DVSDT_D3DCOLOR) {
                uint32_t color;memcpy(&color,source+element->offset,sizeof(color));
                destination[element->reg*4]=((color>>16)&255)*(1.0f/255.0f);
                destination[element->reg*4+1]=((color>>8)&255)*(1.0f/255.0f);
                destination[element->reg*4+2]=(color&255)*(1.0f/255.0f);
                destination[element->reg*4+3]=((color>>24)&255)*(1.0f/255.0f);continue;
            }
            if(element->type==D3DVSDT_PBYTE3) {
                destination[element->reg*4]=source[element->offset]*(1.0f/255.0f);
                destination[element->reg*4+1]=source[element->offset+1]*(1.0f/255.0f);
                destination[element->reg*4+2]=source[element->offset+2]*(1.0f/255.0f);
                destination[element->reg*4+3]=1.0f;continue;
            }
            for(k=0;k<components;k++){
                float value;memcpy(&value,source+element->offset+k*sizeof(float),sizeof(value));
                if(value!=value||value>3.402823466e+38f||value< -3.402823466e+38f)
                    return 0;
                destination[element->reg*4+k]=value;
            }
            for(k=components;k<4;k++)destination[element->reg*4+k]=k==3?1.0f:0.0f;
        }
    return 1;
}
static void mac_world_vbo_release(struct world_vbo_entry *entry)
{
    if(entry->buffer)glDeleteBuffers(1,&entry->buffer);
    if(entry->live&&!entry->rejected)gpu_world_vbo_bytes-=entry->bytes;
    free(entry->addresses);
    memset(entry,0,sizeof(*entry));
}
/* The offset in the index arena (bound to GL_ELEMENT_ARRAY_BUFFER) of a
   span equal to these indices, or -1 to draw from client memory. */
static long mac_index_cache_span(const WORD *indices,unsigned long count)
{
    unsigned long bytes=count*sizeof(WORD),slot;
    struct index_cache_entry *entry;
    if(!gpu_index_cache_enabled||!count||bytes>INDEX_CACHE_BYTES/8){++gpu_index_bypasses;return -1;}
    if(!gpu_index_arena) {
        gpu_index_shadow=(WORD *)malloc(INDEX_CACHE_BYTES);
        if(!gpu_index_shadow){gpu_index_cache_enabled=0;return -1;}
        glGenBuffers(1,&gpu_index_arena);
        if(!gpu_index_arena){free(gpu_index_shadow);gpu_index_shadow=NULL;gpu_index_cache_enabled=0;return -1;}
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu_index_arena);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)INDEX_CACHE_BYTES,NULL,GL_STATIC_DRAW);
        {
            /* New spans never overlap ones pending draws read. */
            const char *extensions=(const char *)glGetString(GL_EXTENSIONS);
            if(extensions&&strstr(extensions,"GL_APPLE_flush_buffer_range")) {
                /* port: looked up at run time; Mac OS X 10.4 has no such symbol, so it cannot be bound at load */
                typedef void (*buffer_parameteri_fn)(GLenum,GLenum,GLint);
                buffer_parameteri_fn buffer_parameteri=(buffer_parameteri_fn)dlsym(RTLD_DEFAULT,"glBufferParameteriAPPLE");
                if(buffer_parameteri)
                    buffer_parameteri(GL_ELEMENT_ARRAY_BUFFER,GL_BUFFER_SERIALIZED_MODIFY_APPLE,GL_FALSE);
            }
        }
        gpu_index_arena_used=0;
    } else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu_index_arena);
    slot=(((unsigned long)indices>>1)^(count*2654435761UL))%INDEX_CACHE_SLOTS;
    entry=&gpu_index_entries[slot];
    if(entry->source==indices&&entry->count==count&&
       !memcmp((const char *)gpu_index_shadow+entry->offset,indices,bytes)) {
        ++gpu_index_hits;return (long)entry->offset;
    }
    if(gpu_index_arena_used+bytes>INDEX_CACHE_BYTES) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)INDEX_CACHE_BYTES,NULL,GL_STATIC_DRAW);
        memset(gpu_index_entries,0,sizeof(gpu_index_entries));
        gpu_index_arena_used=0;++gpu_index_resets;
    }
    entry->source=indices;entry->count=count;entry->offset=gpu_index_arena_used;
    memcpy((char *)gpu_index_shadow+entry->offset,indices,bytes);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,(GLintptr)entry->offset,(GLsizeiptr)bytes,indices);
    gpu_index_arena_used=(gpu_index_arena_used+bytes+15UL)&~15UL;
    ++gpu_index_uploads;
    return (long)entry->offset;
}
static void mac_index_cache_release(void)
{
    if(gpu_index_arena)glDeleteBuffers(1,&gpu_index_arena);
    free(gpu_index_shadow);
    gpu_index_arena=0;gpu_index_shadow=NULL;gpu_index_arena_used=0;
    memset(gpu_index_entries,0,sizeof(gpu_index_entries));
}
static void mac_world_vbo_clear(void)
{
    unsigned long i;
    for(i=0;i<WORLD_VBO_MAX_ENTRIES;i++)if(gpu_world_vbos[i].live)mac_world_vbo_release(&gpu_world_vbos[i]);
    gpu_world_vbo_bytes=0;
    mac_index_cache_release();
}
/* The GL buffer of this draw's certified streams, decoded once; NULL keeps
   the per-draw decoder (uncertified streams, a buffer too large, or one with
   a value the decoder refuses, which only fails if a draw reads it). */
static const struct world_vbo_entry *mac_world_vbo_get(const struct vertex_shader_object *vertex,
    const unsigned char used_streams[16],const unsigned char *const streams[16],
    const unsigned long strides[16],unsigned long vertex_count)
{
    unsigned char declaration[BSP_DECODE_MAX_DECLARATION_BYTES];
    struct world_vbo_stream_key keys[16];
    struct world_vbo_entry *entry=NULL;
    unsigned long mask=0,key_count=0,capacity=0xffffffffUL,i,v,e,stride,bytes;
    size_t declaration_bytes;float *packed;float record[XGPU_VERTEX_ATTRIBUTE_COUNT*4];
    unsigned long hint;
    GLint old_buffer=0;
    uint64_t segment_last=gpu_diag_segments>0?mach_absolute_time():0;
    declaration_bytes=mac_bsp_cache_declaration(vertex,declaration,&mask);
    if(!declaration_bytes)return NULL;
    memset(keys,0,sizeof(keys));
    SEGMENT_MARK(7);
    for(i=0;i<16;i++)if(used_streams[i]) {
        unsigned long long owner=0;unsigned long generation=0,offset=0,remaining=0;
        if(!strides[i]||vertex_count>0xffffffffUL/strides[i])return NULL;
        if(!mac_resource_certificate_query(streams[i],vertex_count*strides[i],&owner,&generation,
               &offset,&remaining)||!owner||!generation||!remaining)return NULL;
        keys[key_count].slot=i;keys[key_count].owner=owner;keys[key_count].generation=generation;
        keys[key_count].base_offset=offset;keys[key_count].stride=strides[i];
        keys[key_count].capacity=remaining/strides[i];
        if(keys[key_count].capacity<capacity)capacity=keys[key_count].capacity;
        ++key_count;
    }
    if(!key_count||capacity<vertex_count)return NULL;
    SEGMENT_MARK(8);
    {
        unsigned long h=(unsigned long)(keys[0].owner*2654435761ULL)^keys[0].base_offset*40503UL^
            keys[0].generation*97UL^declaration_bytes*131UL;
        for(e=0;e<declaration_bytes;e++)h=h*31UL+declaration[e];
        hint=h%WORLD_VBO_HINTS;
    }
    for(v=0;v<=WORLD_VBO_MAX_ENTRIES;v++) {
        /* (the hinted entry first, then every entry) */
        struct world_vbo_entry *candidate;
        if(!v)candidate=&gpu_world_vbos[gpu_world_vbo_hints[hint]%WORLD_VBO_MAX_ENTRIES];
        else candidate=&gpu_world_vbos[v-1];
        if(candidate->live&&candidate->keys[0].owner==keys[0].owner&&
           candidate->declaration_bytes==declaration_bytes&&
           candidate->key_count==key_count&&
           !memcmp(candidate->declaration,declaration,declaration_bytes)&&
           !memcmp(candidate->keys,keys,sizeof(keys[0])*key_count)) {
            gpu_world_vbo_hints[hint]=(unsigned short)(candidate-gpu_world_vbos);
            if(candidate->rejected) {
                candidate->last_use=++gpu_world_vbo_serial;
                if(candidate->gather)++gpu_world_vbo_gather_skips;
                return NULL;
            }
            candidate->last_use=++gpu_world_vbo_serial;SEGMENT_MARK(9);return candidate;
        }
    }
    stride=0;
    for(e=0;e<vertex->element_count;e++)stride+=mac_decoded_components(vertex->elements[e].type)*sizeof(float);
    if(capacity>WORLD_VBO_MAX_VERTICES||!stride||capacity>WORLD_VBO_BUDGET/stride)return NULL;
    bytes=capacity*stride;
    /* a free slot, evicting the least recently used until the budget holds */
    for(;;) {
        struct world_vbo_entry *oldest=NULL;
        entry=NULL;
        for(i=0;i<WORLD_VBO_MAX_ENTRIES;i++) {
            if(!gpu_world_vbos[i].live){if(!entry)entry=&gpu_world_vbos[i];continue;}
            if(!oldest||gpu_world_vbos[i].last_use<oldest->last_use)oldest=&gpu_world_vbos[i];
        }
        if(entry&&gpu_world_vbo_bytes+bytes<=WORLD_VBO_BUDGET)break;
        if(!oldest)return NULL;
        mac_world_vbo_release(oldest);++gpu_world_vbo_evictions;
    }
    memcpy(entry->declaration,declaration,declaration_bytes);
    entry->declaration_bytes=declaration_bytes;
    memcpy(entry->keys,keys,sizeof(keys[0])*key_count);entry->key_count=key_count;
    entry->live=1;entry->last_use=++gpu_world_vbo_serial;
    gpu_world_vbo_hints[hint]=(unsigned short)(entry-gpu_world_vbos);
    packed=(float *)malloc(bytes);
    if(packed&&(mask&(1UL<<5))) {
        entry->addresses=(float *)malloc(capacity*2*sizeof(float));
        if(!entry->addresses){free(packed);packed=NULL;}
    }
    if(packed) {
        for(v=0;v<capacity;v++) {
            float *out=packed+v*(stride/sizeof(float));
            memset(record,0,sizeof(record));
            if(!mac_device_decode_vertex_elements(vertex,streams,strides,v,record))break;
            for(e=0;e<vertex->element_count;e++) {
                unsigned components=mac_decoded_components(vertex->elements[e].type);
                memcpy(out,record+vertex->elements[e].reg*4,components*sizeof(float));
                out+=components;
            }
            if(entry->addresses) {
                entry->addresses[v*2]=record[20];entry->addresses[v*2+1]=record[21];
                if(record[20]>entry->bone_max)entry->bone_max=record[20];
                if(record[21]>entry->bone_max)entry->bone_max=record[21];
            }
        }
    }
    if(!packed||v<capacity) {
        free(packed);free(entry->addresses);entry->addresses=NULL;
        entry->rejected=1;++gpu_world_vbo_rejections;return NULL;
    }
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&old_buffer);
    glGenBuffers(1,&entry->buffer);
    glBindBuffer(GL_ARRAY_BUFFER,entry->buffer);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)bytes,packed,GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,(GLuint)old_buffer);
    free(packed);
    if(glGetError()!=GL_NO_ERROR||!entry->buffer) {
        if(entry->buffer)glDeleteBuffers(1,&entry->buffer);
        entry->buffer=0;entry->rejected=1;++gpu_world_vbo_rejections;return NULL;
    }
    entry->vertex_count=capacity;entry->stride=stride;entry->mask=mask;
    entry->bytes=bytes+(entry->addresses?capacity*2*sizeof(float):0);
    bytes=entry->bytes;
    {
        unsigned long offset=0;
        for(e=0;e<vertex->element_count;e++) {
            unsigned components=mac_decoded_components(vertex->elements[e].type);
            entry->offsets[vertex->elements[e].reg]=offset;
            entry->sizes[vertex->elements[e].reg]=(unsigned char)components;
            offset+=components*sizeof(float);
        }
    }
    gpu_world_vbo_bytes+=bytes;++gpu_world_vbo_builds;
    if(gpu_world_vbo_bytes>gpu_world_vbo_peak)gpu_world_vbo_peak=gpu_world_vbo_bytes;
    return entry;
}
/* (a world draw whose program needs bone gathering) the records the
   per-draw decoder would have made */
static void mac_world_draw_decode_records(void)
{
    unsigned long i,count=device.immediate_count,floats=XGPU_VERTEX_ATTRIBUTE_COUNT*4;
    const struct vertex_shader_object *vertex=device.program_address<VERTEX_PROGRAM_SLOTS?
        device.program_slots[device.program_address]:device.vertex_shader;
    if(count>device.immediate_capacity){
        float *grown=(float *)realloc(device.immediate_vertices,(size_t)count*floats*sizeof(float));
        if(!grown)mac_gpu_shader_failure("world draw record allocation failed");
        device.immediate_vertices=grown;device.immediate_capacity=count;
    }
    for(i=0;i<count;i++) {
        float *destination=device.immediate_vertices+i*floats;
        memcpy(destination,device.attributes,floats*sizeof(float));
        if(!mac_device_decode_vertex_elements(vertex,gpu_world_draw.streams,gpu_world_draw.strides,
               gpu_world_draw.base_vertex+gpu_world_draw.indices[i],destination))
            mac_gpu_shader_failure("indexed stream contains a non-finite float");
    }
    {
        /* This buffer and declaration feed a gathering program: free its GL
           copy and keep the per-draw decoder for it from now on. */
        struct world_vbo_entry *entry=(struct world_vbo_entry *)gpu_world_draw.entry;
        if(entry->buffer)glDeleteBuffers(1,&entry->buffer);
        entry->buffer=0;
        if(!entry->rejected)gpu_world_vbo_bytes-=entry->bytes;
        entry->bytes=0;entry->rejected=1;entry->gather=1;
    }
    gpu_world_draw.active=0;++gpu_world_vbo_fallbacks;
}
/* (HALO_MAC_PERF_MINMAX) the smallest and largest index of a draw, in
   registers (the loop in the draw function keeps both in memory) */
static void mac_index_range(const WORD *indices,UINT count,WORD *minimum_out,WORD *maximum_out)
{
    unsigned long low=0xffffUL,high=0,i=0;
    for(;i+4<=count;i+=4) {
        unsigned long a=indices[i],b=indices[i+1],c=indices[i+2],d=indices[i+3];
        if(a<low)low=a;if(a>high)high=a;
        if(b<low)low=b;if(b>high)high=b;
        if(c<low)low=c;if(c>high)high=c;
        if(d<low)low=d;if(d>high)high=d;
    }
    for(;i<count;i++){unsigned long a=indices[i];if(a<low)low=a;if(a>high)high=a;}
    *minimum_out=(WORD)low;*maximum_out=(WORD)high;
}
static void mac_device_draw_stream_indices(D3DPRIMITIVETYPE type,UINT count,
    CONST WORD *indices,unsigned long base_vertex,int validate_index_buffer,int indexed_call)
{
    int range_known=0;struct index_range_entry *range_entry=NULL;unsigned long range_generation=0;
    struct vertex_shader_object *vertex=device.program_address<VERTEX_PROGRAM_SLOTS?
        device.program_slots[device.program_address]:device.vertex_shader;
    unsigned long index_base,index_start,index_offset,vertex_count,needed,strides[16]={0};
    const unsigned char *streams[16]={0};
    unsigned long i,j,k,floats_per_vertex;WORD minimum=0xffffU,maximum=0;
    mac_render_timing_stamp decode_stamp;
    unsigned char used_streams[16]={0};
    unsigned char used[XGPU_VERTEX_ATTRIBUTE_COUNT]={0};
    struct bsp_decode_cache_view bsp_view;
    int bsp_draw_result=BSP_DECODE_CACHE_BYPASS,bsp_draw_active=0;
    uint64_t segment_last=0;
    if(gpu_diag_segments<0){const char *v=getenv("HALO_MAC_DIAG_SEGMENTS");gpu_diag_segments=v&&v[0]=='1';}
    if(gpu_diag_segments>0){segment_last=mach_absolute_time();++gpu_segment_calls;}
    if(device.headless){require_headless();if(count&&indices)device.skipped_draws++;return;}
    if(!device.gpu||!device.gpu_frame_active)mac_gpu_shader_failure("indexed draw is outside an active GPU frame");
    decode_stamp=mac_render_timing_begin(mac_render_timing_stream_decode);
    if(!indices||!count||count>65535||!vertex||!vertex->instructions||!vertex->instruction_count)
        mac_gpu_shader_failure("indexed draw has no bounded indices or live vertex program");
    if(mac_gpu_primitive(type)==(GLenum)-1)mac_gpu_shader_failure("indexed primitive kind is unsupported");
    if((type==D3DPT_TRIANGLELIST&&count%3)||(type==D3DPT_LINELIST&&count%2)||
       (type==D3DPT_QUADLIST&&count%4))
        mac_gpu_shader_failure("incomplete indexed list primitive");
    if(!vertex->element_count)mac_gpu_shader_failure("empty vertex declarations are unsupported");
    floats_per_vertex=XGPU_VERTEX_ATTRIBUTE_COUNT*4;
    for(i=0;i<vertex->element_count;i++){
        const struct vertex_element *element=&vertex->elements[i];unsigned components=mac_float_components(element->type);
        unsigned element_bytes=mac_indexed_type_bytes(element->type);
        if(!element_bytes||element->stream>=16||element->reg>=XGPU_VERTEX_ATTRIBUTE_COUNT||
           element->bytes!=element_bytes||used[element->reg]||
           !device.streams[element->stream].data||!device.streams[element->stream].stride||
           device.streams[element->stream].stride>4096||
           (unsigned long)element->offset+element->bytes>device.streams[element->stream].stride)
            mac_gpu_shader_failure("vertex declaration is outside the bounded live-stream subset");
        used_streams[element->stream]=1;
        used[element->reg]=1;
    }
    SEGMENT_MARK(0);
    if(validate_index_buffer){
        index_base=(unsigned long)device.index_data;index_start=(unsigned long)indices;
        if(!index_base||index_start<index_base||index_start-index_base>0xffffffffUL-count*sizeof(WORD))
            mac_gpu_shader_failure("index pointer is not in the selected buffer");
        index_offset=index_start-index_base;
        /* port: (HALO_MAC_PERF_INDEX_RANGE, on) a draw of indices seen before, with no write to the pages of its indices since
           (mac_resource_range_generation, as the Xbox textures are kept; freeing or allocating a buffer counts as a write
           to all of it) is in the same live allocation and has the same range */
        if(gpu_index_range_enabled<0)gpu_index_range_enabled=mac_perf_flag("HALO_MAC_PERF_INDEX_RANGE");
        if(gpu_index_range_enabled) {
            unsigned long generation=mac_resource_range_generation(indices,count*sizeof(WORD));
            unsigned long slot=(((unsigned long)indices>>1)^(count*2654435761UL))%INDEX_RANGE_SLOTS;
            struct index_range_entry *entry=&gpu_index_ranges[slot];
            if(generation&&entry->indices==indices&&entry->count==count&&entry->base==index_base&&entry->generation==generation) {
                minimum=entry->minimum;maximum=entry->maximum;range_known=1;++gpu_index_range_hits;
            } else {range_entry=generation?entry:NULL;range_generation=generation;}
        }
        if(!range_known&&!mac_resource_range_is_allocated((const void *)index_base,index_offset+count*sizeof(WORD))) {
            fprintf(stderr,"native index range: base=%08lx start=%08lx offset=%lu count=%u type=%u\n",
                index_base,index_start,index_offset,(unsigned)count,(unsigned)type);
            mac_gpu_shader_failure("indexed draw exceeds a live index-buffer allocation");
        }
    }
    SEGMENT_MARK(1);
    if(range_known);
    else if(gpu_fast_minmax)mac_index_range(indices,count,&minimum,&maximum);
    else for(i=0;i<count;i++){WORD value=indices[i];if(value<minimum)minimum=value;if(value>maximum)maximum=value;}
    if(range_entry) {
        range_entry->indices=indices;range_entry->count=count;range_entry->base=index_base;range_entry->generation=range_generation;
        range_entry->minimum=minimum;range_entry->maximum=maximum;
    }
    SEGMENT_MARK(2);
    if(base_vertex>0xffffffffUL-(unsigned long)maximum-1UL)
        mac_gpu_shader_failure("base vertex index overflows");
    vertex_count=base_vertex+maximum+1;
    for(i=0;i<16;i++)if(used_streams[i]){
        strides[i]=device.streams[i].stride;
        if(vertex_count>0xffffffffUL/strides[i])
            mac_gpu_shader_failure("vertex stream range overflows");
        streams[i]=(const unsigned char *)PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[i].data);
        needed=vertex_count*strides[i];
        if(!mac_resource_range_is_allocated(streams[i],needed)) {
            /* port: a secondary stream the draw left unbound. The effect
               vertex shader declares stream 1 (the z-sprite texture
               coordinates) for every effect draw, but the game sets it only
               for a z-sprite (the first-person plasma rifle's flash does not);
               on the Xbox the program does not read the stale stream then.
               The attributes it feeds are zeros here. */
            static unsigned char *unbound_stream;static unsigned long unbound_bytes;
            if(i==0||needed>0x1000000UL)
                mac_gpu_shader_failure("draw exceeds a live vertex-stream allocation");
            if(needed>unbound_bytes) {
                unsigned char *grown=(unsigned char *)realloc(unbound_stream,needed);
                if(!grown)mac_gpu_shader_failure("unbound vertex stream allocation failed");
                unbound_stream=grown;unbound_bytes=needed;
            }
            memset(unbound_stream,0,needed);
            streams[i]=unbound_stream;
        }
    }
    SEGMENT_MARK(3);
    if(gpu_world_vbo_enabled) {
        const struct world_vbo_entry *world=type==D3DPT_QUADLIST?NULL:
            mac_world_vbo_get(vertex,used_streams,streams,strides,vertex_count);
        if(world)SEGMENT_MARK(4);else SEGMENT_MARK(10);
        if(world) {
            /* Drawn from the decoded GL buffer with the game's own indices. */
            gpu_world_draw.active=1;gpu_world_draw.entry=world;gpu_world_draw.indices=indices;
            gpu_world_draw.base_vertex=base_vertex;
            gpu_world_draw.minimum=minimum;gpu_world_draw.maximum=maximum;gpu_world_draw.gather_active=0;
            memcpy(gpu_world_draw.streams,streams,sizeof(streams));
            memcpy(gpu_world_draw.strides,strides,sizeof(strides));
            device.immediate_type=type;device.immediate_count=count;device.immediate_active=FALSE;
            mac_render_timing_end(mac_render_timing_stream_decode,decode_stamp);
            SEGMENT_MARK(5);
            device.gpu_stream_draw_active=1;mac_gpu_draw_immediate();device.gpu_stream_draw_active=0;
            SEGMENT_MARK(6);
            ++gpu_world_vbo_draws;
            gpu_world_draw.active=0;gpu_world_draw.gather_active=0;
            if(indexed_call)device.gpu_indexed_draw_count++;
            device.immediate_count=0;
            return;
        }
        ++gpu_world_vbo_bypasses;
    }
    if(count>0xffffffffUL/(floats_per_vertex*sizeof(float)))
        mac_gpu_shader_failure("expanded indexed draw is too large");
    if(count>device.immediate_capacity){
        float *grown=(float *)realloc(device.immediate_vertices,(size_t)count*floats_per_vertex*sizeof(float));
        if(!grown)mac_gpu_shader_failure("indexed expansion allocation failed");
        device.immediate_vertices=grown;device.immediate_capacity=count;
    }
    /* Reuse only fully decoded records from this draw. Bounds checks above
       still cover every stream, and bone gathering remains downstream. */
    mac_index_decode_cache_begin(&gpu_index_decode_cache,count,minimum,maximum);
    if(gpu_bsp_decode_cache_enabled) {
        bsp_draw_result=mac_bsp_cache_begin_draw(vertex,used_streams,streams,
            strides,vertex_count,&bsp_view);
        if(bsp_draw_result==BSP_DECODE_CACHE_HIT) {
            ++gpu_bsp_draw_hits;bsp_draw_active=1;
        } else if(bsp_draw_result==BSP_DECODE_CACHE_MISS) {
            ++gpu_bsp_draw_misses;bsp_draw_active=1;
        } else ++gpu_bsp_draw_bypasses;
    }
    gpu_stream_remap_count=0;
    if(gpu_indexed_vertex_reuse&&gpu_index_decode_cache.active) {
        if(count>gpu_stream_remap_capacity) {
            WORD *grown=(WORD *)realloc(gpu_stream_remap,(size_t)count*sizeof(WORD));
            if(grown) {gpu_stream_remap=grown;gpu_stream_remap_capacity=count;}
        }
        /* Allocation failure retains the ordinary expanded draw. */
        if(count<=gpu_stream_remap_capacity)gpu_stream_remap_count=count;
    }
    for(i=0;i<count;i++){
        unsigned long source_index=base_vertex+indices[i];
        const unsigned char *source;
        float *destination=device.immediate_vertices+i*floats_per_vertex;
        int bsp_vertex_result=BSP_DECODE_CACHE_BYPASS;
        if(gpu_index_decode_cache.active) {
            unsigned first_output;int found;
            found=mac_index_decode_cache_lookup(&gpu_index_decode_cache,indices[i],&first_output);
            if(found<0||(found&&first_output>=i))
                mac_gpu_shader_failure("decoded index cache references an invalid output");
            if(found) {
                memcpy(destination,device.immediate_vertices+first_output*floats_per_vertex,
                    floats_per_vertex*sizeof(float));
                if(gpu_stream_remap_count)gpu_stream_remap[i]=(WORD)first_output;
                gpu_stream_reused++;continue;
            }
        }
        if(gpu_stream_remap_count)gpu_stream_remap[i]=(WORD)i;
        memcpy(destination,device.attributes,floats_per_vertex*sizeof(float));
        if(bsp_draw_active) {
            bsp_vertex_result=bsp_decode_cache_lookup_vertex(&bsp_view,
                source_index,&device.attributes[0][0],destination);
            if(bsp_vertex_result==BSP_DECODE_CACHE_HIT) {
                ++gpu_bsp_vertex_hits;
                if(gpu_index_decode_cache.active&&
                   mac_index_decode_cache_record(&gpu_index_decode_cache,indices[i],(unsigned)i)<0)
                    mac_gpu_shader_failure("decoded index cache could not record a BSP-cache hit");
                continue;
            }
            if(bsp_vertex_result==BSP_DECODE_CACHE_MISS)++gpu_bsp_vertex_misses;
            else {
                /* A cache-side bounds/ownership mismatch is a safe bypass for
                   this draw; the normal per-draw validation already passed. */
                ++gpu_bsp_draw_bypasses;bsp_draw_active=0;
                bsp_vertex_result=BSP_DECODE_CACHE_BYPASS;
                memcpy(destination,device.attributes,floats_per_vertex*sizeof(float));
            }
        }
        ++gpu_stream_decoded;
        if(!mac_device_decode_vertex_elements(vertex,streams,strides,source_index,destination))
            mac_gpu_shader_failure("indexed stream contains a non-finite float");
        if(bsp_draw_active&&bsp_vertex_result==BSP_DECODE_CACHE_MISS&&
           bsp_decode_cache_store_vertex(&bsp_view,source_index,destination)==BSP_DECODE_CACHE_INVALID)
            ++gpu_bsp_store_rejections;
        if(gpu_index_decode_cache.active&&
           mac_index_decode_cache_record(&gpu_index_decode_cache,indices[i],(unsigned)i)<0)
            mac_gpu_shader_failure("decoded index cache could not record an output");
    }
    mac_index_decode_cache_end(&gpu_index_decode_cache);
    device.immediate_type=type;device.immediate_count=count;device.immediate_active=FALSE;
    mac_render_timing_end(mac_render_timing_stream_decode,decode_stamp);
    device.gpu_stream_draw_active=1;mac_gpu_draw_immediate();device.gpu_stream_draw_active=0;
    gpu_stream_remap_count=0;
    if(indexed_call)device.gpu_indexed_draw_count++;
    device.immediate_count=0;
}
void WINAPI D3DDevice_DrawIndexedVertices(D3DPRIMITIVETYPE type,UINT count,CONST WORD *indices)
{
    jmp_buf escape;jmp_buf *previous=gpu_draw_escape;
    GPU_GUARDED(escape,mac_device_draw_stream_indices(type,count,indices,device.base_vertex_index,1,1));
    gpu_draw_escape=previous;device.gpu_stream_draw_active=0;
}
void WINAPI D3DDevice_DrawVertices(D3DPRIMITIVETYPE type,UINT start,UINT count)
{
    WORD *indices;unsigned long index_count,i;D3DPRIMITIVETYPE draw_type=type;
    if(device.headless){require_headless();if(count)device.skipped_draws++;return;}
    if(!device.gpu||!device.gpu_frame_active)
        mac_gpu_shader_failure("nonindexed draw is outside an active GPU frame");
    if(!count||count>65535||(type!=D3DPT_QUADLIST&&mac_gpu_primitive(type)==(GLenum)-1))
        mac_gpu_shader_failure("nonindexed draw has an unsupported primitive or vertex count");
    if(type==D3DPT_QUADLIST){
        unsigned long quads;
        if(count%4)mac_gpu_shader_failure("quad list has an incomplete four-vertex primitive");
        quads=count/4;
        if(quads>65535UL/6UL)mac_gpu_shader_failure("quad list expansion exceeds the bounded draw size");
        index_count=quads*6UL;draw_type=D3DPT_TRIANGLELIST;
    }else index_count=count;
    if(start>65535UL||count>65536UL-start)
        mac_gpu_shader_failure("nonindexed vertex range exceeds 16-bit stream indices");
    indices=(WORD *)malloc((size_t)index_count*sizeof(WORD));
    if(!indices)mac_gpu_shader_failure("nonindexed index expansion allocation failed");
    if(type==D3DPT_QUADLIST){
        for(i=0;i<count;i+=4){unsigned long out=(i/4)*6UL;WORD v=(WORD)(start+i);
            indices[out+0]=v;indices[out+1]=(WORD)(v+1);indices[out+2]=(WORD)(v+2);
            indices[out+3]=v;indices[out+4]=(WORD)(v+2);indices[out+5]=(WORD)(v+3);}
    }else for(i=0;i<count;i++)indices[i]=(WORD)(start+i);
    {
        jmp_buf escape;jmp_buf *previous=gpu_draw_escape;
        GPU_GUARDED(escape,mac_device_draw_stream_indices(draw_type,(UINT)index_count,indices,0,0,0));
        gpu_draw_escape=previous;device.gpu_stream_draw_active=0;
    }
    free(indices);
}
void WINAPI D3DDevice_Clear(DWORD count,CONST D3DRECT *rects,DWORD flags,D3DCOLOR color,float z,DWORD stencil)
{
    if(device.headless){require_headless();device.skipped_clears++;return;}
    if(gpu_target_skipped){++gpu_target_skipped_draws;return;}
    if(device.render_target==&device.back_buffer)++device.gpu_back_buffer_writes;
    mac_gl_shadow_forget();
    if(!active||!device.gpu||!device.gpu_frame_active||!device.render_target) {
        fprintf(stderr,"D3D clear requires an active native GPU render target\n");abort();
    }
    {
        GLbitfield mask=0;float rgba[4];long i;unsigned long height=device.gpu_height;
        if(device.render_target!=&device.back_buffer) {
            unsigned target_width,target_height;
            if(!mac_engine_device_resources_target_info(device.render_target,NULL,NULL,
                &target_width,&target_height)) {
                fprintf(stderr,"D3D clear target has no native framebuffer backing\n");abort();
            }
            height=target_height;
        }
        color_to_vec4(color,rgba);
        if(flags&D3DCLEAR_TARGET) {
            glColorMask((flags&D3DCLEAR_TARGET_R)!=0,(flags&D3DCLEAR_TARGET_G)!=0,
                (flags&D3DCLEAR_TARGET_B)!=0,(flags&D3DCLEAR_TARGET_A)!=0);
            glClearColor(rgba[0],rgba[1],rgba[2],rgba[3]);mask|=GL_COLOR_BUFFER_BIT;
        }
        if(device.depth_stencil&&(flags&D3DCLEAR_ZBUFFER)) {
            glDepthMask(GL_TRUE);glClearDepth(z);mask|=GL_DEPTH_BUFFER_BIT;
        }
        if(device.depth_stencil&&(flags&D3DCLEAR_STENCIL)) {
            GLint bits=0;glGetIntegerv(GL_STENCIL_BITS,&bits);
            if(bits<8)mac_gpu_shader_failure("clear target lacks an eight-bit stencil attachment");
            glStencilMask(0xff);glClearStencil((GLint)stencil);mask|=GL_STENCIL_BUFFER_BIT;
        }
        if(mask) {
            glEnable(GL_SCISSOR_TEST);
            if(!count||!rects) {
                GLint x=device.viewport.X,y=mac_target_flipped()?(GLint)device.viewport.Y:(GLint)(height-(device.viewport.Y+device.viewport.Height));
                GLsizei w=(GLsizei)device.viewport.Width,h=(GLsizei)device.viewport.Height;
                if(mac_render_scaled())mac_render_scale_rect(&x,&y,&w,&h);
                glScissor(x,y,w,h);
                glClear(mask);
            } else for(i=0;i<(long)count;i++) {
                long left=rects[i].x1>device.viewport.X?rects[i].x1:device.viewport.X;
                long top=rects[i].y1>device.viewport.Y?rects[i].y1:device.viewport.Y;
                long right=rects[i].x2<(LONG)(device.viewport.X+device.viewport.Width)?rects[i].x2:device.viewport.X+device.viewport.Width;
                long bottom=rects[i].y2<(LONG)(device.viewport.Y+device.viewport.Height)?rects[i].y2:device.viewport.Y+device.viewport.Height;
                if(left<right&&top<bottom) {
                    GLint x=(GLint)(left+mac_engine_ui_offset()),y=mac_target_flipped()?(GLint)top:(GLint)(height-bottom);GLsizei w=(GLsizei)(right-left),h=(GLsizei)(bottom-top);
                    if(mac_render_scaled())mac_render_scale_rect(&x,&y,&w,&h);
                    glScissor(x,y,w,h);
                    glClear(mask);
                }
            }
            glDisable(GL_SCISSOR_TEST);
        }
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        if(mac_gpu_draws_strict()&&glGetError()!=GL_NO_ERROR){fprintf(stderr,"OpenGL error during D3D clear\n");abort();}
    }
}
/* Stretch the scaled frame in the back buffer's lower left over all of it. */
static void mac_render_scale_resolve(void)
{
    GLsizei width=(GLsizei)device.gpu_width,height=(GLsizei)device.gpu_height;
    GLint x=0,y=0;GLsizei w=width,h=height;
    float s,t;
    mac_render_scale_rect(&x,&y,&w,&h);
    if(!gpu_scale_texture||gpu_scale_texture_width<(unsigned)width||gpu_scale_texture_height<(unsigned)height) {
        unsigned tw=1,th=1;
        while(tw<(unsigned)width)tw<<=1;
        while(th<(unsigned)height)th<<=1;
        if(gpu_scale_texture){mac_gpu_textures_forget(gpu_scale_texture);glDeleteTextures(1,&gpu_scale_texture);}
        glGenTextures(1,&gpu_scale_texture);
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glBindTexture(GL_TEXTURE_2D,gpu_scale_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,(GLsizei)tw,(GLsizei)th,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        gpu_scale_texture_width=tw;gpu_scale_texture_height=th;
    }
    /* fixed function, no programs or tests: one textured quad */
    glUseProgram(0);gpu_current_program=0;
    glDisable(GL_VERTEX_PROGRAM_ARB);glDisable(GL_FRAGMENT_PROGRAM_ARB);glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_FOG);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    mac_ff_leave();
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_2D,gpu_scale_texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,w,h);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    glViewport(0,0,width,height);
    s=(float)w/(float)gpu_scale_texture_width;t=(float)h/(float)gpu_scale_texture_height;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f,0.0f);glVertex2f(-1.0f,-1.0f);
    glTexCoord2f(s,0.0f);glVertex2f(1.0f,-1.0f);
    glTexCoord2f(s,t);glVertex2f(1.0f,1.0f);
    glTexCoord2f(0.0f,t);glVertex2f(-1.0f,1.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    /* (everything the draws keep track of has changed) */
    mac_gl_shadow_forget();mac_gpu_textures_shadow_reset();
}
/* ---- port: display.anti_aliasing "fxaa", and a window or display larger than
   the 640x480 the game draws (display.window_size, display.resolution).
   Both read the picture back into a texture and draw it again: FXAA over the
   3D view's region once it is drawn (halo_screen_anti_alias, before the HUD
   and the menus), and the scaled present over the whole window once the frame
   is finished (the game's 640x480 stays in the lower left of the window's
   framebuffer; the window is cleared and the picture drawn into it, as large
   as it goes with its shape, centred). Rectangle textures (ARB_texture_rectangle)
   keep the texels' own coordinates, and no more than the picture is read.
   The FXAA is the one in NVIDIA's FXAA white paper, as an ARB fragment program
   (about 55 instructions, so that the Radeons of the time run it in the
   hardware; one that does not is not used), with the later version's
   threshold: where the contrast of the neighbours is small, the pixel stays
   as it is, rather than having its textures blurred. */
#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif
static GLuint gpu_post_texture,gpu_post_fxaa;
static int gpu_post_rectangles=-1,gpu_post_fxaa_state;/* 0 not tried, 1 ready, -1 failed */
static GLsizei gpu_post_width,gpu_post_height;
static const char gpu_post_fxaa_source[]=
"!!ARBfp1.0\n"
"PARAM luma={0.299,0.587,0.114,0.0};\n"
"PARAM k={0.03125,0.0078125,8.0,1.0};\n"
"PARAM c={0.1666667,-0.1666667,0.5,0.25};\n"
"TEMP nw,ne,sw,se,m,uv,s,dir,d,a,b,t,lo,hi;\n"
"ATTRIB tc=fragment.texcoord[0];\n"
"ADD uv,tc,{-1.0,1.0,0.0,0.0};\nTEX nw,uv,texture[0],RECT;\n"
"ADD uv,tc,{1.0,1.0,0.0,0.0};\nTEX ne,uv,texture[0],RECT;\n"
"ADD uv,tc,{-1.0,-1.0,0.0,0.0};\nTEX sw,uv,texture[0],RECT;\n"
"ADD uv,tc,{1.0,-1.0,0.0,0.0};\nTEX se,uv,texture[0],RECT;\n"
"TEX m,tc,texture[0],RECT;\n"
"DP3 nw.w,nw,luma;\nDP3 ne.w,ne,luma;\nDP3 sw.w,sw,luma;\nDP3 se.w,se,luma;\nDP3 m.w,m,luma;\n"
"ADD s.x,nw.w,ne.w;\nADD s.y,sw.w,se.w;\nADD s.z,nw.w,sw.w;\nADD s.w,ne.w,se.w;\n"
"SUB dir.x,s.y,s.x;\nSUB dir.y,s.w,s.z;\n"
"ADD t.x,s.x,s.y;\nMUL t.x,t.x,k.x;\nMAX t.x,t.x,k.y;\n"
"ABS d.x,dir.x;\nABS d.y,dir.y;\nMIN d.x,d.x,d.y;\nADD d.x,d.x,t.x;\nRCP d.x,d.x;\n"
"MUL dir.xy,dir,d.x;\nMIN dir.xy,dir,k.z;\nMAX dir.xy,dir,-k.z;\n"
"MAD uv,dir,c.y,tc;\nTEX a,uv,texture[0],RECT;\n"
"MAD uv,dir,c.x,tc;\nTEX t,uv,texture[0],RECT;\n"
"ADD a,a,t;\nMUL a,a,c.z;\n"
"MAD uv,dir,-c.z,tc;\nTEX b,uv,texture[0],RECT;\n"
"MAD uv,dir,c.z,tc;\nTEX t,uv,texture[0],RECT;\n"
"ADD b,b,t;\nMUL b,b,c.w;\nMAD b,a,c.z,b;\n"
"DP3 b.w,b,luma;\n"
"MIN lo.x,nw.w,ne.w;\nMIN lo.y,sw.w,se.w;\nMIN lo.x,lo.x,lo.y;\nMIN lo.x,lo.x,m.w;\n"
"MAX hi.x,nw.w,ne.w;\nMAX hi.y,sw.w,se.w;\nMAX hi.x,hi.x,hi.y;\nMAX hi.x,hi.x,m.w;\n"
"SUB t.x,b.w,lo.x;\nSUB t.y,hi.x,b.w;\nMIN t.x,t.x,t.y;\n"
"CMP b,t.x,a,b;\n"
"SUB t.x,hi.x,lo.x;\nMUL t.y,hi.x,0.125;\nMAX t.y,t.y,0.0416667;\nSUB t.z,t.x,t.y;\n"
"CMP result.color,t.z,m,b;\nMOV result.color.w,k.w;\n"
"END\n";
static int mac_post_available(void)
{
    if(gpu_post_rectangles<0) {
        const char *extensions=(const char *)glGetString(GL_EXTENSIONS);
        gpu_post_rectangles=extensions&&strstr(extensions,"GL_ARB_texture_rectangle")?1:0;
    }
    return gpu_post_rectangles;
}
/* the program, made when first wanted (0 when this card cannot run it) */
static int mac_post_fxaa_program(void)
{
    GLint native=0,position=-1;
    const char *extensions;
    if(gpu_post_fxaa_state)return gpu_post_fxaa_state>0;
    gpu_post_fxaa_state=-1;
    extensions=(const char *)glGetString(GL_EXTENSIONS);
    if(!mac_post_available()||!extensions||!strstr(extensions,"GL_ARB_fragment_program"))return 0;
    while(glGetError()!=GL_NO_ERROR){}
    glGenProgramsARB(1,&gpu_post_fxaa);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,gpu_post_fxaa);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(gpu_post_fxaa_source),gpu_post_fxaa_source);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&position);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&native);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,0);
    if(glGetError()!=GL_NO_ERROR||position>=0||!native) {
        fprintf(stderr,"native renderer: FXAA is not available here (program error at %ld, native %d); not applied\n",
            (long)position,(int)native);
        glDeleteProgramsARB(1,&gpu_post_fxaa);gpu_post_fxaa=0;
        return 0;
    }
    gpu_post_fxaa_state=1;
    return 1;
}
/* the state of the back buffer's frame as these passes need it: a plain
   textured quad over the rectangle texture of the picture (read from
   (x,y), w by h, into its lower left) */
static void mac_post_begin(GLint x,GLint y,GLsizei w,GLsizei h)
{
    unsigned region_width,region_height;GLsizei width,height;
    mac_device_render_region(&region_width,&region_height);
    width=(GLsizei)region_width;height=(GLsizei)region_height;
    glUseProgram(0);gpu_current_program=0;
    glDisable(GL_VERTEX_PROGRAM_ARB);glDisable(GL_FRAGMENT_PROGRAM_ARB);glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_FOG);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    mac_ff_leave();
    glActiveTextureARB(GL_TEXTURE0_ARB);
    if(!gpu_post_texture||gpu_post_width!=width||gpu_post_height!=height) {
        if(gpu_post_texture){mac_gpu_textures_forget(gpu_post_texture);glDeleteTextures(1,&gpu_post_texture);}
        glGenTextures(1,&gpu_post_texture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,gpu_post_texture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        gpu_post_width=width;gpu_post_height=height;
    }
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,gpu_post_texture);
    glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,x,y,w,h);
    glDisable(GL_TEXTURE_2D);glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glMatrixMode(GL_TEXTURE);glLoadIdentity();
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glMatrixMode(GL_MODELVIEW);glLoadIdentity();
}
static void mac_post_quad(GLint x,GLint y,GLsizei w,GLsizei h,GLsizei source_w,GLsizei source_h)
{
    glViewport(x,y,w,h);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f,0.0f);glVertex2f(-1.0f,-1.0f);
    glTexCoord2f((float)source_w,0.0f);glVertex2f(1.0f,-1.0f);
    glTexCoord2f((float)source_w,(float)source_h);glVertex2f(1.0f,1.0f);
    glTexCoord2f(0.0f,(float)source_h);glVertex2f(-1.0f,1.0f);
    glEnd();
}
static void mac_post_end(void)
{
    glDisable(GL_FRAGMENT_PROGRAM_ARB);glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,0);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glViewport(0,0,(GLsizei)device.gpu_width,(GLsizei)device.gpu_height);
    /* (everything the draws keep track of has changed) */
    mac_gl_shadow_forget();mac_gpu_textures_shadow_reset();
}
/* display.anti_aliasing "fxaa": the 3D view's bounds (the game's coordinates,
   from the top left) smoothed in place. 0 when it is not done */
/* (installed for halo_screen_anti_alias, engine_render_services.c) */
int (*mac_device_post_fxaa_hook)(int,int,int,int); /* (a common symbol: also in engine_render_services.c) */
int mac_device_post_fxaa(int x0,int y0,int x1,int y1);
static void install_post_fxaa(void) __attribute__((constructor));
static void install_post_fxaa(void) { mac_device_post_fxaa_hook=mac_device_post_fxaa; }
int mac_device_post_fxaa(int x0,int y0,int x1,int y1)
{
    GLsizei width=(GLsizei)device.gpu_width,height=(GLsizei)device.gpu_height,w,h;
    GLint px,py;
    if(!device.gpu||!device.gpu_frame_active||device.render_target!=&device.back_buffer||
       (mac_render_scaled()&&gpu_render_scale<1.0f))return 0;
    if(x0<0)x0=0;if(y0<0)y0=0;if(x1>width)x1=width;if(y1>height)y1=height;
    if(x1<=x0||y1<=y0||!mac_post_fxaa_program())return 0;
    w=(GLsizei)(x1-x0);h=(GLsizei)(y1-y0);
    px=x0;py=height-y1;
    /* (the 3D view's pixels where the frame is drawn at more than the Xbox's) */
    if(mac_render_scaled())mac_render_scale_rect(&px,&py,&w,&h);
    mac_post_begin(px,py,w,h);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,gpu_post_fxaa);
    mac_post_quad(px,py,w,h,w,h);
    mac_post_end();
    return 1;
}
/* port: display.gamma. The whole finished picture is raised to 1 / gamma in
   place, once the frame is complete and before the scaled present: an ARB
   fragment program over the rectangle texture of the picture (the same
   read-back as the passes above). At 1.0, the default, nothing is done: no
   read-back, no program. The value comes from engine_render_services.c, which
   reads config.toml (a common symbol, as mac_device_post_fxaa_hook is). */
double (*mac_device_gamma_hook)(void);
static GLuint gpu_post_gamma;
static int gpu_post_gamma_state;/* 0 not tried, 1 ready, -1 failed */
static const char gpu_post_gamma_source[]=
"!!ARBfp1.0\n"
"PARAM e=program.local[0];\n"
"PARAM k={0.000001,0.000001,0.000001,1.0};\n"
"TEMP c;\n"
"TEX c,fragment.texcoord[0],texture[0],RECT;\n"
"MAX c.xyz,c,k;\n"
"POW c.x,c.x,e.x;\nPOW c.y,c.y,e.x;\nPOW c.z,c.z,e.x;\n"
"MOV result.color,c;\n"
"END\n";
static int mac_post_gamma_program(void)
{
    GLint native=0,position=-1;
    const char *extensions;
    if(gpu_post_gamma_state)return gpu_post_gamma_state>0;
    gpu_post_gamma_state=-1;
    extensions=(const char *)glGetString(GL_EXTENSIONS);
    if(!mac_post_available()||!extensions||!strstr(extensions,"GL_ARB_fragment_program"))return 0;
    while(glGetError()!=GL_NO_ERROR){}
    glGenProgramsARB(1,&gpu_post_gamma);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,gpu_post_gamma);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(gpu_post_gamma_source),gpu_post_gamma_source);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&position);
    glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&native);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,0);
    if(glGetError()!=GL_NO_ERROR||position>=0||!native) {
        fprintf(stderr,"native renderer: display.gamma is not available here (program error at %ld, native %d); not applied\n",
            (long)position,(int)native);
        glDeleteProgramsARB(1,&gpu_post_gamma);gpu_post_gamma=0;
        return 0;
    }
    gpu_post_gamma_state=1;
    return 1;
}
static void mac_post_gamma(void)
{
    unsigned region_width,region_height;GLsizei width,height;
    double gamma;float exponent;
    mac_device_render_region(&region_width,&region_height);
    width=(GLsizei)region_width;height=(GLsizei)region_height;
    if(!mac_device_gamma_hook||!device.gpu||!device.gpu_frame_active||device.render_target!=&device.back_buffer)return;
    gamma=mac_device_gamma_hook();
    if(gamma==1.0||gpu_post_gamma_state<0||!mac_post_gamma_program())return;
    exponent=(float)(1.0/gamma);
    mac_post_begin(0,0,width,height);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,gpu_post_gamma);
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,0,exponent,exponent,exponent,exponent);
    mac_post_quad(0,0,width,height,width,height);
    mac_post_end();
}
/* HALO_MAC_PRESENT_CAPTURE=<file> (diagnostic): the window's whole picture
   after the scaled present, as a PPM (a hidden run's window is its
   framebuffer's lower left, HALO_MAC_HIDDEN_WINDOW=WxH) */
static void mac_post_capture(GLsizei width,GLsizei height)
{
    const char *path=getenv("HALO_MAC_PRESENT_CAPTURE");
    unsigned char *pixels;FILE *file;GLsizei y;GLint alignment=4;
    if(!path||!*path)return;
    pixels=(unsigned char *)malloc((size_t)width*(size_t)height*3);
    if(!pixels)return;
    glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    glPixelStorei(GL_PACK_ALIGNMENT,alignment);
    file=fopen(path,"wb");
    if(file) {
        fprintf(file,"P6\n%ld %ld\n255\n",(long)width,(long)height);
        for(y=height-1;y>=0;y--)fwrite(pixels+(size_t)y*(size_t)width*3,3,(size_t)width,file);
        fclose(file);
    }
    free(pixels);
}
/* the window larger than the frame's pixels: the frame scaled to it as far as
   its shape allows (the menus' pointer takes the same square, engine_input.c),
   with linear filtering, or without where the window is a whole number of the
   frame's pixels each way (no blur). A window of the frame's own size has
   nothing to draw. */
static void mac_post_present_scaled(void)
{
    int window_width=0,window_height=0;
    unsigned region_width,region_height;
    GLsizei width,height,w,h;
    float scale,scale_y;
    GLint filter=GL_LINEAR;
    if(!device.gpu||!device.gpu_frame_active||device.render_target!=&device.back_buffer||
       (mac_render_scaled()&&gpu_render_scale<1.0f))return;
    platform_video_window_size(&window_width,&window_height);
    mac_device_render_region(&region_width,&region_height);
    width=(GLsizei)region_width;height=(GLsizei)region_height;
    if(window_width<width||window_height<height)return;
    if(window_width==width&&window_height==height){mac_post_capture((GLsizei)window_width,(GLsizei)window_height);return;}
    if(!mac_post_available())return;
    scale=(float)window_width/(float)width;scale_y=(float)window_height/(float)height;
    if(scale_y<scale)scale=scale_y;
    if(scale>=2.0f) {
        /* (a whole number of pixels each way: doubled, not blurred) */
        float whole=(float)(int)(scale+0.5f),gap=scale-whole;
        if(gap<0.0f)gap=-gap;
        if(gap<0.005f*whole)filter=GL_NEAREST;
    }
    w=(GLsizei)((float)width*scale+0.5f);h=(GLsizei)((float)height*scale+0.5f);
    mac_post_begin(0,0,width,height);
    if(filter!=GL_LINEAR) {
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,filter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,filter);
    }
    glViewport(0,0,(GLsizei)window_width,(GLsizei)window_height);
    glClearColor(0.0f,0.0f,0.0f,1.0f);glClear(GL_COLOR_BUFFER_BIT);
    mac_post_quad((window_width-w)/2,(window_height-h)/2,w,h,width,height);
    if(filter!=GL_LINEAR) {
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    }
    mac_post_capture((GLsizei)window_width,(GLsizei)window_height);
    mac_post_end();
}
void WINAPI D3DDevice_Present(CONST RECT *source_rectangle,CONST RECT *destination_rectangle,
    void *unused,void *unused2)
{
    unsigned int *pixels,*destination;unsigned long y,pitch;
    GLint old_pack_alignment,old_pack_swap_bytes;GLenum readback_error;
    unsigned region_width,region_height;
    mac_render_timing_stamp phase_stamp;
    (void)source_rectangle;(void)destination_rectangle;(void)unused;(void)unused2;
    if(!device.gpu||!device.gpu_frame_active||device.render_target!=&device.back_buffer) {
        fprintf(stderr,"D3D present requires the active native GPU back buffer frame\n");abort();
    }
    phase_stamp=mac_render_timing_begin(mac_render_timing_present_readback);
    if(mac_render_scaled()&&gpu_render_scale<1.0f)mac_render_scale_resolve();
    mac_post_gamma(); /* port: display.gamma, over the finished picture */
    if(gpu_lazy_present&&!gpu_readback_requested) {
        mac_post_present_scaled(); /* port: a window larger than the game's picture */
        glFlush();
        mac_render_timing_end(mac_render_timing_present_readback,phase_stamp);
        device.gpu_present_count++;mac_engine_vblank_presented();
        return;
    }
    gpu_readback_requested=0;
    mac_device_render_region(&region_width,&region_height);
    pixels=(unsigned int *)malloc((size_t)region_width*(size_t)region_height*4);
    if(!pixels){fprintf(stderr,"D3D present readback allocation failed\n");abort();}
    glGetIntegerv(GL_PACK_ALIGNMENT,&old_pack_alignment);
    glGetIntegerv(GL_PACK_SWAP_BYTES,&old_pack_swap_bytes);
    glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_SWAP_BYTES,GL_FALSE);
    /* Packed BGRA produces native numeric ARGB words on either endian.
       ReadPixels completes the transfer without an additional glFinish. */
    glReadPixels(0,0,(GLsizei)region_width,(GLsizei)region_height,
        GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,pixels);
    readback_error=glGetError();
    glPixelStorei(GL_PACK_ALIGNMENT,old_pack_alignment);
    glPixelStorei(GL_PACK_SWAP_BYTES,old_pack_swap_bytes);
    if(readback_error!=GL_NO_ERROR){free(pixels);fprintf(stderr,"D3D present framebuffer readback failed\n");abort();}
    pitch=(device.gpu_width*4+D3DTEXTURE_PITCH_ALIGNMENT-1)&~(unsigned long)(D3DTEXTURE_PITCH_ALIGNMENT-1);
    destination=(unsigned int *)PLATFORM_PHYSICAL_TO_VIRTUAL(device.back_buffer.Data);
    if(region_width==device.gpu_width&&region_height==device.gpu_height) {
        for(y=0;y<device.gpu_height;y++)
            memcpy(destination+(pitch/4)*y,pixels+(device.gpu_height-1-y)*device.gpu_width,
                (size_t)device.gpu_width*4);
    } else {
        /* port: the game's memory holds its own size: the frame, drawn with
           more pixels, sampled at the centre of each of the game's */
        unsigned long x;
        for(y=0;y<device.gpu_height;y++) {
            unsigned long source_row=(2*(device.gpu_height-1-y)+1)*region_height/(2*device.gpu_height);
            const unsigned int *row=pixels+source_row*region_width;
            unsigned int *out=destination+(pitch/4)*y;
            for(x=0;x<device.gpu_width;x++)out[x]=row[(2*x+1)*region_width/(2*device.gpu_width)];
        }
    }
    free(pixels);
    mac_post_present_scaled(); /* port: a window larger than the game's picture */
    mac_render_timing_end(mac_render_timing_present_readback,phase_stamp);
    device.gpu_present_count++;mac_engine_vblank_presented();
}
void WINAPI D3DDevice_BeginVisibilityTest(void)
{
    if(device.headless){require_headless();return;}
    if(!device.gpu||!device.gpu_frame_active)
        mac_gpu_shader_failure("visibility test requires an active GPU frame");
    if(!gpu_visibility_started) {
        if(!mac_gl_visibility_start())mac_gpu_shader_failure("native GPU visibility queries are unavailable");
        gpu_visibility_started=1;
    }
    if(!mac_gl_visibility_begin())mac_gpu_shader_failure("native GPU visibility query begin failed");
    gpu_visibility_active=1;
}
HRESULT WINAPI D3DDevice_EndVisibilityTest(DWORD index)
{
    GLint viewport[4],samples=0;
    float area;
    int result;
    if(device.headless){require_headless();return S_OK;}
    if(!device.gpu||!device.gpu_frame_active||!gpu_visibility_started||
       !device.viewport.Width||!device.viewport.Height)return E_FAIL;
    glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SAMPLES,&samples);
    if(glGetError()!=GL_NO_ERROR||viewport[2]<=0||viewport[3]<=0)return E_FAIL;
    /* Use actual drawn pixels, not the power-of-two allocation padding. */
    area=((float)viewport[2]/(float)device.viewport.Width)*
        ((float)viewport[3]/(float)device.viewport.Height)*(float)(samples>0?samples:1);
    result=mac_gl_visibility_end((uint32_t)index,area);
    gpu_visibility_active=0;
    return result==MAC_GL_VISIBILITY_OK?S_OK:
        result==MAC_GL_VISIBILITY_INVALID?E_INVALIDARG:E_FAIL;
}
HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD index,UINT *result,ULONGLONG *time_stamp)
{
    uint32_t count=0;uint64_t stamp=0;int status;
    mac_render_timing_stamp phase_stamp;
    if(result)*result=0;if(time_stamp)*time_stamp=0;
    if(device.headless){require_headless();return S_OK;}
    if(!device.gpu)return E_FAIL;
    /* The original renderer polls slots before their first submission. */
    if(!gpu_visibility_started)return S_OK;
    phase_stamp=mac_render_timing_begin(mac_render_timing_visibility_results);
    status=mac_gl_visibility_result((uint32_t)index,&count,&stamp);
    mac_render_timing_end(mac_render_timing_visibility_results,phase_stamp);
    if(status!=MAC_GL_VISIBILITY_OK)return E_FAIL;
    if(result)*result=(UINT)count;if(time_stamp)*time_stamp=(ULONGLONG)stamp;
    return S_OK;
}
