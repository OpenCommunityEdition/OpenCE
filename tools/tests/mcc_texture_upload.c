/* GL boundary recorder for the actual MCC upload function. Decoders supply
 * distinct ARGB channels; this fixture checks transport, not format decoding. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned long DWORD,D3DCOLOR;
typedef unsigned GLuint,GLenum;
typedef int BOOL,GLint,GLsizei;
#define TRUE 1
#define FALSE 0
#define GL_NO_ERROR 0
#define GL_TEXTURE_2D 1
#define GL_TEXTURE_3D 2
#define GL_TEXTURE_CUBE_MAP 3
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 10
#define GL_MAX_TEXTURE_SIZE 20
#define GL_MAX_3D_TEXTURE_SIZE 21
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 22
#define GL_TEXTURE_SWIZZLE_R 30
#define GL_TEXTURE_SWIZZLE_G 31
#define GL_TEXTURE_SWIZZLE_B 32
#define GL_TEXTURE_SWIZZLE_A 33
#define GL_RED 40
#define GL_GREEN 41
#define GL_BLUE 42
#define GL_ALPHA 43
#define GL_UNPACK_ALIGNMENT 50
#define GL_TEXTURE_BASE_LEVEL 51
#define GL_TEXTURE_MAX_LEVEL 52
#define GL_RGBA8 60
#define GL_RGBA 61
#ifdef HALO_GLES
#define GL_BGRA GL_RGBA
#else
#define GL_BGRA 62
#endif
#define GL_UNSIGNED_BYTE 63
#define _texel_unknown 0
/* DESCRIPTION */
struct format_information {unsigned char kind;};
struct {int s3tc;} xgpu_capabilities;
static unsigned uploaded,compressed,swizzled,decoded,wrong;
static int expected_decode;
static GLenum error;
static struct format_information format_information(DWORD f) {
    struct format_information i;i.kind=(unsigned char)f;return i;
}
static unsigned long level_dimension(unsigned long n,unsigned long l) {n>>=l;return n?n:1;}
static unsigned long level_bytes(const struct xgpu_texture_description *d,unsigned long l) {(void)d;(void)l;return 16;}
static unsigned long xgpu_texture_level_offset(const struct xgpu_texture_description *d,unsigned long l) {(void)d;return l*16;}
static unsigned long xgpu_texture_face_size(const struct xgpu_texture_description *d) {return d->levels*16;}
static GLenum compressed_format(unsigned char k) {(void)k;return 70;}
static void platform_log(const char *fmt,...) {(void)fmt;}
static void texture_dump(GLenum t,const struct xgpu_texture_description *d) {(void)t;(void)d;}
static void xgpu_gl_state_invalidate(void) {}
static void glGetIntegerv(GLenum p,GLint *v) {(void)p;*v=16384;}
static GLenum glGetError(void) {GLenum e=error;error=0;return e;}
static void glBindTexture(GLenum t,GLuint n) {(void)t;(void)n;}
static void glPixelStorei(GLenum p,GLint v) {(void)p;(void)v;}
static void glTexParameteri(GLenum t,GLenum p,GLint v) {
    (void)t;
    if(p>=GL_TEXTURE_SWIZZLE_R && p<=GL_TEXTURE_SWIZZLE_A) {
        swizzled++;
#ifdef HALO_WEB
        error=0x500;
#endif
#ifdef HALO_GLES
        if(v!=(GLint)((expected_decode && p==GL_TEXTURE_SWIZZLE_R)?GL_BLUE:
            (expected_decode && p==GL_TEXTURE_SWIZZLE_B)?GL_RED:
            GL_RED+p-GL_TEXTURE_SWIZZLE_R))wrong++;
#else
        if(v!=(GLint)(GL_RED+p-GL_TEXTURE_SWIZZLE_R))wrong++;
#endif
    }
}
static void fill(unsigned long *p,unsigned long n) {while(n--)*p++=0x7f123456UL;decoded++;}
static BOOL decode_level(const struct xgpu_texture_description *d,unsigned long l,const unsigned char *s,const D3DCOLOR *p,unsigned long *o) {
    (void)s;(void)p;fill(o,level_dimension(d->width,l)*level_dimension(d->height,l)*level_dimension(d->depth,l));return TRUE;
}
#ifdef HALO_GLES
static void dxt_decode_level(unsigned char k,const unsigned char *s,unsigned long w,unsigned long h,unsigned long d,unsigned long *o) {(void)k;(void)s;fill(o,w*h*d);}
#endif
static void glTexImage3D(GLenum t,GLint l,GLint i,GLsizei w,GLsizei h,GLsizei d,GLint b,GLenum f,GLenum type,const void *p) {
    const unsigned long *words=p;unsigned long n=(unsigned long)w*h*d;
    (void)t;(void)l;(void)i;(void)b;(void)type;uploaded++;
    if(f!=GL_BGRA)wrong++;
    while(n--) {
#ifdef HALO_WEB
        if(*words++!=0x7f563412UL)wrong++;
#else
        if(*words++!=0x7f123456UL)wrong++;
#endif
    }
}
static void glTexImage2D(GLenum t,GLint l,GLint i,GLsizei w,GLsizei h,GLint b,GLenum f,GLenum type,const void *p) {glTexImage3D(t,l,i,w,h,1,b,f,type,p);}
static void glCompressedTexImage3D(GLenum t,GLint l,GLenum f,GLsizei w,GLsizei h,GLsizei d,GLint b,GLsizei n,const void *p) {
    (void)t;(void)l;(void)f;(void)w;(void)h;(void)d;(void)b;(void)n;(void)p;compressed++;
#ifdef HALO_WEB
    if(t==GL_TEXTURE_3D || ((w<<l)&3) || ((h<<l)&3))error=0x500;
#endif
}
static void glCompressedTexImage2D(GLenum t,GLint l,GLenum f,GLsizei w,GLsizei h,GLint b,GLsizei n,const void *p) {glCompressedTexImage3D(t,l,f,w,h,1,b,n,p);}
/* UPLOAD */
int main(int argc,char **argv) {
    struct xgpu_texture_description d;
    unsigned char pixels[4096];GLenum target=GL_TEXTURE_2D;unsigned count;
    if(argc!=2)return 90;
    memset(&d,0,sizeof(d));memset(pixels,0x55,sizeof(pixels));
    d.width=d.height=8;d.depth=1;d.levels=4;d.format=1;
    d.compressed=strstr(argv[1],"dxt")!=NULL;xgpu_capabilities.s3tc=strstr(argv[1],"fallback")==NULL;
    if(strstr(argv[1],"volume")){d.depth=4;target=GL_TEXTURE_3D;}
    if(strstr(argv[1],"cube")){d.cube_map=1;target=GL_TEXTURE_CUBE_MAP;}
    if(strstr(argv[1],"odd")){d.width=6;d.levels=1;}
    if(strstr(argv[1],"twelve"))d.width=d.height=12;
    if(strstr(argv[1],"tiny")){d.width=d.height=2;d.levels=2;}
    expected_decode=!d.compressed;
#ifdef HALO_GLES
    expected_decode|=!xgpu_capabilities.s3tc;
#endif
#ifdef HALO_WEB
    expected_decode|=target==GL_TEXTURE_3D || (d.width&3) || (d.height&3);
#endif
    if(!xgpu_mcc_texture_upload(1,target,&d,pixels,sizeof(pixels),NULL))return 1;
    count=(d.cube_map?6:1)*d.levels;
    if(wrong || uploaded!=(expected_decode?count:0) || compressed!=(expected_decode?0:count) || decoded!=uploaded)return 2;
#ifdef HALO_WEB
    if(swizzled)return 3;
#else
    if(swizzled!=4)return 3;
#endif
    for(count=0;count<sizeof(pixels);count++)if(pixels[count]!=0x55)return 4;
    return 0;
}
