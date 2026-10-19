#include "mac_loading.h"
#include "mac_window.h"
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

extern int config_boolean(const char *name);

static struct {
    int started;
    unsigned *base;
    unsigned base_width,base_height;
    GLuint feedback,mask;
    int feedback_ready;
    unsigned feedback_width,feedback_height;
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
        /* port: the last picture was the game's own size (kept in its shape, centred, as the present
           step places it in a larger window) and the window is not: fitted into black of the window's */
        unsigned *fitted=(unsigned *)calloc((size_t)width*height,4);
        if(fitted&&loading.base&&loading.base_width&&loading.base_height) {
            unsigned fit_width=width,fit_height=(unsigned)((unsigned long long)width*loading.base_height/loading.base_width);
            unsigned left,bottom,x,y;
            if(fit_height>height){fit_height=height;fit_width=(unsigned)((unsigned long long)height*loading.base_width/loading.base_height);}
            if(!fit_width)fit_width=1;
            if(!fit_height)fit_height=1;
            left=(width-fit_width)/2;bottom=(height-fit_height)/2;
            for(y=0;y<fit_height;++y) {
                const unsigned *row=loading.base+(size_t)((2*(unsigned long long)y+1)*loading.base_height/(2*fit_height))*loading.base_width;
                unsigned *out=fitted+(size_t)(bottom+y)*width+left;
                for(x=0;x<fit_width;++x)out[x]=row[(2*(unsigned long long)x+1)*loading.base_width/(2*fit_width)];
            }
        }
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA8,(GLsizei)width,(GLsizei)height,0,GL_BGRA,
            GL_UNSIGNED_INT_8_8_8_8_REV,fitted);
        free(fitted);
    }
    loading.feedback_width=width;loading.feedback_height=height;
    free(loading.base);
    loading.base=NULL;
    loading.feedback_ready=1;
}

/* The indicator (mac.loading_indicator): "LOADING" in 5x7 block letters drawn here (no font is
   loaded on this screen), a ring of dots turning, and a thin bar of the progress. All sizes are
   of the window's height over 480 lines. */
static const unsigned char indicator_glyphs[7][7]={
    /* L */ {0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
    /* O */ {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},
    /* A */ {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},
    /* D */ {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
    /* I */ {0x0e,0x04,0x04,0x04,0x04,0x04,0x0e},
    /* N */ {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    /* G */ {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f}
};
static void draw_indicator(float w,float h,float unit,float t)
{
    float cell=2.5f*unit,gap=cell*1.5f,text_width=7.0f*(5.0f*cell)+6.0f*gap;
    float left=(w-text_width)*0.5f,base=h*0.5f-3.5f*cell-48.0f*unit,dot_size=4.0f*unit;
    float centre_x=w*0.5f,centre_y=h*0.5f+14.0f*unit,radius=22.0f*unit;
    float bar_left=w*0.5f-120.0f*unit,bar_right=w*0.5f+120.0f*unit,bar_y=base-28.0f*unit,bar_h=3.0f*unit;
    int letter,row,column,dot;
    glDisable(GL_TEXTURE_2D);glDisable(GL_TEXTURE_RECTANGLE_ARB);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.85f,0.92f,1.0f,0.9f);
    glBegin(GL_QUADS);
    for(letter=0;letter<7;++letter) {
        const unsigned char *rows=indicator_glyphs[letter];
        float x0=left+(float)letter*(5.0f*cell+gap);
        for(row=0;row<7;++row)for(column=0;column<5;++column) {
            if(rows[row]&(0x10>>column)) {
                float x=x0+(float)column*cell,y=base+(float)(6-row)*cell;
                glVertex2f(x,y);glVertex2f(x+cell,y);glVertex2f(x+cell,y+cell);glVertex2f(x,y+cell);
            }
        }
    }
    /* (twelve dots; the head of the turn is the brightest, the tail fades) */
    for(dot=0;dot<12;++dot) {
        float turn=loading.wobble*40.0f; /* (the wobble advances 0.01 a frame) */
        int head=(int)turn%12;
        int age=(head-dot+12)%12;
        float angle=(float)dot*(float)(2.0*LOADING_PI/12.0);
        float x=centre_x+(float)sin(angle)*radius,y=centre_y+(float)cos(angle)*radius;
        glColor4f(0.85f,0.92f,1.0f,1.0f-(float)age*0.08f);
        glVertex2f(x-dot_size,y-dot_size);glVertex2f(x+dot_size,y-dot_size);
        glVertex2f(x+dot_size,y+dot_size);glVertex2f(x-dot_size,y+dot_size);
    }
    glColor4f(0.85f,0.92f,1.0f,0.25f);
    glVertex2f(bar_left,bar_y);glVertex2f(bar_right,bar_y);glVertex2f(bar_right,bar_y+bar_h);glVertex2f(bar_left,bar_y+bar_h);
    glColor4f(0.85f,0.92f,1.0f,0.9f);
    {
        float fill=bar_left+(bar_right-bar_left)*(t<0.0f?0.0f:t>1.0f?1.0f:t);
        glVertex2f(bar_left,bar_y);glVertex2f(fill,bar_y);glVertex2f(fill,bar_y+bar_h);glVertex2f(bar_left,bar_y+bar_h);
    }
    glEnd();
    glDisable(GL_BLEND);
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
    float w,h,unit,t,tone,alpha,position,blur=0.1f;
    int pass,window_width=0,window_height=0;
    static const float taps[4][2]={{-1.0f,1.0f},{-1.0f,-1.0f},{1.0f,-1.0f},{1.0f,1.0f}};
    if(!loading.started||!width||!height)return;
    /* port: over the whole window, as the movies are: the game's picture is 640x480 (or wider), but the
       window's framebuffer is not, and a viewport of the picture's size sits in its lower left corner */
    platform_video_window_size(&window_width,&window_height);
    if(window_width>0&&window_height>0){width=(unsigned)window_width;height=(unsigned)window_height;}
    w=(float)width;h=(float)height;
    unit=h/480.0f; /* (the Xbox's band is 128 pixels wide in its 480 lines) */
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
    if(loading.feedback_ready&&(loading.feedback_width!=width||loading.feedback_height!=height)) {
        /* (the window changed size: the fade starts again from black) */
        glDeleteTextures(1,&loading.feedback);loading.feedback=0;loading.feedback_ready=0;
    }
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
    position=(t*768.0f-64.0f-64.0f)*unit;
    glBegin(GL_QUADS);
    glTexCoord2f((0.0f-position)/(MASK_WIDTH*unit),0.0f);glVertex2f(0.0f,h);
    glTexCoord2f((w-position)/(MASK_WIDTH*unit),0.0f);glVertex2f(w,h);
    glTexCoord2f((w-position)/(MASK_WIDTH*unit),1.0f);glVertex2f(w,0.0f);
    glTexCoord2f((0.0f-position)/(MASK_WIDTH*unit),1.0f);glVertex2f(0.0f,0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    /* what the next frame fades */
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,loading.feedback);
    glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,0,0,(GLsizei)width,(GLsizei)height);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    if(config_boolean("mac.loading_indicator"))draw_indicator(w,h,unit,t);
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
