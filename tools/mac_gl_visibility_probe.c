#include "mac_window.h"
#include "mac_gl_visibility.h"
#include <OpenGL/gl.h>
#include <stdio.h>
#include <string.h>

static unsigned checks,failures;
#define CHECK(x) do { ++checks; if(!(x)) { ++failures; fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__); } } while(0)

static void draw_fullscreen(float z)
{
    glBegin(GL_QUADS);
    glVertex3f(-1.0f,-1.0f,z);glVertex3f(1.0f,-1.0f,z);
    glVertex3f(1.0f,1.0f,z);glVertex3f(-1.0f,1.0f,z);
    glEnd();
}

static void probe_color_only_visibility(void)
{
    GLint old_framebuffer=0,old_texture=0,old_viewport[4];
    GLuint framebuffer=0,texture=0,depth=0;
    uint32_t samples=0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&old_framebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_texture);
    glGetIntegerv(GL_VIEWPORT,old_viewport);
    glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,16,8,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glGenFramebuffersEXT(1,&framebuffer);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,framebuffer);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,texture,0);
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
    glViewport(0,0,16,8);glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK(mac_gl_visibility_begin());draw_fullscreen(0.0f);
    CHECK(mac_gl_visibility_end(19,1.0f)==MAC_GL_VISIBILITY_OK);glFinish();
    CHECK(mac_gl_visibility_result(19,&samples,NULL)==MAC_GL_VISIBILITY_OK&&samples>0);
    printf("visibility color-only depth-disabled samples=%u\n",(unsigned)samples);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_FALSE);samples=0;
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK(mac_gl_visibility_begin());draw_fullscreen(0.0f);
    CHECK(mac_gl_visibility_end(20,1.0f)==MAC_GL_VISIBILITY_OK);glFinish();
    CHECK(mac_gl_visibility_result(20,&samples,NULL)==MAC_GL_VISIBILITY_OK&&samples>0);
    printf("visibility color-only depth-always samples=%u\n",(unsigned)samples);
    glGenRenderbuffersEXT(1,&depth);glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depth);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,16,8);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depth);
    CHECK(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
    glDisable(GL_DEPTH_TEST);samples=0;glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    CHECK(mac_gl_visibility_begin());draw_fullscreen(0.0f);
    CHECK(mac_gl_visibility_end(21,1.0f)==MAC_GL_VISIBILITY_OK);glFinish();
    CHECK(mac_gl_visibility_result(21,&samples,NULL)==MAC_GL_VISIBILITY_OK&&samples>0);
    printf("visibility depth-attached depth-disabled samples=%u\n",(unsigned)samples);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_FALSE);samples=0;
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    CHECK(mac_gl_visibility_begin());draw_fullscreen(0.0f);
    CHECK(mac_gl_visibility_end(22,1.0f)==MAC_GL_VISIBILITY_OK);glFinish();
    CHECK(mac_gl_visibility_result(22,&samples,NULL)==MAC_GL_VISIBILITY_OK&&samples>0);
    printf("visibility depth-attached depth-always samples=%u\n",(unsigned)samples);
    glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);glEnable(GL_DEPTH_TEST);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,(GLuint)old_framebuffer);
    glViewport(old_viewport[0],old_viewport[1],old_viewport[2],old_viewport[3]);
    glBindTexture(GL_TEXTURE_2D,(GLuint)old_texture);
    glDeleteRenderbuffersEXT(1,&depth);glDeleteFramebuffersEXT(1,&framebuffer);glDeleteTextures(1,&texture);
    CHECK(glGetError()==GL_NO_ERROR);
}

static int run_probe(void)
{
    unsigned int query_ids[4097];
    uint32_t samples=0;uint64_t timestamp=99;
    glViewport(0,0,32,32);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);glClearDepth(1.0);glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    CHECK(mac_gl_visibility_start());
    CHECK(!mac_gl_visibility_start());
    CHECK(mac_gl_visibility_end(0,1.0f)==MAC_GL_VISIBILITY_INVALID);
    CHECK(mac_gl_visibility_begin());
    CHECK(!mac_gl_visibility_begin());
    draw_fullscreen(0.0f);
    CHECK(mac_gl_visibility_end(17,4.0f)==MAC_GL_VISIBILITY_OK);
    glFinish();
    CHECK(mac_gl_visibility_result(17,&samples,&timestamp)==MAC_GL_VISIBILITY_OK);
    CHECK(timestamp==0&&samples==256);
    glClearDepth(1.0);glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    draw_fullscreen(-0.5f);
    CHECK(mac_gl_visibility_begin());
    draw_fullscreen(0.5f);
    CHECK(mac_gl_visibility_end(18,1.0f)==MAC_GL_VISIBILITY_OK);
    glFinish();samples=99;timestamp=99;
    CHECK(mac_gl_visibility_result(18,&samples,&timestamp)==MAC_GL_VISIBILITY_OK);
    CHECK(samples==0&&timestamp==0);
    probe_color_only_visibility();
    glClearDepth(1.0);glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    CHECK(mac_gl_visibility_begin());draw_fullscreen(0.0f);
    CHECK(mac_gl_visibility_end(17,1.0f)==MAC_GL_VISIBILITY_OK);glFinish();samples=0;
    CHECK(mac_gl_visibility_result(17,&samples,&timestamp)==MAC_GL_VISIBILITY_OK&&samples==1024);
    CHECK(mac_gl_visibility_result(4096+17,&samples,&timestamp)==MAC_GL_VISIBILITY_OK&&samples==1024);
    CHECK(mac_gl_visibility_test_query_ids(query_ids,4097)==4097);
    CHECK(glIsQueryARB(query_ids[17])&&glIsQueryARB(query_ids[18])&&glIsQueryARB(query_ids[4096]));
    CHECK(mac_gl_visibility_begin());
    mac_gl_visibility_stop();
    CHECK(!glIsQueryARB(query_ids[17])&&!glIsQueryARB(query_ids[18])&&!glIsQueryARB(query_ids[4096]));
    CHECK(!mac_gl_visibility_begin());
    CHECK(mac_gl_visibility_result(17,&samples,&timestamp)==MAC_GL_VISIBILITY_UNAVAILABLE);
    CHECK(glGetError()==GL_NO_ERROR);
    return failures==0;
}

static int frame(void *context,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    int *result=(int *)context;
    if(!input||width!=32||height!=32){*result=0;return 0;}
    *result=run_probe();return *result;
}

int main(void)
{
    struct mac_window window;int result=0;
    memset(&window,0,sizeof(window));
    if(!mac_window_open(&window,32,32,0,"OpenCE visibility query probe")) {
        fprintf(stderr,"hidden CGL offscreen context unavailable\n");return 2;
    }
    printf("GL_RENDERER=%s\n",(const char *)glGetString(GL_RENDERER));
    printf("GL_ARB_occlusion_query=%s\n",strstr((const char *)glGetString(GL_EXTENSIONS),"GL_ARB_occlusion_query")?"present":"absent");
    CHECK(mac_window_is_accelerated(&window));
    CHECK(!mac_window_is_visible(&window));
    if(!mac_window_frame(&window,frame,&result))result=0;
    mac_window_close(&window);
    if(!result||failures){fprintf(stderr,"mac visibility probe: %u checks, %u failures\n",checks,failures);return 1;}
    printf("mac GL visibility queries: %u checks, 0 failures; visible/full and depth-occluded samples, slot replacement and scaling verified in hidden context\n",checks);
    return 0;
}
