/* GPU comparison of the production NV2A->ARBvp1.0 translator (port/mac/mac_nv2a_arb.c)
   against the GLSL path for retail vertex shader table entries: every varying, the position
   (through the depth buffer and the covered pixels), fog and point size.
   Modes (the translator's bone_gather argument): 0 no relative reads; 1 bone gather
   (attributes 10..15 from mac_bone_gather_apply; shaders 31 and 50 by default); 2 native
   relative addressing (MAC_NV2A_ARB_RELATIVE: attributes 10..15 are zeroed for the ARB draw),
   compared with the GLSL path as the renderer uses it today (gather rewrite when the shader
   matches the pattern, else the GLSL's own relative reads).
   Arguments: mode:index tokens (default: 1:31 1:50, mode 0 on 34 37 66, mode 2 on every retail
   shader with relative reads). Hidden CGL context only. See tools/mac_nv2a_arb_gpu_checks.sh. */
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "../port/mac/mac_nv2a_shader_api.h"
#include "../port/mac/mac_glsl120.h"
#include "../port/mac/mac_nv2a_arb.h"
#include "../port/mac/mac_bone_gather.h"
#include "../source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.h"
#undef memset
#undef memcpy
#undef memcmp
#undef malloc
#undef realloc
#undef free
#undef strcmp
#undef strlen
#undef strcpy
#undef strcat
#undef sprintf
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#endif
#ifndef GL_PROGRAM_ERROR_POSITION_ARB
#define GL_PROGRAM_ERROR_POSITION_ARB 0x864B
#endif
#ifndef GL_PROGRAM_ERROR_STRING_ARB
#define GL_PROGRAM_ERROR_STRING_ARB 0x8874
#endif
#ifndef GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#endif
#ifndef GL_PROGRAM_NATIVE_PARAMETERS_ARB
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AA
#endif
#ifndef GL_PROGRAM_NATIVE_TEMPORARIES_ARB
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#endif
#ifndef GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#endif
#ifndef GL_PROGRAM_NATIVE_ATTRIBS_ARB
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AE
#endif

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); } } while (0)

static int has_extension(const char *list,const char *name)
{
    size_t n=strlen(name);const char *p=list;
    while((p=strstr(p,name))!=NULL){if((p==list||p[-1]==' ')&&(p[n]=='\0'||p[n]==' '))return 1;p+=n;}
    return 0;
}

static GLuint compile_glsl(GLenum type,const char *source)
{
    GLuint shader=glCreateShader(type);GLint ok=GL_FALSE;
    if(!shader)return 0;glShaderSource(shader,1,&source,NULL);glCompileShader(shader);
    glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(ok!=GL_TRUE){char log[2048];GLint n=0;glGetShaderInfoLog(shader,sizeof(log),&n,log);if(n<0)n=0;if((size_t)n>=sizeof(log))n=sizeof(log)-1;log[n]=0;fprintf(stderr,"GLSL compile error: %s\n",log);glDeleteShader(shader);return 0;}
    return shader;
}

/* How the GLSL baseline hands its outputs to the fragment shader. Default: the generated vertex shader's own
   varyings, read by name by the fragment shader exactly as the renderer links them (production_fragment).
   MAC_NV2A_ARB_PROBE_BUILTIN=1: gl_TexCoord[0..7] with xFog in gl_FogFragCoord (Apple's R300 GLSL draws nothing
   with that on a hardware vertex path); MAC_NV2A_ARB_PROBE_FOGCOLOR=1: gl_TexCoord with xFog in gl_FrontColor
   (clamped to [0,1] between the stages, so a fog beyond that range interpolates differently from the ARB path). */
static int probe_fog_color,probe_builtin;

static char *glsl_builtin_outputs(const char *source)
{
    static const char assignments[]=
        "gl_TexCoord[0]=xB0; gl_TexCoord[1]=xB1;\n"
        "gl_TexCoord[2]=xT0; gl_TexCoord[3]=xT1;\n"
        "gl_TexCoord[4]=xT2; gl_TexCoord[5]=xT3;\n"
        "gl_TexCoord[6]=xD0; gl_TexCoord[7]=xD1;\n";
    static const char fog_coord[]=" gl_FogFragCoord=xFog;\n",fog_color[]=" gl_FrontColor=vec4(xFog,0.0,0.0,1.0);\n";
    const char *fog=probe_fog_color?fog_color:fog_coord;
    const char *end=strrchr(source,'}');size_t a,b;char *out;
    if(!end)return NULL;a=(size_t)(end-source);b=strlen(source);
    out=(char *)malloc(b+sizeof(assignments)+strlen(fog));if(!out)return NULL;
    memcpy(out,source,a);memcpy(out+a,assignments,sizeof(assignments)-1);memcpy(out+a+sizeof(assignments)-1,fog,strlen(fog));
    memcpy(out+a+sizeof(assignments)-1+strlen(fog),end,b-a+1);return out;
}


#ifndef GL_VERTEX_PROGRAM_POINT_SIZE_ARB
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB 0x8642
#endif

/* output selector: 0..7 xB0 xB1 xT0 xT1 xT2 xT3 xD0 xD1, 8 xFog, read by component.
   The position is checked through the depth buffer (z) and the covered pixels
   (x, y), not gl_FragCoord: a fragment shader that reads gl_FragCoord makes
   Apple's R300 driver run even an ARB vertex program in software
   (kCGLCPGPUVertexProcessing 0), which would hide what this probe measures. */
#define OUTPUT_COUNT 9
#define BODY \
    "uniform int which; uniform int component; void main() { vec4 c;" \
    " if(which==0)c=xB0; else if(which==1)c=xB1; else if(which==2)c=xT0;" \
    " else if(which==3)c=xT1; else if(which==4)c=xT2; else if(which==5)c=xT3;" \
    " else if(which==6)c=xD0; else if(which==7)c=xD1; else c=vec4(xFog);" \
    " float v=component==0?c.r:(component==1?c.g:(component==2?c.b:c.a));" \
    " gl_FragColor=vec4(clamp(v,0.0,1.0),clamp(v,0.0,1.0),clamp(v,0.0,1.0),1.0); }\n"
/* the fragment shader as the GLSL vertex program is linked with it in this
   probe: the vertex source gets gl_TexCoord/gl_Color assignments appended */
static const char glsl_fragment[]=
    "#version 120\n"
    "#define xB0 gl_TexCoord[0]\n#define xB1 gl_TexCoord[1]\n#define xT0 gl_TexCoord[2]\n"
    "#define xT1 gl_TexCoord[3]\n#define xT2 gl_TexCoord[4]\n#define xT3 gl_TexCoord[5]\n"
    "#define xD0 gl_TexCoord[6]\n#define xD1 gl_TexCoord[7]\n#define xFog (gl_FogFragCoord)\n" BODY;
static const char glsl_fragment_color[]=
    "#version 120\n"
    "#define xB0 gl_TexCoord[0]\n#define xB1 gl_TexCoord[1]\n#define xT0 gl_TexCoord[2]\n"
    "#define xT1 gl_TexCoord[3]\n#define xT2 gl_TexCoord[4]\n#define xT3 gl_TexCoord[5]\n"
    "#define xD0 gl_TexCoord[6]\n#define xD1 gl_TexCoord[7]\n#define xFog (gl_Color.r)\n" BODY;
/* the fragment shader in the form mac_glsl120_pixel produces: varyings by name */
static const char production_fragment[]=
    "#version 120\n"
    "varying vec4 xD0;\nvarying vec4 xD1;\nvarying vec4 xB0;\nvarying vec4 xB1;\n"
    "varying vec4 xT0;\nvarying vec4 xT1;\nvarying vec4 xT2;\nvarying vec4 xT3;\n"
    "varying float xFog;\n" BODY;

static GLuint make_glsl_program(const char *vertex_source,GLuint fragment)
{
    GLuint vs=compile_glsl(GL_VERTEX_SHADER,vertex_source),program;GLint linked=GL_FALSE;unsigned i;
    if(!vs)return 0;program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fragment);
    for(i=0;i<16;++i){char name[24];sprintf(name,"v%u_in",i);glBindAttribLocation(program,i,name);}
    glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){char log[2048];GLint n=0;glGetProgramInfoLog(program,sizeof(log),&n,log);if(n<0)n=0;if((size_t)n>=sizeof(log))n=sizeof(log)-1;log[n]=0;fprintf(stderr,"GLSL link error: %s\n",log);glDeleteProgram(program);program=0;}
    glDeleteShader(vs);return program;
}

/* experiment switches for diagnosing a vertex-processing status of 0:
   MAC_NV2A_ARB_PROBE_NOPTS=1 leaves GL_VERTEX_PROGRAM_POINT_SIZE_ARB off for the ARB draw,
   MAC_NV2A_ARB_PROBE_DUMP=1 prints each ARB program and its native resources after a draw */
static int probe_no_point_size,probe_dump,probe_local;

/* MAC_NV2A_ARB_PROBE_LOCAL=1: the program text uses program.local[] (set with glProgramLocalParameter) instead of program.env[] */
static void set_param(GLuint index,const GLfloat *value)
{
    if(probe_local)glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB,index,value);
    else glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB,index,value);
}
struct variant { float screen_offset, point_size; };

static void prepare(GLuint fbo)
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glViewport(0,0,32,32);
    glDisable(GL_DITHER);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_ALWAYS);glDepthMask(GL_TRUE);glClearDepth(1.0);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

static float last_depth[32*32];
/* Apple's R300 GLSL path interpolates a fog beyond [0,1] differently from the unclamped value that the ARB path (and the
   GeForce GLSL path) produce: shader 5 pixel (9,8) is 66 there against 72 on the GeForce (GLSL and ARB) and on the R300 (ARB).
   On a Radeon the xFog comparison of a shader whose fog travels in a texcoord component is reported, not checked. */
static int radeon_renderer,fog_in_texcoord;
static unsigned fog_known_worst;
static unsigned gather_delta_worst,gather_depth_worst,gather_compared,depth_worst,empty_draws,arb_gpu_seen_zero,glsl_gpu_seen_zero,draws_seen;
static float depth_seen_min=1.0f,depth_seen_max=0.0f; /* of the ARB draws: shows the depth buffer was written */

static int finish_draw(CGLContextObj context,const char *label,int *gpu_out,unsigned char pixels[32*32*4])
{
    GLint gpu=-1;CGLError ce=CGLGetParameter(context,kCGLCPGPUVertexProcessing,&gpu);
    *gpu_out=(int)gpu;(void)label;
    if(ce!=kCGLNoError)return 0;
    glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glReadPixels(0,0,32,32,GL_DEPTH_COMPONENT,GL_FLOAT,last_depth);return glGetError()==GL_NO_ERROR;
}

static void set_attributes(GLfloat attributes[3][16][4])
{
    unsigned i;
    for(i=0;i<16;++i){glEnableVertexAttribArray(i);glVertexAttribPointer(i,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][i][0]);}
}

static int draw_glsl(GLuint program,GLfloat constants[192][4],GLfloat attributes[3][16][4],GLuint fbo,
    unsigned which,unsigned component,GLenum mode,const struct variant *v,unsigned char pixels[32*32*4],CGLContextObj context,int *gpu)
{
    GLint loc;
    prepare(fbo);glDisable(GL_VERTEX_PROGRAM_ARB);
    glUseProgram(program);loc=glGetUniformLocation(program,"c");if(loc>=0)glUniform4fv(loc,192,&constants[0][0]);
    loc=glGetUniformLocation(program,"which");if(loc>=0)glUniform1i(loc,(GLint)which);
    loc=glGetUniformLocation(program,"component");if(loc>=0)glUniform1i(loc,(GLint)component);
    loc=glGetUniformLocation(program,"viewport_scale");if(loc>=0)glUniform4f(loc,16,-16,1,0);
    loc=glGetUniformLocation(program,"viewport_offset");if(loc>=0)glUniform4f(loc,16,16,0,0);
    loc=glGetUniformLocation(program,"screen_offset");if(loc>=0)glUniform1f(loc,v->screen_offset);
    loc=glGetUniformLocation(program,"point_size");if(loc>=0)glUniform1f(loc,v->point_size);
    set_attributes(attributes);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    glDrawArrays(mode,0,3);
    {int r=finish_draw(context,"GLSL",gpu,pixels);glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);return r;}
}

static int draw_arb(GLuint program,GLfloat constants[192][4],GLfloat attributes[3][16][4],GLuint fragment_program,GLuint fbo,
    unsigned which,unsigned component,GLenum mode,const struct variant *v,unsigned char pixels[32*32*4],CGLContextObj context,int *gpu)
{
    unsigned i;GLint loc;GLfloat screen_point[4];
    screen_point[0]=v->screen_offset;screen_point[1]=v->point_size;screen_point[2]=0;screen_point[3]=0;
    prepare(fbo);
    glUseProgram(fragment_program);glEnable(GL_VERTEX_PROGRAM_ARB);if(!probe_no_point_size)glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,program);
    loc=glGetUniformLocation(fragment_program,"which");if(loc>=0)glUniform1i(loc,(GLint)which);
    loc=glGetUniformLocation(fragment_program,"component");if(loc>=0)glUniform1i(loc,(GLint)component);
    for(i=0;i<192;++i)set_param(i,constants[i]);
    {static const GLfloat scale[4]={16,-16,1,0},offset[4]={16,16,0,0};
     set_param(MAC_NV2A_ARB_ENV_VIEWPORT_SCALE,scale);
     set_param(MAC_NV2A_ARB_ENV_VIEWPORT_OFFSET,offset);
     set_param(MAC_NV2A_ARB_ENV_SCREEN_POINT,screen_point);}
    set_attributes(attributes);
    glDrawArrays(mode,0,3);
    /* the vertex-processing status is that of the state in force: query before disabling the program */
    i=(unsigned)finish_draw(context,"ARB",gpu,pixels);
    if(probe_dump){GLint ni=-1,np=-1,nt=-1,under=-1;
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&under);
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB,&ni);
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_PARAMETERS_ARB,&np);
        glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_TEMPORARIES_ARB,&nt);
        printf("after draw: gpu=%d under=%d native_inst=%d params=%d temps=%d glError=0x%x\n",*gpu,(int)under,(int)ni,(int)np,(int)nt,(unsigned)glGetError());}
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);glDisable(GL_VERTEX_PROGRAM_ARB);
    return (int)i;
}

static const char *const output_names[OUTPUT_COUNT]={"xB0","xB1","xT0","xT1","xT2","xT3","xD0","xD1","xFog"};
static const char component_names[]="xyzw";

/* returns the largest 8-bit difference over all outputs and components */
static unsigned compare_outputs(const char *tag,GLuint glsl_program,GLuint arb_program,GLuint gather_program,GLuint fragment_program,
    GLfloat constants[192][4],GLfloat attributes[3][16][4],GLfloat arb_attributes[3][16][4],GLuint fbo,CGLContextObj context,GLenum mode,const struct variant *v,
    unsigned worst[OUTPUT_COUNT][4])
{
    unsigned i,component,overall=0;unsigned char a[32*32*4],b[32*32*4];int gpu_glsl=-1,gpu_arb=-1;
    unsigned outputs=mode==GL_POINTS?1:OUTPUT_COUNT;
    for(i=0;i<outputs;++i)for(component=0;component<(mode==GL_POINTS?1u:4u);++component){
        unsigned pixel,delta=0,glsl_covered=0,arb_covered=0,k,depth_delta=0;float depth_a[32*32];
        CHECK(draw_glsl(glsl_program,constants,attributes,fbo,mode==GL_POINTS?2:i,component,mode,v,a,context,&gpu_glsl));
        memcpy(depth_a,last_depth,sizeof(depth_a));
        CHECK(draw_arb(arb_program,constants,arb_attributes,fragment_program,fbo,mode==GL_POINTS?2:i,component,mode,v,b,context,&gpu_arb));
        if(gather_program){ /* the same hardware, the bone gather program fed the gathered rows: must be identical */
            unsigned char g[32*32*4];float depth_b[32*32];int gpu_gather=-1;unsigned gd=0,gz=0;
            memcpy(depth_b,last_depth,sizeof(depth_b));
            CHECK(draw_arb(gather_program,constants,attributes,fragment_program,fbo,mode==GL_POINTS?2:i,component,mode,v,g,context,&gpu_gather));
            CHECK(gpu_gather==1);
            for(pixel=0;pixel<32*32*4;++pixel){unsigned x=g[pixel],y=b[pixel],d=x>y?x-y:y-x;if(d>gd)gd=d;}
            for(pixel=0;pixel<32*32;++pixel){float d=depth_b[pixel]-last_depth[pixel];unsigned lsb;if(d<0)d=-d;lsb=(unsigned)(d*16777216.0f+0.5f);if(lsb>gz)gz=lsb;}
            CHECK(gd==0);CHECK(gz==0);++gather_compared;
            if(gd>gather_delta_worst)gather_delta_worst=gd;if(gz>gather_depth_worst)gather_depth_worst=gz;
        }
        CHECK(gpu_arb==1);++draws_seen;if(gpu_arb!=1)++arb_gpu_seen_zero;if(gpu_glsl!=1)++glsl_gpu_seen_zero;
        for(pixel=0;pixel<32*32;++pixel){unsigned nz=0,nzb=0;for(k=0;k<4;++k){unsigned x=a[pixel*4+k],y=b[pixel*4+k],d=x>y?x-y:y-x;if(d>delta)delta=d;if(x)nz=1;if(y)nzb=1;}
            glsl_covered+=nz;arb_covered+=nzb;}
        for(pixel=0;pixel<32*32;++pixel){float d=depth_a[pixel]-last_depth[pixel];unsigned lsb;if(last_depth[pixel]<1.0f){if(last_depth[pixel]<depth_seen_min)depth_seen_min=last_depth[pixel];if(last_depth[pixel]>depth_seen_max)depth_seen_max=last_depth[pixel];}if(d<0)d=-d;lsb=(unsigned)(d*16777216.0f+0.5f);if(lsb>depth_delta)depth_delta=lsb;}
        if(depth_delta>depth_worst&&(mode!=GL_POINTS||glsl_covered==arb_covered))depth_worst=depth_delta;
        if(delta>worst[i][component]&&(mode!=GL_POINTS||glsl_covered==arb_covered)&&!(radeon_renderer&&fog_in_texcoord&&i==8))worst[i][component]=delta;
        if(delta>overall)overall=delta;
        if(getenv("MAC_NV2A_ARB_PROBE_PIXELS")&&!strcmp(getenv("MAC_NV2A_ARB_PROBE_PIXELS"),"2"))printf("    sample pixel (9,8): GLSL %u ARB %u\n",a[(8*32+9)*4],b[(8*32+9)*4]);
        else if(delta>1&&getenv("MAC_NV2A_ARB_PROBE_PIXELS")){unsigned shown=0;
            for(pixel=0;pixel<32*32&&shown<6;++pixel){unsigned x=a[pixel*4],y=b[pixel*4];if((x>y?x-y:y-x)>1){printf("    pixel (%u,%u): GLSL %u ARB %u\n",pixel%32,pixel/32,x,y);++shown;}}}
        printf("%s %s.%c: GLSLcovered=%u ARBcovered=%u max_delta=%u depth_delta_lsb24=%u vertex_gpu(glsl=%d arb=%d)\n",tag,
            mode==GL_POINTS?"points":output_names[i],component_names[component],glsl_covered,arb_covered,delta,depth_delta,gpu_glsl,gpu_arb);
        if(mode==GL_POINTS){
            /* three points of point_size squared pixels each. Apple's R300 GLSL path
               (software vertex processing) can ignore gl_PointSize, so the ARB draw is
               checked against the count, and against GLSL where GLSL honours the size */
            unsigned expected=3*(unsigned)(v->point_size*v->point_size+0.5f);
            CHECK(arb_covered==expected||arb_covered==glsl_covered); /* skinned fixtures overlap or clip their points */
            if(glsl_covered==arb_covered){CHECK(delta<=1);CHECK(depth_delta<=256);}
            else printf("  note: the GLSL baseline drew %u pixels, ignoring gl_PointSize %.0f; ARB drew the expected %u\n",glsl_covered,v->point_size,expected);
        }else{int fog_known=radeon_renderer&&fog_in_texcoord&&i==8;if(!glsl_covered)++empty_draws;CHECK(arb_covered==glsl_covered);if(fog_known){if(delta>fog_known_worst)fog_known_worst=delta;}else CHECK(delta<=1);CHECK(depth_delta<=256);}
    }
    return overall;
}

static float hashed(unsigned x)
{
    x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;
    return (float)(x&0xFFFF)/65536.0f;
}

#define BONE_COUNT 44 /* rows 60 + 3 * 43 + 2 = 191 */

/* kind 0: identity bone matrices, fractional bone addresses (bones 0 and 1);
   kinds 1, 2 and 3: a different nonidentity matrix for every bone, bone indices (attribute 5 x, y)
   that differ across the three vertices, include bone 0 and the largest valid bone (43), and
   per-vertex varied attributes. Every other constant and attribute is a fixed pseudo-random
   value in [0.05, 0.95) so that the shaders' lighting and texture paths produce unclamped outputs. */
static void fill_fixture(unsigned kind,GLfloat constants[192][4],GLfloat attributes[3][16][4])
{
    static const float bones[3][3][2]={{{0,43},{43,7},{12,12}},{{43,43},{0,0},{21,43}},{{0,43},{43,43},{7,21}}};
    static const float packed[3][3]={{0.0f,2.5f,1.25f},{1.0f,3.75f,0.5f},{0.0f,2.5f,1.25f}};
    unsigned i,j,k,b;
    for(i=0;i<192;++i)for(k=0;k<4;++k)constants[i][k]=0.05f+0.9f*hashed(i*4+k+1000u*kind);
    constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;
    constants[0][1]=constants[0][2]=constants[0][3]=constants[1][0]=constants[1][2]=constants[1][3]=0.0f;
    constants[2][0]=constants[2][1]=constants[2][3]=constants[3][0]=constants[3][1]=constants[3][2]=0.0f;
    constants[7][3]=3.0f;
    constants[13][0]=constants[14][1]=1.0f;
    constants[15][0]=1.0f;constants[15][1]=4.0f;
    constants[28][0]=constants[29][1]=constants[30][2]=constants[31][3]=1.0f;
    constants[58][0]=16.0f;constants[58][1]=-16.0f;constants[58][2]=1.0f;constants[58][3]=0.0f;
    constants[59][0]=16.0f;constants[59][1]=16.0f;constants[59][2]=0.0f;constants[59][3]=0.0f;
    for(b=0;b<BONE_COUNT;++b){
        float (*row)[4]=&constants[60+3*b];float angle=0.07f*(float)b,scale=0.9f+0.004f*(float)b;
        float cs=cosf(angle),sn=sinf(angle);
        for(j=0;j<3;++j)for(k=0;k<4;++k)row[j][k]=0.0f;
        if(!kind){row[0][0]=row[1][1]=row[2][2]=1.0f;continue;}
        row[0][0]=scale*cs;row[0][1]=scale*sn;row[0][3]=0.01f*(float)(b%5);
        row[1][0]=-scale*sn;row[1][1]=scale*cs;row[1][3]=-0.01f*(float)(b%3);
        row[2][2]=scale;row[2][0]=0.002f*(float)(b%7);row[2][3]=0.005f*(float)(b%4);
    }
    memset(attributes,0,3*sizeof(attributes[0]));
    for(i=0;i<3;++i){
        attributes[i][0][0]=i==0?-0.5f:(i==1?0.5f:0.0f);attributes[i][0][1]=i==2?0.5f:-0.5f;attributes[i][0][2]=0.5f;attributes[i][0][3]=1.0f;
        for(j=1;j<16;++j){for(k=0;k<3;++k)attributes[i][j][k]=0.05f+0.9f*hashed(5000u+i*64+j*4+k+300u*kind);attributes[i][j][3]=1.0f;}
        attributes[i][1][0]=attributes[i][1][1]=0.0f;attributes[i][1][2]=1.0f;
        attributes[i][4][0]=attributes[i][4][1]=0.0f;attributes[i][4][2]=1.0f;attributes[i][4][3]=1.0f;
        attributes[i][6][0]=1.0f;attributes[i][6][1]=attributes[i][6][2]=0.0f;attributes[i][6][3]=1.0f;
        attributes[i][5][3]=1.0f;
        if(!kind){attributes[i][5][0]=0.25f;attributes[i][5][1]=0.5f;continue;}
        attributes[i][4][0]=0.1f+(GLfloat)i*0.05f;attributes[i][4][1]=0.08f;
        attributes[i][1][0]=0.2f*(GLfloat)i;attributes[i][1][1]=0.3f-0.1f*(GLfloat)i;
        attributes[i][2][0]=0.25f+0.2f*(GLfloat)i;attributes[i][2][1]=0.75f-0.25f*(GLfloat)i;
        attributes[i][3][0]=0.9f-0.3f*(GLfloat)i;attributes[i][3][1]=0.1f*(GLfloat)i;
        attributes[i][6][0]=0.8f-0.2f*(GLfloat)i;attributes[i][6][1]=0.3f+0.1f*(GLfloat)i;
        attributes[i][5][0]=bones[kind-1][i][0];attributes[i][5][1]=bones[kind-1][i][1];
        attributes[i][8][0]=packed[kind-1][i];
        if(kind==3){ /* the positions of the compressed-vertex shaders (11, 33, ...) stay on screen */
            attributes[i][2][0]=attributes[i][2][1]=attributes[i][2][2]=0.0f;
            attributes[i][3][0]=attributes[i][3][1]=0.0f;attributes[i][3][2]=1.0f;attributes[i][3][3]=0.0f;
        }
    }
    if(kind==3){
        constants[16][0]=constants[16][1]=constants[16][2]=1.0f;constants[16][3]=0.0f;
        constants[6][0]=0.1f;constants[6][1]=constants[6][2]=constants[6][3]=0.0f;
        constants[7][0]=constants[7][2]=0.0f;constants[7][1]=0.1f;
    }
}

static int test_shader(int mode,unsigned index,GLuint fbo,CGLContextObj context,GLuint glsl_fragment_shader)
{
    const struct vertex_shader_entry *entry=&vertex_shader_table[index];const DWORD *function=(const DWORD *)entry->code;
    unsigned long count=function[0]>>16;char error[256],*modern=NULL,*gathered=NULL,*glsl_source=NULL,*vertex120=NULL;unsigned long packed=0;
    struct mac_bone_gather_plan plan;char *arb=NULL;GLuint arb_program=0,gather_program=0,glsl_program=0,fragment_program=0,fragment_shader=0;char *arb_gather=NULL;
    GLint arb_error=-1,under=0,ni=0,np=0,nt=0,na=-1;unsigned vi,kind,worst[OUTPUT_COUNT][4],pworst[OUTPUT_COUNT][4];
    GLfloat constants[192][4],attributes[3][16][4],arb_attributes[3][16][4];int ok=1,gather_ok=0,rewrite;struct mac_nv2a_arb_layout layout;unsigned env_count=0;char *fragment_text=NULL;
    const char *glsl_text;
    static const struct variant variants[]={{0.0f,1.0f},{0.5f,3.0f}};
    probe_no_point_size=getenv("MAC_NV2A_ARB_PROBE_NOPTS")!=NULL;probe_dump=getenv("MAC_NV2A_ARB_PROBE_DUMP")!=NULL;probe_local=getenv("MAC_NV2A_ARB_PROBE_LOCAL")!=NULL;
    memset(worst,0,sizeof(worst));memset(pworst,0,sizeof(pworst));memset(&layout,0,sizeof(layout));memset(&plan,0,sizeof(plan));
    fog_known_worst=0;depth_worst=0;gather_delta_worst=gather_depth_worst=gather_compared=0;empty_draws=0;arb_gpu_seen_zero=glsl_gpu_seen_zero=draws_seen=0;
    modern=nv2a_vertex_shader_to_glsl(function+1,count,0,NULL);CHECK(modern!=NULL);if(!modern)goto failed;
    glsl_text=modern;
    if(mode>=1){
        rewrite=mac_bone_gather_rewrite(modern,&plan,&gathered,error,sizeof(error));
        if(mode==1)CHECK(rewrite==1);
        if(rewrite==1){gather_ok=1;glsl_text=gathered;}
        else if(mode==1)goto failed;
        printf("shader %u mode %d: GLSL baseline is %s\n",index,mode,gather_ok?"the bone gather rewrite":"the GLSL's own relative reads (no gather pattern)");
    }
    CHECK(mac_nv2a_arb_translate((const unsigned long *)function+1,count,mode,&arb,&env_count,&layout,error,sizeof(error))==1);
    if(!arb){fprintf(stderr,"translate refused: %s\n",error);goto failed;}
    printf("shader %u mode %d: xbox=%lu env=%u bone_gather=%d relative=%d fog_texcoord=%d fog_component=%d arb_bytes=%u\n",index,mode,count,env_count,layout.bone_gather_used,layout.relative_used,layout.fog_texcoord,layout.fog_component,(unsigned)strlen(arb));
    if(probe_dump)printf("ARB SOURCE %u BEGIN\n%sARB SOURCE END\n",index,arb);
    if(probe_local){
        char *grown=(char *)malloc(strlen(arb)*2+1),*out=grown;const char *in=arb,*hit;
        if(!grown)goto failed;
        while((hit=strstr(in,"program.env["))!=NULL){memcpy(out,in,(size_t)(hit-in));out+=hit-in;memcpy(out,"program.local[",14);out+=14;in=hit+12;}
        strcpy(out,in);free(arb);arb=grown;
    }
    fog_in_texcoord=layout.fog_texcoord>=0;
    CHECK(env_count==195);CHECK(layout.bone_gather_used==(mode==1));CHECK(mode!=2||layout.relative_used==1);CHECK(mode==2||layout.relative_used==0);
    CHECK(mac_glsl120_vertex(glsl_text,&vertex120,&packed,error,sizeof(error)));if(!vertex120)goto failed;
    if(probe_builtin||probe_fog_color)glsl_source=glsl_builtin_outputs(vertex120);
    else{glsl_source=(char *)malloc(strlen(vertex120)+1);if(glsl_source)strcpy(glsl_source,vertex120);}
    CHECK(glsl_source!=NULL);if(!glsl_source)goto failed;
    glsl_program=make_glsl_program(glsl_source,glsl_fragment_shader);CHECK(glsl_program!=0);if(!glsl_program)goto failed;
    /* the fragment program as the renderer builds it, through the production rewrite */
    CHECK(mac_nv2a_arb_fragment_rewrite(production_fragment,&layout,&fragment_text,error,sizeof(error))==1);
    if(!fragment_text){fprintf(stderr,"fragment rewrite: %s\n",error);goto failed;}
    fragment_shader=compile_glsl(GL_FRAGMENT_SHADER,fragment_text);CHECK(fragment_shader!=0);if(!fragment_shader)goto failed;
    fragment_program=glCreateProgram();glAttachShader(fragment_program,fragment_shader);glLinkProgram(fragment_program);
    {GLint linked=0;glGetProgramiv(fragment_program,GL_LINK_STATUS,&linked);CHECK(linked==GL_TRUE);if(!linked)goto failed;}
    {GLint max_env=0;glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_MAX_PROGRAM_ENV_PARAMETERS_ARB,&max_env);printf("shader %u: GL_MAX_PROGRAM_ENV_PARAMETERS_ARB=%d\n",index,(int)max_env);CHECK(max_env>=(GLint)env_count);}
    glGenProgramsARB(1,&arb_program);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,arb_program);
    glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(arb),arb);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&arb_error);
    if(arb_error>=0){const GLubyte *msg=glGetString(GL_PROGRAM_ERROR_STRING_ARB);fprintf(stderr,"ARB shader %u error at %d: %s\n",index,(int)arb_error,msg?(const char *)msg:"?");fprintf(stderr,"ARB SOURCE BEGIN\n%sARB SOURCE END\n",arb);}
    CHECK(arb_error<0);if(arb_error>=0)goto failed;
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&under);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB,&ni);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_PARAMETERS_ARB,&np);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_TEMPORARIES_ARB,&nt);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB,&na);
    printf("shader %u mode %d ARB resources: under_native_limits=%d native_inst=%d params=%d temps=%d address_regs=%d\n",index,mode,(int)under,(int)ni,(int)np,(int)nt,(int)na);
    CHECK(under==GL_TRUE);if(under!=GL_TRUE)goto failed;
    if(mode==2&&gather_ok){ /* the bone gather ARB program of the same shader, as a second hardware reference */
        struct mac_nv2a_arb_layout gl;
        CHECK(mac_nv2a_arb_translate((const unsigned long *)function+1,count,1,&arb_gather,NULL,&gl,error,sizeof(error))==1);
        if(arb_gather){
            CHECK(gl.fog_texcoord==layout.fog_texcoord&&gl.fog_component==layout.fog_component);
            glGenProgramsARB(1,&gather_program);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,gather_program);
            glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(arb_gather),arb_gather);
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&arb_error);CHECK(arb_error<0);
            if(arb_error>=0){glDeleteProgramsARB(1,&gather_program);gather_program=0;}
        }
    }
    for(kind=0;kind<4;++kind){
        if(mode==0&&kind>=2)break; /* no bones: two fixtures */
        fill_fixture(kind,constants,attributes);
        for(vi=0;vi<2;++vi){
            char tag[64];
            if(gather_ok)CHECK(mac_bone_gather_apply(&plan,&attributes[0][0][0],3,16,(const float (*)[4])constants,error,sizeof(error))==1);
            memcpy(arb_attributes,attributes,sizeof(attributes));
            if(mode==2){unsigned v,a,k;for(v=0;v<3;++v)for(a=10;a<16;++a)for(k=0;k<4;++k)arb_attributes[v][a][k]=0.0f;}
            sprintf(tag,"shader %u mode %d fixture %u v%u",index,mode,kind,vi);
            compare_outputs(tag,glsl_program,arb_program,gather_program,fragment_program,constants,attributes,arb_attributes,fbo,context,GL_TRIANGLES,&variants[vi],worst);
            sprintf(tag,"shader %u mode %d fixture %u points v%u",index,mode,kind,vi);
            compare_outputs(tag,glsl_program,arb_program,gather_program,fragment_program,constants,attributes,arb_attributes,fbo,context,GL_POINTS,&variants[vi],pworst);
        }
    }
    {unsigned o,c,m=0;for(o=0;o<OUTPUT_COUNT;++o)for(c=0;c<4;++c){if(worst[o][c]>m)m=worst[o][c];}
     printf("shader %u mode %d SUMMARY max_delta over all outputs/components=%u (per component, 0..255 scale)\n",index,mode,m);
     for(o=0;o<OUTPUT_COUNT;++o)printf("  %s max_delta x=%u y=%u z=%u w=%u\n",output_names[o],worst[o][0],worst[o][1],worst[o][2],worst[o][3]);
     printf("  points max_delta=%u\n  depth (z) max difference = %u LSB of 24 bits (ARB depths written: %.6f..%.6f)\n",pworst[0][0],depth_worst,depth_seen_min,depth_seen_max);
     if(gather_program)printf("  ARB relative vs ARB bone gather program (same GPU): %u comparisons, max pixel delta %u, max depth difference %u LSB\n",gather_compared,gather_delta_worst,gather_depth_worst);
     printf("RESULT shader %u mode %d: max_delta=%u points_delta=%u depth_lsb24=%u empty_draws=%u arb_vertex_gpu_zero_draws=%u glsl_vertex_gpu_zero_draws=%u of %u draws; native_inst=%d under_native_limits=%d address_regs=%d vs_gather_delta=%u vs_gather_depth=%u r300_glsl_fog_delta=%u\n",
        index,mode,m,pworst[0][0],depth_worst,empty_draws,arb_gpu_seen_zero,glsl_gpu_seen_zero,draws_seen,(int)ni,(int)under,(int)na,gather_program?gather_delta_worst:0u,gather_program?gather_depth_worst:0u,fog_known_worst);}
    goto done;
failed:
    ok=0;fprintf(stderr,"shader %u mode %d translation/setup failed\n",index,mode);
done:
    if(glsl_program)glDeleteProgram(glsl_program);if(fragment_program)glDeleteProgram(fragment_program);if(fragment_shader)glDeleteShader(fragment_shader);
    if(gather_program){glBindProgramARB(GL_VERTEX_PROGRAM_ARB,0);glDeleteProgramsARB(1,&gather_program);}free(arb_gather);
    if(arb_program){glBindProgramARB(GL_VERTEX_PROGRAM_ARB,0);glDeleteProgramsARB(1,&arb_program);}
    free(modern);free(gathered);free(glsl_source);free(vertex120);free(arb);free(fragment_text);return ok;
}

int main(int argc,char **argv)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats=0;GLuint fbo=0,texture=0,fragment=0,depth=0;GLenum status;const GLubyte *ext;unsigned i;int modes[96];unsigned shaders[96],n=0;
    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError&&format);if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError&&context);if(!context)goto done;CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    printf("GL_RENDERER=%s GL_VERSION=%s\n",(const char *)glGetString(GL_RENDERER),(const char *)glGetString(GL_VERSION));
    radeon_renderer=strstr((const char *)glGetString(GL_RENDERER),"Radeon")!=NULL;
    ext=glGetString(GL_EXTENSIONS);CHECK(ext&&has_extension((const char *)ext,"GL_ARB_vertex_program"));if(!ext||!has_extension((const char *)ext,"GL_ARB_vertex_program"))goto done;
    glGenFramebuffersEXT(1,&fbo);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,texture,0);glGenRenderbuffersEXT(1,&depth);glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depth);glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,32,32);glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depth);
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);CHECK(status==GL_FRAMEBUFFER_COMPLETE_EXT);
    probe_fog_color=getenv("MAC_NV2A_ARB_PROBE_FOGCOLOR")!=NULL;
    probe_builtin=getenv("MAC_NV2A_ARB_PROBE_BUILTIN")!=NULL;
    fragment=compile_glsl(GL_FRAGMENT_SHADER,probe_fog_color?glsl_fragment_color:(probe_builtin?glsl_fragment:production_fragment));CHECK(fragment!=0);if(!fragment)goto done;
    if(argc>1){for(i=1;i<(unsigned)argc&&n<96;++i){int m;unsigned x;if(sscanf(argv[i],"%d:%u",&m,&x)==2&&x<67){modes[n]=m;shaders[n++]=x;}}}
    else{
        static const unsigned relative_shaders[]={5,9,10,11,13,14,15,17,18,30,31,32,33,36,39,45,47,50,57,60,62,63,64};
        static const unsigned plain_shaders[]={34,37,29};
        modes[n]=1;shaders[n++]=31;modes[n]=1;shaders[n++]=50;
        for(i=0;i<3;++i){modes[n]=0;shaders[n++]=plain_shaders[i];}
        for(i=0;i<sizeof(relative_shaders)/sizeof(relative_shaders[0]);++i){modes[n]=2;shaders[n++]=relative_shaders[i];}
    }
    for(i=0;i<n;++i)test_shader(modes[i],shaders[i],fbo,context,fragment);
done:
    if(context){CGLSetCurrentContext(context);if(fragment)glDeleteShader(fragment);if(fbo)glDeleteFramebuffersEXT(1,&fbo);if(depth)glDeleteRenderbuffersEXT(1,&depth);if(texture)glDeleteTextures(1,&texture);CGLSetCurrentContext(NULL);CGLDestroyContext(context);}if(format)CGLDestroyPixelFormat(format);
    printf("NV2A ARB GPU probe: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
