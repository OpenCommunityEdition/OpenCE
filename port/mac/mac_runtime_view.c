/* Diagnostic bridge between supplied engine camera state and the native
   hidden/explicitly-visible host. This is not the Xbox renderer or game loop. */
#include "mac_runtime_view.h"
#include "mac_bsp_renderer.h"
#include "mac_window.h"
#include <OpenGL/gl.h>
#include <stdlib.h>
#include <string.h>

struct mac_runtime_view_state {
    struct mac_window window;
    struct mac_bsp_renderer *renderer;
    const struct mac_retail_cache *cache;
    struct mac_bsp_camera camera;
    struct mac_input_frame frame_input;
    mac_runtime_view_overlay_proc overlay;
    void *overlay_context;
    char *bitmap_path;
    char renderer_name[128];
    uint32_t bsp_index;
    unsigned width,height,frames,scene_pixels;
    uint32_t pixel_digest;
    unsigned char *pixels;
    const char *error;
};

static int mac_runtime_view_render(void *context,const struct mac_input_frame *input,
    unsigned width,unsigned height)
{
    struct mac_runtime_view_state *state=(struct mac_runtime_view_state *)context;
    size_t i;unsigned count=0;unsigned char red=20,green=31,blue=51;uint32_t digest=UINT32_C(2166136261);
    const GLubyte *renderer_name;
    if(width!=state->width||height!=state->height||!input) {
        state->error="window dimensions or input snapshot";return 0;
    }
    if(!state->renderer) {
        state->renderer=mac_bsp_renderer_open(state->cache,state->bsp_index,state->bitmap_path,&state->error);
        if(!state->renderer)return 0;
    }
    renderer_name=glGetString(GL_RENDERER);
    if(!renderer_name) { state->error="GL renderer unavailable";return 0; }
    strncpy(state->renderer_name,(const char *)renderer_name,sizeof(state->renderer_name)-1);
    state->renderer_name[sizeof(state->renderer_name)-1]=0;
    /* Camera is copied in state by draw() before entering this callback. */
    if(!mac_bsp_renderer_draw_camera(state->renderer,width,height,&state->camera)) {
        state->error="diagnostic BSP camera draw";return 0;
    }
    if(state->overlay&&!state->overlay(state->overlay_context,state->renderer)) {
        state->error="diagnostic object overlay draw";return 0;
    }
    glFinish();glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,(GLsizei)width,(GLsizei)height,GL_RGBA,GL_UNSIGNED_BYTE,state->pixels);
    if(glGetError()!=GL_NO_ERROR) { state->error="GL readback error";return 0; }
    for(i=0;i<(size_t)width*height;++i) {
        const unsigned char *p=state->pixels+i*4;
        if(abs((int)p[0]-red)>4||abs((int)p[1]-green)>4||abs((int)p[2]-blue)>4)++count;
        digest=(digest^p[0])*UINT32_C(16777619);
        digest=(digest^p[1])*UINT32_C(16777619);
        digest=(digest^p[2])*UINT32_C(16777619);
    }
    state->scene_pixels=count;state->pixel_digest=digest;state->frame_input=*input;++state->frames;
    return 1;
}

int mac_runtime_view_open(struct mac_runtime_view *view,const struct mac_retail_cache *cache,
    uint32_t bsp_index,const char *bitmap_path,unsigned width,unsigned height,
    enum mac_runtime_view_mode mode,const char *title,const char **error)
{
    struct mac_runtime_view_state *state;size_t pixel_count,bytes,path_length;
    if(error)*error=NULL;
    if(!view||view->state||!cache||!bitmap_path||!*bitmap_path||!width||!height||
       width>8192||height>8192||bsp_index>=cache->bsp_count||
       (mode!=MAC_RUNTIME_VIEW_HIDDEN&&mode!=MAC_RUNTIME_VIEW_VISIBLE)) {
        if(error)*error="runtime view arguments";return 0;
    }
    if((size_t)width>((size_t)-1)/(size_t)height/4) {
        if(error)*error="runtime view pixel size overflow";return 0;
    }
    pixel_count=(size_t)width*height;bytes=pixel_count*4;path_length=strlen(bitmap_path);
    if(path_length==(size_t)-1) { if(error)*error="bitmap path size";return 0; }
    state=(struct mac_runtime_view_state *)calloc(1,sizeof(*state));
    if(!state) { if(error)*error="runtime view allocation";return 0; }
    state->bitmap_path=(char *)malloc(path_length+1);
    state->pixels=(unsigned char *)malloc(bytes);
    if(!state->bitmap_path||!state->pixels) {
        free(state->bitmap_path);free(state->pixels);free(state);
        if(error)*error="runtime view pixel allocation";return 0;
    }
    memcpy(state->bitmap_path,bitmap_path,path_length+1);
    state->cache=cache;state->bsp_index=bsp_index;state->width=width;state->height=height;
    if(!mac_window_open(&state->window,width,height,mode==MAC_RUNTIME_VIEW_VISIBLE,title)) {
        free(state->bitmap_path);free(state->pixels);free(state);
        if(error)*error="native diagnostic window open";return 0;
    }
    view->state=state;return 1;
}

int mac_runtime_view_draw(struct mac_runtime_view *view,const struct mac_bsp_camera *camera,
    struct mac_runtime_view_frame *frame,const char **error)
{
    struct mac_runtime_view_state *state;struct mac_runtime_view_frame result;
    if(error)*error=NULL;
    if(!view||!view->state||!camera||!frame) { if(error)*error="runtime view frame arguments";return 0; }
    state=(struct mac_runtime_view_state *)view->state;state->camera=*camera;state->error=NULL;
    if(!mac_window_frame(&state->window,mac_runtime_view_render,state)) {
        if(error)*error=state->error?state->error:"native window frame failed";return 0;
    }
    result.input=state->frame_input;result.scene_pixels=state->scene_pixels;result.frame_number=state->frames;
    result.pixel_digest=state->pixel_digest;
    *frame=result;return 1;
}

int mac_runtime_view_set_overlay(struct mac_runtime_view *view,mac_runtime_view_overlay_proc overlay,void *context)
{
    struct mac_runtime_view_state *state;
    if(!view||!view->state||(!overlay&&context))return 0;
    state=(struct mac_runtime_view_state *)view->state;
    state->overlay=overlay;state->overlay_context=context;return 1;
}

int mac_runtime_view_write_ppm(const struct mac_runtime_view *view,const char *path)
{
    const struct mac_runtime_view_state *state;FILE *file;unsigned char *row;
    unsigned int y,x;size_t row_bytes;int ok=1;
    if(!view||!view->state||!path||!*path)return 0;
    state=(const struct mac_runtime_view_state *)view->state;
    row_bytes=(size_t)state->width*3;row=(unsigned char *)malloc(row_bytes);
    if(!row)return 0;
    file=fopen(path,"wb");if(!file){free(row);return 0;}
    if(fprintf(file,"P6\n%u %u\n255\n",state->width,state->height)<0)ok=0;
    for(y=0;ok&&y<state->height;++y) {
        const unsigned char *src=state->pixels+(size_t)(state->height-1-y)*state->width*4;
        for(x=0;x<state->width;++x) {
            row[x*3]=src[x*4];row[x*3+1]=src[x*4+1];row[x*3+2]=src[x*4+2];
        }
        if(fwrite(row,1,row_bytes,file)!=row_bytes)ok=0;
    }
    if(fclose(file))ok=0;free(row);return ok;
}

int mac_runtime_view_is_visible(const struct mac_runtime_view *view)
{
    return view&&view->state&&mac_window_is_visible(&((struct mac_runtime_view_state *)view->state)->window);
}
int mac_runtime_view_is_accelerated(const struct mac_runtime_view *view)
{
    return view&&view->state&&mac_window_is_accelerated(&((struct mac_runtime_view_state *)view->state)->window);
}
const char *mac_runtime_view_renderer(const struct mac_runtime_view *view)
{
    return view&&view->state?((struct mac_runtime_view_state *)view->state)->renderer_name:"";
}
void mac_runtime_view_close(struct mac_runtime_view *view)
{
    struct mac_runtime_view_state *state;
    if(!view||!view->state)return;
    state=(struct mac_runtime_view_state *)view->state;
    if(state->renderer)mac_bsp_renderer_close(state->renderer);
    mac_window_close(&state->window);
    free(state->bitmap_path);free(state->pixels);free(state);view->state=NULL;
}
