#include "retail_texture.h"
#include <string.h>
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static unsigned int u16(const unsigned char *p) { return p[0]|(unsigned int)p[1]<<8; }
static uint32_t level_size(uint32_t f,uint32_t w,uint32_t h)
{
    if (!w||!h||w>2048||h>2048) return 0;
    if (f>=14&&f<=16) return ((w+3)/4)*((h+3)/4)*(f==14?8:16);
    if (f==6||f==8||f==9) return w*h*2;
    if (f==10||f==11) return w*h*4;
    if (f==17) return w*h;
    return 0;
}
/* bitmaps.c's palette of the P8 bump maps (format 17), which the Xbox
   rasterizer sets for them; absent where the game is not linked */
extern unsigned long global_vector_palette[] __attribute__((weak));
int mac_retail_bitmap_get(const struct mac_retail_cache *c,uint32_t handle,
    uint32_t index,struct mac_retail_bitmap *out)
{
    struct mac_retail_tag tag;
    struct mac_retail_bitmap b;
    const unsigned char *root,*entries,*p;
    uint32_t count,address,flags;
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    if (!mac_retail_tag_get(c,handle&65535,&tag)||tag.handle!=handle||tag.group!=UINT32_C(0x6269746d)) return 0;
    root=mac_retail_tag_span(c,tag.address,0x6c); if (!root) return 0;
    count=u32(root+0x60); address=u32(root+0x64);
    if (index>=count||count>c->tag_size/48||(address&3)) return 0;
    entries=mac_retail_tag_span(c,address,count*48); if (!entries) return 0;
    p=entries+index*48; flags=u16(p+14);
    if (u16(p+8)!=1||u16(p+10)!=0||(flags&8)||((flags&4)&&u16(p+12)!=17)) return 0;
    b.width=u16(p+4); b.height=u16(p+6); b.format=u16(p+12);
    b.level_bytes=level_size(b.format,b.width,b.height);
    b.external=(flags>>8)&1; b.file_offset=u32(p+24); b.stored_bytes=u32(p+28);
    if (!b.level_bytes||b.level_bytes>b.stored_bytes) return 0;
    *out=b; return 1;
}
static void color565(unsigned int v,unsigned char *p)
{
    unsigned int r=v>>11,g=(v>>5)&63,b=v&31;
    p[0]=(unsigned char)((r<<3)|(r>>2)); p[1]=(unsigned char)((g<<2)|(g>>4));
    p[2]=(unsigned char)((b<<3)|(b>>2)); p[3]=255;
}
int mac_retail_texture_decode(uint32_t format,uint32_t width,uint32_t height,
    const unsigned char *source,size_t source_bytes,unsigned char *rgba,size_t rgba_bytes)
{
    uint32_t need=level_size(format,width,height),i,bx,by,x,y,blocks;
    if (!need||!source||!rgba||source_bytes!=need||rgba_bytes!=(size_t)width*height*4) return 0;
    if (format==17) {
        /* each index a normal, from the palette's A8R8G8B8 */
        if (!global_vector_palette) return 0;
        for (i=0;i<width*height;++i) {
            unsigned long v=global_vector_palette[source[i]];
            rgba[i*4]=(unsigned char)(v>>16); rgba[i*4+1]=(unsigned char)(v>>8);
            rgba[i*4+2]=(unsigned char)v; rgba[i*4+3]=(unsigned char)(v>>24);
        }
        return 1;
    }
    if (format==6||format==8||format==9||format==10||format==11) {
        for (i=0;i<width*height;++i) {
            if (format==6) color565(u16(source+i*2),rgba+i*4);
            else if(format==8) {
                unsigned int v=u16(source+i*2),r=(v>>10)&31,g=(v>>5)&31,b=v&31;
                rgba[i*4]=(unsigned char)((r<<3)|(r>>2));
                rgba[i*4+1]=(unsigned char)((g<<3)|(g>>2));
                rgba[i*4+2]=(unsigned char)((b<<3)|(b>>2));
                rgba[i*4+3]=(v&0x8000)?255:0;
            } else if(format==9) {
                /* Match bitmap_get_pixel's four-bit channel replication. */
                unsigned int v=u16(source+i*2);
                rgba[i*4]=(unsigned char)(((v>>8)&15)*17);
                rgba[i*4+1]=(unsigned char)(((v>>4)&15)*17);
                rgba[i*4+2]=(unsigned char)((v&15)*17);
                rgba[i*4+3]=(unsigned char)((v>>12)*17);
            } else { rgba[i*4]=source[i*4+2]; rgba[i*4+1]=source[i*4+1];
                rgba[i*4+2]=source[i*4]; rgba[i*4+3]=format==11?source[i*4+3]:255; }
        }
        return 1;
    }
    blocks=(width+3)/4;
    for (by=0;by<(height+3)/4;++by) for (bx=0;bx<blocks;++bx) {
        const unsigned char *block=source+(by*blocks+bx)*(format==14?8:16);
        const unsigned char *colors=block+(format==14?0:8);
        unsigned char palette[4][4],alpha[8];
        unsigned int c0=u16(colors),c1=u16(colors+2),j,k;
        uint32_t choices=u32(colors+4);
        uint64_t abits=0;
        color565(c0,palette[0]); color565(c1,palette[1]);
        for (j=0;j<3;++j) {
            palette[2][j]=(unsigned char)(c0>c1||format!=14?(2*palette[0][j]+palette[1][j])/3:(palette[0][j]+palette[1][j])/2);
            palette[3][j]=(unsigned char)(c0>c1||format!=14?(palette[0][j]+2*palette[1][j])/3:0);
        }
        palette[2][3]=255; palette[3][3]=(c0>c1||format!=14)?255:0;
        if (format==16) {
            alpha[0]=block[0]; alpha[1]=block[1];
            if (alpha[0]>alpha[1]) for (j=2;j<8;++j) alpha[j]=(unsigned char)(((8-j)*alpha[0]+(j-1)*alpha[1])/7);
            else { for (j=2;j<6;++j) alpha[j]=(unsigned char)(((6-j)*alpha[0]+(j-1)*alpha[1])/5); alpha[6]=0; alpha[7]=255; }
            for (j=0;j<6;++j) abits|=(uint64_t)block[j+2]<<(j*8);
        }
        for (y=0;y<4;++y) for (x=0;x<4;++x) {
            unsigned char *p;
            if (bx*4+x>=width||by*4+y>=height) continue;
            k=y*4+x; p=rgba+((by*4+y)*width+bx*4+x)*4;
            memcpy(p,palette[(choices>>(k*2))&3],4);
            if (format==15) p[3]=(unsigned char)(((block[k/2]>>((k&1)*4))&15)*17);
            if (format==16) p[3]=alpha[(unsigned int)(abits>>(k*3))&7];
        }
    }
    return 1;
}

int mac_retail_base_bitmap(const struct mac_retail_cache *c,uint32_t handle,uint32_t *bitmap)
{
    struct mac_retail_tag tag;
    const unsigned char *root;
    uint32_t result;
    if (!bitmap || !mac_retail_tag_get(c,handle&65535,&tag) || tag.handle!=handle) return 0;
    if (tag.group==UINT32_C(0x73656e76)) {
        root=mac_retail_tag_span(c,tag.address,0x98); if (!root) return 0;
        result=u32(root+0x94);
    } else if (tag.group==UINT32_C(0x73636869)) {
        float u,v;
        if(!mac_retail_chicago_base_bitmap(c,handle,&result,&u,&v))return 0;
    } else return 0;
    if (!mac_retail_tag_get(c,result&65535,&tag)||tag.handle!=result||tag.group!=UINT32_C(0x6269746d)) return 0;
    *bitmap=result; return 1;
}

/* Source witness: source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c
   defines shader_transparent_chicago_definition.chicago.maps as 0xDC-byte
   shader_transparent_chicago_map records. In each record U/V scales are at
   0x54/0x58, offsets at 0x5C/0x60, rotation at 0x64 and the bitmap index
   within its tag_reference at 0x78. The engine draws map index zero first.
   This adapter binds only that opaque base image; later maps and Chicago's
   blending, alpha, lighting, function and animation behavior are not drawn. */
int mac_retail_chicago_base_bitmap(const struct mac_retail_cache *c,uint32_t handle,
    uint32_t *bitmap,float *u_scale,float *v_scale)
{
    struct mac_retail_tag shader,base;
    const unsigned char *root,*map;
    uint32_t count,address,bits,result;
    float u,v;
    if(bitmap)*bitmap=0;if(u_scale)*u_scale=0.0f;if(v_scale)*v_scale=0.0f;
    if(!c||!bitmap||!u_scale||!v_scale||!mac_retail_tag_get(c,handle&65535,&shader)||
       shader.handle!=handle||shader.group!=UINT32_C(0x73636869))return 0;
    root=mac_retail_tag_span(c,shader.address,0x60);if(!root)return 0;
    count=u32(root+0x54);address=u32(root+0x58);
    if(!count||count>4||(address&3)||!mac_retail_tag_span(c,address,count*0xDC))return 0;
    map=mac_retail_tag_span(c,address,0xDC);if(!map)return 0;
    /* Keep the existing fixed-function diagnostic honest: it has no sampler
       address/filter implementation for Chicago's per-map flags. */
    if(u16(map)!=0||u32(map+0x5C)!=0||u32(map+0x60)!=0||u32(map+0x64)!=0)return 0;
    bits=u32(map+0x54);memcpy(&u,&bits,sizeof(u));
    bits=u32(map+0x58);memcpy(&v,&bits,sizeof(v));
    if(u!=u||v!=v||u>1000.0f||u< -1000.0f||v>1000.0f||v< -1000.0f)return 0;
    if(u32(map+0x6C)!=UINT32_C(0x6269746d))return 0;
    result=u32(map+0x78);
    if(!mac_retail_tag_get(c,result&65535,&base)||base.handle!=result||
       base.group!=UINT32_C(0x6269746d))return 0;
    *bitmap=result;*u_scale=u;*v_scale=v;return 1;
}

/* Source witness: source/rasterizer/xbox/rasterizer_xbox_models.c declares
   shader_model_properties.map_u_scale/map_v_scale at 0x9C/0xA0 and
   base_map.index at 0xB0. The generated tag-index cache retains that field
   layout; only the opaque base texture and UV scale are consumed here. */
int mac_retail_model_base_bitmap(const struct mac_retail_cache *c,uint32_t handle,
    uint32_t *bitmap,float *u_scale,float *v_scale)
{
    struct mac_retail_tag shader,base;
    const unsigned char *root;
    uint32_t bitmap_handle,uw,vw;
    float u,v;
    if(bitmap)*bitmap=0;
    if(u_scale)*u_scale=0.0f;
    if(v_scale)*v_scale=0.0f;
    if(!bitmap||!u_scale||!v_scale||
       !mac_retail_tag_get(c,handle&65535,&shader)||shader.handle!=handle||
       shader.group!=UINT32_C(0x736f736f))return 0;
    root=mac_retail_tag_span(c,shader.address,0xB4);
    if(!root||u16(root+0x24)!=4)return 0;
    uw=u32(root+0x9C);vw=u32(root+0xA0);
    memcpy(&u,&uw,sizeof(u));memcpy(&v,&vw,sizeof(v));
    if(u!=u||v!=v||u>1000.0f||u< -1000.0f||v>1000.0f||v< -1000.0f)return 0;
    bitmap_handle=u32(root+0xB0);
    if(!mac_retail_tag_get(c,bitmap_handle&65535,&base)||base.handle!=bitmap_handle||
       base.group!=UINT32_C(0x6269746d))return 0;
    *bitmap=bitmap_handle;*u_scale=u;*v_scale=v;
    return 1;
}
