#include "retail_animation.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint16_t u16(const unsigned char *p) { return (uint16_t)(p[0]|(unsigned)p[1]<<8); }
static int16_t s16(const unsigned char *p) { unsigned n=u16(p);return (int16_t)(n<32768?(int)n:(int)n-65536); }
static int scalar(const unsigned char *p,void *out)
{
    uint32_t bits=u32(p);
    if((bits&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))return 0;
    memcpy(out,&bits,4);return 1;
}
static const unsigned char *graph(const struct mac_retail_cache *c,uint32_t handle,uint32_t *count)
{
    struct mac_retail_tag t;
    const unsigned char *p;
    if(!c || !mac_retail_tag_get(c,handle&65535,&t) || t.handle!=handle || t.group!=UINT32_C(0x616e7472))return NULL;
    p=mac_retail_tag_span(c,t.address,128);
    if(!p || (*count=u32(p+116))>256 || u32(p+104)>64 || u32(p+84)>257)return NULL;
    if(*count && !mac_retail_tag_span(c,u32(p+120),*count*180))return NULL;
    return p;
}
int mac_retail_animation_count(const struct mac_retail_cache *c,uint32_t h,uint32_t *count,const char **error)
{
    if(error)*error="animation graph range or handle";
    if(!count)return 0;
    *count=0;
    if(!graph(c,h,count)) { *count=0;return 0; }
    if(error)*error=NULL;return 1;
}
void mac_retail_animation_free(struct mac_retail_animation *a)
{
    if(!a)return;
    free(a->frame_info);free(a->defaults);free(a->data);memset(a,0,sizeof(*a));
}
static int index_ok(int value,unsigned count) { return value==-1 || (value>=0 && (unsigned)value<count); }
static int stream(const struct mac_retail_cache *c,const unsigned char *record,uint32_t expected,
    const unsigned char **src,unsigned char **out)
{
    *src=NULL;*out=NULL;
    if(u32(record)!=expected)return 0;
    if(!expected)return 1;
    *src=mac_retail_tag_span(c,u32(record+12),expected);
    if(!*src)return 0;
    *out=(unsigned char *)malloc(expected);return *out!=NULL;
}
static int convert_nodes(const struct mac_retail_animation *a,const unsigned char *src,unsigned char *dst,int animated)
{
    unsigned node,k,at=0,kind;
    const uint32_t *masks[3];
    static const unsigned sizes[3]={8,12,4};
    masks[0]=a->rotation_flags;masks[1]=a->translation_flags;masks[2]=a->scale_flags;
    for(node=0;node<(unsigned)a->node_count;++node)for(kind=0;kind<3;++kind) {
        if((int)((masks[kind][node/32]>>(node%32))&1)!=animated)continue;
        if(kind==0)for(k=0;k<8;k+=2) { uint16_t value=u16(src+at+k);memcpy(dst+at+k,&value,2); }
        else for(k=0;k<sizes[kind];k+=4)if(!scalar(src+at+k,dst+at+k))return 0;
        at+=sizes[kind];
    }
    return 1;
}
/* port: a compressed animation (flag 1: cinematic clips). Its data is one
   blob whose header holds offsets to the node headers, keyframe frame
   indices (words), default and keyframe rotations (three words), and
   translations and scales (floats); model_animations.c decompresses it as it
   plays. Every section is located from the header and the node masks, must
   lie inside the blob without overlapping another, and is swapped once. */
struct section { uint32_t at,size,unit; };
static int count_nodes(const uint32_t *mask,int node_count)
{
    int i,n=0;
    for(i=0;i<node_count;++i)if((mask[i/32]>>(i%32))&1)++n;
    return n;
}
static int convert_compressed(const struct mac_retail_animation *a,const unsigned char *src,
    unsigned char *dst,uint32_t size)
{
    struct section sections[16],tmp;
    uint32_t header[11],at,i,j,k,count=0,keys[3]={0,0,0},end;
    unsigned counts[3];
    const uint32_t *masks[3];
    static const unsigned key_sizes[3]={6,12,4};
    if(size<0x2c || size>16u*1024*1024)return 0;
    for(i=0;i<11;++i)header[i]=u32(src+i*4);
    masks[0]=a->rotation_flags;masks[1]=a->translation_flags;masks[2]=a->scale_flags;
    for(i=0;i<3;++i)counts[i]=(unsigned)count_nodes(masks[i],a->node_count);
    /* node header arrays: rotation follows the header, the others are named */
    {
        uint32_t starts[3];
        starts[0]=0x2c;starts[1]=header[3];starts[2]=header[7];
        for(i=0;i<3;++i) {
            if(!counts[i])continue;
            if(starts[i]&1 || starts[i]>size || counts[i]*4>size-starts[i])return 0;
            for(j=0;j<counts[i];++j) {
                uint32_t v=u32(src+starts[i]+j*4),first=v>>12,n=v&4095;
                if(first+n>32767)return 0;
                if(first+n>keys[i])keys[i]=first+n;
            }
            sections[count].at=starts[i];sections[count].size=counts[i]*4;sections[count++].unit=4;
        }
    }
    sections[count].at=0;sections[count].size=0x2c;sections[count++].unit=4;
    /* per kind: frame indices, defaults, keyframes (offsets 0/4/8 of each group) */
    for(i=0;i<3;++i) {
        static const unsigned first_header[3]={0,4,8};
        uint32_t index_at=header[first_header[i]],default_at=header[first_header[i]+1],key_at=header[first_header[i]+2];
        uint32_t default_size=i==2&&!counts[2]?0:(uint32_t)a->node_count*key_sizes[i];
        if(keys[i]) {
            sections[count].at=index_at;sections[count].size=keys[i]*2;sections[count++].unit=2;
            sections[count].at=key_at;sections[count].size=keys[i]*key_sizes[i];sections[count++].unit=i==0?2:4;
        }
        if(default_size) {
            sections[count].at=default_at;sections[count].size=default_size;sections[count++].unit=i==0?2:4;
        }
    }
    for(i=0;i<count;++i) {
        if(sections[i].at&1 || sections[i].at>size || sections[i].size>size-sections[i].at)return 0;
    }
    for(i=1;i<count;++i)for(j=i;j>0 && sections[j].at<sections[j-1].at;--j){tmp=sections[j];sections[j]=sections[j-1];sections[j-1]=tmp;}
    for(i=1;i<count;++i)if(sections[i].at<sections[i-1].at+sections[i-1].size)return 0;
    memcpy(dst,src,size);
    for(i=0;i<count;++i) {
        end=sections[i].at+sections[i].size;
        if(sections[i].unit==2)for(at=sections[i].at;at+2<=end;at+=2){uint16_t v=u16(src+at);memcpy(dst+at,&v,2);}
        else for(at=sections[i].at;at+4<=end;at+=4) {
            uint32_t bits=u32(src+at);
            /* node headers, offsets and float values; floats must be finite */
            memcpy(dst+at,&bits,4);
        }
    }
    /* float sections (not the headers) must hold finite values */
    for(i=0;i<3;++i) {
        uint32_t base,n;
        if(i==0)continue;
        base=header[i==1?5:9];n=(i==1?3u:1u)*(uint32_t)a->node_count;
        if(i==2&&!counts[2])n=0;
        for(k=0;k<n;++k)if((u32(src+base+k*4)&0x7f800000u)==0x7f800000u)return 0;
        base=header[i==1?6:10];n=keys[i]*(i==1?3u:1u);
        for(k=0;k<n;++k)if((u32(src+base+k*4)&0x7f800000u)==0x7f800000u)return 0;
    }
    return 1;
}
int mac_retail_animation_decode(const struct mac_retail_cache *c,uint32_t h,uint32_t index,
    struct mac_retail_animation *out,const char **error)
{
    struct mac_retail_animation a;
    const unsigned char *g,*p,*info,*defaults,*frames;
    uint32_t count,i,j,frame_size=0,default_size,info_stride;
    const uint32_t *masks[3];
    static const unsigned sizes[3]={12,8,4};
    memset(&a,0,sizeof(a));
    if(error)*error="invalid animation";
    if(!out)return 0;
    memset(out,0,sizeof(*out));
#define FAIL(message) do { if(error)*error=(message);mac_retail_animation_free(&a);return 0; } while(0)
    g=graph(c,h,&count);if(!g || index>=count)FAIL("animation graph or index");
    p=mac_retail_tag_span(c,u32(g+120)+index*180,180);
    if(!p || !memchr(p,0,32))FAIL("animation record or name");
    memcpy(a.name,p,32);
    a.type=s16(p+32);a.frame_count=s16(p+34);a.frame_size=s16(p+36);a.frame_info_type=s16(p+38);
    a.node_list_checksum=u32(p+40);a.node_count=s16(p+44);a.loop_frame_index=s16(p+46);
    if(!scalar(p+48,&a.weight) || !scalar(p+68,&a.normalized_weight))FAIL("animation weight");
    a.key_frame_index=s16(p+52);a.second_key_frame_index=s16(p+54);a.next_animation_index=s16(p+56);
    a.flags=u16(p+58);a.sound_index=s16(p+60);a.sound_frame_index=s16(p+62);
    a.left_foot_frame_index=p[64];a.right_foot_frame_index=p[65];a.parent_animation_index=s16(p+66);
    /* port: compressed animations keep their blob in the data stream at
       offset 0 and have no default data (every retail one) */
    if(u32(p+136) || ((a.flags&1) && u32(p+140)))FAIL("compressed animation offset or default data unsupported");
    if(a.flags&~7 || a.type<0 || a.type>2 || a.frame_count<1 || a.frame_count>2048 ||
       a.node_count<1 || a.node_count>64 || (u32(g+104) && (uint32_t)a.node_count!=u32(g+104)) ||
       a.frame_info_type<0 || a.frame_info_type>3)FAIL("animation counts or type");
    /* The engine clamps a positive loop marker to frame_count-1. Retail
       animation 148 has loop 30 for 29 frames. Foot markers are comparison
       events, not memory indices, and can likewise exceed frame_count. */
    if(!index_ok(a.next_animation_index,count) || !index_ok(a.parent_animation_index,count) ||
       !index_ok(a.sound_index,u32(g+84)) || a.loop_frame_index<0 ||
       !index_ok(a.key_frame_index,a.frame_count) || !index_ok(a.second_key_frame_index,a.frame_count) ||
       !index_ok(a.sound_frame_index,a.frame_count))FAIL("animation metadata index");
    for(i=0;i<2;++i) {
        a.translation_flags[i]=u32(p+92+i*4);a.rotation_flags[i]=u32(p+108+i*4);a.scale_flags[i]=u32(p+124+i*4);
    }
    masks[0]=a.translation_flags;masks[1]=a.rotation_flags;masks[2]=a.scale_flags;
    /* Retail masks contain set bits beyond node_count; the engine never reads
       those bits. Preserve them, but count only the declared node streams. */
    for(i=0;i<(uint32_t)a.node_count;++i)for(j=0;j<3;++j)if(masks[j][i/32]&(UINT32_C(1)<<(i%32))) {
        frame_size+=sizes[j];
    }
    default_size=(uint32_t)a.node_count*24-frame_size;
    if(a.flags&1) {
        /* the node masks still say which nodes have keyframes; the blob's
           size is the data stream's own */
        if(a.frame_size<0)FAIL("animation frame size");
        default_size=0;frame_size=u32(p+160);
        if(!frame_size || frame_size>16u*1024*1024)FAIL("compressed animation size");
    } else
    if(a.frame_size<0 || frame_size!=(uint32_t)a.frame_size)FAIL("animation frame size");
    info_stride=a.frame_info_type==0?0:4*((uint32_t)a.frame_info_type+1);
    a.frame_info_size=info_stride*a.frame_count;a.default_size=default_size;a.data_size=(a.flags&1)?frame_size:frame_size*a.frame_count;
    if(!stream(c,p+72,a.frame_info_size,&info,&a.frame_info) ||
       !stream(c,p+140,a.default_size,&defaults,&a.defaults) ||
       !stream(c,p+160,a.data_size,&frames,&a.data))FAIL("animation stream length, address or allocation");
    for(i=0;i<a.frame_info_size;i+=4)if(!scalar(info+i,a.frame_info+i))FAIL("nonfinite frame info");
    if(!(a.flags&1) && !convert_nodes(&a,defaults,a.defaults,0))FAIL("nonfinite animation default");
    if(a.flags&1) {
        if(!convert_compressed(&a,frames,a.data,a.data_size))FAIL("compressed animation layout");
    } else if(frame_size)for(i=0;i<(uint32_t)a.frame_count;++i)
        if(!convert_nodes(&a,frames+i*frame_size,a.data+i*frame_size,1))FAIL("nonfinite animation frame");
    *out=a;if(error)*error=NULL;return 1;
#undef FAIL
}
