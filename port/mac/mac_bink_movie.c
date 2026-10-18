#include "mac_bink_movie.h"
#include "mac_input.h"
#include "mac_gamepad.h"
#include "mac_window.h"
#include "mac_audio_output.h"
#include "../linux/src/bink_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#if defined(__APPLE__) && !defined(__LP64__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <libkern/OSAtomic.h>

extern int config_boolean(const char *name);
extern const char *config_string(const char *name);
extern double config_real(const char *name);

#define KEY_ESCAPE 53
#define BINK_ATTRIB_BITS (GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_DEPTH_BUFFER_BIT|GL_VIEWPORT_BIT|GL_TRANSFORM_BIT|GL_SCISSOR_BIT|GL_STENCIL_BUFFER_BIT|GL_LIGHTING_BIT|GL_FOG_BIT|GL_POLYGON_BIT|GL_PIXEL_MODE_BIT)
/* the picture is presented again at least this often (seconds), so that the window's events are served */
#define BINK_IDLE_PRESENT 0.1
#define BINK_POLL_MICROSECONDS 2000
/* the audio unit's rate and the sound ring's frames (a power of two) */
#define BINK_OUTPUT_RATE 48000.0
#define BINK_RING_FRAMES 65536u
#define BINK_MAXIMUM_EXECUTABLE (16UL<<20)

struct bink_player {
    struct bink_decoder *decoder;
    const struct bink_info *info;
    FILE *file;
    unsigned char *pixels;
    GLuint texture;
    int texture_ready;
    /* the picture's frame numbers: the one on show, and the one the clock calls for */
    unsigned long shown,target;
    int have_first,done,skip,skip_all,have_input;
    double started,period,presented;
    unsigned char keys[128];
    unsigned mouse_buttons,pad_buttons;
    unsigned char pad_analog[MAC_GAMEPAD_ANALOG_COUNT];
    /* the sound: the decoder's frames of 16-bit stereo go to the ring here and
       the audio unit's thread takes them, resampled to its rate; only this
       thread writes write_at and only that one reads read_at */
    struct mac_audio_output output;
    int audio_open,audio_started;
    short *ring;
    volatile unsigned write_at,read_at;
    double phase,step;
    float volume;
};

static struct bink_tables bink_tables_loaded;
static int bink_tables_tried=0,bink_tables_ok=0;

static double bink_now(void)
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return (double)now.tv_sec+(double)now.tv_usec/1000000.0;
}

static int bink_read(void *context,unsigned long offset,void *buffer,unsigned long size)
{
    FILE *file=(FILE *)context;
    return fseek(file,(long)offset,SEEK_SET)==0&&fread(buffer,1,size,file)==size;
}

/* the decoder's tables, from the disc's executable (once) */
static int bink_load_tables(const char *root)
{
    char path[1100],error[160];
    FILE *file;
    unsigned char *image;
    long size;
    if(bink_tables_tried)return bink_tables_ok;
    bink_tables_tried=1;
    snprintf(path,sizeof(path),"%s/default.xbe",root);
    file=fopen(path,"rb");
    if(!file)return 0;
    fseek(file,0,SEEK_END);
    size=ftell(file);
    fseek(file,0,SEEK_SET);
    image=size>0&&(unsigned long)size<=BINK_MAXIMUM_EXECUTABLE?(unsigned char *)malloc((size_t)size):NULL;
    if(image&&fread(image,1,(size_t)size,file)==(size_t)size) {
        bink_tables_ok=bink_tables_from_xbe(image,(unsigned long)size,&bink_tables_loaded,error,(int)sizeof(error));
        if(!bink_tables_ok)fprintf(stderr,"movie: %s is not usable for Bink: %s\n",path,error);
    }
    free(image);
    fclose(file);
    return bink_tables_ok;
}

/* what the sound ring holds, in frames */
static unsigned bink_ring_frames_held(const struct bink_player *p)
{
    return (unsigned)(p->write_at-p->read_at);
}

/* the audio unit's thread: stereo floats at its own rate, from the ring at
   the movie's (linear interpolation); silence when the ring runs dry */
static int bink_audio_fill(void *context,float *stereo,uint32_t frames)
{
    struct bink_player *p=(struct bink_player *)context;
    uint32_t index;
    for(index=0;index<frames;++index) {
        unsigned held=bink_ring_frames_held(p);
        unsigned whole=(unsigned)p->phase;
        if(whole+1>=held||p->volume<=0.0f) {
            stereo[index*2]=0.0f;stereo[index*2+1]=0.0f;
            continue;
        }
        {
            float fraction=(float)(p->phase-(double)whole);
            unsigned first=(p->read_at+whole)&(BINK_RING_FRAMES-1);
            unsigned second=(first+1)&(BINK_RING_FRAMES-1);
            float left=(float)p->ring[first*2]+fraction*(float)(p->ring[second*2]-p->ring[first*2]);
            float right=(float)p->ring[first*2+1]+fraction*(float)(p->ring[second*2+1]-p->ring[first*2+1]);
            stereo[index*2]=left*(1.0f/32768.0f)*p->volume;
            stereo[index*2+1]=right*(1.0f/32768.0f)*p->volume;
        }
        p->phase+=p->step;
        {
            /* (whole frames used: given back to the writer) */
            unsigned used=(unsigned)p->phase;
            if(used) {
                p->read_at+=used;
                p->phase-=(double)used;
            }
        }
    }
    return 1;
}

/* the decoder's audio, taken into the ring (what the ring cannot hold is dropped) */
static void bink_audio_feed(struct bink_player *p)
{
    short chunk[2048];
    unsigned long frames;
    if(!p->ring)return;
    while(bink_audio_available(p->decoder)&&(frames=bink_take_audio(p->decoder,chunk,1024))>0) {
        unsigned long index;
        unsigned at=p->write_at;
        for(index=0;index<frames;++index) {
            if((unsigned)(at-p->read_at)>=BINK_RING_FRAMES-2)break;
            p->ring[(at&(BINK_RING_FRAMES-1))*2]=chunk[index*2];
            p->ring[(at&(BINK_RING_FRAMES-1))*2+1]=chunk[index*2+1];
            ++at;
        }
        OSMemoryBarrier();
        p->write_at=at;
    }
}

static void bink_audio_open(struct bink_player *p)
{
    long status=0;
    const char *device=config_string("audio.output_device");
    double volume=config_real("audio.volume");
    if(p->info->sample_rate==0||p->info->channels!=2||volume<=0.0)return;
    p->ring=(short *)calloc(BINK_RING_FRAMES*2,sizeof(short));
    if(!p->ring)return;
    p->step=(double)p->info->sample_rate/BINK_OUTPUT_RATE;
    p->volume=volume>1.0?1.0f:(float)volume;
    memset(&p->output,0,sizeof(p->output));
    if(!mac_audio_output_open_device(&p->output,bink_audio_fill,p,&status,device)) {
        fprintf(stderr,"movie: no audio output (%ld); the movie is silent\n",status);
        free(p->ring);p->ring=NULL;
        return;
    }
    p->audio_open=1;
}

/* a key, a click or a pad button newly down, against the last frame (as
   mac_movie.c's, for the QuickTime movies) */
static void bink_read_input(struct bink_player *p,const struct mac_input_frame *input)
{
    struct mac_gamepad_frame pad;
    unsigned index,pressed=0;
    memset(&pad,0,sizeof(pad));
    mac_gamepad_poll(&pad);
    if(!p->have_input) {
        memcpy(p->keys,input->keys,sizeof(p->keys));
        p->mouse_buttons=input->mouse_buttons;
        p->pad_buttons=pad.connected?pad.buttons:0;
        for(index=0;index<MAC_GAMEPAD_ANALOG_COUNT;++index)p->pad_analog[index]=pad.connected?pad.analog[index]:0;
        p->have_input=1;
        return;
    }
    for(index=0;index<sizeof(p->keys);++index) {
        if(input->keys[index]&&!p->keys[index]) {
            pressed=1;
            if(index==KEY_ESCAPE)p->skip_all=1;
        }
        p->keys[index]=input->keys[index];
    }
    if(input->mouse_buttons&~p->mouse_buttons)pressed=1;
    if(input->left_clicks||input->right_clicks)pressed=1;
    p->mouse_buttons=input->mouse_buttons;
    if(pad.connected) {
        if(pad.buttons&~p->pad_buttons)pressed=1;
        if(pad.buttons&MAC_GAMEPAD_START&&!(p->pad_buttons&MAC_GAMEPAD_START))p->skip_all=1;
        p->pad_buttons=pad.buttons;
        for(index=0;index<MAC_GAMEPAD_ANALOG_COUNT;++index) {
            if(pad.analog[index]>=128&&p->pad_analog[index]<128)pressed=1;
            p->pad_analog[index]=pad.analog[index];
        }
    }
    if(pressed||p->skip_all)p->skip=1;
}

/* decodes the frames up to the one the clock calls for and makes the last one
   the picture; the frames that are late are decoded and not drawn */
static void bink_advance(struct bink_player *p)
{
    char error[160];
    unsigned long last=0;
    int decoded=0;
    while(bink_next_frame(p->decoder)<=p->target&&bink_next_frame(p->decoder)<p->info->frames) {
        if(!bink_decode_frame(p->decoder,error,(int)sizeof(error))) {
            fprintf(stderr,"movie: Bink movie stopped: %s\n",error);
            p->done=1;
            break;
        }
        last=bink_next_frame(p->decoder)-1;
        decoded=1;
        bink_audio_feed(p);
    }
    if(decoded) {
        bink_copy_to_buffer(p->decoder,p->pixels,(long)p->info->width*4,p->info->height,0,0);
        p->shown=last;
        p->texture_ready=0;
    }
    if(bink_next_frame(p->decoder)>=p->info->frames&&p->target>=p->info->frames)p->done=1;
}

/* the picture over the whole window in its aspect, in the fixed pipeline with the game's state saved and restored */
static void bink_draw(struct bink_player *p,unsigned width,unsigned height)
{
    GLhandleARB old_program=glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    float picture_width=(float)p->info->width,picture_height=(float)p->info->height;
    float fit_width=(float)width,fit_height=(float)height,left,bottom,right,top;
    glPushAttrib(BINK_ATTRIB_BITS);
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
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if(fit_width/fit_height>picture_width/picture_height)fit_width=fit_height*picture_width/picture_height;
    else fit_height=fit_width*picture_height/picture_width;
    left=((float)width-fit_width)*0.5f;right=left+fit_width;
    bottom=((float)height-fit_height)*0.5f;top=bottom+fit_height;
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    if(!p->texture)glGenTextures(1,&p->texture);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,p->texture);
    if(!p->have_first) {
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT,4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGB,(GLsizei)p->info->width,(GLsizei)p->info->height,0,
            GL_BGRA,GL_UNSIGNED_BYTE,p->pixels);
        p->have_first=1;
        p->texture_ready=1;
    } else if(!p->texture_ready) {
        glPixelStorei(GL_UNPACK_ALIGNMENT,4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,(GLsizei)p->info->width,(GLsizei)p->info->height,
            GL_BGRA,GL_UNSIGNED_BYTE,p->pixels);
        p->texture_ready=1;
    }
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f,picture_height);glVertex2f(left,bottom);
    glTexCoord2f(picture_width,picture_height);glVertex2f(right,bottom);
    glTexCoord2f(picture_width,0.0f);glVertex2f(right,top);
    glTexCoord2f(0.0f,0.0f);glVertex2f(left,top);
    glEnd();
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glMatrixMode(GL_PROJECTION);glPopMatrix();
    glPopAttrib();
    glUseProgramObjectARB(old_program);
    while(glGetError()!=GL_NO_ERROR){}
}

static int bink_frame(void *opaque,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    struct bink_player *p=(struct bink_player *)opaque;
    int window_width=0,window_height=0;
    bink_read_input(p,input);
    if(!p->started) {
        /* the first picture, and the clock and the sound start with it */
        p->target=0;
        bink_advance(p);
        p->started=bink_now();
        if(p->audio_open&&mac_audio_output_start(&p->output,NULL))p->audio_started=1;
    } else if(p->target>p->shown||!p->have_first) {
        bink_advance(p);
    }
    platform_video_window_size(&window_width,&window_height);
    if(window_width>0&&window_height>0){width=(unsigned)window_width;height=(unsigned)window_height;}
    bink_draw(p,width,height);
    if(p->skip)p->done=1;
    if(p->done&&p->texture) {
        glDeleteTextures(1,&p->texture);
        p->texture=0;
    }
    return 1;
}

int mac_bink_movie_play(const char *data_root,const char *name,mac_movie_frame_runner runner,int visible)
{
    struct bink_player p;
    const char *root=getenv("HALO_MAC_XBOX_DATA");
    char path[1100],error[160];
    struct stat st;
    int window_open=1;
    if(!root||!*root)root=data_root;
    if(!root||!*root||!name||!runner||!config_boolean("game.movies"))return -1;
    if(!visible) {
        const char *hidden=getenv("HALO_MAC_MOVIES_HIDDEN");
        if(!hidden||hidden[0]!='1'||hidden[1])return -1;
    }
    snprintf(path,sizeof(path),"%s/bink/%s",root,name);
    if(stat(path,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<=0)return -1;
    if(!bink_load_tables(root))return -1;
    memset(&p,0,sizeof(p));
    p.file=fopen(path,"rb");
    if(!p.file)return -1;
    p.decoder=bink_open(&bink_tables_loaded,bink_read,p.file,(unsigned long)st.st_size,error,(int)sizeof(error));
    if(!p.decoder) {
        fprintf(stderr,"movie: %s cannot be played: %s\n",path,error);
        fclose(p.file);
        return -1;
    }
    p.info=bink_get_info(p.decoder);
    p.pixels=(unsigned char *)malloc((size_t)p.info->width*p.info->height*4);
    if(!p.pixels) {
        bink_close(p.decoder);fclose(p.file);
        return -1;
    }
    p.period=(double)p.info->frame_rate_divisor/(double)p.info->frame_rate;
    fprintf(stderr,"movie: playing %s (%lux%lu, %lu frames, Xbox disc's Bink)\n",path,p.info->width,p.info->height,
        p.info->frames);
    bink_audio_open(&p);
    while(!p.done) {
        double now=bink_now();
        if(p.started>0.0) {
            p.target=(unsigned long)((now-p.started)/p.period);
            if(p.presented>0.0&&p.target<=p.shown&&now-p.presented<BINK_IDLE_PRESENT) {
                usleep(BINK_POLL_MICROSECONDS);
                continue;
            }
        }
        if(!runner(bink_frame,&p)){window_open=0;break;}
        p.presented=bink_now();
    }
    if(p.audio_open) {
        if(p.audio_started)mac_audio_output_stop(&p.output,NULL);
        mac_audio_output_close(&p.output,NULL);
    }
    free(p.ring);
    free(p.pixels);
    bink_close(p.decoder);
    fclose(p.file);
    if(!window_open)return 0;
    return 1;
}
#else
int mac_bink_movie_play(const char *data_root,const char *name,mac_movie_frame_runner runner,int visible)
{
    (void)data_root;(void)name;(void)runner;(void)visible;
    return -1;
}
#endif
