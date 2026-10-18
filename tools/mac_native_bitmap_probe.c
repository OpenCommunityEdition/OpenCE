#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "native_bitmap_tags.h"
#include "native_tags.h"
#include "native_resource_memory.h"
#include "retail_texture.h"
#include "mapped_file.h"
/* Source definition in source/bitmaps/bitmaps.c is not declared by bitmaps.h.
   Keep the probe's float ABI exact under the repository's C89 mode. */
pixel32 bitmap_2d_get_pixel(struct bitmap_data *,union real_point2d const *,real);
#undef malloc
#undef free
#undef memset
#undef memcpy
#undef memcmp
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
void *_texture_cache_bitmap_get_hardware_format(struct bitmap_data *,boolean,boolean);
/* Exercise source's non-cached, rasterizer-created texture route. The probe
   calls the shared CPU resource owner without requiring a live GPU device. */
#undef D3DDevice_CreateTexture
HRESULT WINAPI D3DDevice_CreateTexture(UINT,UINT,UINT,DWORD,D3DFORMAT,
	D3DPOOL,D3DTexture **);
/* The release-body fixture has its own physical-memory macro. */
#undef PLATFORM_PHYSICAL_TO_VIRTUAL
#include "mac_resource_release_probe.h"
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line%d\n",#x,__LINE__);}}while(0)
static void check_bitmap_cache_capacity(void)
{
    struct mac_native_bitmap_tag tags[65];unsigned i;
    memset(tags,0,sizeof(tags));
    for(i=0;i<65;++i) {
        tags[i].owner=(void *)(unsigned long)(i+1);
        tags[i].root=(struct bitmap_group *)(unsigned long)(i+1);
    }
    CHECK(mac_native_bitmap_cache_bind(tags,65));
    CHECK(!mac_native_bitmap_cache_bind(tags,65));
    mac_native_bitmap_cache_unbind();
    CHECK(!mac_native_bitmap_cache_bind(tags,0));
    CHECK(!mac_native_bitmap_cache_bind(tags,65537));
    tags[64].owner=NULL;
    CHECK(!mac_native_bitmap_cache_bind(tags,65));
    tags[64].owner=(void *)(unsigned long)65;
    CHECK(mac_native_bitmap_cache_bind(tags,65));
    mac_native_bitmap_cache_unbind();
    CHECK(mac_native_bitmap_cache_bind(tags,65));
    mac_native_bitmap_cache_unbind();
}
static unsigned char *read_file(const char*p,size_t*n){FILE*f=fopen(p,"rb");long s;unsigned char*b;if(!f)return NULL;if(fseek(f,0,SEEK_END)||(s=ftell(f))<0||s>0x30000000||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}b=malloc(s);if(!b||fread(b,1,s,f)!=(size_t)s){free(b);fclose(f);return NULL;}fclose(f);*n=s;return b;}
static unsigned r32(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static uint32_t cube_level_bytes(uint32_t format,uint32_t w,uint32_t h)
{return format<14?w*h*(format==6||format==8||format==9?2U:4U):((w+3)/4)*((h+3)/4)*(format==14?8U:16U);}
static void check_encoded_dxt(struct mac_native_bitmap_tag *tag,
    struct bitmap_data *bitmap,const unsigned char *external,size_t external_bytes)
{
    D3DTexture *hardware=(D3DTexture *)bitmap->hardware_format;
    size_t source_offset=0;
    unsigned mip;
    if(bitmap->format<14||bitmap->format>16)return;
    for(mip=0;mip<=(unsigned)bitmap->mipmap_count;++mip) {
        uint32_t width=(uint32_t)bitmap->width>>mip;
        uint32_t height=(uint32_t)bitmap->height>>mip;
        uint32_t format=999,out_width=999,out_height=999;
        size_t bytes=999,expected;
        const unsigned char *owner_bytes,*cache_bytes;
        if(!width)width=1;if(!height)height=1;
        expected=cube_level_bytes(bitmap->format,width,height);
        owner_bytes=mac_native_bitmap_encoded_face(tag->owner,0,0,mip,
            &format,&out_width,&out_height,&bytes);
        CHECK(owner_bytes&&format==(uint32_t)bitmap->format&&out_width==width&&
            out_height==height&&bytes==expected);
        format=out_width=out_height=999;bytes=999;
        cache_bytes=mac_native_bitmap_cache_encoded_face(hardware,0,mip,
            &format,&out_width,&out_height,&bytes);
        CHECK(cache_bytes==owner_bytes&&format==(uint32_t)bitmap->format&&
            out_width==width&&out_height==height&&bytes==expected);
        CHECK(bitmap->pixels_offset<=external_bytes&&source_offset+expected<=
            external_bytes-bitmap->pixels_offset);
        if(owner_bytes&&bitmap->pixels_offset<=external_bytes&&source_offset+expected<=
           external_bytes-bitmap->pixels_offset)
            CHECK(memcmp(owner_bytes,external+bitmap->pixels_offset+source_offset,expected)==0);
        source_offset+=expected;
    }
    {
        uint32_t format=999,width=999,height=999;size_t bytes=999;
        CHECK(!mac_native_bitmap_cache_encoded_face(hardware,1,0,&format,&width,&height,&bytes));
        CHECK(format==0&&width==0&&height==0&&bytes==0);
        format=width=height=999;bytes=999;
        CHECK(!mac_native_bitmap_encoded_face(tag->owner,0,0,
            (uint32_t)bitmap->mipmap_count+1,&format,&width,&height,&bytes));
        CHECK(format==0&&width==0&&height==0&&bytes==0);
    }
}
static void reject(struct mac_retail_cache*c,unsigned char*p,unsigned n,unsigned v,const void*pixels,size_t pixel_bytes)
{unsigned char save[4];unsigned i;struct mac_native_bitmap_tag t,z;const char*e;memcpy(save,p,n);for(i=0;i<n;++i)p[i]=(unsigned char)(v>>(i*8));memset(&z,0,sizeof(z));memset(&t,0xa5,sizeof(t));CHECK(!mac_native_bitmap_decode(c,0xe2a1012dU,pixels,pixel_bytes,&t,&e));CHECK(!memcmp(&t,&z,sizeof(t)));mac_native_bitmap_tag_free(&t);memcpy(p,save,n);}
static void check_cube(struct mac_retail_cache*c,uint32_t handle,const void*pixels,size_t pixel_bytes)
{
    struct mac_native_bitmap_tag t,z,bad;struct mac_native_bitmap_memory_usage memory;struct mac_retail_tag tag;struct bitmap_data*b;D3DTexture *hardware;unsigned char*root,*images,*save;const char*e;
    uint32_t m,f,w,h,disk_flags,pixel_offset,stored;size_t bytes;uint64_t required=0;
    const unsigned char*source;size_t source_size;unsigned char*expected;
    memset(&t,0,sizeof(t));memset(&z,0,sizeof(z));memset(&bad,0,sizeof(bad));memset(&memory,0,sizeof(memory));
    CHECK(mac_retail_tag_get(c,handle&0xffffU,&tag));CHECK(tag.handle==handle&&tag.group==0x6269746dU);
    root=(unsigned char*)mac_retail_tag_span(c,tag.address,108);CHECK(root!=NULL);if(!root)return;
    images=(unsigned char*)mac_retail_tag_span(c,r32(root+100),48);CHECK(images!=NULL);if(!images)return;
    CHECK(mac_native_bitmap_decode(c,handle,pixels,pixel_bytes,&t,&e));if(!t.owner){fprintf(stderr,"cube %08lx: %s\n",(unsigned long)handle,e?e:"decode");return;}
    CHECK(mac_native_bitmap_memory_usage_get(t.owner,&memory));
    CHECK(memory.total_owned_bytes==t.bytes_owned);
    CHECK(memory.total_owned_bytes==memory.encoded_mapped_bytes+memory.host_sample_bytes+
        memory.rgba_mip_bytes+memory.auxiliary_bytes);
    CHECK(memory.encoded_payload_bytes<=memory.encoded_mapped_bytes);
    printf("native bitmap memory %08lx: total=%lu encoded=%lu payload=%lu host-sample=%lu rgba-mips=%lu auxiliary=%lu\n",
        (unsigned long)handle,(unsigned long)memory.total_owned_bytes,
        (unsigned long)memory.encoded_mapped_bytes,(unsigned long)memory.encoded_payload_bytes,
        (unsigned long)memory.host_sample_bytes,(unsigned long)memory.rgba_mip_bytes,
        (unsigned long)memory.auxiliary_bytes);
    b=t.root->bitmaps.address;hardware=b->hardware_format;CHECK(t.image_count==1&&b->type==2&&b->depth==1&&b->width==b->height);
    CHECK((hardware->Format&D3DFORMAT_CUBEMAP)!=0);
    CHECK(((hardware->Format&D3DFORMAT_DIMENSION_MASK)>>D3DFORMAT_DIMENSION_SHIFT)==2);
    CHECK(t.level_count==((uint32_t)b->mipmap_count+1)*6);
    if(b->format<14||b->format>16){uint32_t f0=999,w0=999,h0=999;size_t n0=999;
        CHECK(!mac_native_bitmap_encoded_face(t.owner,0,0,0,&f0,&w0,&h0,&n0));
        CHECK(f0==0&&w0==0&&h0==0&&n0==0);}
    if(handle==0xe24900d5U){CHECK(b->width==128&&b->height==128&&b->format==10);CHECK(r32(images)==0x6269746dU);CHECK((unsigned)(images[14]|((unsigned)images[15]<<8))==385);}
    disk_flags=(unsigned)(images[14]|((unsigned)images[15]<<8));pixel_offset=r32(images+24);stored=r32(images+28);
    source=(disk_flags&256)?pixels:c->file;source_size=(disk_flags&256)?pixel_bytes:c->file_size;
    CHECK(source!=NULL);expected=malloc((size_t)b->width*b->height*4);CHECK(expected!=NULL);
    CHECK(pixel_offset<=source_size&&stored<=source_size-pixel_offset);
    for(m=0;m<= (uint32_t)b->mipmap_count;++m){uint64_t mip_offset=0;uint32_t k;
        w=(uint32_t)b->width>>m;if(!w)w=1;h=(uint32_t)b->height>>m;if(!h)h=1;
        for(k=0;k<m;++k){uint32_t mw=(uint32_t)b->width>>k,mh=(uint32_t)b->height>>k;if(!mw)mw=1;if(!mh)mh=1;mip_offset+=(uint64_t)cube_level_bytes(b->format,mw,mh)*6;}
        required+=(uint64_t)cube_level_bytes(b->format,w,h)*6;
        for(f=0;f<6;++f){uint32_t out_w,out_h;const unsigned char*q=mac_native_bitmap_rgba_face(t.owner,0,f,m,&out_w,&out_h,&bytes);
            CHECK(q!=NULL&&out_w==w&&out_h==h&&bytes==(size_t)w*h*4);
            if(source&&expected&&q&&pixel_offset<=source_size&&stored<=source_size-pixel_offset&&
               mip_offset+(uint64_t)cube_level_bytes(b->format,w,h)*(f+1)<=stored&&
               mac_retail_texture_decode(b->format,w,h,source+pixel_offset+mip_offset+(uint64_t)cube_level_bytes(b->format,w,h)*f,
                   cube_level_bytes(b->format,w,h),expected,(size_t)w*h*4))CHECK(!memcmp(q,expected,(size_t)w*h*4));
            else CHECK(0);
        }
    }
    CHECK(required<=(uint32_t)b->pixels_size);CHECK(t.bytes_owned>required);
    if(source&&pixel_offset<=source_size&&required<=source_size-pixel_offset)
        CHECK(!memcmp((void *)((unsigned long)hardware->Data|0x80000000UL),source+pixel_offset,(size_t)required));
    else CHECK(0);
    /* Actual malformed variants must fail without publishing partial roots. */
    save=images;{unsigned char old[2];memcpy(old,save+10,2);save[10]=3;save[11]=0;memset(&bad,0xa5,sizeof(bad));CHECK(!mac_native_bitmap_decode(c,handle,pixels,pixel_bytes,&bad,&e));CHECK(!memcmp(&bad,&z,sizeof(bad)));memcpy(save+10,old,2);}
    {unsigned char old[2];memcpy(old,save+4,2);save[4]=(unsigned char)((b->width/2)&255);save[5]=(unsigned char)((b->width/2)>>8);memset(&bad,0xa5,sizeof(bad));CHECK(!mac_native_bitmap_decode(c,handle,pixels,pixel_bytes,&bad,&e));CHECK(!memcmp(&bad,&z,sizeof(bad)));memcpy(save+4,old,2);}
    printf("native cubemap %08lx: %ux%u format=%u mips=%u bytes=%lu\n",(unsigned long)handle,(unsigned)b->width,(unsigned)b->height,(unsigned)b->format,(unsigned)b->mipmap_count+1,(unsigned long)required);
    free(expected);
    mac_native_bitmap_tag_free(&t);
}
static void check_lazy_owner(struct mac_retail_cache*c,const void*pixels,size_t pixel_bytes)
{
    struct mac_native_bitmap_tag tag;struct mac_native_bitmap_memory_usage memory;
    struct mac_native_tags tags,moved;const char*error=NULL;const unsigned char *first,*again,*second;
    size_t base,first_bytes,second_bytes;uint32_t w,h;unsigned long checks_before=checks;
    memset(&tag,0,sizeof(tag));memset(&memory,0,sizeof(memory));memset(&tags,0,sizeof(tags));memset(&moved,0,sizeof(moved));
    CHECK(mac_native_tags_create(c,&tags,&error));
    CHECK(mac_native_bitmap_decode(c,0xe2a1012dU,pixels,pixel_bytes,&tag,&error));
    CHECK(mac_native_bitmap_owner_accounting_bind(tag.owner,&tags,tag.handle));
    CHECK(mac_native_tags_publish(&tags,tag.handle,tag.group,tag.root,tag.root_bytes,
        tag.bytes_owned,tag.owner,mac_native_bitmap_owner_free,&error));
    base=mac_native_tags_owned_bytes(&tags);CHECK(base==tag.bytes_owned);
    CHECK(!mac_native_tags_owner_bytes_release(&tags,tag.handle,tag.owner,base));
    CHECK(mac_native_tags_owned_bytes(&tags)==base);
    first_bytes=32U*32U*4U;second_bytes=16U*16U*4U;
    CHECK(mac_native_tags_test_set_budget(&tags,base+first_bytes));
    first=mac_native_bitmap_rgba(tag.owner,0,0,&w,&h,&memory.rgba_mip_bytes);
    CHECK(first&&w==32&&h==32);CHECK(mac_native_tags_owned_bytes(&tags)==base+first_bytes);
    again=mac_native_bitmap_rgba(tag.owner,0,0,&w,&h,NULL);CHECK(again==first);
    CHECK(mac_native_tags_owned_bytes(&tags)==base+first_bytes);
    CHECK(mac_native_bitmap_memory_usage_get(tag.owner,&memory));
    CHECK(memory.total_owned_bytes==base+first_bytes&&memory.rgba_mip_bytes==first_bytes);
    CHECK(mac_native_bitmap_rgba(tag.owner,0,1,&w,&h,NULL)==NULL);
    CHECK(mac_native_tags_owned_bytes(&tags)==base+first_bytes);
    CHECK(mac_native_tags_test_set_budget(&tags,256U*1024U*1024U));
    moved=tags;tags.state=NULL;
    second=mac_native_bitmap_rgba(tag.owner,0,1,&w,&h,NULL);CHECK(second&&w==16&&h==16);
    CHECK(mac_native_tags_owned_bytes(&moved)==base+first_bytes+second_bytes);
    CHECK(mac_native_bitmap_rgba(tag.owner,0,1,NULL,NULL,NULL)==second);
    CHECK(mac_native_tags_owned_bytes(&moved)==base+first_bytes+second_bytes);
    mac_native_tags_free(&moved);
    if(checks==checks_before)CHECK(0);
}
static void check_a4r4g4b4(struct mac_retail_cache*c,const void*pixels,size_t pixel_bytes)
{
    struct mac_native_bitmap_tag tag;struct bitmap_data*b;D3DTexture*hardware;const char*error=NULL;
    const unsigned char*rgba;uint32_t w,h;size_t bytes;unsigned x,y;
    union real_point2d point;
    memset(&tag,0,sizeof(tag));
    CHECK(mac_native_bitmap_decode(c,0xe72305afU,pixels,pixel_bytes,&tag,&error));
    if(!tag.owner)return;
    b=tag.root->bitmaps.address;hardware=b->hardware_format;CHECK(tag.image_count==1&&b->type==0&&b->format==9);
    CHECK(((hardware->Format&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT)==D3DFMT_A4R4G4B4);
    CHECK(bitmap_verify(b,FALSE));
    rgba=mac_native_bitmap_rgba(tag.owner,0,0,&w,&h,&bytes);
    CHECK(rgba&&w==256&&h==256&&bytes==(size_t)w*h*4);
    if(rgba)for(y=0;y<h;++y)for(x=0;x<w;++x){size_t at=((size_t)y*w+x)*4;pixel32 actual,expected;
        point.x=((real)x+0.5f)/(real)w;point.y=((real)y+0.5f)/(real)h;
        actual=bitmap_2d_get_pixel(b,&point,1.0f);
        expected=(pixel32)rgba[at+3]<<24|(pixel32)rgba[at]<<16|(pixel32)rgba[at+1]<<8|rgba[at+2];
        if(actual!=expected) { static unsigned shown;if(shown++<8){long ax=fast_ftol((real)w*point.x-0.5f),ay=fast_ftol((real)h*point.y-0.5f);unsigned short *raw=(unsigned short*)bitmap_mipmap_address(b,0);fprintf(stderr,"format9 sample %u,%u point=%f,%f bitmap=%d,%d mip=%d,%d flags=%u type=%u fmt=%u base=%p mipbase=%p mapped=%ld,%ld native=%04x mappedraw=%04x direct=%08lx actual=%08lx expected=%08lx rgba=%02x,%02x,%02x,%02x\n",x,y,point.x,point.y,b->width,b->height,bitmap_mipmap_get_width(b,0),bitmap_mipmap_get_height(b,0),b->flags,b->type,b->format,b->base_address,raw,ax,ay,raw[y*w+x],raw[ay*w+ax],(unsigned long)bitmap_format_to_a8r8g8b8(9,raw,y*w+x),(unsigned long)actual,(unsigned long)expected,rgba[at],rgba[at+1],rgba[at+2],rgba[at+3]);} }
        CHECK(actual==expected);
    }
    mac_native_bitmap_tag_free(&tag);
}
int main(int argc,char**argv)
{
    unsigned char *map;const unsigned char *pixels;struct mac_mapped_file pixel_map={0};size_t map_bytes,pixel_bytes;const char*e;struct mac_retail_cache c;struct mac_native_bitmap_tag t;struct mac_native_bitmap_memory_usage memory;
    struct bitmap_data*b;struct mac_retail_tag tag;unsigned char *raw,*image;unsigned m;uint32_t digest=2166136261U;
    memset(&memory,0,sizeof(memory));
    check_bitmap_cache_capacity();
    if(argc<3)return 2;map=read_file(argv[1],&map_bytes);if(!map||!mac_mapped_file_open(argv[2],0x40000000U,&pixel_map,&e))return 2;pixels=pixel_map.data;pixel_bytes=pixel_map.size;
    if(!mac_retail_open(map,map_bytes,&c,&e)||!mac_native_bitmap_decode(&c,0xe2a1012dU,pixels,pixel_bytes,&t,&e)){fprintf(stderr,"bitmap: %s\n",e);return 1;}
    CHECK(mac_native_bitmap_memory_usage_get(t.owner,&memory));
    CHECK(memory.total_owned_bytes==t.bytes_owned);
    CHECK(memory.total_owned_bytes==memory.encoded_mapped_bytes+memory.host_sample_bytes+
        memory.rgba_mip_bytes+memory.auxiliary_bytes);
    CHECK(memory.encoded_payload_bytes<=memory.encoded_mapped_bytes);
#if defined(__ppc__)
    CHECK(memory.host_sample_bytes>0);
#else
    CHECK(memory.host_sample_bytes==0);
#endif
    CHECK(mac_retail_tag_get(&c,0x12d,&tag));raw=(unsigned char*)mac_retail_tag_span(&c,tag.address,108);image=(unsigned char*)mac_retail_tag_span(&c,r32(raw+100),48);
    reject(&c,raw+96,4,2049,pixels,pixel_bytes);reject(&c,raw+100,4,0xffffffffU,pixels,pixel_bytes);reject(&c,raw+8,4,0x7f800000U,pixels,pixel_bytes);
    reject(&c,image,4,0,pixels,pixel_bytes);
    reject(&c,image+4,2,0,pixels,pixel_bytes);reject(&c,image+12,2,11,pixels,pixel_bytes);reject(&c,image+20,2,12,pixels,pixel_bytes);
    reject(&c,image+24,4,0xffffffffU,pixels,pixel_bytes);reject(&c,image+28,4,1,pixels,pixel_bytes);
    CHECK(t.image_count==1);CHECK(t.level_count==6);CHECK(t.root_bytes==108);
    CHECK(mac_native_bitmap_cache_bind(&t,1));CHECK(!mac_native_bitmap_cache_bind(&t,1));CHECK(mac_native_bitmap_cache_requests()==0);
    b=t.root->bitmaps.address;CHECK(b->width==32&&b->height==32&&b->mipmap_count==5);CHECK(b->flags==0x03);
    check_encoded_dxt(&t,b,pixels,pixel_bytes);
    {D3DTexture *r=b->hardware_format;CHECK(r->Common==(D3DCOMMON_TYPE_TEXTURE|1));CHECK(r->Format==0x05560c29U);
#if defined(__ppc__)
        /* Sampling has a host-endian CPU view; D3D still addresses the
           unchanged encoded Xbox bytes. */
        CHECK((r->Data|0x80000000UL)!=(DWORD)b->base_address);
#else
        CHECK((r->Data|0x80000000UL)==(DWORD)b->base_address);
#endif
        CHECK(mac_shared_resource_release((D3DResource*)r)==0);CHECK(mac_shared_resource_release((D3DResource*)r)==0);}
    if(argc==3)check_cube(&c,0xe24900d5U,pixels,pixel_bytes);
    else {int a;for(a=3;a<argc;++a)check_cube(&c,(uint32_t)strtoul(argv[a],NULL,16),pixels,pixel_bytes);}
    check_lazy_owner(&c,pixels,pixel_bytes);
    check_a4r4g4b4(&c,pixels,pixel_bytes);
    free(map);mac_mapped_file_close(&pixel_map);pixels=NULL;CHECK(!pixel_map.data&&!pixel_map.size);
    for(m=0;m<t.level_count;++m){uint32_t w,h;size_t n,i;const unsigned char*p=mac_native_bitmap_rgba(t.owner,0,m,&w,&h,&n);CHECK(p!=NULL);CHECK(w==(32U>>m)&&h==w&&n==w*h*4);for(i=0;i<n;++i)digest=(digest^p[i])*16777619U;}
    CHECK(digest==t.pixel_digest);CHECK(_texture_cache_bitmap_get_hardware_format(b,FALSE,TRUE)==b->hardware_format);
    CHECK(_texture_cache_bitmap_get_hardware_format(b,TRUE,TRUE)==b->hardware_format);
    CHECK(_texture_cache_bitmap_get_hardware_format(b,FALSE,FALSE)==b->hardware_format);
    CHECK(mac_native_bitmap_cache_requests()==3);
    {
        struct bitmap_data dynamic_bitmap;
        D3DTexture *dynamic_texture=NULL;
        D3DLOCKED_RECT locked;
        CHECK(D3DDevice_CreateTexture(8,8,4,0,D3DFMT_A4R4G4B4,
            D3DPOOL_MANAGED,&dynamic_texture)==S_OK&&dynamic_texture);
        memset(&dynamic_bitmap,0,sizeof(dynamic_bitmap));
        dynamic_bitmap.width=8;dynamic_bitmap.height=8;dynamic_bitmap.depth=1;
        dynamic_bitmap.type=0;dynamic_bitmap.format=9;dynamic_bitmap.flags=0x40;
        dynamic_bitmap.mipmap_count=3;dynamic_bitmap.hardware_format=dynamic_texture;
        CHECK(mac_resource_range_is_allocated(
            (const void *)((unsigned long)dynamic_texture->Data|PLATFORM_CONTIGUOUS_BASE),170));
        CHECK(_texture_cache_bitmap_get_hardware_format(&dynamic_bitmap,FALSE,TRUE)==dynamic_texture);
        CHECK(IDirect3DTexture8_LockRect(dynamic_texture,0,&locked,NULL,0)==S_OK&&locked.pBits&&locked.Pitch==16);
        ((unsigned short *)locked.pBits)[0]=0x7fff;
        CHECK(((unsigned short *)locked.pBits)[0]==0x7fff);
        CHECK(mac_native_bitmap_cache_requests()==4);
        CHECK(D3DResource_Release((D3DResource *)dynamic_texture)==0);
        CHECK(mac_resource_allocation_count()==0);
    }
    CHECK(!mac_native_bitmap_rgba(t.owner,1,0,NULL,NULL,NULL));CHECK(!mac_native_bitmap_rgba(t.owner,0,6,NULL,NULL,NULL));
    printf("native bitmap memory e2a1012d: total=%lu encoded=%lu payload=%lu host-sample=%lu rgba-mips=%lu auxiliary=%lu\n",
        (unsigned long)memory.total_owned_bytes,(unsigned long)memory.encoded_mapped_bytes,
        (unsigned long)memory.encoded_payload_bytes,(unsigned long)memory.host_sample_bytes,
        (unsigned long)memory.rgba_mip_bytes,(unsigned long)memory.auxiliary_bytes);
    printf("native bitmap: images=%lu levels=%lu owned=%lu digest=%08lx checks=%u failures=%u\n",(unsigned long)t.image_count,(unsigned long)t.level_count,(unsigned long)t.bytes_owned,(unsigned long)digest,checks,failures);
    mac_native_bitmap_cache_unbind();mac_native_bitmap_tag_free(&t);mac_native_bitmap_tag_free(&t);return failures?1:0;
}
