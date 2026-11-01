#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
typedef unsigned int GLuint;
typedef int GLint;
typedef uint64_t Uint64;
typedef void (*PFNGLGETSHADERSOURCEPROC)(GLuint,GLint,GLint *,char *);
typedef void (*PFNGLDETACHSHADERPROC)(GLuint,GLuint);
#define GL_VERTEX_SHADER 1
#define GL_FRAGMENT_SHADER 2
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 3
#define GL_LINK_STATUS 4
#define GL_SHADER_SOURCE_LENGTH 5
#define GL_TRUE 1
static char sources[4096][128];
static unsigned next_id=10,deleted,compiled,linked,stored,loaded,polls,detached;
static int enabled=1,cached,failed,no_shader;
static int config_boolean(char const *key) {(void)key;return enabled;}
static char const *platform_save_root(void) {return ".";}
static void platform_log(char const *format,...) {(void)format;}
static void posix_make_directory(char const *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path,0700);
#endif
}
static void get_source(GLuint shader,GLint size,GLint *length,char *out) {
    snprintf(out,size,"%s",sources[shader]);if(length)*length=(GLint)strlen(out);
}
static void detach_shader(GLuint program,GLuint shader) {(void)program;(void)shader;detached++;}
static void (*SDL_GL_GetProcAddress(char const *name))(void) {
    return !strcmp(name,"glGetShaderSource") ? (void (*)(void))get_source : (void (*)(void))detach_shader;
}
static int SDL_GL_ExtensionSupported(char const *name) {(void)name;return 1;}
static Uint64 SDL_GetTicksNS(void) {static Uint64 now;return ++now;}
static void SDL_Delay(unsigned ms) {(void)ms;}
static GLuint glCreateShader(unsigned type) {(void)type;return no_shader ? 0 : ++next_id;}
static GLuint glCreateProgram(void) {return ++next_id;}
static void glDeleteProgram(GLuint id) {assert(id);deleted++;}
static void glDeleteShader(GLuint id) {(void)id;}
static void glShaderSource(GLuint shader,int count,char const *const *source,GLint const *length) {
    (void)count;(void)length;snprintf(sources[shader],sizeof(sources[shader]),"%s",*source);
}
static void glCompileShader(GLuint shader) {(void)shader;compiled++;}
static void glAttachShader(GLuint program,GLuint shader) {(void)program;(void)shader;}
static void glProgramParameteri(GLuint program,unsigned name,int value) {(void)program;(void)name;(void)value;}
static void glLinkProgram(GLuint program) {(void)program;linked++;}
static void glGetProgramiv(GLuint program,unsigned name,GLint *value) {
    (void)program;
    if(name==GL_LINK_STATUS)*value=!failed;
    else {*value=++polls>1;}
}
static void glGetShaderiv(GLuint shader,unsigned name,GLint *value) {
    assert(name==GL_SHADER_SOURCE_LENGTH);*value=(GLint)strlen(sources[shader])+1;
}
#include "under_test.inc"
static unsigned load(shader_prepare_hash a,shader_prepare_hash b) {(void)a;(void)b;if(cached){loaded++;return glCreateProgram();}return 0;}
static void store(unsigned p,shader_prepare_hash a,shader_prepare_hash b) {(void)p;(void)a;(void)b;stored++;}
int main(int argc,char **argv) {
    shader_prepare_hash a,b;
    char path[512];
    GLuint program;
    assert(argc==2);
    strcpy(sources[1],"vertex source");strcpy(sources[2],"fragment source");
    a=prepare_hash(sources[1]);b=prepare_hash(sources[2]);
    if(!strcmp(argv[1],"disabled")) {
        enabled=0;shader_prepare_begin("custom_maps/a",load,store);
        shader_prepare_record(1,2,a,b);assert(!prepare.active && !prepare.count && !compiled);
        shader_prepare_end();return 0;
    }
    shader_prepare_begin("custom_maps/a",load,store);
    shader_prepare_record(1,2,a,b);shader_prepare_record(1,2,a,b);
    assert(prepare.count==1 && !compiled);
    strcpy(path,prepare.path);shader_prepare_end();
    if(!strcmp(argv[1],"invalid")) {
        FILE *file=fopen(path,"wb");fputs("truncated",file);fclose(file);
        shader_prepare_begin("custom_maps/a",load,store);
        assert(prepare.active && !prepare.count && !compiled);
        shader_prepare_record(1,2,a,b);shader_prepare_end();
        shader_prepare_begin("custom_maps/a",load,store);assert(prepare.count==1 && linked==1);
    } else if(!strncmp(argv[1],"malformed_",10)) {
        FILE *file=fopen(path,"r+b");
        struct prepare_header header;
        struct prepare_pair pair;
        char const *kind=argv[1]+10;
        assert(file && fread(&header,sizeof(header),1,file)==1 && fread(&pair,sizeof(pair),1,file)==1);
        if(!strcmp(kind,"length")) {
            pair.vertex_bytes=PREPARE_SOURCE+1;
            fseek(file,sizeof(header),SEEK_SET);fwrite(&pair,sizeof(pair),1,file);
        } else if(!strcmp(kind,"hash")) {
            pair.vertex^=1;
            fseek(file,sizeof(header),SEEK_SET);fwrite(&pair,sizeof(pair),1,file);
        } else if(!strcmp(kind,"terminator")) {
            fseek(file,sizeof(header)+sizeof(pair)+pair.vertex_bytes-1,SEEK_SET);fputc('x',file);
        } else if(!strcmp(kind,"embedded_nul")) {
            fseek(file,sizeof(header)+sizeof(pair),SEEK_SET);fputc(0,file);
        } else if(!strcmp(kind,"trailing")) {
            fseek(file,0,SEEK_END);fputc('x',file);
        } else if(!strcmp(kind,"duplicate")) {
            header.count=2;fseek(file,0,SEEK_SET);fwrite(&header,sizeof(header),1,file);
            fseek(file,0,SEEK_END);fwrite(&pair,sizeof(pair),1,file);
            fwrite(sources[1],pair.vertex_bytes,1,file);fwrite(sources[2],pair.fragment_bytes,1,file);
        } else assert(0);
        fclose(file);shader_prepare_begin("custom_maps/a",load,store);
        assert(prepare.active && !prepare.count && !compiled && !linked);
    } else if(!strcmp(argv[1],"ownership")) {
        shader_prepare_begin("custom_maps/a",load,store);
        assert(prepare.count==1 && compiled==2 && linked==1 && stored==1 && detached==2);
        program=shader_prepare_take(a,b);assert(program && !shader_prepare_take(a,b));
        shader_prepare_end();assert(!deleted);glDeleteProgram(program);
        shader_prepare_begin("mcc_maps/a",load,store);assert(!prepare.count);
    } else if(!strcmp(argv[1],"cached")) {
        cached=1;shader_prepare_begin("custom_maps/a",load,store);
        assert(loaded==1 && !compiled && !linked && !stored);
        shader_prepare_end();assert(deleted==1);
    } else if(!strcmp(argv[1],"failed")) {
        failed=1;shader_prepare_begin("custom_maps/a",load,store);
        assert(linked==1 && !stored && !shader_prepare_take(a,b) && deleted==1);
    } else if(!strcmp(argv[1],"allocation")) {
        no_shader=1;shader_prepare_begin("custom_maps/a",load,store);
        assert(!linked && !stored && !polls && !shader_prepare_take(a,b) && deleted==1);
    } else if(!strcmp(argv[1],"bounds")) {
        FILE *file=fopen(path,"wb");struct prepare_header header={PREPARE_MAGIC,PREPARE_COUNT+1,prepare_hash("custom_maps/a")};
        fwrite(&header,sizeof(header),1,file);fclose(file);
        shader_prepare_begin("custom_maps/a",load,store);assert(!prepare.count && !compiled);
        prepare.bytes=PREPARE_BYTES;shader_prepare_record(1,2,a,b);assert(!prepare.count);
    } else if(!strcmp(argv[1],"toggle")) {
        shader_prepare_begin("custom_maps/a",load,store);
        enabled=0;shader_prepare_begin("custom_maps/b",load,store);
        assert(!shader_prepare_take(a,b) && !prepare.active && deleted==1);
    } else assert(0);
    shader_prepare_end();return 0;
}
