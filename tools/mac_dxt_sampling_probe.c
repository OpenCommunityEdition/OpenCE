#include "cseries.h"
#include "real_math.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps_sampling.h"
#include "native_bitmap_tags.h"
#include "mapped_file.h"
#undef malloc
#undef free
#undef memset
#undef memcpy
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned checks,failures,rounding_differences;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line%d\n",#x,__LINE__);}}while(0)
static unsigned char *read_file(const char*p,size_t*n)
{
    FILE*f=fopen(p,"rb");long s;unsigned char*b;
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)||(s=ftell(f))<0||s>0x30000000||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    b=malloc((size_t)s);if(!b||fread(b,1,(size_t)s,f)!=(size_t)s){free(b);fclose(f);return NULL;}
    fclose(f);*n=(size_t)s;return b;
}
static void put16(unsigned char*p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void put32(unsigned char*p,unsigned long v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
static unsigned level_bytes(unsigned format,unsigned w,unsigned h)
{return ((w+3)/4)*((h+3)/4)*(format==14?8:16);}
static unsigned chain_bytes(unsigned format,unsigned w,unsigned h)
{unsigned n=0;for(;;){n+=level_bytes(format,w,h);if(w==1&&h==1)break;w=w>1?w/2:1;h=h>1?h/2:1;}return n;}
static void fill_color(unsigned char*p)
{
    /* Red and green 565 endpoints, with all four color indices represented. */
    put16(p,0xf800);put16(p+2,0x07e0);put32(p+4,0xe4e4e4e4UL);
}
static void fill_block(unsigned format,unsigned char*p)
{
    unsigned i;
    if(format==15)
    {
        put16(p+0,0x1234);put16(p+2,0x5678);put16(p+4,0x9abc);put16(p+6,0xdef0);
        fill_color(p+8);
    }
    else if(format==16)
    {
        p[0]=240;p[1]=10;
        for(i=0;i<6;++i)p[2+i]=(unsigned char)(0x96U+i*29U);
        fill_color(p+8);
    }
}
static unsigned fill_cube_chain(unsigned format,unsigned char*p,unsigned w,unsigned h)
{
    unsigned offset=0,bytes,face,i;
    for(;;)
    {
        bytes=level_bytes(format,w,h);
        for(face=0;face<6;++face)for(i=0;i<bytes;i+=format==14?8:16) {
            unsigned char *block=p+offset+(size_t)face*bytes+i;
            if(format==14)fill_color(block);
            else fill_block(format,block);
            /* Distinct but valid block bytes prove face offsets do not alias. */
            block[0]^=(unsigned char)(face*17U);
        }
        offset+=bytes*6;
        if(w==1&&h==1)break;
        w=w>1?w/2:1;h=h>1?h/2:1;
    }
    return offset;
}
static pixel32 pack_s3tc_pixel(const unsigned char*p)
{
    /* pixel32 consumers extract numeric AARRGGBB on every architecture. */
    return (pixel32)p[3]<<24|(pixel32)p[0]<<16|(pixel32)p[1]<<8|p[2];
}
static int compare_samples(struct bitmap_data*b,void*owner,unsigned image,int allow_dxt1_rounding)
{
    const unsigned char*rgba;uint32_t w,h;size_t bytes;unsigned x,y;int ok=1;
    rgba=mac_native_bitmap_rgba(owner,image,0,&w,&h,&bytes);
    if(!rgba||bytes!=(size_t)w*h*4)return 0;
    for(y=0;y<h;++y)for(x=0;x<w;++x)
    {
        union real_point2d point;pixel32 actual;size_t at=((size_t)y*w+x)*4;
        point.x=((real)x+0.5f)/(real)w;point.y=((real)y+0.5f)/(real)h;
        actual=bitmap_2d_get_pixel(b,&point,1.0f);
        if(actual!=pack_s3tc_pixel(rgba+at))
        {
            unsigned ar,ag,ab,aa,dr,dg,db;
            ar=(actual>>16)&255;ag=(actual>>8)&255;ab=actual&255;aa=(actual>>24)&255;
            dr=ar>rgba[at]?ar-rgba[at]:rgba[at]-ar;
            dg=ag>rgba[at+1]?ag-rgba[at+1]:rgba[at+1]-ag;
            db=ab>rgba[at+2]?ab-rgba[at+2]:rgba[at+2]-ab;
            /* January's S3TC decoder rounds its DXT1 1/3 and 2/3
               interpolants with +1; the portable decoder floors them.
               Permit only that documented one-level RGB difference on
               the installed DXT1 asset, never alpha or synthetic DXT3/5. */
            if(allow_dxt1_rounding&&b->format==14&&aa==rgba[at+3]&&dr<=1&&dg<=1&&db<=1)
                ++rounding_differences;
            else
            {if(ok)fprintf(stderr,"sample mismatch format=%d at %u,%u: %08lx != %08lx\n",b->format,x,y,(unsigned long)actual,(unsigned long)pack_s3tc_pixel(rgba+at));ok=0;}
        }
    }
    CHECK(ok);return ok;
}
static int decode_and_compare(struct mac_retail_cache*c,unsigned handle,const unsigned char*external,size_t external_bytes,unsigned expected_format,int allow_dxt1_rounding)
{
    struct mac_native_bitmap_tag t;struct bitmap_data*b;D3DTexture*r;unsigned char*encoded;int ok;
    const char*error=NULL;
    if(!mac_native_bitmap_decode(c,handle,external,external_bytes,&t,&error))
    {fprintf(stderr,"decode %08x: %s\n",handle,error?error:"failed");CHECK(0);return 0;}
    CHECK(t.image_count==1);b=t.root->bitmaps.address;CHECK(b->format==expected_format);
    CHECK(mac_native_bitmap_cache_bind(&t,1));r=b->hardware_format;
    encoded=(unsigned char*)((r->Data&0x7fffffffUL)|0x80000000UL);
    CHECK(memcmp(encoded,external+b->pixels_offset,b->pixels_size)==0);
#if defined(__ppc__)
    CHECK((unsigned long)b->base_address!=(unsigned long)encoded);
#else
    CHECK((unsigned long)b->base_address==(unsigned long)encoded);
#endif
    {
        unsigned faces=b->type==2?6U:1U,m;size_t source_offset=0;
        for(m=0;m<=(unsigned)b->mipmap_count;++m) {
            uint32_t w=(uint32_t)b->width>>m,h=(uint32_t)b->height>>m;
            unsigned f;size_t face_bytes;
            uint32_t got_format=999,got_w=999,got_h=999;size_t got_bytes=999;
            const unsigned char *owned,*cached;
            if(!w)w=1;if(!h)h=1;face_bytes=level_bytes(b->format,w,h);
            for(f=0;f<faces;++f) {
                owned=mac_native_bitmap_encoded_face(t.owner,0,f,m,&got_format,&got_w,&got_h,&got_bytes);
                CHECK(owned!=NULL&&got_format==expected_format&&got_w==w&&got_h==h&&got_bytes==face_bytes);
                got_format=got_w=got_h=999;got_bytes=999;
                cached=mac_native_bitmap_cache_encoded_face(r,f,m,&got_format,&got_w,&got_h,&got_bytes);
                CHECK(cached==owned&&got_format==expected_format&&got_w==w&&got_h==h&&got_bytes==face_bytes);
                CHECK(b->pixels_offset<=external_bytes&&
                    source_offset+(size_t)(f+1)*face_bytes<=external_bytes-b->pixels_offset);
                if(owned&&b->pixels_offset<=external_bytes&&
                   source_offset+(size_t)(f+1)*face_bytes<=external_bytes-b->pixels_offset)
                    CHECK(memcmp(owned,external+b->pixels_offset+source_offset+(size_t)f*face_bytes,face_bytes)==0);
            }
            source_offset+=(size_t)faces*face_bytes;
        }
        {
            uint32_t got_format=999,got_w=999,got_h=999;size_t got_bytes=999;
            CHECK(!mac_native_bitmap_cache_encoded_face(r,faces,0,&got_format,&got_w,&got_h,&got_bytes));
            CHECK(got_format==0&&got_w==0&&got_h==0&&got_bytes==0);
            got_format=got_w=got_h=999;got_bytes=999;
            CHECK(!mac_native_bitmap_encoded_face(t.owner,0,0,(uint32_t)b->mipmap_count+1,
                &got_format,&got_w,&got_h,&got_bytes));
            CHECK(got_format==0&&got_w==0&&got_h==0&&got_bytes==0);
        }
        mac_native_bitmap_cache_unbind();
    }
    /* bitmap_2d_get_pixel is the renderer's 2D sampler. The DXT3/5
       synthetic images are cubemaps and exercise the encoded face accessor
       above without routing them through that 2D-only source function. */
    ok=b->type==2?1:compare_samples(b,t.owner,0,allow_dxt1_rounding);
    mac_native_bitmap_tag_free(&t);return ok;
}
int main(int argc,char**argv)
{
    unsigned char*map,*synthetic;size_t map_bytes;struct mac_mapped_file pixel_map={0};
    const char*error=NULL;struct mac_retail_cache cache;struct mac_retail_tag tag;
    unsigned char*root,*image,saved[48];unsigned offset,total,format;int ok=1;
    if(argc!=3)return 2;
    map=read_file(argv[1],&map_bytes);if(!map||!mac_mapped_file_open(argv[2],0x40000000U,&pixel_map,&error))return 2;
    if(!mac_retail_open(map,map_bytes,&cache,&error)||!mac_retail_tag_get(&cache,0x12d,&tag))return 2;
    root=(unsigned char*)mac_retail_tag_span(&cache,tag.address,108);if(!root)return 2;
    image=(unsigned char*)mac_retail_tag_span(&cache,(unsigned long)root[100]|(unsigned long)root[101]<<8|(unsigned long)root[102]<<16|(unsigned long)root[103]<<24,48);
    if(!image)return 2;
    memcpy(saved,image,48);
    /* Installed assault-rifle tracer bitmap: actual DXT1 tag and payload. */
    ok=decode_and_compare(&cache,0xe2a1012dU,pixel_map.data,pixel_map.size,14,1)&&ok;
    CHECK(rounding_differences>0);
    offset=(unsigned)saved[24]|(unsigned)saved[25]<<8|(unsigned)saved[26]<<16|(unsigned)saved[27]<<24;
    for(format=15;format<=16;++format)
    {
        unsigned w=(unsigned)saved[4]|(unsigned)saved[5]<<8;
        unsigned h=(unsigned)saved[6]|(unsigned)saved[7]<<8;
        size_t synthetic_bytes;
        total=chain_bytes(format,w,h);
        synthetic_bytes=pixel_map.size>(size_t)offset+(size_t)total*6?pixel_map.size:(size_t)offset+(size_t)total*6;
        synthetic=malloc(synthetic_bytes);if(!synthetic)return 2;
        memcpy(synthetic,pixel_map.data,pixel_map.size);
        fill_cube_chain(format,synthetic+offset,w,h);
        memcpy(image,saved,48);put16(image+10,2);put16(image+12,format);put16(image+14,0x183);put32(image+28,total*6);
        ok=decode_and_compare(&cache,0xe2a1012dU,synthetic,synthetic_bytes,format,0)&&ok;
        memcpy(image,saved,48);free(synthetic);
    }
    printf("DXT source sampling: DXT1 rounding pixels=%u checks=%u failures=%u\n",rounding_differences,checks,failures);
    mac_mapped_file_close(&pixel_map);free(map);return ok&&!failures?0:1;
}
