/* Stock renderer commands must differ only in the horizontal projection.
 * The actual device capture must use the matching horizontal extent. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float real;
typedef struct {float red,green,blue;} real_rgb_color;
typedef struct {float _11;} D3DXMATRIX;
typedef struct {int unused;} D3DBaseTexture;
typedef struct {unsigned long Data;} D3DSurface;
typedef int D3DPIXELSHADERDEF;
typedef int GLint;
typedef unsigned int GLuint;
#define TRUE 1
#define FALSE 0
#define NUMBER_OF_PROGRESS_BAR_SOUNDS 4
#define D3DTS_WORLD 0
#define D3DTS_VIEW 1
#define D3DTS_PROJECTION 2
#define FRONT_BUFFER_WIDTH 640
#define GL_READ_FRAMEBUFFER 0
#define GL_DRAW_FRAMEBUFFER 1
#define GL_SCISSOR_TEST 2
#define GL_COLOR_BUFFER_BIT 3
#define GL_LINEAR 4
static int enabled;
static int config_boolean(char const *name) {assert(!strcmp(name,"display.widescreen_loading"));return enabled;}
/* FEATURE */
static struct {
    D3DXMATRIX saved_projection,saved_world,saved_view,projection,world,view;
    void *sounds[4];
} progress_bar_globals;
static struct {void *texture0,*texture1;} progress_bar_mode;
static float wobble_phase,last_t;
static unsigned long current_time;
static int global_d3d_device,blur_shader,regular_shader;
static long screen_width;
static struct trace {
    float projection,fade,progress;
    real_rgb_color color;
    long volume[4];
    int fade_calls,initial_calls,shaders,vblank,invalidated;
} trace;
static long halo_screen_width(void) {return screen_width;}
static unsigned long GetTickCount(void) {return 100;}
static void IDirectSoundBuffer_SetVolume(void *sound,long volume) {trace.volume[(size_t)sound-1]=volume;}
static void IDirect3DDevice8_GetTransform(int d,int kind,D3DXMATRIX *matrix) {(void)d;(void)kind;matrix->_11=1.f;}
static void IDirect3DDevice8_SetTransform(int d,int kind,D3DXMATRIX const *matrix) {
    (void)d;if(kind==D3DTS_PROJECTION && !trace.initial_calls)trace.projection=matrix->_11;
}
static void IDirect3DDevice8_SetPixelShaderProgram(int d,D3DPIXELSHADERDEF *shader) {
    (void)d;assert(shader==(trace.shaders ? &regular_shader : &blur_shader));trace.shaders++;
}
static D3DSurface surface;
static void IDirect3DDevice8_GetBackBuffer(int d,int index,int type,D3DSurface **out) {
    (void)d;(void)type;assert(index==-1);*out=&surface;
}
static void this_is_awful(D3DBaseTexture *texture,D3DSurface *s) {(void)texture;assert(s==&surface);}
static void IDirect3DSurface8_Release(D3DSurface *s) {assert(s==&surface);}
static void draw_fade_layer(float x,float y,float alpha) {assert(x==0 && y==0);trace.fade=alpha;trace.fade_calls++;}
static void draw_initial_layer(real_rgb_color *color,float progress) {trace.color=*color;trace.progress=progress;trace.initial_calls++;}
static void IDirect3DDevice8_BlockUntilVerticalBlank(int d) {(void)d;trace.vblank++;}
static void rasterizer_forget_hardware_state(void) {trace.invalidated++;}
/* UNUSED DEVICE CALLS */
/* RENDERER */
struct render_target_entry {
    struct {GLuint texture;float scale[2];unsigned long gl_width,gl_height,written;} target;
    unsigned long last_rendered;
};
static struct render_target_entry back,front;
static struct {int gl_ready;D3DSurface back_buffer,front_buffer;unsigned long front_buffer_asked,front_buffer_taken,frame;} device;
static unsigned long render_target_write_serial;
static int copies,copy_left,copy_right;
static struct render_target_entry *render_target_get(D3DSurface *s) {return s==&device.back_buffer ? &back : &front;}
static void draw_flush(void) { }
static void render_target_resolve(void *t) {(void)t;}
static GLuint framebuffer_get(GLuint t,int d) {(void)d;return t;}
static void glBindFramebuffer(int a,GLuint b) {(void)a;(void)b;}
static void glDisable(int a) {(void)a;}
static void glBlitFramebuffer(int x0,int y0,int x1,int y1,int dx0,int dy0,int dx1,int dy1,int mask,int filter) {
    assert(y0==0 && y1==(int)back.target.gl_height && dx0==0 && dy0==0);
    assert(dx1==640 && dy1==480 && mask==GL_COLOR_BUFFER_BIT && filter==GL_LINEAR);
    copies++;copy_left=x0;copy_right=x1;
}
static void gl_check_errors(char const *label) {(void)label;}
static void xgpu_gl_state_invalidate(void) { }
/* CAPTURE */
static struct trace render(float progress) {
    memset(&trace,0,sizeof(trace));wobble_phase=0;last_t=progress;
    progress_bar_render(progress);
    assert(current_time==100);
    assert(trace.fade_calls==1 && trace.initial_calls==1 && trace.shaders==2 && trace.vblank==1 && trace.invalidated==1);
    return trace;
}
int main(void) {
    static const long widths[]={640,854,1120};
    static const float progress[]={.1f,.5f,.95f};
    struct trace stock,wide;unsigned int i,j;
    progress_bar_globals.projection._11=1.f;
    for(i=0;i<4;i++)progress_bar_globals.sounds[i]=(void *)(size_t)(i+1);
    device.gl_ready=1;device.front_buffer.Data=1;device.frame=4;device.front_buffer_asked=5;
    back.target.scale[0]=2.f;back.target.scale[1]=2.f;back.target.gl_height=960;
    front.target.gl_width=640;front.target.gl_height=480;
    assert(!loading_widescreen_active());
    for(i=0;i<3;i++)for(j=0;j<3;j++) {
        screen_width=widths[i];back.target.gl_width=screen_width*2;
        enabled=0;loading_widescreen_begin();stock=render(progress[j]);
        assert(fabs(stock.projection-640.f/screen_width)<.00001f);
        front_buffer_take();assert(copy_left==(screen_width*2-1280)/2 && copy_right-copy_left==1280);
        loading_widescreen_end();enabled=1;loading_widescreen_begin();wide=render(progress[j]);
        assert(wide.projection==1.f);wide.projection=stock.projection;
        assert(!memcmp(&stock,&wide,sizeof(stock))); /* all effect/audio commands unchanged */
        front_buffer_take();assert(copy_left==0 && copy_right==screen_width*2);
        assert(device.front_buffer_taken==device.frame+1);
        enabled=0;assert(loading_widescreen_active()); /* latched for this session */
        loading_widescreen_end();assert(!loading_widescreen_active());
        front_buffer_take();assert(copy_right-copy_left==1280);
    }
    i=copies;device.front_buffer_asked=0;front_buffer_take();assert(copies==(int)i);
    loading_widescreen_end();assert(!loading_widescreen_active());
    puts("stock effect/audio commands preserved; full-width feedback and session reset passed");return 0;
}
