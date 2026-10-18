#include "../port/mac/mac_window.h"
#include <OpenGL/gl.h>
#include <stdio.h>
#include <string.h>

struct probe { unsigned frames;int failures; };
static int draw_frame(void *context,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    struct probe *probe=(struct probe *)context;
    unsigned char pixel[4]={0,0,0,0},background[4]={0,0,0,0};
    float depth=1.0f;unsigned char stencil=0;GLint stencil_bits=0;
    if(!input||input->held||input->pressed||input->released||width!=64||height!=48)++probe->failures;
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_DITHER);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glClearDepth(1.0);
    glGetIntegerv(GL_STENCIL_BITS,&stencil_bits);
    glStencilMask(0xff);glClearStencil(37);
    glClearColor(0.25f,0.5f,0.75f,1.0f);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    /* Draw near red first, then farther green over the same center pixel. */
    glColor4f(1,0,0,1);glBegin(GL_TRIANGLES);
    glVertex3f(-0.8f,-0.8f,-0.5f);glVertex3f(0.8f,-0.8f,-0.5f);glVertex3f(0,0.8f,-0.5f);glEnd();
    glColor4f(0,1,0,1);glBegin(GL_TRIANGLES);
    glVertex3f(-0.8f,-0.8f,0.5f);glVertex3f(0.8f,-0.8f,0.5f);glVertex3f(0,0.8f,0.5f);glEnd();
    glReadPixels(32,24,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    glReadPixels(1,1,1,1,GL_RGBA,GL_UNSIGNED_BYTE,background);
    glReadPixels(32,24,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);
    glReadPixels(32,24,1,1,GL_STENCIL_INDEX,GL_UNSIGNED_BYTE,&stencil);
    if(stencil_bits<8||stencil!=37||glGetError()!=GL_NO_ERROR||pixel[0]<250||pixel[1]>5||pixel[2]>5||pixel[3]<250||depth>=0.5f||
        background[0]<60||background[0]>68||background[1]<124||background[1]>132||
        background[2]<188||background[2]>196||background[3]<250)++probe->failures;
    ++probe->frames;return 1;
}
int main(void)
{
    struct mac_window window,rejected;struct probe probe;
    int width=0,height=0;
    memset(&window,0,sizeof(window));memset(&rejected,0,sizeof(rejected));memset(&probe,0,sizeof(probe));
    platform_video_window_size(&width,&height);
    if(width!=0||height!=0||halo_screen_width()!=640)++probe.failures;
    if(mac_window_open(&rejected,8193,48,0,"invalid oversized probe")||rejected.state)++probe.failures;
    if(mac_window_open(&rejected,0,48,0,"invalid empty probe")||rejected.state)++probe.failures;
    if(!mac_window_open(&window,64,48,0,"OpenCE offscreen host probe")) {
        fprintf(stderr,"could not create accelerated CGL offscreen framebuffer\n");return 1;
    }
    printf("GL_RENDERER=%s\n",(const char *)glGetString(GL_RENDERER));
    printf("CGL_accelerated=%d\n",mac_window_is_accelerated(&window));
    if(!mac_window_is_accelerated(&window))++probe.failures;
    if(mac_window_is_visible(&window))++probe.failures;
    platform_video_window_size(&width,&height);
    if(width!=64||height!=48||halo_screen_width()!=64)++probe.failures;
    if(!mac_window_run(&window,draw_frame,&probe)||!mac_window_frame(&window,draw_frame,&probe))++probe.failures;
    if(!mac_window_resize_active(80,60))++probe.failures;
    platform_video_window_size(&width,&height);
    if(width!=80||height!=60||halo_screen_width()!=80)++probe.failures;
    if(!mac_window_resize_active(64,48))++probe.failures;
    platform_video_window_size(&width,&height);
    if(width!=64||height!=48||halo_screen_width()!=64)++probe.failures;
    if(!mac_window_frame(&window,draw_frame,&probe))++probe.failures;
    platform_request_quit();
    if(mac_window_frame(&window,draw_frame,&probe)||probe.frames!=3)++probe.failures;
    mac_window_close(&window);
    platform_video_window_size(&width,&height);
    if(window.state||width||height||halo_screen_width()!=640||probe.frames!=3||probe.failures) {
        fprintf(stderr,"mac window offscreen depth test: frames=%u failures=%d state=%p\n",probe.frames,probe.failures,window.state);return 1;
    }
    printf("mac window CGL offscreen: %u frames, accelerated renderer, attached depth/stencil readback after resize and near-over-far occlusion, no desktop window\n",probe.frames);
    return 0;
}
