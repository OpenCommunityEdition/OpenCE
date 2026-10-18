#include "mac_movie.h"
#include "mac_bink_movie.h"
#include "mac_input.h"
#include "mac_gamepad.h"
#include "mac_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dlfcn.h>

#if defined(__APPLE__) && !defined(__LP64__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <QuickTime/QuickTime.h>
#include <CoreVideo/CoreVideo.h>

/* the Xbox game's logos, in its order (the Mac version's MacSoft and
   Gearbox movies are left out: this is a port of the Xbox game) */
static const char *const movie_names[]={"mgs.mov","bungie.mov"};
#define MOVIE_COUNT (sizeof(movie_names)/sizeof(movie_names[0]))
#define KEY_ESCAPE 53
#define MOVIE_ATTRIB_BITS (GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_DEPTH_BUFFER_BIT|GL_VIEWPORT_BIT|GL_TRANSFORM_BIT|GL_SCISSOR_BIT|GL_STENCIL_BUFFER_BIT|GL_LIGHTING_BIT|GL_FOG_BIT|GL_POLYGON_BIT|GL_PIXEL_MODE_BIT|GL_COLOR_BUFFER_BIT)
#define MOVIE_IDLE_PRESENT 0.05
#define MOVIE_POLL_MICROSECONDS 4000
#define MOVIE_PIXELS_DEFAULT "default"

struct movie_player {
    Movie movie;
    QTVisualContextRef context;
    CVOpenGLTextureRef image;
    float natural_width,natural_height;
    int index,done,skip,skip_all,hidden,have_input;
    unsigned shots;
    double started,limit,shot_every,next_shot;
    unsigned char keys[128];
    unsigned mouse_buttons,pad_buttons;
    unsigned char pad_analog[MAC_GAMEPAD_ANALOG_COUNT];
    const char *capture;
    struct movie_stats *stats;
    int step,waiting,legacy;
    double presented;
    TimeValue step_time;
    double step_began;
};

static double movie_now(void)
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return (double)now.tv_sec+(double)now.tv_usec/1000000.0;
}

/* Timing log (HALO_MAC_MOVIE_STATS=1): where each frame's time goes and how
   evenly the movie's images arrive, printed when the movie ends. */
#define STATS_LOOPS 16384
#define STATS_IMAGES 2048
struct movie_stats {
    Track video;
    Media video_media;
    double fps;
    double task[STATS_LOOPS],context[STATS_LOOPS],draw[STATS_LOOPS],loop[STATS_LOOPS];
    unsigned loops;
    double arrival[STATS_IMAGES];
    long sample[STATS_IMAGES];
    unsigned images;
    double last_loop_end;
    double sync_error_max;
    double first_image_at;
    double cpu_at_start;
    int finish;
    double step[STATS_IMAGES];
    unsigned steps;
};

static int stats_compare(const void *a,const void *b)
{
    double x=*(const double *)a,y=*(const double *)b;
    return x<y?-1:(x>y?1:0);
}

static double movie_cpu(void)
{
    struct rusage usage;
    getrusage(RUSAGE_SELF,&usage);
    return (double)(usage.ru_utime.tv_sec+usage.ru_stime.tv_sec)+(double)(usage.ru_utime.tv_usec+usage.ru_stime.tv_usec)/1000000.0;
}

static void stats_line(const char *name,const double *values,unsigned count)
{
    double *sorted,sum=0.0;
    unsigned index;
    if(!count)return;
    sorted=(double *)malloc(count*sizeof(double));
    if(!sorted)return;
    for(index=0;index<count;++index){sorted[index]=values[index];sum+=values[index];}
    qsort(sorted,count,sizeof(double),stats_compare);
    fprintf(stderr,"movie-stats: %-8s mean %7.2f  p50 %7.2f  p95 %7.2f  max %7.2f ms\n",name,
        sum/count*1000.0,sorted[count/2]*1000.0,sorted[(count*95)/100]*1000.0,sorted[count-1]*1000.0);
    free(sorted);
}

static void stats_print(struct movie_player *p,double elapsed)
{
    struct movie_stats *s=p->stats;
    double *gaps;
    unsigned index,late=0,uneven=0;
    long dropped=0,span=0;
    double duration=(double)GetMovieDuration(p->movie)/(double)GetMovieTimeScale(p->movie);
    if(!s)return;
    fprintf(stderr,"movie-stats: movie %d: %u loops in %.2f s (movie %.2f s), %u new images\n",
        p->index,s->loops,elapsed,duration,s->images);
    fprintf(stderr,"movie-stats: cpu %.2f s over %.2f s (%.0f%% of one core)\n",movie_cpu()-s->cpu_at_start,elapsed,
        elapsed>0.0?(movie_cpu()-s->cpu_at_start)/elapsed*100.0:0.0);
    stats_line("MoviesTask",s->task,s->loops);
    stats_line("context",s->context,s->loops);
    stats_line("draw",s->draw,s->loops);
    stats_line("loop",s->loop,s->loops);
    stats_line("step",s->step,s->steps);
    if(s->images>1) {
        gaps=(double *)malloc(s->images*sizeof(double));
        if(gaps) {
            for(index=1;index<s->images;++index) {
                gaps[index-1]=s->arrival[index]-s->arrival[index-1];
                if(gaps[index-1]>0.050)++late;
                if(gaps[index-1]>0.045||gaps[index-1]<0.022)++uneven;
            }
            stats_line("gap",gaps,s->images-1);
            free(gaps);
        }
        span=s->sample[s->images-1]-s->sample[0]+1;
        dropped=span>(long)s->images?span-(long)s->images:0;
        fprintf(stderr,"movie-stats: images %u of %ld samples spanned: %ld dropped, %u gaps over 50 ms, %u gaps off the 33 ms beat; %.1f images/s\n",
            s->images,span,dropped,late,uneven,
            s->arrival[s->images-1]>s->first_image_at?(double)(s->images-1)/(s->arrival[s->images-1]-s->first_image_at):0.0);
    }
    fprintf(stderr,"movie-stats: end-of-movie wall %.3f s vs movie %.3f s (late %+.0f ms); first image after %.0f ms\n",
        elapsed,duration,(elapsed-duration)*1000.0,s->first_image_at*1000.0);
}

/* a key, a click or a pad button newly down, against the last frame (what
   was held as the movie began, such as the launcher's Return, does not count) */
static void movie_read_input(struct movie_player *p,const struct mac_input_frame *input)
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
            /* (the triggers are analog; a press is past half way) */
            if(pad.analog[index]>=128&&p->pad_analog[index]<128)pressed=1;
            p->pad_analog[index]=pad.analog[index];
        }
    }
    if(pressed||p->skip_all)p->skip=1;
}

static void movie_write_shot(struct movie_player *p,unsigned width,unsigned height)
{
    char path[1100];
    unsigned char *pixels,*row;
    FILE *file;
    unsigned y;
    GLint old_alignment=4;
    pixels=(unsigned char *)malloc((size_t)width*height*3);
    row=(unsigned char *)malloc((size_t)width*3);
    if(!pixels||!row){free(pixels);free(row);return;}
    glGetIntegerv(GL_PACK_ALIGNMENT,&old_alignment);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,(GLsizei)width,(GLsizei)height,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    glPixelStorei(GL_PACK_ALIGNMENT,old_alignment);
    snprintf(path,sizeof(path),"%s-%d-%u.ppm",p->capture,p->index,p->shots++);
    file=fopen(path,"wb");
    if(file) {
        fprintf(file,"P6\n%u %u\n255\n",width,height);
        for(y=height;y>0;--y)fwrite(pixels+(size_t)(y-1)*width*3,1,(size_t)width*3,file);
        fclose(file);
        fprintf(stderr,"movie: shot %s\n",path);
    }
    free(pixels);free(row);
}

/* the newest image over the whole window, kept in its aspect (bars above and
   below or at the sides), in the fixed pipeline with the game's state saved
   and restored */
static void movie_draw(struct movie_player *p,unsigned width,unsigned height)
{
    GLhandleARB old_program=glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    float left=0.0f,bottom=0.0f,right=(float)width,top=(float)height;
    glPushAttrib(MOVIE_ATTRIB_BITS);
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
    if(p->image&&p->natural_width>0.0f&&p->natural_height>0.0f) {
        GLenum target=CVOpenGLTextureGetTarget(p->image);
        GLfloat lower_left[2],lower_right[2],upper_right[2],upper_left[2];
        float fit_width=(float)width,fit_height=(float)height;
        float aspect=p->natural_width/p->natural_height;
        if(fit_width/fit_height>aspect)fit_width=fit_height*aspect;
        else fit_height=fit_width/aspect;
        left=((float)width-fit_width)*0.5f;right=left+fit_width;
        bottom=((float)height-fit_height)*0.5f;top=bottom+fit_height;
        CVOpenGLTextureGetCleanTexCoords(p->image,lower_left,lower_right,upper_right,upper_left);
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);glDisable(GL_TEXTURE_RECTANGLE_ARB);
        glEnable(target);
        glBindTexture(target,CVOpenGLTextureGetName(p->image));
        glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(lower_left[0],lower_left[1]);glVertex2f(left,bottom);
        glTexCoord2f(lower_right[0],lower_right[1]);glVertex2f(right,bottom);
        glTexCoord2f(upper_right[0],upper_right[1]);glVertex2f(right,top);
        glTexCoord2f(upper_left[0],upper_left[1]);glVertex2f(left,top);
        glEnd();
        glBindTexture(target,0);
        glDisable(target);
    }
    if(p->capture&&p->shots<64&&movie_now()>=p->next_shot) {
        movie_write_shot(p,width,height);
        p->next_shot=movie_now()+p->shot_every;
    }
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glMatrixMode(GL_PROJECTION);glPopMatrix();
    glPopAttrib();
    glUseProgramObjectARB(old_program);
    while(glGetError()!=GL_NO_ERROR){}
}

static int movie_frame(void *opaque,const struct mac_input_frame *input,unsigned width,unsigned height)
{
    struct movie_player *p=(struct movie_player *)opaque;
    struct movie_stats *s=p->stats;
    double t0=0.0,t1=0.0,t2=0.0,t3=0.0;
    int is_new=0;
    if(s)t0=movie_now();
    movie_read_input(p,input);
    if(p->step&&!p->waiting) {
        /* diagnostic: the next frame only when asked, to time one decode */
        SetMovieTimeValue(p->movie,p->step_time);
        p->step_began=movie_now();
        p->waiting=1;
    }
    if(p->legacy)MoviesTask(p->movie,0);
    if(s)t1=movie_now();
    if(p->legacy)QTVisualContextTask(p->context);
    if(QTVisualContextIsNewImageAvailable(p->context,NULL)) {
        CVImageBufferRef image=NULL;
        if(QTVisualContextCopyImageForTime(p->context,NULL,NULL,&image)==noErr&&image) {
            if(p->image)CVOpenGLTextureRelease(p->image);
            p->image=(CVOpenGLTextureRef)image;
            is_new=1;
            if(p->step&&p->waiting) {
                if(s&&s->steps<STATS_IMAGES)s->step[s->steps++]=movie_now()-p->step_began;
                p->waiting=0;
                p->step_time+=(TimeValue)((double)GetMovieTimeScale(p->movie)/(s?s->fps:30.0));
            }
        }
    }
    if(s)t2=movie_now();
    /* port: over the whole window, which is larger than the game's own 640x480
       picture when display.window_size or display.resolution say so */
    {
        int window_width=0,window_height=0;
        platform_video_window_size(&window_width,&window_height);
        if(window_width>0&&window_height>0){width=(unsigned)window_width;height=(unsigned)window_height;}
    }
    movie_draw(p,width,height);
    if(s) {
        /* (HALO_MAC_MOVIE_STATS=2: finished, so that the time includes the card's
           upload and draw; the wait costs the processor time it measures) */
        if(s->finish)glFinish();
        t3=movie_now();
        if(s->loops<STATS_LOOPS) {
            s->task[s->loops]=t1-t0;s->context[s->loops]=t2-t1;s->draw[s->loops]=t3-t2;
            s->loop[s->loops]=s->last_loop_end>0.0?t3-s->last_loop_end:0.0;
            ++s->loops;
        }
        s->last_loop_end=t3;
        if(is_new&&s->images<STATS_IMAGES) {
            TimeValue media_time=TrackTimeToMediaTime(GetMovieTime(p->movie,NULL),s->video);
            long number=(long)((double)media_time*s->fps/(double)GetMediaTimeScale(s->video_media)+0.5);
            if(!s->images)s->first_image_at=t3-p->started;
            s->arrival[s->images]=t3-p->started;
            s->sample[s->images]=number;
            ++s->images;
        }
    }
    if(p->step&&!p->waiting&&p->step_time>=GetMovieDuration(p->movie))p->done=1;
    if(p->step&&p->waiting&&movie_now()-p->step_began>3.0){fprintf(stderr,"movie-stats: no image for 3 s; stopped\n");p->done=1;}
    if((!p->step&&IsMovieDone(p->movie))||p->skip||(p->limit>0.0&&movie_now()-p->started>=p->limit))p->done=1;
    return 1;
}

/* The context's pixel format (CGLGetPixelFormat, which the 10.4 SDK lacks:
   found at run time), else one chosen to match */
static CGLPixelFormatObj movie_context_format(CGLContextObj cgl)
{
    typedef CGLPixelFormatObj (*get_format_proc)(CGLContextObj);
    get_format_proc get_format=(get_format_proc)dlsym(RTLD_DEFAULT,"CGLGetPixelFormat");
    CGLPixelFormatAttribute attributes[]={kCGLPFAColorSize,(CGLPixelFormatAttribute)24,
        kCGLPFADepthSize,(CGLPixelFormatAttribute)24,(CGLPixelFormatAttribute)0};
    CGLPixelFormatObj format=NULL;
    GLint formats=0;
    if(!cgl)return NULL;
    if(get_format)format=get_format(cgl);
    if(!format&&CGLChoosePixelFormat(attributes,&format,&formats)!=kCGLNoError)format=NULL;
    return format;
}


/* The pixel buffers QuickTime decodes into.  HALO_MAC_MOVIE_PIXELS: yuv
   (2vuy, 4:2:2: half the bytes of RGB, converted by the card), argb, bgra,
   or default (QuickTime's own choice). */
static CFDictionaryRef movie_context_attributes(const char **name)
{
    const char *choice=getenv("HALO_MAC_MOVIE_PIXELS");
    SInt32 format=0;
    CFNumberRef number;
    CFMutableDictionaryRef buffer,attributes;
    if(!choice||!*choice)choice=MOVIE_PIXELS_DEFAULT;
    if(name)*name=choice;
    if(!strcmp(choice,"yuv"))format=k2vuyPixelFormat;
    else if(!strcmp(choice,"argb"))format=k32ARGBPixelFormat;
    else if(!strcmp(choice,"bgra"))format=k32BGRAPixelFormat;
    else return NULL;
    number=CFNumberCreate(NULL,kCFNumberSInt32Type,&format);
    buffer=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    attributes=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if(number&&buffer&&attributes) {
        CFDictionarySetValue(buffer,kCVPixelBufferPixelFormatTypeKey,number);
        CFDictionarySetValue(attributes,kQTVisualContextPixelBufferAttributesKey,buffer);
    } else if(attributes){CFRelease(attributes);attributes=NULL;}
    if(number)CFRelease(number);
    if(buffer)CFRelease(buffer);
    return attributes;
}


/* The movie prepared before its clock starts: the first MoviesTask of a
   started movie blocks for a long time on an old machine while the clock
   runs, and the picture then runs behind and drops frames to catch up
   (HALO_MAC_MOVIE_PRIME=0 starts at once). */
static void movie_prime(struct movie_player *p)
{
    double began=movie_now();
    const char *text=getenv("HALO_MAC_MOVIE_PRIME");
    if(text&&text[0]=='0')return;
    PrerollMovie(p->movie,0,fixed1);
    if(p->stats)fprintf(stderr,"movie-stats: preroll %.0f ms\n",(movie_now()-began)*1000.0);
}

/* Plays one file; 0 if the window closed. */
static int movie_play_file(const char *path,int index,mac_movie_frame_runner runner,int hidden)
{
    struct movie_player p;
    CGLContextObj cgl=CGLGetCurrentContext();
    CGLPixelFormatObj format=movie_context_format(cgl);
    CFStringRef string;
    CFDictionaryRef attributes;
    const char *pixels_name="default";
    QTNewMoviePropertyElement properties[4];
    Boolean yes=true;
    OSStatus status;
    Rect bounds;
    const char *text;
    int window_open=1;
    memset(&p,0,sizeof(p));
    p.index=index;p.hidden=hidden;
    if(!cgl||!format)return 1;
    attributes=movie_context_attributes(&pixels_name);
    status=QTOpenGLTextureContextCreate(NULL,cgl,format,attributes,&p.context);
    if(attributes)CFRelease(attributes);
    if(status!=noErr||!p.context) {
        fprintf(stderr,"movie: no QuickTime texture context (%ld); skipped\n",(long)status);
        return 1;
    }
    string=CFStringCreateWithCString(NULL,path,kCFStringEncodingUTF8);
    if(!string){QTVisualContextRelease(p.context);return 1;}
    properties[0].propClass=kQTPropertyClass_DataLocation;
    properties[0].propID=kQTDataLocationPropertyID_CFStringPosixPath;
    properties[0].propValueSize=sizeof(string);properties[0].propValueAddress=&string;properties[0].propStatus=0;
    properties[1].propClass=kQTPropertyClass_NewMovieProperty;
    properties[1].propID=kQTNewMoviePropertyID_Active;
    properties[1].propValueSize=sizeof(yes);properties[1].propValueAddress=&yes;properties[1].propStatus=0;
    properties[2].propClass=kQTPropertyClass_NewMovieProperty;
    properties[2].propID=kQTNewMoviePropertyID_DontInteractWithUser;
    properties[2].propValueSize=sizeof(yes);properties[2].propValueAddress=&yes;properties[2].propStatus=0;
    properties[3].propClass=kQTPropertyClass_Context;
    properties[3].propID=kQTContextPropertyID_VisualContext;
    properties[3].propValueSize=sizeof(p.context);properties[3].propValueAddress=&p.context;properties[3].propStatus=0;
    status=NewMovieFromProperties(4,properties,0,NULL,&p.movie);
    CFRelease(string);
    if(status!=noErr||!p.movie) {
        fprintf(stderr,"movie: %s cannot be opened by QuickTime (%ld); skipped\n",path,(long)status);
        QTVisualContextRelease(p.context);
        return 1;
    }
    GetMovieBox(p.movie,&bounds);
    p.natural_width=(float)(bounds.right-bounds.left);
    p.natural_height=(float)(bounds.bottom-bounds.top);
    if(hidden)SetMovieVolume(p.movie,kNoVolume);
    text=getenv("HALO_MAC_MOVIE_LIMIT");
    p.limit=text?atof(text):0.0;
    p.capture=getenv("HALO_MAC_MOVIE_CAPTURE");
    if(p.capture&&!*p.capture)p.capture=NULL;
    text=getenv("HALO_MAC_MOVIE_SHOT_EVERY");
    p.shot_every=text&&atof(text)>0.0?atof(text):1.0;
    fprintf(stderr,"movie: playing %s (%.0fx%.0f, %.1f s, %s pixels)\n",path,p.natural_width,p.natural_height,
        (double)GetMovieDuration(p.movie)/(double)GetMovieTimeScale(p.movie),pixels_name);
    text=getenv("HALO_MAC_MOVIE_STATS");
    if(text&&(text[0]=='1'||text[0]=='2')&&!text[1]) {
        long tracks=GetMovieTrackCount(p.movie),track_index;
        p.stats=(struct movie_stats *)calloc(1,sizeof(struct movie_stats));
        for(track_index=1;p.stats&&track_index<=tracks;++track_index) {
            Track track=GetMovieIndTrack(p.movie,track_index);
            Media media=track?GetTrackMedia(track):NULL;
            OSType type=0;
            if(media)GetMediaHandlerDescription(media,&type,NULL,NULL);
            if(media&&type==VideoMediaType){p.stats->video=track;p.stats->video_media=media;break;}
        }
        if(p.stats)p.stats->finish=text[0]=='2';
        if(p.stats&&!p.stats->video){free(p.stats);p.stats=NULL;}
        else if(p.stats)p.stats->fps=(double)GetMediaSampleCount(p.stats->video_media)*(double)GetMediaTimeScale(p.stats->video_media)/(double)GetMediaDuration(p.stats->video_media);
    }
    if(p.stats)p.stats->cpu_at_start=movie_cpu();
    SetMovieRate(p.movie,0);
    GoToBeginningOfMovie(p.movie);
    movie_prime(&p);
    p.started=movie_now();
    p.next_shot=p.started+0.5;
    p.legacy=getenv("HALO_MAC_MOVIE_LEGACY")!=NULL;
    text=getenv("HALO_MAC_MOVIE_STEP");
    p.step=text&&text[0]=='1'&&!text[1]&&p.stats;
    if(p.step)p.legacy=1;
    if(!p.step)StartMovie(p.movie);
    while(!p.done) {
        /* The movie's task and the context's run here, every few
           milliseconds; the window is drawn and presented (the runner: its
           events, the swap that may wait for the display) only for a new
           image, or now and then so that its events are served.  A frame
           presented over and over costs the processor the decoder needs. */
        double now=movie_now();
        if(!p.legacy) {
            MoviesTask(p.movie,0);
            QTVisualContextTask(p.context);
            if(p.presented>0.0&&now-p.presented<MOVIE_IDLE_PRESENT&&!QTVisualContextIsNewImageAvailable(p.context,NULL)) {
                usleep(p.step?500:MOVIE_POLL_MICROSECONDS);
                continue;
            }
        }
        if(!runner(movie_frame,&p)){
            window_open=0;break;
        }
        p.presented=movie_now();
        usleep(p.step?500:(p.legacy?4000:1000));
    }
    StopMovie(p.movie);
    if(p.stats){stats_print(&p,movie_now()-p.started);free(p.stats);}
    if(p.image)CVOpenGLTextureRelease(p.image);
    DisposeMovie(p.movie);
    QTVisualContextTask(p.context);
    QTVisualContextRelease(p.context);
    if(p.skip_all)return -1;
    return window_open;
}

static int movie_play_names(const char *data_root,const char *const *names,unsigned count,
    mac_movie_frame_runner runner,int visible)
{
    unsigned index;
    const char *folders[2]={"Movies","movies"};
    OSErr entered;
    if(!data_root||!*data_root||!runner)return 1;
    if(!visible) {
        const char *hidden=getenv("HALO_MAC_MOVIES_HIDDEN");
        if(!hidden||hidden[0]!='1'||hidden[1])return 1;
    }
    entered=EnterMovies();
    if(entered!=noErr) {
        fprintf(stderr,"movie: QuickTime did not start (%ld); no movie\n",(long)entered);
        return 1;
    }
    for(index=0;index<count;++index) {
        char path[1100];
        struct stat st;
        unsigned folder;
        int found=0,result;
        for(folder=0;folder<2&&!found;++folder) {
            snprintf(path,sizeof(path),"%s/%s/%s",data_root,folders[folder],names[index]);
            found=stat(path,&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0;
        }
        if(!found){
            fprintf(stderr,"movie: %s is not in %s/Movies; skipped\n",names[index],data_root);
            continue;
        }
        result=movie_play_file(path,(int)index,runner,!visible);
        if(result==0){ExitMovies();return 0;}
        if(result<0)break;
    }
    ExitMovies();
    return 1;
}

int mac_movie_play_intro(const char *data_root,mac_movie_frame_runner runner,int visible)
{
    /* port: the Xbox disc's intro (the Microsoft and Bungie logos in one movie),
       decoded by the port, when the Xbox data has it (game.movies); else
       MacSoft's logos */
    int played=mac_bink_movie_play(data_root,"intro.bik",runner,visible);
    if(played>=0)return played;
    return movie_play_names(data_root,movie_names,(unsigned)MOVIE_COUNT,runner,visible);
}

int mac_movie_play_named(const char *data_root,const char *name,mac_movie_frame_runner runner,int visible)
{
    /* port: the ending is the Xbox disc's credits when the Xbox data has them */
    if(!strcmp(name,"ending.mov")) {
        int played=mac_bink_movie_play(data_root,"credits.bik",runner,visible);
        if(played>=0)return played;
    }
    return movie_play_names(data_root,&name,1,runner,visible);
}
#else
int mac_movie_play_intro(const char *data_root,mac_movie_frame_runner runner,int visible)
{
    (void)data_root;(void)runner;(void)visible;
    return 1;
}
int mac_movie_play_named(const char *data_root,const char *name,mac_movie_frame_runner runner,int visible)
{
    (void)data_root;(void)name;(void)runner;(void)visible;
    return 1;
}
#endif
