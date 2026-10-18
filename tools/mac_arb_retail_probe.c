#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include "../port/mac/mac_nv2a_shader_api.h"
#include "../port/mac/mac_glsl120.h"
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
#ifndef GL_PROGRAM_NATIVE_ATTRIBS_ARB
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AE
#endif

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); } } while (0)

static unsigned long field(const DWORD *p,int word,int low,int count)
{
    return (p[word]>>low)&((1UL<<count)-1);
}

struct text_buffer { char data[65536]; size_t used; int failed; };
static void append(struct text_buffer *t,const char *format,...)
{
    va_list args;int n;
    if(t->failed)return;
    va_start(args,format);n=vsnprintf(t->data+t->used,sizeof(t->data)-t->used,format,args);va_end(args);
    if(n<0||(size_t)n>=sizeof(t->data)-t->used){t->failed=1;return;}
    t->used+=(size_t)n;
}

static unsigned char mac_codes[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13};
static unsigned char ilu_codes[]={0,1,2,3,4,5,6,7};

static int operand_arb(struct text_buffer *t,const DWORD *p,char which,unsigned *relative_read,char *scalar)
{
    unsigned long mux,index,negate,sw[4],address;struct text_buffer base;memset(&base,0,sizeof(base));
    if(which=='A'){
        negate=field(p,1,8,1);sw[0]=field(p,1,6,2);sw[1]=field(p,1,4,2);sw[2]=field(p,1,2,2);sw[3]=field(p,1,0,2);
        index=field(p,2,28,4);mux=field(p,2,26,2);
    }else if(which=='B'){
        negate=field(p,2,25,1);sw[0]=field(p,2,23,2);sw[1]=field(p,2,21,2);sw[2]=field(p,2,19,2);sw[3]=field(p,2,17,2);
        index=field(p,2,13,4);mux=field(p,2,11,2);
    }else{
        negate=field(p,2,10,1);sw[0]=field(p,2,8,2);sw[1]=field(p,2,6,2);sw[2]=field(p,2,4,2);sw[3]=field(p,2,2,2);
        index=(field(p,2,0,2)<<2)|field(p,3,30,2);mux=field(p,3,28,2);
    }
    append(&base,"%s",negate?"-":"");
    if(mux==1){if(index==12)append(&base,"pos");else if(index<12)append(&base,"r%lu",index);else return 0;}
    else if(mux==2)append(&base,"v%lu",field(p,1,9,4));
    else if(mux==3){
        if(field(p,3,1,1)){
            address=field(p,1,13,8);
            if(address!=60+(*relative_read%3)||*relative_read>=6)return 0;
            append(&base,"v%u",10+(*relative_read));++(*relative_read);
        }else append(&base,"c[%lu]",field(p,1,13,8));
    }else return 0;
    if(base.failed)return 0;
    if(scalar)sprintf(scalar,"%s.%c",base.data,"xyzw"[sw[0]]);
    append(t,"%s.%c%c%c%c",base.data,"xyzw"[sw[0]],"xyzw"[sw[1]],"xyzw"[sw[2]],"xyzw"[sw[3]]);
    return !t->failed;
}

static int write_mask(unsigned long mask,char out[5])
{
    unsigned n=0;if(mask&8)out[n++]='x';if(mask&4)out[n++]='y';if(mask&2)out[n++]='z';if(mask&1)out[n++]='w';out[n]=0;return n!=0;
}

static const char *output_register(unsigned long address)
{
    switch(address){case 0:return "pos";case 3:return "od0";case 4:return "od1";
    case 5:return "fog";case 6:return "pts";case 7:return "ob0";
    case 8:return "ob1";case 9:return "ot0";case 10:return "ot1";
    case 11:return "ot2";case 12:return "ot3";default:return NULL;}
}

static void emit_mac(struct text_buffer *t,unsigned long op,const char *dst,const char *a,const char *b,const char *c)
{
    static const char *names[]={"","MOV","MUL","ADD","MAD","DP3","DPH","DP4","DST","MIN","MAX","SLT","SGE",""};
    if(op==0||op==13)return;
    if(op<1||op>13){t->failed=1;return;}
    if(op==1)append(t,"MOV %s, %s;\n",dst,a);
    else if(op==2)append(t,"MUL %s, %s, %s;\n",dst,a,b);
    else if(op==3)append(t,"ADD %s, %s, %s;\n",dst,a,c);
    else if(op==4)append(t,"MAD %s, %s, %s, %s;\n",dst,a,b,c);
    else if(op==5||op==6||op==7||op==8||op==9||op==10||op==11||op==12)
        append(t,"%s %s, %s, %s;\n",names[op],dst,a,b);
}

static void emit_ilu(struct text_buffer *t,unsigned long op,const char *dst,const char *c,const char *cvector)
{
    switch(op){case 0:break;case 1:append(t,"MOV %s, %s;\n",dst,cvector);break;
    case 2:append(t,"RCP %s, %s;\n",dst,c);break;
    case 3:
        append(t,"RCP %s.x, %s;\n",dst,c);
        append(t,"SLT q.x, 0.0, %s.x;\n",dst);
        append(t,"MAX t0.x, %s.x, 5.42101e-20; MIN t0.x, t0.x, 1.884467e+19;\n",dst);
        append(t,"MAX inv.x, %s.x, -1.884467e+19; MIN inv.x, inv.x, -5.42101e-20;\n",dst);
        append(t,"SUB q.y, t0.x, inv.x; MAD %s.x, q.x, q.y, inv.x; MOV %s, %s.xxxx;\n",dst,dst,dst);break;
    case 4:append(t,"RSQ %s, %s;\n",dst,c);break;
    default:t->failed=1;break;}
}

static int translate_arb(const DWORD *instructions,unsigned long count,char **result,unsigned *relative_reads,int *clip_seen)
{
    struct text_buffer *t=(struct text_buffer *)calloc(1,sizeof(*t));unsigned long i;unsigned reads=0;
    if(!t)return 0;
    append(t,"!!ARBvp1.0\n");
    {unsigned a;for(a=0;a<16;++a)append(t,"ATTRIB v%u = vertex.attrib[%u];\n",a,a);}
    append(t,"PARAM c[192] = { program.local[0..191] };\nPARAM half = { 0.5, 0.5, 0.0, 0.0 };\nPARAM viewport_scale = { 16.0, -16.0, 1.0, 0.0 };\nPARAM viewport_offset = { 16.0, 16.0, 0.0, 0.0 };\nTEMP r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, pos, clip, m, u, q, t0, inv, od0, od1, fog, pts, ob0, ob1, ot0, ot1, ot2, ot3;\nADDRESS a0;\n");
    append(t,"MOV pos, { 0.0, 0.0, 0.0, 1.0 };\n");
    for(i=0;i<12;++i)append(t,"MOV r%lu, 0.0;\n",i);
    append(t,"MOV od0, { 0.0, 0.0, 0.0, 1.0 };\nMOV od1, { 0.0, 0.0, 0.0, 1.0 };\nMOV ob0, { 0.0, 0.0, 0.0, 1.0 };\nMOV ob1, { 0.0, 0.0, 0.0, 1.0 };\n");
    for(i=0;i<4;++i)append(t,"MOV ot%lu, { 0.0, 0.0, 0.0, 1.0 };\n",i);
    append(t,"MOV fog, 1.0;\nMOV pts, 1.0;\n");
    for(i=0;i<count&&!t->failed;++i){
        const DWORD *p=instructions+i*4;unsigned long mac=field(p,1,21,4),ilu=field(p,1,25,3),temporary=field(p,3,20,4);
        unsigned long macmask=field(p,3,24,4), ilumask=field(p,3,16,4),outmask=field(p,3,12,4);
        unsigned long outreg=field(p,3,11,1),outaddr=field(p,3,3,8),outilu=field(p,3,2,1);
        char a[96]="0.0",b[96]="0.0",c[96]="0.0",cs[96]="0.0",mask[5];struct text_buffer at,bt,ct;
        memset(&at,0,sizeof(at));memset(&bt,0,sizeof(bt));memset(&ct,0,sizeof(ct));
        if(mac>13||ilu>7||mac_codes[mac]!=mac||ilu_codes[ilu]!=ilu){t->failed=1;break;}
        if((mac==1||mac==2||mac==3||mac==4||(mac>=5&&mac<=13))&&!operand_arb(&at,p,'A',&reads,NULL)){t->failed=1;break;}
        if((mac==2||mac==4||(mac>=5&&mac<=12))&&!operand_arb(&bt,p,'B',&reads,NULL)){t->failed=1;break;}
        if((mac==3||mac==4||ilu!=0)&&!operand_arb(&ct,p,'C',&reads,ilu?cs:NULL)){t->failed=1;break;}
        if(at.used)strcpy(a,at.data);if(bt.used)strcpy(b,bt.data);if(ct.used)strcpy(c,ct.data);
        if(ilu==3&&field(p,3,28,2)==1&&(((field(p,2,0,2)<<2)|field(p,3,30,2))==12)){
            append(t,"MOV clip, pos;\n");*clip_seen=1;
        }
        if(mac!=0&&mac!=13)emit_mac(t,mac,"m",a,b,c);
        if(ilu!=0)emit_ilu(t,ilu,"u",cs,c);
        if(mac!=0&&mac!=13&&macmask&&write_mask(macmask,mask)){
            const char *dest=temporary==12?"pos":(temporary<12?NULL:NULL);
            if(dest)append(t,"MOV %s.%s, m.xyzw;\n",dest,mask);
            else if(temporary<12)append(t,"MOV r%lu.%s, m.xyzw;\n",temporary,mask);
            else t->failed=1;
        }
        if(ilu!=0&&ilumask&&write_mask(ilumask,mask)){
            unsigned long idest=mac!=0?1:temporary;
            if(idest==12)append(t,"MOV pos.%s, u.xyzw;\n",mask);
            else if(idest<12)append(t,"MOV r%lu.%s, u.xyzw;\n",idest,mask);
            else t->failed=1;
        }
        if(outmask&&outreg&&(outilu?ilu:mac)!=0&&write_mask(outmask,mask)){
            const char *dest=output_register(outaddr);if(!dest)t->failed=1;
            else append(t,"MOV %s.%s, %s.xyzw;\n",dest,mask,outilu?"u":"m");
        }
        if(field(p,3,0,1))break;
    }
    if(reads!=6)t->failed=1;
    if(!t->failed&&*clip_seen){
        append(t,"MUL q.xyz, clip, c[58];\nADD t0.xyz, c[59], half;\nSUB t0.xyz, t0, viewport_offset;\nMUL t0.xyz, t0, clip.w;\nADD q.xyz, q, t0;\n");
        append(t,"RCP inv.x, viewport_scale.x; RCP inv.y, viewport_scale.y; RCP inv.z, viewport_scale.z;\nMUL q.xyz, q, inv;\nMAD q.z, q.z, 2.0, -clip.w;\nMOV result.position.xyz, q.xyzw;\nMOV result.position.w, clip.w;\n");
    }else if(!t->failed)t->failed=1;
    append(t,"MAX q, ob0, 0.0; MIN result.texcoord[0], q, 1.0;\nMAX q, ob1, 0.0; MIN result.texcoord[1], q, 1.0;\n");
    append(t,"MOV result.texcoord[2], ot0; MOV result.texcoord[3], ot1; MOV result.texcoord[4], ot2; MOV result.texcoord[5], ot3;\n");
    append(t,"MAX q, od0, 0.0; MIN result.texcoord[6], q, 1.0;\nMAX q, od1, 0.0; MIN result.texcoord[7], q, 1.0;\nMOV result.color.x, fog.x; MOV result.pointsize.x, pts.x;\nEND\n");
    if(t->failed){free(t);return 0;}
    *result=(char *)malloc(t->used+1);if(!*result){free(t);return 0;}
    memcpy(*result,t->data,t->used+1);*relative_reads=reads;free(t);return 1;
}

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

static char *glsl_builtin_outputs(const char *source)
{
    static const char assignments[]=
        "gl_TexCoord[0]=xB0; gl_TexCoord[1]=xB1;\n"
        "gl_TexCoord[2]=xT0; gl_TexCoord[3]=xT1;\n"
        "gl_TexCoord[4]=xT2; gl_TexCoord[5]=xT3;\n"
        "gl_TexCoord[6]=xD0; gl_TexCoord[7]=xD1; gl_FrontColor=vec4(xFog,0.0,0.0,1.0);\n";
    const char *end=strrchr(source,'}');size_t a,b;char *out;
    if(!end)return NULL;a=(size_t)(end-source);b=strlen(source);
    out=(char *)malloc(b+sizeof(assignments));if(!out)return NULL;
    memcpy(out,source,a);memcpy(out+a,assignments,sizeof(assignments)-1);
    memcpy(out+a+sizeof(assignments)-1,end,b-a+1);return out;
}

static const char fragment_source[]=
    "#version 120\n"
    "uniform int which; uniform int component; void main() { vec4 c; if(which==0)c=gl_TexCoord[0]; else if(which==1)c=gl_TexCoord[1];"
    " else if(which==2)c=gl_TexCoord[2]; else if(which==3)c=gl_TexCoord[3];"
    " else if(which==4)c=gl_TexCoord[4]; else if(which==5)c=gl_TexCoord[5];"
    " else if(which==6)c=gl_TexCoord[6]; else if(which==7)c=gl_TexCoord[7];"
    " else c=vec4(gl_Color.r); float v=component==0?c.r:(component==1?c.g:(component==2?c.b:c.a));"
    " gl_FragColor=vec4(clamp(v,0.0,1.0),clamp(v,0.0,1.0),clamp(v,0.0,1.0),1.0); }\n";

static GLuint make_glsl_program(const char *vertex_source,GLuint fragment)
{
    GLuint vs=compile_glsl(GL_VERTEX_SHADER,vertex_source),program;GLint linked=GL_FALSE;unsigned i;
    if(!vs)return 0;program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fragment);
    for(i=0;i<16;++i){char name[24];sprintf(name,"v%u_in",i);glBindAttribLocation(program,i,name);}
    glLinkProgram(program);glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if(linked!=GL_TRUE){char log[2048];GLint n=0;glGetProgramInfoLog(program,sizeof(log),&n,log);if(n<0)n=0;if((size_t)n>=sizeof(log))n=sizeof(log)-1;log[n]=0;fprintf(stderr,"GLSL link error: %s\n",log);glDeleteProgram(program);program=0;}
    glDeleteShader(vs);return program;
}

static int draw_glsl(GLuint program,GLfloat constants[192][4],GLfloat attributes[3][16][4],
    GLuint fbo,unsigned which,unsigned component,unsigned char pixels[32*32*4],CGLContextObj context,const char *label)
{
    GLint loc;unsigned i;GLenum error;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glViewport(0,0,32,32);
    glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);loc=glGetUniformLocation(program,"c");if(loc>=0)glUniform4fv(loc,192,&constants[0][0]);
    loc=glGetUniformLocation(program,"which");if(loc>=0)glUniform1i(loc,(GLint)which);
    loc=glGetUniformLocation(program,"component");if(loc>=0)glUniform1i(loc,(GLint)component);
    loc=glGetUniformLocation(program,"viewport_scale");if(loc>=0)glUniform4f(loc,16,-16,1,0);
    loc=glGetUniformLocation(program,"viewport_offset");if(loc>=0)glUniform4f(loc,16,16,0,0);
    loc=glGetUniformLocation(program,"screen_offset");if(loc>=0)glUniform1f(loc,0);
    loc=glGetUniformLocation(program,"point_size");if(loc>=0)glUniform1f(loc,1);
    for(i=0;i<16;++i){glEnableVertexAttribArray(i);glVertexAttribPointer(i,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][i][0]);}
    glDrawArrays(GL_TRIANGLES,0,3);
    {GLint gpu=-1;CGLError ce=CGLGetParameter(context,kCGLCPGPUVertexProcessing,&gpu);printf("%s vertex_gpu=%d query=%d\n",label,(int)gpu,(int)ce);if(ce!=kCGLNoError)return 0;}
    glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,pixels);error=glGetError();return error==GL_NO_ERROR;
}

static int draw_arb(GLuint program,GLfloat constants[192][4],GLfloat attributes[3][16][4],
    GLuint fragment_program,GLuint fbo,unsigned which,unsigned component,unsigned char pixels[32*32*4],CGLContextObj context,const char *label)
{
    unsigned i;GLenum error;
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glViewport(0,0,32,32);
    glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(fragment_program);glEnable(GL_VERTEX_PROGRAM_ARB);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,program);
    {GLint loc=glGetUniformLocation(fragment_program,"which");if(loc>=0)glUniform1i(loc,(GLint)which);loc=glGetUniformLocation(fragment_program,"component");if(loc>=0)glUniform1i(loc,(GLint)component);}
    for(i=0;i<192;++i)glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB,i,constants[i]);
    for(i=0;i<16;++i){glEnableVertexAttribArray(i);glVertexAttribPointer(i,4,GL_FLOAT,GL_FALSE,sizeof(attributes[0]),&attributes[0][i][0]);}
    glDrawArrays(GL_TRIANGLES,0,3);
    {GLint gpu=-1;CGLError ce=CGLGetParameter(context,kCGLCPGPUVertexProcessing,&gpu);printf("%s vertex_gpu=%d query=%d\n",label,(int)gpu,(int)ce);if(ce!=kCGLNoError)return 0;}
    glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,pixels);error=glGetError();return error==GL_NO_ERROR;
}

static int compare_outputs(unsigned shader_case,GLuint glsl_program,GLuint arb_program,GLuint fragment_program,
    GLfloat constants[192][4],GLfloat attributes[3][16][4],GLuint fbo,CGLContextObj context)
{
    unsigned i,ok=1;unsigned char a[32*32*4],b[32*32*4];
    for(i=0;i<9;++i){unsigned component;for(component=0;component<4;++component){unsigned pixel,delta=0,pass_covered=0,candidate_covered=0;
        CHECK(draw_glsl(glsl_program,constants,attributes,fbo,i,component,a,context,"GLSL baseline"));
        CHECK(draw_arb(arb_program,constants,attributes,fragment_program,fbo,i,component,b,context,"ARB lowered"));
        for(pixel=0;pixel<32*32;++pixel){unsigned k,nonzero=0,candidate_nonzero=0;for(k=0;k<4;++k){unsigned x=a[pixel*4+k],y=b[pixel*4+k],d=x>y?x-y:y-x;if(d>delta)delta=d;if(x)nonzero=1;if(y)candidate_nonzero=1;}if(nonzero)++pass_covered;if(candidate_nonzero)++candidate_covered;}
        printf("shader case %u output %u component %u parity: GLSLcovered=%u ARBcovered=%u max_delta=%u\n",shader_case,i,component,pass_covered,candidate_covered,delta);
        CHECK(pass_covered>0);CHECK(candidate_covered==pass_covered);CHECK(delta<=1);if(!pass_covered||candidate_covered!=pass_covered||delta>1)ok=0;
    }}
    return ok;
}

static int test_shader(unsigned index,GLuint fbo,GLuint texture,CGLContextObj context,GLuint fragment_shader,GLuint fragment_program)
{
    const struct vertex_shader_entry *entry=&vertex_shader_table[index];const DWORD *function=(const DWORD *)entry->code;
    unsigned long count=function[0]>>16;unsigned reads=0;char error[256],*modern=NULL,*gathered=NULL,*glsl_source=NULL,*vertex120=NULL;unsigned long packed=0;
    struct mac_bone_gather_plan plan;char *arb=NULL;GLuint arb_program=0,glsl_program=0;GLint arb_error=-1,under=0,ni=0,np=0,nt=0;unsigned i;
    GLfloat constants[192][4],attributes[3][16][4];int clip_seen=0,ok=1;
    (void)texture;
    modern=nv2a_vertex_shader_to_glsl(function+1,count,0,NULL);CHECK(modern!=NULL);if(!modern)goto failed;
    CHECK(mac_bone_gather_rewrite(modern,&plan,&gathered,error,sizeof(error))==1);if(!gathered)goto failed;
    CHECK(translate_arb(function+1,count,&arb,&reads,&clip_seen));if(!arb)goto failed;
    CHECK(reads==6);printf("shader %u: xbox=%lu relative=%u\n",index,count,reads);
    glsl_source=NULL;CHECK(mac_glsl120_vertex(gathered,&vertex120,&packed,error,sizeof(error)));if(!vertex120)goto failed;
    glsl_source=glsl_builtin_outputs(vertex120);CHECK(glsl_source!=NULL);if(!glsl_source)goto failed;
    glsl_program=make_glsl_program(glsl_source,fragment_shader);CHECK(glsl_program!=0);if(!glsl_program)goto failed;
    glGenProgramsARB(1,&arb_program);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,arb_program);
    glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(arb),arb);
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&arb_error);
    if(arb_error>=0){const GLubyte *msg=glGetString(GL_PROGRAM_ERROR_STRING_ARB);fprintf(stderr,"ARB shader %u error at %d: %s\n",index,(int)arb_error,msg?(const char *)msg:"?");fprintf(stderr,"ARB SOURCE BEGIN\n%sARB SOURCE END\n",arb);}
    CHECK(arb_error<0);if(arb_error>=0)goto failed;
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,&under);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB,&ni);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_PARAMETERS_ARB,&np);
    glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_NATIVE_TEMPORARIES_ARB,&nt);
    printf("shader %u ARB resources: under=%d native_inst=%d params=%d temps=%d\n",index,(int)under,(int)ni,(int)np,(int)nt);
    CHECK(under==GL_TRUE);if(under!=GL_TRUE)goto failed;
    memset(constants,0,sizeof(constants));memset(attributes,0,sizeof(attributes));
    constants[0][0]=constants[1][1]=constants[2][2]=constants[3][3]=1.0f;constants[7][3]=1.0f;
    constants[13][0]=constants[14][1]=1.0f;constants[28][0]=constants[29][1]=constants[30][2]=constants[31][3]=1.0f;
    constants[58][0]=16.0f;constants[58][1]=-16.0f;constants[58][2]=1.0f;constants[59][0]=16.0f;constants[59][1]=16.0f;
    constants[60][0]=1.0f;constants[61][1]=1.0f;constants[62][2]=1.0f;
    for(i=0;i<3;++i){unsigned j;attributes[i][0][0]=i==0?-0.5f:(i==1?0.5f:0.0f);attributes[i][0][1]=i==2?0.5f:-0.5f;attributes[i][0][2]=0.5f;attributes[i][0][3]=1.0f;
        attributes[i][1][2]=1.0f;attributes[i][4][2]=1.0f;attributes[i][4][3]=1.0f;attributes[i][5][3]=1.0f;attributes[i][6][0]=1.0f;attributes[i][6][3]=1.0f;
        attributes[i][plan.address_attribute][0]=0.25f;attributes[i][plan.address_attribute][1]=0.5f;
        for(j=0;j<16;++j)if(j!=1&&j!=4&&j!=5&&j!=6)attributes[i][j][3]=1.0f;}
    CHECK(mac_bone_gather_apply(&plan,&attributes[0][0][0],3,16,(const float (*)[4])constants,error,sizeof(error))==1);
    ok=compare_outputs(index*10,glsl_program,arb_program,fragment_program,constants,attributes,fbo,context);
    constants[60][0]=0.96f;constants[60][1]=0.025f;constants[60][2]=0.0f;constants[60][3]=0.01f;
    constants[61][0]=0.0f;constants[61][1]=0.97f;constants[61][2]=0.02f;constants[61][3]=-0.01f;
    constants[62][0]=0.015f;constants[62][1]=0.0f;constants[62][2]=0.95f;constants[62][3]=0.005f;
    for(i=0;i<3;++i){attributes[i][4][0]=0.1f+(GLfloat)i*0.05f;attributes[i][4][1]=0.08f;attributes[i][5][0]=0.05f;attributes[i][5][1]=0.04f;}
    CHECK(mac_bone_gather_apply(&plan,&attributes[0][0][0],3,16,(const float (*)[4])constants,error,sizeof(error))==1);
    {int varied_ok=compare_outputs(index*10+1,glsl_program,arb_program,fragment_program,constants,attributes,fbo,context);if(!varied_ok)ok=0;}
    if(!ok)goto done;
    goto done;
failed:
    if(!arb||!vertex120)fprintf(stderr,"shader %u translation/setup rejected\n",index);
done:
    if(glsl_program)glDeleteProgram(glsl_program);if(arb_program){glDisable(GL_VERTEX_PROGRAM_ARB);glDeleteProgramsARB(1,&arb_program);}
    free(modern);free(gathered);free(glsl_source);free(vertex120);free(arb);return ok;
}

static int test_rcc(GLuint fbo, CGLContextObj context)
{
    struct rcc_vertex { GLfloat value; GLfloat position[4]; };
    static const char fs[]="#version 120\nvoid main(){float x=0.5+gl_TexCoord[0].r*0.25;gl_FragColor=vec4(clamp(x,0.0,1.0),0.0,0.0,1.0);}\n";
    static const char glsl_vs[]="#version 120\nattribute float input_value; attribute vec4 input_pos;\nvoid main(){float r=1.0/input_value;if(r>0.0)r=clamp(r,5.42101e-20,1.884467e+19);else r=clamp(r,-1.884467e+19,-5.42101e-20);gl_TexCoord[0]=vec4(r);gl_Position=input_pos;gl_PointSize=1.0;}\n";
    const GLfloat values[6]={2.0f,-2.0f,0.0f,-0.0f,-1.0e-30f,1.0f};
    const GLfloat positions[6]={-0.83f,-0.50f,-0.17f,0.17f,0.50f,0.83f};
    const unsigned expected[6]={159,96,255,0,0,191};
    struct text_buffer vp_source;struct rcc_vertex data[6];GLuint vp=0,shader=0,vs=0,program=0,glsl_program=0;GLint errorpos=-1;unsigned char pixels[32*32*4],reference[32*32*4];unsigned i;int ok=1;
    memset(&vp_source,0,sizeof(vp_source));append(&vp_source,"!!ARBvp1.0\nATTRIB x = vertex.attrib[0];\nATTRIB p = vertex.attrib[1];\nTEMP q, t0, inv, u;\nADDRESS a0;\n");
    emit_ilu(&vp_source,3,"u","x.x","x");
    append(&vp_source,"MOV result.texcoord[0].x, u.x; MOV result.position, p; MOV result.pointsize.x, 1.0; END\n");
    CHECK(!vp_source.failed);if(vp_source.failed)return 0;
    for(i=0;i<6;++i){data[i].value=values[i];data[i].position[0]=positions[i];data[i].position[1]=0.0f;data[i].position[2]=0.0f;data[i].position[3]=1.0f;}
    glGenProgramsARB(1,&vp);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,vp);glProgramStringARB(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)vp_source.used,vp_source.data);glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&errorpos);
    CHECK(errorpos<0);if(errorpos>=0)goto done;
    shader=compile_glsl(GL_FRAGMENT_SHADER,fs);CHECK(shader!=0);if(!shader)goto done;
    program=glCreateProgram();glAttachShader(program,shader);glLinkProgram(program);{GLint linked=GL_FALSE;glGetProgramiv(program,GL_LINK_STATUS,&linked);CHECK(linked==GL_TRUE);if(linked!=GL_TRUE)goto done;}
    vs=compile_glsl(GL_VERTEX_SHADER,glsl_vs);CHECK(vs!=0);if(!vs)goto done;
    glsl_program=glCreateProgram();glAttachShader(glsl_program,vs);glAttachShader(glsl_program,shader);glBindAttribLocation(glsl_program,0,"input_value");glBindAttribLocation(glsl_program,1,"input_pos");glLinkProgram(glsl_program);{GLint linked=GL_FALSE;glGetProgramiv(glsl_program,GL_LINK_STATUS,&linked);CHECK(linked==GL_TRUE);if(linked!=GL_TRUE)goto done;}
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glViewport(0,0,32,32);glDisable(GL_DITHER);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_VERTEX_PROGRAM_ARB);glUseProgram(glsl_program);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,1,GL_FLOAT,GL_FALSE,sizeof(data[0]),&data[0].value);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(data[0]),&data[0].position[0]);
    glDrawArrays(GL_POINTS,0,6);glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,reference);
    glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);glEnable(GL_VERTEX_PROGRAM_ARB);glBindProgramARB(GL_VERTEX_PROGRAM_ARB,vp);glDrawArrays(GL_POINTS,0,6);
    {GLint gpu=-1;CGLError ce=CGLGetParameter(context,kCGLCPGPUVertexProcessing,&gpu);printf("RCC control vertex_gpu=%d query=%d\n",(int)gpu,(int)ce);CHECK(ce==kCGLNoError);}
    glFinish();glReadPixels(0,0,32,32,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    for(i=0;i<6;++i){int center=(int)((positions[i]+1.0f)*16.0f),dx,dy,covered=0,glsl_value=-1,arb_value=-1,bad=0;
        for(dy=-2;dy<=2;++dy)for(dx=-2;dx<=2;++dx){int x=center+dx,y=16+dy;unsigned offset;if(x<0||x>=32||y<0||y>=32)continue;offset=((unsigned)y*32+(unsigned)x)*4;
            if(reference[offset+3]==255){++covered;glsl_value=reference[offset];if(pixels[offset+3]!=255||pixels[offset]+1<reference[offset]||pixels[offset]>reference[offset]+1)bad=1;arb_value=pixels[offset];}}
        printf("RCC input=% .9g GLSL=%d ARB=%d expected=%u covered=%d\n",values[i],glsl_value,arb_value,expected[i],covered);
        CHECK(covered>0);CHECK(!bad);if(i!=3){CHECK(glsl_value>=0&&(unsigned)(glsl_value>expected[i]?glsl_value-expected[i]:expected[i]-glsl_value)<=2);}if(!covered||bad)ok=0;}
done:
    glDisable(GL_VERTEX_PROGRAM_ARB);glDisableVertexAttribArray(0);glDisableVertexAttribArray(1);glUseProgram(0);
    if(glsl_program)glDeleteProgram(glsl_program);if(vs)glDeleteShader(vs);if(program)glDeleteProgram(program);if(shader)glDeleteShader(shader);if(vp)glDeleteProgramsARB(1,&vp);return ok;
}

int main(void)
{
    CGLPixelFormatAttribute attrs[]={kCGLPFAAccelerated,kCGLPFAColorSize,(CGLPixelFormatAttribute)24,0};
    CGLPixelFormatObj format=NULL;CGLContextObj context=NULL;GLint formats=0;GLuint fbo=0,texture=0,fragment=0,fragment_program=0;GLenum status;const GLubyte *ext;unsigned indices[]={31,50},i;
    CHECK(CGLChoosePixelFormat(attrs,&format,&formats)==kCGLNoError&&format);if(!format)goto done;
    CHECK(CGLCreateContext(format,NULL,&context)==kCGLNoError&&context);if(!context)goto done;CHECK(CGLSetCurrentContext(context)==kCGLNoError);
    ext=glGetString(GL_EXTENSIONS);CHECK(ext&&has_extension((const char *)ext,"GL_ARB_vertex_program"));if(!ext||!has_extension((const char *)ext,"GL_ARB_vertex_program"))goto done;
    glGenFramebuffersEXT(1,&fbo);glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo);glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,texture,0);status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);CHECK(status==GL_FRAMEBUFFER_COMPLETE_EXT);
    fragment=compile_glsl(GL_FRAGMENT_SHADER,fragment_source);CHECK(fragment!=0);if(!fragment)goto done;
    fragment_program=glCreateProgram();glAttachShader(fragment_program,fragment);glLinkProgram(fragment_program);{GLint linked=0;glGetProgramiv(fragment_program,GL_LINK_STATUS,&linked);CHECK(linked==GL_TRUE);if(!linked)goto done;}
    for(i=0;i<2;++i)test_shader(indices[i],fbo,texture,context,fragment,fragment_program);
    test_rcc(fbo,context);
done:
    if(context){CGLSetCurrentContext(context);if(fragment_program)glDeleteProgram(fragment_program);if(fragment)glDeleteShader(fragment);if(fbo)glDeleteFramebuffersEXT(1,&fbo);if(texture)glDeleteTextures(1,&texture);CGLSetCurrentContext(NULL);CGLDestroyContext(context);}if(format)CGLDestroyPixelFormat(format);
    printf("ARB retail probe: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
