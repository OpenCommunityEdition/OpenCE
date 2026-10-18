/* The menus' PNG art for the Mac: decoded by ImageIO, uploaded once as an
   RGBA texture (padded to a power of two) and handed to the texture binder
   as a borrowed texture. The pictures are read from the folder named by
   HALO_MAC_MENU_ART_DIR (port/assets/menus of a checkout, until the kit
   embeds them). */
#include "engine_menu_art.h"
#include "../linux/include/input_prompts.h"
#include "../linux/src/port_config.h"
#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/gl.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* mac_gpu_textures.c: the binder's shadow of a texture name, let go before the name is */
void mac_gpu_textures_forget(GLuint name);
#define MAXIMUM_ART 1024
/* name, scale_u, scale_v, width, height and failed are the picture's; variant_* the
   PlayStation pad's picture of the frame (its PNG's name with "@ps" before the
   extension), if it has one */
struct art { const void *texture; char *png; GLuint name; float scale_u, scale_v; long width, height; int failed;
    GLuint variant_name; float variant_scale_u, variant_scale_v; int variant_failed; };
static struct art arts[MAXIMUM_ART];
static unsigned art_count;
void mac_menu_art_register(const void *texture, const char *png)
{
    struct art *art;
    if(art_count>=MAXIMUM_ART || !texture || !png)return;
    art=&arts[art_count];memset(art,0,sizeof(*art));
    art->png=(char *)malloc(strlen(png)+1);
    if(!art->png)return;
    strcpy(art->png,png);art->texture=texture;++art_count;
}
void mac_menu_art_forget(void)
{
    unsigned i;
    for(i=0;i<art_count;++i) {
        if(arts[i].name){mac_gpu_textures_forget(arts[i].name);glDeleteTextures(1,&arts[i].name);}
        if(arts[i].variant_name){mac_gpu_textures_forget(arts[i].variant_name);glDeleteTextures(1,&arts[i].variant_name);}
        free(arts[i].png);
    }
    memset(arts,0,sizeof(arts));art_count=0;
}
static unsigned long power_of_two(unsigned long value)
{ unsigned long result=1;while(result<value)result<<=1;return result; }
/* where the pictures are: HALO_MAC_MENU_ART_DIR, else the menus folder beside
   the executable (the kit's copy of port/assets/menus: ce/shell/...), else a
   source checkout's, from its root */
static const char *art_directory(void)
{
    static char found[1100];
    static int looked;
    const char *env=getenv("HALO_MAC_MENU_ART_DIR");
    if(env&&*env)return env;
    if(!looked) {
        char exe[1024],probe[1100];
        uint32_t size=sizeof(exe);
        struct stat info;
        char *slash;
        looked=1;
        strcpy(found,"port/assets/menus");
        if(_NSGetExecutablePath(exe,&size)==0&&(slash=strrchr(exe,'/'))!=NULL) {
            *slash=0;
            snprintf(probe,sizeof(probe),"%s/menus/ce",exe);
            if(stat(probe,&info)==0&&S_ISDIR(info.st_mode))snprintf(found,sizeof(found),"%s/menus",exe);
        }
    }
    return found;
}
/* RGBA bytes, top row first; NULL on failure */
/* port/linux/src/menu_pc_art.c: a frame of the player's own Halo install, if
   it has one of this PNG's picture and there is such an install */
unsigned char *halo_menus_pc_art(const char *png,long *width,long *height);
const char *halo_menus_pc_art_source(void);
static unsigned char *decode(const char *png,unsigned long *width,unsigned long *height,int variant)
{
    const char *directory=art_directory();
    if(!variant) {
        long pc_width=0,pc_height=0;
        unsigned char *pc=halo_menus_pc_art(png,&pc_width,&pc_height);
        if(pc) {
            if(getenv("HALO_MAC_MENU_DEBUG"))fprintf(stderr,"menus: %s is drawn from %s\n",png,halo_menus_pc_art_source());
            *width=(unsigned long)pc_width;*height=(unsigned long)pc_height;
            return pc;
        }
    }
    char path[1024];
    CFStringRef string;CFURLRef url;CGImageSourceRef source;CGImageRef image;
    CGColorSpaceRef space;CGContextRef context;unsigned char *pixels;
    unsigned long w,h,i;
    if(strlen(directory)+strlen(png)+2>sizeof(path))return NULL;
    snprintf(path,sizeof(path),"%s/%s",directory,png);
    string=CFStringCreateWithCString(NULL,path,kCFStringEncodingUTF8);
    if(!string)return NULL;
    url=CFURLCreateWithFileSystemPath(NULL,string,kCFURLPOSIXPathStyle,false);
    CFRelease(string);
    if(!url)return NULL;
    source=CGImageSourceCreateWithURL(url,NULL);CFRelease(url);
    if(!source)return NULL;
    image=CGImageSourceCreateImageAtIndex(source,0,NULL);CFRelease(source);
    if(!image)return NULL;
    w=CGImageGetWidth(image);h=CGImageGetHeight(image);
    if(!w||!h||w>4096||h>4096){CGImageRelease(image);return NULL;}
    pixels=(unsigned char *)calloc(w*h,4);
    space=CGColorSpaceCreateDeviceRGB();
    context=pixels&&space?CGBitmapContextCreate(pixels,w,h,8,w*4,space,
        kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big):NULL;
    if(space)CGColorSpaceRelease(space);
    if(!context){free(pixels);CGImageRelease(image);return NULL;}
    CGContextDrawImage(context,CGRectMake(0,0,(float)w,(float)h),image);
    CGContextRelease(context);CGImageRelease(image);
    /* straight alpha, as the game blends */
    for(i=0;i<w*h;++i) {
        unsigned char *p=pixels+i*4;
        if(p[3]&&p[3]!=255) {
            unsigned c;
            for(c=0;c<3;++c){unsigned v=((unsigned)p[c]*255U+p[3]/2U)/p[3];p[c]=(unsigned char)(v>255U?255U:v);}
        }
    }
    *width=w;*height=h;return pixels;
}
/* ImageIO's rows are top first; so are the game's texture coordinates */
static int upload(struct art *art,long width,long height,int variant)
{
    unsigned long w=0,h=0,pw,ph,y;unsigned char *pixels,*padded;
    GLint binding,unpack;GLuint name;
    char variant_png[1024];
    const char *png=art->png;
    size_t length=strlen(png);
    if(variant) {
        if(length<5||length+4>sizeof(variant_png)||strcmp(png+length-4,".png")){art->variant_failed=1;return 0;}
        memcpy(variant_png,png,length-4);strcpy(variant_png+length-4,"@ps.png");png=variant_png;
    }
    pixels=decode(png,&w,&h,variant);
    if(!pixels){
        if(variant)art->variant_failed=1;
        else {fprintf(stderr,"menus: could not read %s\n",png);art->failed=1;}
        return 0;
    }
    if(getenv("HALO_MAC_MENU_DEBUG")) {
        unsigned long i,opaque=0,visible=0;
        for(i=0;i<w*h;++i){if(pixels[i*4+3])++visible;if(pixels[i*4+3]==255)++opaque;}
        fprintf(stderr,"menus: %s is %lux%lu, %lu texels with alpha, %lu opaque, first %02x%02x%02x%02x\n",art->png,w,h,visible,opaque,
            pixels[0],pixels[1],pixels[2],pixels[3]);
    }
    pw=power_of_two(w);ph=power_of_two(h);
    padded=(unsigned char *)calloc(pw*ph,4);
    if(!padded){free(pixels);if(variant)art->variant_failed=1;else art->failed=1;return 0;}
    for(y=0;y<h;++y)memcpy(padded+y*pw*4,pixels+y*w*4,w*4);
    free(pixels);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);
    glGenTextures(1,&name);
    glBindTexture(GL_TEXTURE_2D,name);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,(GLsizei)pw,(GLsizei)ph,0,GL_RGBA,GL_UNSIGNED_BYTE,padded);
    glBindTexture(GL_TEXTURE_2D,(GLuint)binding);
    glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
    free(padded);
    if(glGetError()!=GL_NO_ERROR){glDeleteTextures(1,&name);if(variant)art->variant_failed=1;else art->failed=1;return 0;}
    if(variant) {
        art->variant_name=name;
        art->variant_scale_u=(float)w/(float)pw;art->variant_scale_v=(float)h/(float)ph;
        return 1;
    }
    art->name=name;art->width=width;art->height=height;
    /* the game's coordinates are 0 to 1 over the picture (the bitmap's own
       size is the picture's, whatever its texels): over the padded texture's
       part that holds it */
    art->scale_u=(float)w/(float)pw;
    art->scale_v=(float)h/(float)ph;
    return 1;
}
int mac_menu_art_texture(const void *texture, long width, long height,
    unsigned int *name, float scale[4])
{
    unsigned i;
    struct art *art=NULL;
    for(i=0;i<art_count;++i)if(arts[i].texture==texture){art=&arts[i];break;}
    if(!art)return 0;
    if(art->failed||width<=0||height<=0)return -1;
    /* the pad in use is a PlayStation pad (input.prompt_glyphs): its picture, if the frame has one */
    if(!art->variant_failed&&input_prompt_glyphs_playstation(config_string("input.prompt_glyphs"))) {
        if(!art->variant_name)upload(art,width,height,1);
        if(art->variant_name) {
            *name=(unsigned int)art->variant_name;
            scale[0]=art->variant_scale_u;scale[1]=art->variant_scale_v;scale[2]=scale[3]=1.0f;
            return 1;
        }
    }
    if(!art->name&&!upload(art,width,height,0))return -1;
    *name=(unsigned int)art->name;
    scale[0]=art->scale_u;scale[1]=art->scale_v;scale[2]=scale[3]=1.0f;
    return 1;
}
