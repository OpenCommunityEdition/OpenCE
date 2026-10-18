#include "mac_loading.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && !defined(__LP64__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#define MASK_WIDTH 128
#define MASK_HEIGHT 16
#define LOADING_PI 3.14159265358979
/* (the Xbox's band is the loading picture, which retail discs do not have, lit by the mask: a
   plain light here, dimmer than the mask itself, which the fade layer builds up on) */
#define BAND_LEVEL 0.16f

static struct {
    int started;
    unsigned *base;
    unsigned base_width,base_height;
    GLuint feedback,mask;
    int feedback_ready;
    /* progress_bar.c's wobble_phase and last_t */
    float wobble,last;
} loading;

void mac_loading_start(const unsigned *pixels,unsigned width,unsigned height,unsigned pitch_pixels)
{
    unsigned y;
    free(loading.base);
    memset(&loading,0,sizeof(loading));
    if(pixels&&width&&height&&width<=4096&&height<=4096&&pitch_pixels>=width) {
        loading.base=(unsigned *)malloc((size_t)width*height*sizeof(unsigned));
        if(loading.base) {
            /* (rows from the bottom, as the framebuffer's) */
            for(y=0;y<height;++y)
                memcpy(loading.base+(size_t)y*width,pixels+(size_t)(height-1-y)*pitch_pixels,(size_t)width*sizeof(unsigned));
            loading.base_width=width;loading.base_height=height;
        }
    }
    loading.started=1;
}

/* the band's mask: progress_bar_create_mask_texture's intensity, a lozenge that is brightest in
   the middle of its width and height, over a faint level */
static void make_mask(void)
{
    unsigned char mask[MASK_WIDTH*MASK_HEIGHT];
    int x,y;
    for(y=0;y<MASK_HEIGHT;++y) {
        double glow=pow(sin(LOADING_PI*(y*(1.0/15.0))),0.75);
        for(x=0;x<MASK_WIDTH;++x) {
            double value=(sin(LOADING_PI*(x*(1.0/127.0)))*glow*(250.0/255.0)+5.0/255.0)*255.0;
            mask[y*MASK_WIDTH+x]=(unsigned char)(value<0?0:value>255?255:value);
        }
    }
    glGenTextures(1,&loading.mask);
    glBindTexture(GL_TEXTURE_2D,loading.mask);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE8,MASK_WIDTH,MASK_HEIGHT,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,mask);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    {
        /* (the Xbox's mask has a border of 5/255 outside it) */
        GLfloat border[4];
        border[0]=border[1]=border[2]=border[3]=5.0f/255.0f;
        glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
    }
}

static void make_feedback(unsigned width,unsigned height)
{
    glGenTextures(1,&loading.feedback);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,loading.feedback);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    if(loading.base&&loading.base_width==width&&loading.base_height==height)
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA8,(GLsizei)width,(GLsizei)height,0,GL_BGRA,
            GL_UNSIGNED_INT_8_8_8_8_REV,loading.base);
    else {
        unsigned char *black=(unsigned char *)calloc((size_t)width*height,4);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA8,(GLsizei)width,(GLsizei)height,0,GL_RGBA,
            GL_UNSIGNED_BYTE,black);
        free(black);
    }
    free(loading.base);
    loading.base=NULL;
    loading.feedback_ready=1;
}

static void write_shot(unsigned width,unsigned height,const char *path)
{
    unsigned char *pixels;
    unsigned y;
    FILE *file;
    pixels=(unsigned char *)malloc((size_t)width*height*3);
    if(!pixels)return;
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,(GLsizei)width,(GLsizei)height,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    file=fopen(path,"wb");
    if(file) {
        fprintf(file,"P6\n%u %u\n255\n",width,height);
        for(y=height;y>0;--y)fwrite(pixels+(size_t)(y-1)*width*3,1,(size_t)width*3,file);
        fclose(file);
    }
    free(pixels);
}

void mac_loading_draw(unsigned width,unsigned height,float progress,const char *shot_path)
{
    GLhandleARB old_program;
    float w=(float)width,h=(float)height,t,tone,alpha,position,blur=0.1f;
    int pass;
    static const float taps[4][2]={{-1.0f,1.0f},{-1.0f,-1.0f},{1.0f,-1.0f},{1.0f,1.0f}};
    if(!loading.started||!width||!height)return;
    old_program=glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
    glOrtho(0.0,(double)width,0.0,(double)height,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    glUseProgramObjectARB(0);
    glDisable(GL_VERTEX_PROGRAM_ARB);glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_COLOR_LOGIC_OP);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    if(!loading.mask)make_mask();
    if(!loading.feedback_ready)make_feedback(width,height);

    /* progress_bar_render: the progress follows the one given, with a wobble */
    loading.wobble+=0.01f;
    t=(float)((sin(loading.wobble)*0.05+progress-loading.last)*0.2+loading.last);
    if(t>1.0f)t=1.0f;
    loading.last=t;
    tone=(float)pow(sin(t*LOADING_PI),0.9);
    if(tone<0.0f)tone=0.0f;
    alpha=t<0.9f?0.9f:0.9f-(t-0.9f)*9.0f;
    if(alpha<0.0f)alpha=0.0f;

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    /* the fade layer: the last frame, four taps a tenth of a texel apart, 254/255 and 254/255 and 1 of it */
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,loading.feedback);
    glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
    glColor4f(alpha*0.25f*254.0f/255.0f,alpha*0.25f*254.0f/255.0f,alpha*0.25f,1.0f);
    for(pass=0;pass<4;++pass) {
        float dx=taps[pass][0]*blur,dy=taps[pass][1]*blur;
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f+dx,0.0f+dy);glVertex2f(0.0f,0.0f);
        glTexCoord2f(w+dx,0.0f+dy);glVertex2f(w,0.0f);
        glTexCoord2f(w+dx,h+dy);glVertex2f(w,h);
        glTexCoord2f(0.0f+dx,h+dy);glVertex2f(0.0f,h);
        glEnd();
    }
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    /* the band: the mask, 128 pixels wide and the screen high, moving across as the load does */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,loading.mask);
    glColor4f(0.55f*BAND_LEVEL*tone,0.75f*BAND_LEVEL*tone,BAND_LEVEL*tone,1.0f);
    position=t*768.0f-64.0f-64.0f;
    glBegin(GL_QUADS);
    glTexCoord2f((0.0f-position)/(float)MASK_WIDTH,0.0f);glVertex2f(0.0f,h);
    glTexCoord2f((w-position)/(float)MASK_WIDTH,0.0f);glVertex2f(w,h);
    glTexCoord2f((w-position)/(float)MASK_WIDTH,1.0f);glVertex2f(w,0.0f);
    glTexCoord2f((0.0f-position)/(float)MASK_WIDTH,1.0f);glVertex2f(0.0f,0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    /* what the next frame fades */
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,loading.feedback);
    glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,0,0,(GLsizei)width,(GLsizei)height);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    if(shot_path&&*shot_path)write_shot(width,height,shot_path);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glBindTexture(GL_TEXTURE_2D,0);
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glMatrixMode(GL_PROJECTION);glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
    glUseProgramObjectARB(old_program);
    while(glGetError()!=GL_NO_ERROR){}
}

float mac_loading_progress(void)
{
    return loading.last;
}

void mac_loading_stop(void)
{
    if(loading.feedback)glDeleteTextures(1,&loading.feedback);
    if(loading.mask)glDeleteTextures(1,&loading.mask);
    free(loading.base);
    memset(&loading,0,sizeof(loading));
}
#else
void mac_loading_start(const unsigned *pixels,unsigned width,unsigned height,unsigned pitch_pixels)
{(void)pixels;(void)width;(void)height;(void)pitch_pixels;}
void mac_loading_draw(unsigned width,unsigned height,float progress,const char *shot_path)
{(void)width;(void)height;(void)progress;(void)shot_path;}
float mac_loading_progress(void){return 0.0f;}
void mac_loading_stop(void){}
#endif
