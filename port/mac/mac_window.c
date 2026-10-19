#include "mac_window.h"
#include "engine_messages.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach/mach_time.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#if !defined(__LP64__)
#include <Carbon/Carbon.h>
#include <AGL/agl.h>
/* port/mac/mac_gpu_textures.c forgets the sampler parameters it recorded for
   a deleted texture name; this weak no-op serves builds without it. */
void mac_gpu_textures_forget(unsigned int name) __attribute__((weak));
void mac_gpu_textures_forget(unsigned int name) { (void)name; }
#endif
#endif

struct mac_window_state {
    struct mac_input input;
    unsigned width,height,attachment_width,attachment_height;
    /* port: the window's own size where it is not the game's picture (the game
       draws width by height, in the lower left of the window's framebuffer;
       engine_device_state.c scales it to the window); 0: the same */
    unsigned window_width,window_height;
    /* port: the least a game window can be (the Xbox's 640x480); width is the
       width of the picture drawn now, which follows the window's shape
       (mac_window_screen_commit) */
    unsigned min_width,min_height;
    int visible,running,owns_context,fixed_size,quit_requested,fullscreen;
    /* port: the mouse held by the window (mac_window_mouse_update): whether
       the cursor is decoupled and hidden now, freed by F12, F12 down at the
       last frame, and the frames the menus have been absent */
    int mouse_held,mouse_freed,f12_down,menus_absent,window_was_active,menu_cursor_hidden;
    /* port: a borderless window over the whole display (display.mode
       "fullscreen" and "borderless"), the same context as the titled window's:
       whether it is one, whether the menu bar and the Dock are hidden now, and
       the titled window's place, for the way back */
    int borderless,ui_hidden,have_windowed_bounds;
    char title[96];
#if defined(__APPLE__) && !defined(__LP64__)
    Rect windowed_bounds;
#endif
    pthread_t owner_thread;
#if defined(__APPLE__)
    CGLPixelFormatObj pixel_format;
    CGLContextObj cgl;
    GLuint framebuffer,color_texture,depth_renderbuffer,color_renderbuffer;
#if !defined(__LP64__)
    WindowRef window;
    AGLPixelFormat agl_format;
    AGLContext agl;
    EventHandlerRef input_handler,window_handler;
#endif
#endif
};

static struct mac_window_state *active_window;

/* port: whether the menus asked for the pointer since the last frame
   (mac_window_mouse_update) */
static int mac_window_menus_seen;

void mac_window_mouse_menus(int menus_active)
{
    if(menus_active)mac_window_menus_seen=1;
}

/* port/linux/src/port_config.c, when linked (the tools that open windows
   without the settings do not) */
extern const char *config_string(const char *name) __attribute__((weak));
extern long config_integer(const char *name) __attribute__((weak));
extern int config_boolean(const char *name) __attribute__((weak));
extern unsigned long config_changes(void) __attribute__((weak));

/* "1280x960" (the settings' size): its width and height, 0 if it is not one */
static int mac_window_parse_size(const char *text,unsigned *width,unsigned *height)
{
    char *end;long w,h;
    if(!text||!*text)return 0;
    w=strtol(text,&end,10);
    if(end==text||(*end!='x'&&*end!='X'))return 0;
    text=end+1;h=strtol(text,&end,10);
    if(end==text||*end||w<1||h<1||w>8192||h>8192)return 0;
    *width=(unsigned)w;*height=(unsigned)h;return 1;
}

#if defined(__APPLE__)
/* port: what display.mode asks of a game window: 0 a titled window, 1 a
   borderless window over the whole display (the desktop's mode, which the
   picture is scaled to: "borderless", and "fullscreen" unless the next), 2 the
   display captured at a mode of its own (the old fullscreen: "fullscreen" with
   display.fullscreen_exclusive, or a display.resolution that is not the
   desktop's). The desktop's size is that of the first call, before any capture */
int mac_window_display_plan(void)
{
    static unsigned desktop_width,desktop_height;
    const char *mode=config_string?config_string("display.mode"):NULL;
    unsigned w,h;
    if(!desktop_width) {
        desktop_width=(unsigned)CGDisplayPixelsWide(kCGDirectMainDisplay);
        desktop_height=(unsigned)CGDisplayPixelsHigh(kCGDirectMainDisplay);
    }
    if(!mode)return 0;
    if(!strcmp(mode,"borderless"))return 1;
    if(strcmp(mode,"fullscreen"))return 0;
    if(config_boolean&&config_boolean("display.fullscreen_exclusive"))return 2;
    if(config_string&&mac_window_parse_size(config_string("display.resolution"),&w,&h)&&
       (w!=desktop_width||h!=desktop_height))return 2;
    return 1;
}
#else
int mac_window_display_plan(void){return 0;}
#endif

/* the window's size for a visible game window (display.window_size, else the
   game's picture): never less than the picture */
static void mac_window_wanted_size(const struct mac_window_state *state,unsigned *width,unsigned *height)
{
    unsigned w=state->min_width,h=state->min_height;
    /* (no size chosen: the game's own; display.window_scale is the desktop
       builds' and its default of 2 is in the Mac's config.toml too) */
    if(config_string&&!mac_window_parse_size(config_string("display.window_size"),&w,&h)){w=state->min_width;h=state->min_height;}
    if(w<state->min_width)w=state->min_width;
    if(h<state->min_height)h=state->min_height;
    *width=w;*height=h;
}

#if defined(__APPLE__)
static int has_extension(const char *extensions,const char *name)
{
    size_t length;const char *at;
    if(!extensions||!name||!*name)return 0;
    length=strlen(name);at=extensions;
    while((at=strstr(at,name))!=NULL) {
        if((at==extensions||at[-1]==' ')&&(at[length]==' '||at[length]=='\0'))return 1;
        at+=length;
    }
    return 0;
}
/* port: what the renderer needs of the GL context, checked once, before the
   first draw. The renderer calls OpenGL 2.0's own shader, buffer and vertex
   attribute entry points and GLSL, and draws shadows and the water into
   framebuffer objects; a driver without them (Mac OS X 10.4's on the older
   Radeons, GMA 900) used to end in the engine's halt, a 20 MB savegame.bin and
   no word to the player. The line it logs always is what a report needs. */
extern void mac_glsl120_set_version(int version) __attribute__((weak));
extern int mac_engine_files_translate_path(const char *,char *,size_t) __attribute__((weak));
extern int mac_gpu_info_vram_mb(void) __attribute__((weak));
extern const char *mac_gpu_info_renderer(void) __attribute__((weak));

static void gl_version_numbers(const char *text,int *major,int *minor)
{
    *major=0;*minor=0;
    if(!text)return;
    while(*text>='0'&&*text<='9'&&*major<100)*major=*major*10+(*text++-'0');
    if(*text=='.') {
        text++;
        /* ("1.10" is 1.10; the digits after the point as a number) */
        while(*text>='0'&&*text<='9'&&*minor<1000)*minor=*minor*10+(*text++-'0');
    }
}

/* 1 when the context can run the renderer. On 0, message holds what to tell
   the player. *glsl is the GLSL text version to generate: 120, or 110 for a
   driver whose language is 1.10 (HALO_MAC_GLSL_VERSION=110 asks for it on any
   driver, to try that text) */
static int mac_window_check_gl(char *message,size_t message_size,int *glsl)
{
    const char *vendor=(const char *)glGetString(GL_VENDOR);
    const char *renderer=(const char *)glGetString(GL_RENDERER);
    const char *version=(const char *)glGetString(GL_VERSION);
    const char *shading=(const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    const char *extensions=(const char *)glGetString(GL_EXTENSIONS);
    int gl_major,gl_minor,sl_major,sl_minor,gl_ok,sl_ok,fbo,packed,npot,rectangle,vertex_program,fragment_program;
    int vram=-1;const char *forced=getenv("HALO_MAC_GL_FORCE_CAPS_FAIL");
    char missing[400];
    while(glGetError()!=GL_NO_ERROR){}
    gl_version_numbers(version,&gl_major,&gl_minor);
    gl_version_numbers(shading,&sl_major,&sl_minor);
    gl_ok=gl_major>=2;
    sl_ok=sl_major>1||(sl_major==1&&sl_minor>=10);
    fbo=has_extension(extensions,"GL_EXT_framebuffer_object");
    packed=has_extension(extensions,"GL_EXT_packed_depth_stencil");
    npot=gl_major>=2&&has_extension(extensions,"GL_ARB_texture_non_power_of_two");
    rectangle=has_extension(extensions,"GL_ARB_texture_rectangle");
    vertex_program=has_extension(extensions,"GL_ARB_vertex_program");
    fragment_program=has_extension(extensions,"GL_ARB_fragment_program");
    if(mac_gpu_info_vram_mb)vram=mac_gpu_info_vram_mb();
    missing[0]='\0';
    if(!gl_ok)snprintf(missing+strlen(missing),sizeof(missing)-strlen(missing),"%sOpenGL 2.0 (this driver has %s)",missing[0]?"; ":"",version?version:"none");
    if(gl_ok&&!sl_ok)snprintf(missing+strlen(missing),sizeof(missing)-strlen(missing),"%sthe OpenGL shading language 1.10 (this driver has %s)",missing[0]?"; ":"",shading?shading:"none");
    if(!fbo)snprintf(missing+strlen(missing),sizeof(missing)-strlen(missing),"%sframebuffer objects (GL_EXT_framebuffer_object)",missing[0]?"; ":"");
    if(forced&&forced[0]=='1')snprintf(missing+strlen(missing),sizeof(missing)-strlen(missing),"%sa feature the test asked to be missing (HALO_MAC_GL_FORCE_CAPS_FAIL)",missing[0]?"; ":"");
    *glsl=(sl_major>1||(sl_major==1&&sl_minor>=20))?120:110;
    if(getenv("HALO_MAC_GLSL_VERSION")&&!strcmp(getenv("HALO_MAC_GLSL_VERSION"),"110"))*glsl=110;
    fprintf(stderr,"native GL capabilities: vendor=%s renderer=%s version=%s glsl=%s vram_mb=%d extensions={framebuffer_object=%d packed_depth_stencil=%d npot=%d texture_rectangle=%d arb_vertex_program=%d arb_fragment_program=%d} glsl_source=%d%s%s\n",
        vendor?vendor:"unknown",renderer?renderer:"unknown",version?version:"unknown",shading?shading:"none",vram,
        fbo,packed,npot,rectangle,vertex_program,fragment_program,*glsl,missing[0]?" unsupported: ":"",missing);
    if(!missing[0])return 1;
    snprintf(message,message_size,
        "The graphics card in this Mac (%s) cannot run PowerSpark. It lacks %s. PowerSpark needs a card and a system with OpenGL 2.0, as Mac OS X 10.5 or later gives most Macs from 2004 on, or a newer driver for it.",
        renderer&&*renderer?renderer:"unknown",missing);
    return 0;
}
static unsigned next_power_of_two(unsigned value)
{
    unsigned result=1;
    while(result<value&&result<8192)result<<=1;
    return result;
}
static void discard_state(struct mac_window_state *state)
{
    struct mac_window temporary;
    temporary.state=state;mac_window_close(&temporary);
}
/* port: the context is current; if the renderer cannot run on it, say so (a
   hidden run only logs it) and end the program here, not later in the
   engine's halt. Otherwise tell the shader text which GLSL to make. */
static void mac_window_gl_gate(struct mac_window_state *state)
{
    char message[600];int glsl=120;
    if(!mac_window_check_gl(message,sizeof(message),&glsl)) {
        fprintf(stderr,"Mac window: %s\n",message);
        if(state->visible)mac_engine_message_now("PowerSpark cannot start",message);
        discard_state(state);
        /* (the game's 20 MB state file was made at start, before the renderer: nothing will use it) */
        if(mac_engine_files_translate_path) {
            char path[1024];
            if(mac_engine_files_translate_path("z:\\savegame.bin",path,sizeof(path))==0)unlink(path);
        }
        exit(1);
    }
    if(mac_glsl120_set_version)mac_glsl120_set_version(glsl);
}
static void mac_window_discard_cgl(struct mac_window_state *state)
{
    if(state->cgl) {
        if(CGLSetCurrentContext(state->cgl)==kCGLNoError) {
            if(state->framebuffer)glDeleteFramebuffersEXT(1,&state->framebuffer);
            if(state->depth_renderbuffer)glDeleteRenderbuffersEXT(1,&state->depth_renderbuffer);
            if(state->color_renderbuffer)glDeleteRenderbuffersEXT(1,&state->color_renderbuffer);
            if(state->color_texture){mac_gpu_textures_forget(state->color_texture);glDeleteTextures(1,&state->color_texture);}
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
            CGLSetCurrentContext(NULL);
        }
        CGLDestroyContext(state->cgl);state->cgl=NULL;
    }
    if(state->pixel_format) { CGLDestroyPixelFormat(state->pixel_format);state->pixel_format=NULL; }
}
static int mac_window_open_cgl_offscreen(struct mac_window_state *state)
{
    CGLPixelFormatAttribute attributes[]={kCGLPFAAccelerated,kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24,kCGLPFADepthSize,(CGLPixelFormatAttribute)24,
        (CGLPixelFormatAttribute)0};
    GLint formats=0;const char *extensions;GLenum status;CGLError cgl_error;
    cgl_error=CGLChoosePixelFormat(attributes,&state->pixel_format,&formats);
    if(cgl_error!=kCGLNoError||!state->pixel_format) { fprintf(stderr,"CGL pixel format: %s\n",CGLErrorString(cgl_error));return 0; }
    cgl_error=CGLCreateContext(state->pixel_format,NULL,&state->cgl);
    if(cgl_error!=kCGLNoError||!state->cgl) { fprintf(stderr,"CGL context: %s\n",CGLErrorString(cgl_error));return 0; }
    cgl_error=CGLSetCurrentContext(state->cgl);
    if(cgl_error!=kCGLNoError) { fprintf(stderr,"CGL current context: %s\n",CGLErrorString(cgl_error));return 0; }
    mac_window_gl_gate(state);
    extensions=(const char *)glGetString(GL_EXTENSIONS);
    if(!has_extension(extensions,"GL_EXT_packed_depth_stencil")) { fprintf(stderr,"CGL missing EXT_packed_depth_stencil\n");return 0; }
    if(!has_extension(extensions,"GL_EXT_framebuffer_object")) { fprintf(stderr,"CGL missing EXT_framebuffer_object\n");return 0; }
    while(glGetError()!=GL_NO_ERROR){}
    glGenFramebuffersEXT(1,&state->framebuffer);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,state->framebuffer);
    if(getenv("HALO_MAC_OFFSCREEN_RENDERBUFFER")) {
        /* Diagnostic only: the hidden frame in a color renderbuffer, as a
           window's framebuffer is, rather than in a texture. */
        glGenRenderbuffersEXT(1,&state->color_renderbuffer);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,state->color_renderbuffer);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_RGBA8,
            (GLsizei)state->attachment_width,(GLsizei)state->attachment_height);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
            GL_RENDERBUFFER_EXT,state->color_renderbuffer);
    } else {
    glGenTextures(1,&state->color_texture);
    glBindTexture(GL_TEXTURE_2D,state->color_texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,(GLsizei)state->attachment_width,(GLsizei)state->attachment_height,
        0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D,state->color_texture,0);
    }
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if(status!=GL_FRAMEBUFFER_COMPLETE_EXT) {
        fprintf(stderr,"CGL color framebuffer incomplete: status=0x%x\n",(unsigned)status);return 0;
    }
    glGenRenderbuffersEXT(1,&state->depth_renderbuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,state->depth_renderbuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8_EXT,
        (GLsizei)state->attachment_width,(GLsizei)state->attachment_height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
        GL_RENDERBUFFER_EXT,state->depth_renderbuffer);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,
        GL_RENDERBUFFER_EXT,state->depth_renderbuffer);
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    {
        GLenum error=glGetError();
        if(status!=GL_FRAMEBUFFER_COMPLETE_EXT||error!=GL_NO_ERROR) {
            fprintf(stderr,"CGL framebuffer incomplete: status=0x%x gl_error=0x%x\n",(unsigned)status,(unsigned)error);return 0;
        }
    }
    state->owns_context=1;mac_input_focus(&state->input,1);return 1;
}
#endif

#if defined(__APPLE__) && !defined(__LP64__)
static OSStatus mac_window_event(EventHandlerCallRef next,EventRef event,void *context)
{
    struct mac_window_state *state=(struct mac_window_state *)context;
    UInt32 kind=GetEventKind(event);
    (void)next;
    if(kind==kEventWindowClose) { state->quit_requested=1;state->running=0;return noErr; }
    if(kind==kEventWindowBoundsChanged) {
        Rect bounds;
        if(GetWindowBounds(state->window,kWindowContentRgn,&bounds)==noErr) {
            unsigned width=(unsigned)(bounds.right-bounds.left);
            unsigned height=(unsigned)(bounds.bottom-bounds.top);
            /* (a game window's size is the window's, not the picture's) */
            if(state->fixed_size) {
                if(width>=state->min_width&&height>=state->min_height) { state->window_width=width;state->window_height=height;aglUpdateContext(state->agl); }
            } else
            if(width&&height) { state->width=width;state->height=height;aglUpdateContext(state->agl); }
        }
        return eventNotHandledErr;
    }
    return eventNotHandledErr;
}
/* port/linux/src/port_config.c, when linked (the tools that open windows
   without the settings do not) */
extern const char *config_string(const char *name) __attribute__((weak));
/* display.mode "fullscreen": the main display captured and set to the
   window's size (640x480, which the panel stretches), as the Mac games of
   the time played, rather than a window on the desktop. */
/* display.vsync (port_config.c): wait for the display between frames.
   The settings are not linked into every probe that opens a window; without
   them the swap interval stays the driver's (no wait). */
extern int config_boolean(const char *name) __attribute__((weak));
static void mac_window_apply_vsync(AGLContext context)
{
    GLint interval;
    if(!context||!config_boolean)return;
    interval=config_boolean("display.vsync")?1:0;
    if(!aglSetInteger(context,AGL_SWAP_INTERVAL,&interval))
        fprintf(stderr,"Mac window: the swap interval could not be set\n");
    else fprintf(stderr,"Mac window: vsync %s\n",interval?"on":"off");
}
static int mac_window_open_fullscreen(struct mac_window_state *state)
{
    GLint attributes[]={AGL_RGBA,AGL_DOUBLEBUFFER,AGL_ACCELERATED,AGL_FULLSCREEN,
        AGL_RED_SIZE,8,AGL_GREEN_SIZE,8,AGL_BLUE_SIZE,8,AGL_DEPTH_SIZE,24,
        AGL_STENCIL_SIZE,8,AGL_NONE};
    GDHandle device=GetMainDevice();
    const char *mode=config_string?config_string("display.mode"):NULL;
    /* port: the display's mode is display.resolution ("native": the desktop's,
       not changed), for a game window (the game's own picture is scaled to it,
       engine_device_state.c); the game's size for the other windows */
    unsigned full_width=state->width,full_height=state->height;
    if(!mode||strcmp(mode,"fullscreen")||mac_window_display_plan()!=2)return 0;
    if(state->fixed_size) {
        const char *resolution=config_string?config_string("display.resolution"):NULL;
        unsigned w,h;
        if(mac_window_parse_size(resolution,&w,&h)&&w>=state->min_width&&h>=state->min_height) {
            full_width=w;full_height=h;
        } else {
            full_width=(unsigned)CGDisplayPixelsWide(kCGDirectMainDisplay);
            full_height=(unsigned)CGDisplayPixelsHigh(kCGDirectMainDisplay);
            if(full_width<state->min_width||full_height<state->min_height){full_width=state->min_width;full_height=state->min_height;}
        }
    }
    if(CGDisplayCapture(kCGDirectMainDisplay)!=kCGErrorSuccess) {
        fprintf(stderr,"Mac fullscreen: the display could not be captured; using a window\n");return 0;
    }
    state->agl_format=aglChoosePixelFormat(&device,1,attributes);
    if(state->agl_format)state->agl=aglCreateContext(state->agl_format,NULL);
    if(!state->agl||!aglSetFullScreen(state->agl,(GLsizei)full_width,(GLsizei)full_height,0,0)||
       !aglSetCurrentContext(state->agl)) {
        fprintf(stderr,"Mac fullscreen: no %ux%u display mode; using a window\n",full_width,full_height);
        if(state->agl){aglSetCurrentContext(NULL);aglDestroyContext(state->agl);state->agl=NULL;}
        if(state->agl_format){aglDestroyPixelFormat(state->agl_format);state->agl_format=NULL;}
        CGReleaseAllDisplays();return 0;
    }
    CGDisplayHideCursor(kCGDirectMainDisplay);
    mac_window_apply_vsync(state->agl);
    state->fullscreen=1;
    if(state->fixed_size) {
        /* (the framebuffer is the display's mode, whatever was asked for) */
        unsigned w=(unsigned)CGDisplayPixelsWide(kCGDirectMainDisplay),h=(unsigned)CGDisplayPixelsHigh(kCGDirectMainDisplay);
        if(w>=state->min_width&&h>=state->min_height&&(w!=state->width||h!=state->height)){state->window_width=w;state->window_height=h;}
        fprintf(stderr,"Mac fullscreen: the display at %ux%u\n",w,h);
    }
    return 1;
}
/* port: the mouse, as the Linux build's SDL relative mode holds it
   (sdl_platform.c): in a game the cursor is hidden and decoupled from the
   mouse, which turns the view by the hardware's own movement; the menus (their
   pointer is asked for every frame: halo_ui_pointer_update), a window that
   is not in front or is minimised, F12 and the end give the cursor back.
   Settings: input.mouse_capture (HALO_NO_MOUSE_CAPTURE=1 sets false).
   Everything here is Quartz Display Services, of Mac OS X 10.0. */
/* port: the system's cursor is hidden over the window while the menus draw
   their own pointer (menu_cursor.c; absent from the tools without it) */
extern int menu_cursor_replaces_system(void) __attribute__((weak));

static void mac_window_menu_cursor(struct mac_window_state *state,int hide)
{
    if(!state||(hide!=0)==(state->menu_cursor_hidden!=0))return;
    state->menu_cursor_hidden=hide!=0;
    if(hide)CGDisplayHideCursor(kCGDirectMainDisplay);
    else CGDisplayShowCursor(kCGDirectMainDisplay);
}

static void mac_window_mouse_release(struct mac_window_state *state)
{
    mac_window_menu_cursor(state,0);
    if(!state||!state->mouse_held)return;
    state->mouse_held=0;
    state->input.raw_motion=0;
    CGAssociateMouseAndMouseCursorPosition(1);
    CGDisplayShowCursor(kCGDirectMainDisplay);
}

static void mac_window_mouse_hold(struct mac_window_state *state)
{
    Rect bounds;CGPoint centre;int32_t dx,dy;
    if(state->mouse_held)return;
    /* (the cursor goes to the middle of the window first, where the clicks
       will land while it is decoupled; and the system does not hold the
       events back after the move) */
    if(GetWindowBounds(state->window,kWindowContentRgn,&bounds)==noErr) {
        centre.x=(float)(bounds.left+bounds.right)*0.5f;
        centre.y=(float)(bounds.top+bounds.bottom)*0.5f;
        CGSetLocalEventsSuppressionInterval(0.0);
        CGWarpMouseCursorPosition(centre);
    }
    CGAssociateMouseAndMouseCursorPosition(0);
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGGetLastMouseDelta(&dx,&dy);
    state->mouse_held=1;
    state->input.raw_motion=1;
}

static void mac_window_mouse_update(struct mac_window_state *state)
{
    int menus=mac_window_menus_seen,f12=state->input.keys[111]!=0,want;
    mac_window_menus_seen=0;
    if(f12&&!state->f12_down)state->mouse_freed=!state->mouse_freed;
    state->f12_down=f12;
    if(state->input.focused&&!state->window_was_active)state->mouse_freed=0;
    state->window_was_active=state->input.focused;
    state->menus_absent=menus?0:(state->menus_absent<1000?state->menus_absent+1:state->menus_absent);
    /* (a few frames without the menus before the cursor goes: a menu that
       closes and another that opens does not flicker it) */
    want=state->input.focused&&!state->mouse_freed&&state->menus_absent>=6&&
        IsWindowActive(state->window)&&!IsWindowCollapsed(state->window)&&
        config_boolean&&config_boolean("input.mouse_capture");
    if(want&&!state->mouse_held)mac_window_mouse_hold(state);
    else if(!want&&state->mouse_held)mac_window_mouse_release(state);
    {
        /* (over the window only: the pointer's place is in its events) */
        Rect bounds;int hide=0;
        if(menus&&!state->mouse_held&&state->input.focused&&state->input.pointer_valid&&
           menu_cursor_replaces_system&&menu_cursor_replaces_system()&&
           IsWindowActive(state->window)&&!IsWindowCollapsed(state->window)&&
           GetWindowBounds(state->window,kWindowContentRgn,&bounds)==noErr)
            hide=state->input.pointer_x>=0.0f&&state->input.pointer_y>=0.0f&&
                state->input.pointer_x<(float)(bounds.right-bounds.left)&&
                state->input.pointer_y<(float)(bounds.bottom-bounds.top);
        mac_window_menu_cursor(state,hide);
    }
    if(state->mouse_held) {
        int32_t dx=0,dy=0;
        CGGetLastMouseDelta(&dx,&dy);
        mac_input_raw_motion(&state->input,(float)dx,(float)dy);
    }
}


/* port: the menu bar and the Dock are hidden while a borderless window is the
   front one, and given back when the application or the window is not (the
   system also puts them back when the process ends, which a crash included) */
static void mac_window_ui_mode(struct mac_window_state *state,int hidden)
{
    if(!state||(hidden!=0)==(state->ui_hidden!=0))return;
    state->ui_hidden=hidden!=0;
    SetSystemUIMode(hidden?kUIModeAllHidden:kUIModeNormal,0);
}

static void mac_window_ui_update(struct mac_window_state *state)
{
    mac_window_ui_mode(state,state->borderless&&state->input.focused&&state->window&&
        IsWindowActive(state->window)&&!IsWindowCollapsed(state->window));
}

/* (the application or the window was deactivated: the cursor is the user's
   at once, without waiting for the next frame, which a map load can delay) */
static void mac_window_focus_lost(void)
{
    mac_window_mouse_release(active_window);
    mac_window_ui_mode(active_window,0);
}

/* the main display's bounds as a Carbon rectangle */
static void mac_window_display_rect(Rect *bounds)
{
    CGRect display=CGDisplayBounds(kCGDirectMainDisplay);
    bounds->left=(short)display.origin.x;bounds->top=(short)display.origin.y;
    bounds->right=(short)(display.origin.x+display.size.width);
    bounds->bottom=(short)(display.origin.y+display.size.height);
}

/* the titled window of a game window (the game's own size or more, never
   smaller), or the plain one over the display; the event handler is
   installed, the window not yet shown */
static int mac_window_create_native(struct mac_window_state *state,int borderless,const Rect *bounds,
    WindowRef *window,EventHandlerRef *handler)
{
    EventTypeSpec events[]={{kEventClassWindow,kEventWindowClose},{kEventClassWindow,kEventWindowBoundsChanged}};
    CFStringRef name;
    *window=NULL;*handler=NULL;
    if(CreateNewWindow(borderless?kPlainWindowClass:kDocumentWindowClass,
        (borderless?kWindowNoShadowAttribute:kWindowStandardDocumentAttributes)|kWindowStandardHandlerAttribute,
        bounds,window)!=noErr)return 0;
    /* port: a game window can be dragged to another size (never smaller than
       the game's picture), as the other platforms' windows; its zoom box is
       not wanted */
    if(state->fixed_size&&!borderless) {
        HISize least;
        /* (a system that refuses to take the zoom box off, as Mac OS X 10.4 may, leaves it: the window
           is still a game window, and failing here ended the game with no window at all) */
        OSStatus zoom=ChangeWindowAttributes(*window,0,kWindowFullZoomAttribute);
        if(zoom!=noErr)fprintf(stderr,"Mac window: the zoom box stays (ChangeWindowAttributes %ld)\n",(long)zoom);
        least.width=(float)state->min_width;least.height=(float)state->min_height;
        SetWindowResizeLimits(*window,&least,NULL);
    }
    name=CFStringCreateWithCString(NULL,state->title[0]?state->title:"Halo",kCFStringEncodingUTF8);
    if(name) { SetWindowTitleWithCFString(*window,name);CFRelease(name); }
    if(InstallWindowEventHandler(*window,mac_window_event,2,events,state,handler)!=noErr) {
        DisposeWindow(*window);*window=NULL;*handler=NULL;return 0;
    }
    return 1;
}

/* the window's size the game draws to, once it is a titled window of w by h or
   a borderless one over the display (0 for the picture's own size) */
static void mac_window_note_size(struct mac_window_state *state,unsigned width,unsigned height)
{
    if(!state->fixed_size)return;
    if(width>=state->min_width&&height>=state->min_height) {
        state->window_width=width==state->min_width&&height==state->min_height?0:width;
        state->window_height=width==state->min_width&&height==state->min_height?0:height;
    }
}

/* between a titled window and a borderless one over the display, live: the
   one AGL context moves to a new window (aglSetDrawable), so that its textures,
   buffers and the game's state stay; the old window goes. 1 when it is in the
   mode asked for, 0 when it could not be done (the window stays as it was) */
int mac_window_set_borderless_active(int borderless)
{
    struct mac_window_state *state=active_window;
    Rect bounds,old_bounds;WindowRef window,old_window;EventHandlerRef handler;
    if(!state||!state->visible||!state->window||!state->agl||!state->fixed_size||state->fullscreen)return 0;
    borderless=borderless!=0;
    if(borderless==(state->borderless!=0))return 1;
    mac_window_mouse_release(state);
    if(borderless) {
        if(GetWindowBounds(state->window,kWindowContentRgn,&old_bounds)==noErr) {
            state->windowed_bounds=old_bounds;state->have_windowed_bounds=1;
        }
        mac_window_display_rect(&bounds);
        /* (hidden first: the system keeps a window below the menu bar) */
        mac_window_ui_mode(state,1);
    } else {
        unsigned width,height;
        if(state->have_windowed_bounds)bounds=state->windowed_bounds;
        else {
            mac_window_wanted_size(state,&width,&height);
            bounds.left=80;bounds.top=80;bounds.right=(short)(80+width);bounds.bottom=(short)(80+height);
        }
        mac_window_ui_mode(state,0);
    }
    if(!mac_window_create_native(state,borderless,&bounds,&window,&handler)) {
        fprintf(stderr,"Mac window: no window for the %s mode; staying as it is\n",borderless?"borderless":"windowed");
        mac_window_ui_mode(state,state->borderless);
        return 0;
    }
    old_window=state->window;
    if(!aglSetDrawable(state->agl,GetWindowPort(window))) {
        fprintf(stderr,"Mac window: the context cannot take the new window; staying as it is\n");
        RemoveEventHandler(handler);DisposeWindow(window);
        aglSetDrawable(state->agl,GetWindowPort(old_window));
        mac_window_ui_mode(state,state->borderless);
        return 0;
    }
    if(state->window_handler)RemoveEventHandler(state->window_handler);
    state->window=window;state->window_handler=handler;
    state->borderless=borderless;
    mac_window_note_size(state,(unsigned)(bounds.right-bounds.left),(unsigned)(bounds.bottom-bounds.top));
    aglUpdateContext(state->agl);
    ShowWindow(window);
    SelectWindow(window);
    DisposeWindow(old_window);
    mac_window_ui_update(state);
    fprintf(stderr,"Mac window: %s, %ldx%ld\n",borderless?"borderless over the display":"a window",
        (long)(bounds.right-bounds.left),(long)(bounds.bottom-bounds.top));
    return 1;
}

static int mac_window_open_agl(struct mac_window_state *state,const char *title)
{
    GLint attributes[]={AGL_RGBA,AGL_DOUBLEBUFFER,AGL_ACCELERATED,
        AGL_RED_SIZE,8,AGL_GREEN_SIZE,8,AGL_BLUE_SIZE,8,AGL_DEPTH_SIZE,24,
        AGL_STENCIL_SIZE,8,AGL_NONE};
    AGLPixelFormat format;Rect bounds;
    unsigned window_width=state->width,window_height=state->height;
    /* (the window's size, for a game window: display.window_size) */
    if(state->fixed_size) {
        mac_window_wanted_size(state,&window_width,&window_height);
        if(window_width!=state->width||window_height!=state->height){state->window_width=window_width;state->window_height=window_height;}
    }
    bounds.left=80;bounds.top=80;bounds.right=(short)(bounds.left+window_width);bounds.bottom=(short)(bounds.top+window_height);
    {
        /* A bare executable started from Terminal is a background-only
           process to Carbon (appIsDaemon, -606) until it becomes an
           ordinary application, which can take the front and the keys. */
        ProcessSerialNumber process={0,kCurrentProcess};
        OSStatus status=TransformProcessType(&process,kProcessTransformToForegroundApplication);
        if(status!=noErr)fprintf(stderr,"Carbon foreground transform failed: %ld\n",(long)status);
    }
    if(mac_window_open_fullscreen(state)) {
        ProcessSerialNumber process={0,kCurrentProcess};
        if(mac_input_install(&state->input,&state->input_handler)!=noErr)return 0;
        if(SetFrontProcess(&process)!=noErr)fprintf(stderr,"Carbon foreground activation failed\n");
        mac_input_focus(&state->input,1);
        state->running=1;
        return 1;
    }
    strncpy(state->title,title?title:"Halo",sizeof(state->title)-1);
    /* port: display.mode "borderless" and "fullscreen" (at the desktop's own
       mode) are a window over the whole display */
    if(state->fixed_size&&mac_window_display_plan()==1) {
        Rect windowed=bounds;
        mac_window_display_rect(&bounds);
        if((unsigned)(bounds.right-bounds.left)>=state->min_width&&(unsigned)(bounds.bottom-bounds.top)>=state->min_height) {
            state->windowed_bounds=windowed;state->have_windowed_bounds=1;state->borderless=1;
            mac_window_ui_mode(state,1);
        } else bounds=windowed;
    }
    if(!mac_window_create_native(state,state->borderless,&bounds,&state->window,&state->window_handler))return 0;
    if(state->borderless)mac_window_note_size(state,(unsigned)(bounds.right-bounds.left),(unsigned)(bounds.bottom-bounds.top));
    format=aglChoosePixelFormat(NULL,0,attributes);
    if(!format)return 0;
    state->agl_format=format;state->agl=aglCreateContext(format,NULL);
    if(!state->agl||!aglSetDrawable(state->agl,GetWindowPort(state->window))||
       !aglSetCurrentContext(state->agl))return 0;
    mac_window_gl_gate(state);
    mac_window_apply_vsync(state->agl);
    if(mac_input_install(&state->input,&state->input_handler)!=noErr)return 0;
    mac_input_focus_lost_hook=mac_window_focus_lost;
    ShowWindow(state->window);
    {
        ProcessSerialNumber process={0,kCurrentProcess};
        OSStatus status=SetFrontProcess(&process);
        /* (a window behind others still plays; say so and go on) */
        if(status!=noErr)
            fprintf(stderr,"Carbon foreground activation failed: %ld\n",(long)status);
    }
    state->running=1;
    return 1;
}
#endif

static int mac_window_open_with_policy(struct mac_window *window,unsigned width,unsigned height,
    int visible,const char *title,int fixed_size)
{
    struct mac_window_state *state;
    if(!window||window->state||active_window||!width||!height||width>8192||height>8192)return 0;
    window->state=NULL;
    state=(struct mac_window_state *)calloc(1,sizeof(*state));if(!state)return 0;
    state->width=width;state->height=height;state->min_width=width;state->min_height=height;state->visible=!!visible;state->running=1;
    state->fixed_size=!!fixed_size;
    state->owner_thread=pthread_self();
    state->attachment_width=next_power_of_two(width);state->attachment_height=next_power_of_two(height);
    /* HALO_MAC_HIDDEN_WINDOW=WxH (diagnostic): a hidden run's window is bigger
       than the game's picture, to try the scaled present (engine_device_state.c) */
    if(!visible&&fixed_size&&getenv("HALO_MAC_HIDDEN_WINDOW")) {
        unsigned w,h;
        if(mac_window_parse_size(getenv("HALO_MAC_HIDDEN_WINDOW"),&w,&h)&&w>=width&&h>=height&&w<=4096&&h<=4096) {
            state->window_width=w;state->window_height=h;
            state->attachment_width=next_power_of_two(w);state->attachment_height=next_power_of_two(h);
        }
    }
    mac_input_init(&state->input);
#if defined(__APPLE__)
    if(visible) {
#if defined(__LP64__)
        (void)title;free(state);return 0;
#else
        if(!mac_window_open_agl(state,title)) { discard_state(state);return 0; }
#endif
    } else if(!mac_window_open_cgl_offscreen(state)) { discard_state(state);return 0; }
#else
    (void)title;(void)visible;free(state);return 0;
#endif
    window->state=state;active_window=state;return 1;
}

int mac_window_open(struct mac_window *window,unsigned width,unsigned height,int visible,const char *title)
{ return mac_window_open_with_policy(window,width,height,visible,title,0); }

int mac_window_open_fixed(struct mac_window *window,unsigned width,unsigned height,int visible,const char *title)
{ return mac_window_open_with_policy(window,width,height,visible,title,1); }

void platform_video_window_size(int *width,int *height)
{
    if(width)*width=active_window?(int)(active_window->window_width?active_window->window_width:active_window->width):0;
    if(height)*height=active_window?(int)(active_window->window_height?active_window->window_height:active_window->height):0;
}

long halo_screen_width(void)
{
	/* The game creates its rasterizer before opening a window. Keep its native
	   640-pixel logical surface available during that startup phase. */
	return active_window?(long)active_window->width:640;
}

/* port: a widescreen picture. The Xbox's is 640x480; a window or display of
   another shape gets 480 lines and as many columns as its shape gives (the
   game's camera takes its horizontal angle from the viewport, so the view
   widens; the menus keep their 640 columns, centred: halo_screen_ui_offset).
   Where display.resolution_scaling is "original" the picture is 640x480 in
   any window. The width is taken up between frames (halo_screen_commit), and
   the device is told (the hook) to resize its back buffer. Not wider than
   WIDE_MAXIMUM_WIDTH: a window beyond that shape has bars at the sides. */
#define WIDE_MAXIMUM_WIDTH 1280
static void (*screen_width_hook)(unsigned width);

void mac_window_screen_width_hook(void (*hook)(unsigned width))
{ screen_width_hook=hook; }

static unsigned mac_window_wide_width(const struct mac_window_state *state)
{
    unsigned window_width,window_height,width;
    const char *scaling;
    if(!state->fixed_size)return state->width;
    scaling=config_string?config_string("display.resolution_scaling"):NULL;
    if(scaling&&!strcmp(scaling,"original"))return state->min_width;
    window_width=state->window_width?state->window_width:state->width;
    window_height=state->window_height?state->window_height:state->height;
    if(!window_width||!window_height||window_height<state->min_height)return state->min_width;
    width=(unsigned)(((unsigned long)state->min_height*window_width+window_height/2)/window_height);
    width&=~1U;
    if(width<state->min_width)width=state->min_width;
    if(width>WIDE_MAXIMUM_WIDTH)width=WIDE_MAXIMUM_WIDTH;
    if(width>window_width)width=window_width&~1U;
    return width;
}

/* between frames: the width the game's picture now has (a game window's;
   another window's is its own) */
long mac_window_screen_commit(void)
{
    unsigned width;
    if(!active_window)return 640;
    width=mac_window_wide_width(active_window);
    if(width!=active_window->width) {
        fprintf(stderr,"Mac window: the picture is %ux%u (window %ux%u)\n",width,active_window->height,
            active_window->window_width?active_window->window_width:active_window->width,
            active_window->window_height?active_window->window_height:active_window->height);
        active_window->width=width;
        if(screen_width_hook)screen_width_hook(width);
    }
    return (long)active_window->width;
}

int mac_window_present_active(void)
{
#if defined(__APPLE__) && !defined(__LP64__)
    if(!active_window||!active_window->visible||!active_window->agl||
       !pthread_equal(active_window->owner_thread,pthread_self()))return 0;
    if(!aglSetCurrentContext(active_window->agl))return 0;
    aglSwapBuffers(active_window->agl);
    return 1;
#else
    return 0;
#endif
}

void platform_request_quit(void)
{
    if(active_window){active_window->quit_requested=1;active_window->running=0;}
}

int mac_window_quit_requested_active(void)
{ return active_window&&active_window->quit_requested; }

#if defined(__APPLE__)
/* port: display.max_fps. With display.vsync off, no more frames a second than
   this (0: auto, twice a 60 Hz display's; -1: no limit); with it on the swap
   waits for the display and nothing is added. Called after each presented
   frame of a visible window. */
static double mac_window_limiter_wait;
static void mac_window_limit_frame_rate(void)
{
    static unsigned long read_at=(unsigned long)-1;
    static long limit;static int vsync=1;
    static uint64_t deadline,interval;
    static mach_timebase_info_data_t timebase;
    uint64_t now;
    if(!config_integer||!config_boolean||!config_changes)return;
    if(read_at!=config_changes()) {
        read_at=config_changes();
        vsync=config_boolean("display.vsync");limit=config_integer("display.max_fps");
        if(limit==0)limit=120;
        if(!timebase.denom)mach_timebase_info(&timebase);
        interval=limit>0&&timebase.numer?(uint64_t)1000000000ULL/(uint64_t)limit*timebase.denom/timebase.numer:0;
        deadline=0;
    }
    if(vsync||limit<0||!interval)return;
    now=mach_absolute_time();
    if(deadline&&now<deadline) {
        mach_wait_until(deadline);
        /* port: the wait is not the frame's work (the adaptive detail, mac_window_take_limiter_wait) */
        if(!timebase.denom)mach_timebase_info(&timebase);
        mac_window_limiter_wait+=(double)(deadline-now)*timebase.numer/timebase.denom/1e9;
    } else deadline=now;
    deadline+=interval;
}
#endif

/* port: the seconds the frame rate limiter has waited since this was last asked */
double mac_window_take_limiter_wait(void)
{
#if defined(__APPLE__)
    double wait=mac_window_limiter_wait;
    mac_window_limiter_wait=0.0;
    return wait;
#else
    return 0.0;
#endif
}

int mac_window_frame(struct mac_window *window,mac_window_frame_proc frame,void *context)
{
    struct mac_window_state *state;struct mac_input_frame input;
    if(!window||!window->state||!frame)return 0;
    state=(struct mac_window_state *)window->state;
    if(!state->running)return 0;
#if defined(__APPLE__)
#if !defined(__LP64__)
    if(state->visible) {
        EventRef event=NULL;EventTargetRef target=GetEventDispatcherTarget();EventTimeout timeout=0;
        while(ReceiveNextEvent(0,NULL,timeout,true,&event)==noErr&&event) {
            SendEventToEventTarget(event,target);ReleaseEvent(event);event=NULL;
        }
        if(!state->running)return 0;
        if(state->window){mac_window_ui_update(state);mac_window_mouse_update(state);}
        if(!aglSetCurrentContext(state->agl))return 0;
        mac_engine_message_dispatch(1);
        if(!aglSetCurrentContext(state->agl))return 0;
    } else
#endif
    {
        if(CGLSetCurrentContext(state->cgl)!=kCGLNoError)return 0;
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,state->framebuffer);
    }
    mac_input_snapshot(&state->input,&input);
    if(!frame(context,&input,state->width,state->height)) { state->running=0;return 0; }
#if !defined(__LP64__)
    if(state->visible&&!mac_window_present_active())return 0;
    if(state->visible)mac_window_limit_frame_rate();
#endif
    return state->running;
#else
    (void)context;(void)input;return 0;
#endif
}

int mac_window_run(struct mac_window *window,mac_window_frame_proc frame,void *context)
{
    struct mac_window_state *state;
    if(!window||!window->state||!frame)return 0;
    state=(struct mac_window_state *)window->state;
#if defined(__APPLE__) && !defined(__LP64__)
    if(!state->visible)return mac_window_frame(window,frame,context);
    while(state->running) {
        EventRef event=NULL;EventTargetRef target=GetEventDispatcherTarget();
        EventTimeout timeout=(EventTimeout)(kEventDurationSecond/60.0);
        if(ReceiveNextEvent(0,NULL,timeout,true,&event)==noErr&&event) {
            SendEventToEventTarget(event,target);ReleaseEvent(event);
        }
        if(state->running&&!mac_window_frame(window,frame,context))break;
    }
    return 1;
#else
    (void)state;
    return mac_window_frame(window,frame,context);
#endif
}

int mac_window_is_visible(const struct mac_window *window)
{ return window&&window->state&&((struct mac_window_state *)window->state)->visible; }

int mac_window_is_accelerated(const struct mac_window *window)
{
#if defined(__APPLE__)
    GLint accelerated=0;struct mac_window_state *state;
    if(!window||!window->state)return 0;
    state=(struct mac_window_state *)window->state;
    if(state->visible||!state->pixel_format)return 0;
    return CGLDescribePixelFormat(state->pixel_format,0,kCGLPFAAccelerated,&accelerated)==kCGLNoError&&accelerated;
#else
    (void)window;return 0;
#endif
}

/* display.vsync changed in the menus: the swap interval of the window's
   context now (a fullscreen or window context; 0 if there is none) */
int mac_window_fullscreen_active(void)
{ return active_window&&active_window->fullscreen; }

int mac_window_set_vsync_active(int on)
{
    struct mac_window_state *state=active_window;
#if defined(__APPLE__) && !defined(__LP64__)
    GLint interval=on?1:0;
    if(!state||!state->agl)return 0;
    return aglSetInteger(state->agl,AGL_SWAP_INTERVAL,&interval)?1:0;
#else
    (void)state;(void)on;return 0;
#endif
}

int mac_window_resize_active(unsigned width,unsigned height)
{
    struct mac_window_state *state=active_window;
    if(!state||!width||!height||width>8192||height>8192)return 0;
    if(state->fixed_size) {
        /* port: the game's picture keeps its size; a window (not a display's mode)
           of a game takes another, no smaller than it */
        unsigned current_width=state->window_width?state->window_width:state->width;
        unsigned current_height=state->window_height?state->window_height:state->height;
        if(width==current_width&&height==current_height)return 1;
#if defined(__APPLE__) && !defined(__LP64__)
        if(state->visible&&state->window&&!state->fullscreen&&!state->borderless&&width>=state->min_width&&height>=state->min_height) {
            Rect bounds;
            if(GetWindowBounds(state->window,kWindowContentRgn,&bounds)!=noErr||
                (long)bounds.left+(long)width>32767L||(long)bounds.top+(long)height>32767L)return 0;
            bounds.right=(short)(bounds.left+(short)width);bounds.bottom=(short)(bounds.top+(short)height);
            if(SetWindowBounds(state->window,kWindowContentRgn,&bounds)!=noErr)return 0;
            state->window_width=width==state->min_width&&height==state->min_height?0:width;
            state->window_height=width==state->min_width&&height==state->min_height?0:height;
            if(state->agl)aglUpdateContext(state->agl);
            return 1;
        }
#endif
        return 0;
    }
#if defined(__APPLE__)
#if !defined(__LP64__)
    if(state->visible) {
        Rect bounds;
        if(!state->window||GetWindowBounds(state->window,kWindowContentRgn,&bounds)!=noErr)return 0;
        if((unsigned)(bounds.right-bounds.left)==width&&
            (unsigned)(bounds.bottom-bounds.top)==height)return 1;
        if((long)bounds.left+(long)width>32767L||
            (long)bounds.top+(long)height>32767L)return 0;
        bounds.right=(short)(bounds.left+(short)width);
        bounds.bottom=(short)(bounds.top+(short)height);
        if(SetWindowBounds(state->window,kWindowContentRgn,&bounds)!=noErr)return 0;
        state->width=width;state->height=height;
        if(state->agl)aglUpdateContext(state->agl);
        return 1;
    }
#endif
    {
        GLuint new_texture=0,new_depth=0,old_texture,old_depth;
        unsigned old_width,old_height,new_width=next_power_of_two(width),new_height=next_power_of_two(height);
        GLenum status;
        if(!state->cgl||CGLSetCurrentContext(state->cgl)!=kCGLNoError)return 0;
        while(glGetError()!=GL_NO_ERROR){}
        old_texture=state->color_texture;old_depth=state->depth_renderbuffer;
        old_width=state->attachment_width;old_height=state->attachment_height;
        glGenTextures(1,&new_texture);glBindTexture(GL_TEXTURE_2D,new_texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,(GLsizei)new_width,(GLsizei)new_height,0,
            GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        glGenRenderbuffersEXT(1,&new_depth);glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,new_depth);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8_EXT,
            (GLsizei)new_width,(GLsizei)new_height);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,state->framebuffer);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,new_texture,0);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,new_depth);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,new_depth);
        status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if(status!=GL_FRAMEBUFFER_COMPLETE_EXT||glGetError()!=GL_NO_ERROR) {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,old_texture,0);
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,old_depth);
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,old_depth);
            if(new_depth)glDeleteRenderbuffersEXT(1,&new_depth);
            if(new_texture){mac_gpu_textures_forget(new_texture);glDeleteTextures(1,&new_texture);}
            state->attachment_width=old_width;state->attachment_height=old_height;
            return 0;
        }
        state->color_texture=new_texture;state->depth_renderbuffer=new_depth;
        state->attachment_width=new_width;state->attachment_height=new_height;
        state->width=width;state->height=height;
        if(old_depth)glDeleteRenderbuffersEXT(1,&old_depth);
        if(old_texture){mac_gpu_textures_forget(old_texture);glDeleteTextures(1,&old_texture);}
        return 1;
    }
#else
    (void)state;return 0;
#endif
}

void mac_window_close(struct mac_window *window)
{
    struct mac_window_state *state;
    if(!window||!window->state)return;
    state=(struct mac_window_state *)window->state;
#if defined(__APPLE__) && !defined(__LP64__)
    mac_window_mouse_release(state);
    mac_window_ui_mode(state,0);
    if(mac_input_focus_lost_hook==mac_window_focus_lost)mac_input_focus_lost_hook=NULL;
#endif
    if(active_window==state)active_window=NULL;
#if defined(__APPLE__)
#if !defined(__LP64__)
    mac_input_remove(state->input_handler);
    if(state->window_handler)RemoveEventHandler(state->window_handler);
    if(state->agl) {
        aglSetCurrentContext(NULL);
        if(state->window||state->fullscreen)aglSetDrawable(state->agl,NULL);
        aglDestroyContext(state->agl);
    }
    if(state->agl_format)aglDestroyPixelFormat(state->agl_format);
    if(state->window)DisposeWindow(state->window);
    if(state->fullscreen){CGDisplayShowCursor(kCGDirectMainDisplay);CGReleaseAllDisplays();}
#endif
    if(state->cgl||state->pixel_format)mac_window_discard_cgl(state);
#endif
    free(state);window->state=NULL;
}
