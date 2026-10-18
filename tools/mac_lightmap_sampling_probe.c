#include "cseries.h"
#include "real_math.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmaps.h"
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

void *_texture_cache_bitmap_get_hardware_format(struct bitmap_data *,boolean,boolean);
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line%d\n",#x,__LINE__);}}while(0)
static unsigned char *read_file(const char*p,size_t*n)
{
    FILE*f=fopen(p,"rb");long s;unsigned char*b;
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)||(s=ftell(f))<0||s>0x30000000||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    b=malloc(s);if(!b||fread(b,1,(size_t)s,f)!=(size_t)s){free(b);fclose(f);return NULL;}
    fclose(f);*n=(size_t)s;return b;
}
int main(int argc,char**argv)
{
    unsigned char*map;size_t map_bytes;struct mac_mapped_file pixels={0};const char*error=NULL;
    struct mac_retail_cache cache;struct mac_native_bitmap_tag tag;struct bitmap_data*bitmaps;
    unsigned image;unsigned long samples=0;
    if(argc!=3)return 2;
    map=read_file(argv[1],&map_bytes);if(!map||!mac_mapped_file_open(argv[2],0x40000000U,&pixels,&error))return 2;
    if(!mac_retail_open(map,map_bytes,&cache,&error)||
       !mac_native_bitmap_decode(&cache,0xeab3093dU,pixels.data,pixels.size,&tag,&error))
    {fprintf(stderr,"lightmap bitmap group: %s\n",error?error:"decode failed");return 1;}
    CHECK(tag.image_count==15);
    CHECK(mac_native_bitmap_cache_bind(&tag,1));
    mac_mapped_file_close(&pixels);free(map);map=NULL;
    bitmaps=tag.root->bitmaps.address;
    for(image=0;image<tag.image_count;++image)
    {
        struct bitmap_data*b=&bitmaps[image];const unsigned char*rgba;uint32_t w,h;size_t bytes;unsigned x,y;
        CHECK(b->format==6&&b->type==0&&b->depth==1&&b->flags==1);
        CHECK(bitmap_verify(b,FALSE));
        CHECK(_texture_cache_bitmap_get_hardware_format(b,FALSE,FALSE)==b->hardware_format);
        rgba=mac_native_bitmap_rgba(tag.owner,image,0,&w,&h,&bytes);
        CHECK(rgba&&w==(uint32_t)b->width&&h==(uint32_t)b->height&&bytes==(size_t)w*h*4);
        if(!rgba)continue;
        for(y=0;y<h;++y)for(x=0;x<w;++x)
        {
            union real_point2d point;pixel32 actual;size_t at=((size_t)y*w+x)*4;
            point.x=((real)x+0.5f)/(real)w;point.y=((real)y+0.5f)/(real)h;
            actual=bitmap_2d_get_pixel(b,&point,1.0f);
            CHECK(actual==((pixel32)rgba[at+3]<<24|(pixel32)rgba[at]<<16|(pixel32)rgba[at+1]<<8|rgba[at+2]));
            ++samples;
        }
    }
    printf("native lightmap sampling: images=%lu samples=%lu checks=%u failures=%u\n",
        (unsigned long)tag.image_count,samples,checks,failures);
    mac_native_bitmap_cache_unbind();mac_native_bitmap_tag_free(&tag);mac_mapped_file_close(&pixels);free(map);
    return failures?1:0;
}
