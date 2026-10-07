"""Check production occlusion queries with delayed completion and real GL depth."""
import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


COMMON = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef REAL_GL_TYPES
typedef unsigned int DWORD, UINT;
typedef unsigned long long ULONGLONG;
typedef int BOOL, HRESULT;
#define WINAPI
#define TRUE 1
#define FALSE 0
#define S_OK 0
#endif
#define D3DERR_TESTINCOMPLETE -1
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); exit(1); } } while (0)
static float target_scale[2] = {1, 1};
static unsigned target_samples = 1;
static void platform_log(const char *text) {(void)text;}
HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD, UINT *, ULONGLONG *);
'''

FAKE_GL = r'''
typedef unsigned int GLuint, GLenum, GLbitfield;
typedef long long GLintptr, GLsizeiptr;
typedef unsigned long long GLuint64;
typedef struct sync_record *GLsync;
enum {GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED, GL_QUERY_RESULT, GL_QUERY_RESULT_AVAILABLE,
      GL_QUERY_BUFFER, GL_ATOMIC_COUNTER_BUFFER, GL_SYNC_GPU_COMMANDS_COMPLETE,
      GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_EXPIRED, GL_ALREADY_SIGNALED,
      GL_CONDITION_SATISFIED, GL_WAIT_FAILED, GL_DYNAMIC_DRAW};
#define GL_MAP_READ_BIT 1
#define GL_MAP_PERSISTENT_BIT 2
#define GL_MAP_COHERENT_BIT 4
struct sync_record {unsigned sequence; int alive;};
struct write_record {unsigned slot, value;};
static struct sync_record syncs[65536];
static struct write_record writes[65536];
static unsigned next_query, active, next_write, completed, bound_query_buffer, live_syncs;
static GLuint gpu[4098], mapped[4096], counters[4096];
static int map_failure, fence_failure, wait_failure, query_available = TRUE;
static unsigned wait_calls, complete_after_polls;
static void complete(unsigned through);
static struct {int atomic_counters;} xgpu_capabilities;
static void glGenQueries(int n, GLuint *queries) {
    for (int i=0; i<n; i++) queries[i]=++next_query;
}
static void glGenBuffers(int n, GLuint *buffers) {for(int i=0;i<n;i++) buffers[i]=123;}
static void glBindBuffer(GLenum target, GLuint buffer) {
    if(target==GL_QUERY_BUFFER) bound_query_buffer=buffer;
}
static void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, unsigned flags) {
    (void)target;(void)size;(void)data;(void)flags;
    memset(mapped,0xcd,sizeof(mapped)); /* Unspecified initial data. */
}
static void *glMapBufferRange(GLenum target, GLintptr first, GLsizeiptr size, unsigned flags) {
    (void)target;(void)first;(void)size;(void)flags;
    return map_failure ? NULL : mapped;
}
static void glBufferData(GLenum target, GLsizeiptr size, const void *data, unsigned flags) {
    (void)target;(void)size;(void)data;(void)flags;
}
static void glBeginQuery(GLenum target, GLuint query) {(void)target;CHECK(!active);active=query;}
static void glEndQuery(GLenum target) {(void)target;CHECK(active);active=0;}
static void glGetQueryObjectuiv(GLuint query, GLenum pname, GLuint *value) {
    CHECK(!bound_query_buffer);
    *value=pname==GL_QUERY_RESULT_AVAILABLE ? query_available : gpu[query];
}
static void glGetQueryBufferObjectuiv(GLuint query, GLuint buffer, GLenum pname, GLintptr offset) {
    (void)buffer;CHECK(pname==GL_QUERY_RESULT);CHECK(next_write+1<65536);
    writes[++next_write]=(struct write_record){(unsigned)offset/sizeof(GLuint),gpu[query]};
}
static GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    CHECK(condition==GL_SYNC_GPU_COMMANDS_COMPLETE && !flags);
    if(fence_failure) return NULL;
    syncs[next_write]=(struct sync_record){next_write,TRUE};live_syncs++;
    return &syncs[next_write];
}
static GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    CHECK(sync && sync->alive && (flags==0 || flags==GL_SYNC_FLUSH_COMMANDS_BIT) && !timeout);
    wait_calls++;
    if(complete_after_polls && wait_calls%complete_after_polls==0) complete(sync->sequence);
    if(wait_failure) return GL_WAIT_FAILED;
    return sync->sequence<=completed ? GL_ALREADY_SIGNALED : GL_TIMEOUT_EXPIRED;
}
static void glDeleteSync(GLsync sync) {CHECK(sync && sync->alive);sync->alive=FALSE;live_syncs--;}
static void complete(unsigned through) {
    while(completed<through) {
        struct write_record w=writes[++completed];mapped[w.slot]=w.value;
    }
}
static void host_gl_buffer_write(GLenum target, unsigned offset, unsigned size, const void *value) {
    CHECK(target==GL_ATOMIC_COUNTER_BUFFER && size==sizeof(GLuint));
    counters[offset/sizeof(GLuint)]=*(const GLuint *)value;
}
static GLuint host_gl_read_buffer_word(GLuint buffer, unsigned offset) {(void)buffer;return counters[offset/sizeof(GLuint)];}
'''

FAKE_TESTS = r'''
static void reset(int mapped_mode, int atomic) {
    memset(&device,0,sizeof(device));memset(syncs,0,sizeof(syncs));
    memset(gpu,0,sizeof(gpu));memset(counters,0,sizeof(counters));
    next_query=active=next_write=completed=bound_query_buffer=live_syncs=0;
    fence_failure=wait_failure=0;query_available=TRUE;map_failure=!mapped_mode;
    wait_calls=complete_after_polls=0;
    target_scale[0]=target_scale[1]=1;target_samples=1;
    xgpu_capabilities.atomic_counters=atomic;
    visibility_init();device.gl_ready=TRUE;
    CHECK(device.active_query && device.active_query!=device.queries[0]);
    CHECK(!bound_query_buffer);
}
static void submit(unsigned slot, unsigned pixels) {
    D3DDevice_BeginVisibilityTest();
#ifdef HALO_ANDROID
    if(xgpu_capabilities.atomic_counters) counters[device.counter_active]=pixels;
    else
#endif
        gpu[active]=pixels;
    CHECK(D3DDevice_EndVisibilityTest(slot)==S_OK);
}
static unsigned read(unsigned slot) {
    unsigned pixels=0xdeadbeef;ULONGLONG timestamp=123;
    CHECK(D3DDevice_GetVisibilityTestResult(slot,&pixels,&timestamp)==S_OK);
    CHECK(timestamp==0);return pixels;
}
static unsigned expected(unsigned pixels, int atomic) {
#ifdef HALO_ANDROID
    return atomic ? pixels : (pixels ? VISIBILITY_ALL_SAMPLES : 0);
#else
    (void)atomic;return pixels;
#endif
}
static void independence(int mapped_mode, int atomic) {
    reset(mapped_mode,atomic);CHECK(read(0)==0);CHECK(read(4095)==0);
    CHECK(D3DDevice_EndVisibilityTest(0)==S_OK);CHECK(!device.query_pending[0]);
    /* Flare 0 fully behind a wall, flare 1 visible; reserved debug slot distinct. */
    submit(0,0);submit(1,256);submit(4095,17);complete(next_write);
    CHECK(read(0)==0 && read(1)==expected(256,atomic) && read(4095)==expected(17,atomic));
    submit(0,256);submit(1,0);complete(next_write);
    CHECK(read(0)==expected(256,atomic) && read(1)==0);
    CHECK(D3DDevice_GetVisibilityTestResult(0,NULL,NULL)==S_OK);
    /* Four windows' 256 slots, read in reverse order; every fourth light hidden. */
    for(unsigned i=0;i<1024;i++) submit(i,i%4 ? i+1 : 0);
    complete(next_write);
    for(unsigned i=1024;i-->0;) CHECK(read(i)==expected(i%4 ? i+1 : 0,atomic));
    /* All legal slots including zero; ring wrap on Android. */
    for(unsigned i=0;i<4096;i++) submit(i,i%2 ? i+1 : 0);
    complete(next_write);
    for(unsigned i=0;i<4096;i++) CHECK(read(i)==expected(i%2 ? i+1 : 0,atomic));
    submit(4096,0);complete(next_write);CHECK(read(0)==0);
    CHECK(live_syncs<=4096);
    device.gl_ready=FALSE;CHECK(read(1)==0);
}
#ifndef HALO_ANDROID
static void nonblocking_caller(void) {
    reset(TRUE,FALSE);submit(0,256);complete_after_polls=128;
    getter_calls=spin_calls=0;
    long value=_rasterizer_widget_get_occlusion_test_result(0);
    printf("Delayed GPU, actual game getter: %u reads, %u spin entries, result %ld\n",getter_calls,spin_calls,value);
    CHECK(getter_calls==1 && spin_calls==0);
    CHECK(value==0); /* Never completed: conservative initial cached visibility. */
    complete(next_write);CHECK(read(0)==256);
    submit(0,0);getter_calls=spin_calls=0;value=_rasterizer_widget_get_occlusion_test_result(0);
    CHECK(getter_calls==1 && spin_calls==0 && value==256);
    complete(next_write);CHECK(read(0)==0);
}
static void stale_completion(void) {
    reset(TRUE,FALSE);submit(2,256);complete(next_write);CHECK(read(2)==256);
    submit(2,0);unsigned value=777;ULONGLONG timestamp=999;
    CHECK(D3DDevice_GetVisibilityTestResult(2,&value,&timestamp)==S_OK);
    CHECK(value==256 && timestamp==0);complete(next_write);CHECK(read(2)==0);
    /* Keep an unfinished result so persistent multi-frame lag cannot starve it. */
    submit(2,512);unsigned old_sequence=next_write;submit(2,0);
    CHECK(next_write==old_sequence && read(2)==0 && live_syncs==1);
    complete(next_write);CHECK(read(2)==512 && live_syncs==0);
    submit(2,0);complete(next_write);CHECK(read(2)==0);
    for(int i=0;i<500;i++) {
        submit(2,i%2 ? 0 : 64);
        CHECK(read(2)==(i%2 ? 64 : 0));
        complete(next_write);CHECK(read(2)==(i%2 ? 0 : 64));CHECK(live_syncs==0);
    }
    /* Sync creation/wait failures must use the fresh query, never old mapping. */
    submit(2,128);complete(next_write);CHECK(read(2)==128);fence_failure=TRUE;submit(2,0);CHECK(read(2)==0);
    fence_failure=FALSE;submit(2,64);complete(next_write);CHECK(read(2)==64);submit(2,0);
    wait_failure=TRUE;CHECK(read(2)==0);CHECK(!device.visibility_sync[2]);
}
static void scaling_and_fallback(void) {
    for(int mapped_mode=0;mapped_mode<=1;mapped_mode++) {
        reset(mapped_mode,FALSE);target_scale[0]=2;target_scale[1]=1.5f;target_samples=4;
        submit(0,768);target_scale[0]=target_scale[1]=1;target_samples=1;
        submit(1,13);complete(next_write);CHECK(read(0)==64 && read(1)==13);
    }
    reset(FALSE,FALSE);submit(0,64);query_available=FALSE;unsigned value=77;
    CHECK(D3DDevice_GetVisibilityTestResult(0,&value,NULL)==S_OK);
    CHECK(value==0);query_available=TRUE;CHECK(read(0)==64);
}
static void sustained_lag(void) {
    reset(TRUE,FALSE);target_scale[0]=2;target_scale[1]=1.5f;target_samples=4;
    submit(0,768);unsigned pending_sequence=next_write;
    target_scale[0]=target_scale[1]=1;target_samples=1;
    for(unsigned frame=0;frame<5;frame++) {
        getter_calls=spin_calls=0;
        CHECK(_rasterizer_widget_get_occlusion_test_result(0)==0);
        CHECK(getter_calls==1 && spin_calls==0);
        submit(0,0);CHECK(next_write==pending_sequence && live_syncs==1);
    }
    complete(pending_sequence);CHECK(read(0)==64); /* Uses original test's area. */
    submit(0,0);CHECK(read(0)==64);
    complete(next_write);CHECK(read(0)==0);
    puts("PASS: five frames of GPU lag still update; cached counts use completed test's scale; hidden count eventually replaces visible count");
}
#endif
int main(void) {
    independence(FALSE,FALSE);
#ifdef HALO_ANDROID
    independence(FALSE,TRUE);
    puts("PASS: Android boolean/atomic paths; independent slots 0/1/4095; four windows; all 4096 slots and counter wrap");
#else
    nonblocking_caller();independence(TRUE,FALSE);stale_completion();scaling_and_fallback();sustained_lag();
    puts("PASS: desktop mapped/unmapped paths; independent slots 0/1/4095; delayed/replaced results; 500 reuses; bounded syncs; failures; four windows; all slots; sample scaling");
#endif
    return 0;
}
'''

CALLER = r'''
typedef int boolean;
#define NONE -1
static struct {int lens_flare_occlusion_enabled;} rasterizer_debug_options={TRUE};
static int global_d3d_device;
enum {_rasterizer_profile_screen_effect, _error_silent};
static unsigned getter_calls,spin_calls;
static HRESULT counted_get(DWORD index,UINT *result,ULONGLONG *timestamp) {
    getter_calls++;return D3DDevice_GetVisibilityTestResult(index,result,timestamp);
}
#define IDirect3DDevice8_GetVisibilityTestResult(device,index,result,timestamp) counted_get(index,result,timestamp)
static void rasterizer_spin_begin(int profile) {(void)profile;spin_calls++;}
static void rasterizer_spin_end(void) {}
static void rasterizer_error(HRESULT result,const char *text) {(void)result;(void)text;CHECK(FALSE);}
#define match_assert(file,line,condition) CHECK(condition)
#define error(...) CHECK(FALSE)
'''

REAL_GL = r'''
#include <SDL3/SDL.h>
#include "gl.h"
#ifdef _WIN32
#define REAL_GL_TYPES
#endif
#define GL_DEFINE(name) __typeof__(name) name;
GL_FUNCTIONS(GL_DEFINE)
#undef GL_DEFINE
'''

REAL_TESTS = r'''
static unsigned read(unsigned slot) {
    unsigned value=0;Uint64 start=SDL_GetTicks();HRESULT result;
    do {result=D3DDevice_GetVisibilityTestResult(slot,&value,NULL);CHECK(SDL_GetTicks()-start<5000);}
    while(result==D3DERR_TESTINCOMPLETE);
    CHECK(result==S_OK);return value;
}
static GLint depth_location;
static void submit(unsigned slot, float depth) {
    D3DDevice_BeginVisibilityTest();glUniform1f(depth_location,depth);
    glDrawArrays(GL_TRIANGLES,0,3);CHECK(D3DDevice_EndVisibilityTest(slot)==S_OK);
}
static GLuint shader(GLenum type,const char *text) {
    GLuint s=glCreateShader(type);glShaderSource(s,1,&text,NULL);glCompileShader(s);
    GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);CHECK(ok);return s;
}
static int time_compare(const void *a,const void *b) {
    double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);
}
static void benchmark(void) {
    enum {FLARES=128,FRAMES=120};double times[FRAMES];
    GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGenRenderbuffers(1,&color);glBindRenderbuffer(GL_RENDERBUFFER,color);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER,0,GL_RGBA8,512,512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color);
    glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER,0,GL_DEPTH_COMPONENT24,512,512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glViewport(0,0,512,512);glDepthMask(GL_TRUE);glClearDepth(.4);glClear(GL_DEPTH_BUFFER_BIT);glDepthMask(GL_FALSE);
    getter_calls=spin_calls=0;
    Uint64 frequency=SDL_GetPerformanceFrequency();
    for(unsigned frame=0;frame<FRAMES;frame++) {
        for(unsigned i=0;i<FLARES;i++) submit(i,i%2 ? .2f : .7f);
        glFlush(); /* Deliberately queued GPU work, without a completion wait. */
        Uint64 start=SDL_GetPerformanceCounter();
        for(unsigned i=0;i<FLARES;i++) _rasterizer_widget_get_occlusion_test_result(i);
        times[frame]=1000.0*(SDL_GetPerformanceCounter()-start)/frequency;
    }
    glFinish();for(unsigned i=0;i<FLARES;i++) CHECK(read(i)==(i%2 ? 512*512 : 0));
    qsort(times,FRAMES,sizeof(times[0]),time_compare);
    printf("BENCH: queued GPU work, %d flares x %d frames; read-batch median %.4f ms, p95 %.4f ms; getter calls %u, spin entries %u\n",
           FLARES,FRAMES,times[FRAMES/2],times[FRAMES*95/100],getter_calls,spin_calls);
    CHECK(glGetError()==GL_NO_ERROR);
}
int main(int argc,char **argv) {
    (void)argv;
    CHECK(SDL_Init(SDL_INIT_VIDEO));
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *window=SDL_CreateWindow("Halo visibility regression",64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);CHECK(window);
    SDL_GLContext context=SDL_GL_CreateContext(window);CHECK(context);
#define LOAD(name) name=(__typeof__(name))SDL_GL_GetProcAddress(#name);CHECK(name);
    GL_FUNCTIONS(LOAD)
#undef LOAD
    printf("GL renderer: %s\n",glGetString(GL_RENDERER));
    visibility_init();device.gl_ready=TRUE;CHECK(device.visibility_results);
    GLint bound=-1;glGetIntegerv(GL_QUERY_BUFFER_BINDING,&bound);CHECK(bound==0);
    GLuint vao,program=glCreateProgram();glGenVertexArrays(1,&vao);glBindVertexArray(vao);
    const char *vertex="#version 450 core\nuniform float z;void main(){vec2 p[3]=vec2[](vec2(-1,-1),vec2(3,-1),vec2(-1,3));gl_Position=vec4(p[gl_VertexID],z,1);}";
    const char *fragment="#version 450 core\nout vec4 color;void main(){color=vec4(1);}";
    GLuint vs=shader(GL_VERTEX_SHADER,vertex),fs=shader(GL_FRAGMENT_SHADER,fragment);
    glAttachShader(program,vs);glAttachShader(program,fs);glLinkProgram(program);
    GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);CHECK(linked);
    glUseProgram(program);depth_location=glGetUniformLocation(program,"z");CHECK(depth_location>=0);
    glClipControl(GL_UPPER_LEFT,GL_ZERO_TO_ONE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    if(argc>1) {benchmark();SDL_GL_DestroyContext(context);SDL_DestroyWindow(window);SDL_Quit();return 0;}
    for(int samples=0;samples<=4;samples+=4) {
        GLuint fbo,color,depth;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glGenRenderbuffers(1,&color);glBindRenderbuffer(GL_RENDERBUFFER,color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_RGBA8,64,64);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color);
        glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_DEPTH_COMPONENT24,64,64);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
        CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
        GLint actual_samples=0;glGetIntegerv(GL_SAMPLES,&actual_samples);
        target_samples=actual_samples>0 ? actual_samples : 1;glViewport(0,0,64,64);
        glDepthMask(GL_TRUE);glClearDepth(.4);glClear(GL_DEPTH_BUFFER_BIT);glDepthMask(GL_FALSE);
        submit(0,.7f);submit(1,.2f);submit(4095,.2f);
        glFinish();unsigned blocked=read(0),visible=read(1),reserved=read(4095);
        CHECK(blocked==0 && visible==4096 && reserved==4096);
        submit(0,.2f);submit(1,.7f);glFinish();CHECK(read(0)==4096 && read(1)==0);
        /* Half the source is blocked: keep the actual visible fraction. */
        glEnable(GL_SCISSOR_TEST);glScissor(0,0,32,64);glDepthMask(GL_TRUE);
        glClearDepth(1);glClear(GL_DEPTH_BUFFER_BIT);glDepthMask(GL_FALSE);glDisable(GL_SCISSOR_TEST);
        submit(2,.7f);glFinish();CHECK(read(2)==2048);
        /* A scaled split window returns game pixels, not physical/MSAA samples. */
        target_scale[0]=target_scale[1]=2;glViewport(0,0,32,32);submit(3,.2f);
        target_scale[0]=target_scale[1]=1;glFinish();CHECK(read(3)==256);
        glViewport(0,0,64,64);
        for(unsigned frame=0;frame<240;frame++) {
            submit(0,frame%2 ? .2f : .7f);submit(1,frame%2 ? .7f : .2f);
            glFinish();
            CHECK(read(0)==(frame%2 ? 4096 : 2048));
            CHECK(read(1)==(frame%2 ? 2048 : 4096));
        }
        CHECK(glGetError()==GL_NO_ERROR);glDeleteFramebuffers(1,&fbo);
    }
    /* The same real depth results when persistent mapping is unavailable. */
    device.visibility_results=NULL;glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);
    glDepthMask(GL_TRUE);glClearDepth(.4);glClear(GL_DEPTH_BUFFER_BIT);glDepthMask(GL_FALSE);
    target_samples=1;submit(0,.7f);submit(1,.2f);glFinish();CHECK(read(0)==0 && read(1)==4096);
    CHECK(glGetError()==GL_NO_ERROR);
    SDL_GL_DestroyContext(context);SDL_DestroyWindow(window);SDL_Quit();
    puts("PASS: real depth-occluded/visible/half-occluded flares; 1x/4x MSAA; split-window scaling; 480 alternating frames; unmapped fallback; no GL errors");
    return 0;
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default='clang')
    p.add_argument('--source', type=Path, default=ROOT / 'port/linux/src/d3d8_gl.c')
    p.add_argument('--benchmark-only', action='store_true')
    args = p.parse_args()
    source = args.source.read_text()
    definitions = source[source.index('#define VISIBILITY_TEST_SLOTS'):source.index('struct gl_device\n')]
    fields = source[source.index('\tGLuint queries[VISIBILITY_TEST_SLOTS];'):source.index('\n\tunsigned long frame;')]
    device = 'static struct {\n' + fields + '\nBOOL gl_ready;\n} device;\n'
    # Compile the production initialization statements, including buffer unbinding.
    init = source[source.index('\tglGenQueries(VISIBILITY_TEST_SLOTS, device.queries);'):source.index('\n\t{\n\t\tlong every = config_integer("debug.gpu_flush_draws");')]
    init += '\n#endif\n'  # Close the desktop branch whose config block follows.
    atomic = source[source.index('#ifdef HALO_ANDROID\n\tif (xgpu_capabilities.atomic_counters)\n\t{\n\t\tglGenBuffers'):source.index('\n\tfor (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)')]
    functions = '\n'.join(block(source, s) for s in (
        'void WINAPI D3DDevice_BeginVisibilityTest(', 'HRESULT WINAPI D3DDevice_EndVisibilityTest(',
        'static GLuint visibility_unscaled(', 'HRESULT WINAPI D3DDevice_GetVisibilityTestResult('))
    body = definitions + device + 'static void visibility_init(void) {\n' + init + atomic + '\n}\n' + functions
    caller = CALLER + block((ROOT / 'source/rasterizer/xbox/rasterizer_xbox_widgets.c').read_text(),
                           'long _rasterizer_widget_get_occlusion_test_result(')
    compiler = [args.cc, '-std=gnu11', '-O2', '-fuse-ld=lld']
    if sys.platform == 'win32':
        compiler += ['--target=i686-pc-windows-msvc']
    env = dict(os.environ)
    with tempfile.TemporaryDirectory(prefix='halo-visibility-test-') as directory:
        path = Path(directory)
        for name, flags in ([] if args.benchmark_only else [('desktop', []), ('android', ['-DHALO_ANDROID'])]):
            c, exe = path / (name + '.c'), path / (name + '.exe')
            c.write_text(COMMON + FAKE_GL + body + caller + FAKE_TESTS)
            subprocess.run([*compiler, *flags, str(c), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=20)
        gl_compiler = compiler + ['-I' + str(ROOT / 'port/linux/src')]
        if sys.platform == 'win32':
            sdl = next((ROOT / 'build/windows/third_party').glob('SDL3-*/include')).parent
            gl_compiler += ['-I' + str(sdl / 'include')]
            libraries = [str(sdl / 'lib/x86/SDL3.lib')]
            env['PATH'] = str(sdl / 'lib/x86') + os.pathsep + env['PATH']
        else:
            gl_compiler += subprocess.check_output(['pkg-config', '--cflags', 'sdl3'], text=True).split()
            libraries = subprocess.check_output(['pkg-config', '--libs', 'sdl3'], text=True).split()
        c, exe = path / 'real_gl.c', path / 'real_gl.exe'
        c.write_text(REAL_GL + COMMON + body + caller + REAL_TESTS)
        subprocess.run([*gl_compiler, str(c), *libraries, '-o', str(exe)], check=True)
        subprocess.run([str(exe), *(['benchmark'] if args.benchmark_only else [])], env=env, check=True, timeout=45)


if __name__ == '__main__':
    main()
