/* Real retail model-shader base-map lookup and bounded malformed fixtures. */
#include "../port/mac/retail_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);} } while(0)
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void write32(unsigned char *p,uint32_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24); }
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_retail_tag tag;const char *error;uint32_t i,bitmap=0,supported=0,models=0;
    uint32_t chicago_bitmap=0,chicago_count=0;
    if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){fprintf(stderr,"cache: %s\n",error?error:"open failed");free(bytes);return 1;}
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&tag)&&tag.group==UINT32_C(0x736f736f)) {
        unsigned char *root=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0xB4);
        uint32_t selected=0;float u=0.0f,v=0.0f;
        ++models;CHECK(root!=NULL);if(!root)continue;
        if(mac_retail_model_base_bitmap(&cache,tag.handle,&selected,&u,&v)) {
            unsigned char saved_scale[4],saved_ref[4];uint32_t mutated=0;float mu=0.0f,mv=0.0f;
            ++supported;CHECK(selected!=0&&u<=1000.0f&&u>=-1000.0f&&v<=1000.0f&&v>=-1000.0f);
            if(!bitmap)bitmap=selected;
            /* A real valid definition must reject a NaN UV scale without
               returning a partially initialized bitmap or scale. */
            memcpy(saved_scale,root+0x9C,4);write32(root+0x9C,UINT32_C(0x7fc00000));
            CHECK(!mac_retail_model_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
            CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(root+0x9C,saved_scale,4);
            memcpy(saved_ref,root+0xB0,4);write32(root+0xB0,UINT32_MAX);
            CHECK(!mac_retail_model_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
            CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(root+0xB0,saved_ref,4);
            if(supported==1)fprintf(stderr,"first soso base-map: %s shader=%08lx bitmap=%08lx uv=%g,%g\n",
                tag.name,(unsigned long)tag.handle,(unsigned long)selected,(double)u,(double)v);
        }
    }
    /* Blood Gulch's assault-rifle bolt shader has two genuine Chicago maps.
       The renderer supports only source map zero, after verifying its bitmap
       reference and UV transform; it must not reject the tag merely because
       the omitted composition has another layer. */
    if(mac_retail_tag_get(&cache,0xFC,&tag)&&tag.handle==UINT32_C(0xE27000FC)&&
       tag.group==UINT32_C(0x73636869)) {
        unsigned char *root=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0x60);
        unsigned char *maps;uint32_t address;unsigned char saved[4];
        uint32_t selected=0,mutated=0;float u=0.0f,v=0.0f,mu=0.0f,mv=0.0f;
        CHECK(root!=NULL);
        if(root) {
            chicago_count=u32(root+0x54);address=u32(root+0x58);
            maps=(unsigned char *)mac_retail_tag_span(&cache,address,0xDC);
            CHECK(chicago_count==2&&maps!=NULL);
            CHECK(mac_retail_chicago_base_bitmap(&cache,tag.handle,&selected,&u,&v));
            CHECK(selected==UINT32_C(0xE27100FD)&&u==1.0f&&v==1.0f);
            chicago_bitmap=selected;
            memcpy(saved,root+0x54,4);write32(root+0x54,0);
            CHECK(!mac_retail_chicago_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
            CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(root+0x54,saved,4);
            memcpy(saved,root+0x58,4);write32(root+0x58,UINT32_MAX);
            CHECK(!mac_retail_chicago_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
            CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(root+0x58,saved,4);
            if(maps) {
                memcpy(saved,maps+0x5C,4);write32(maps+0x5C,UINT32_C(0x3f000000));
                CHECK(!mac_retail_chicago_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
                CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(maps+0x5C,saved,4);
                memcpy(saved,maps+0x54,4);write32(maps+0x54,UINT32_C(0x7fc00000));
                CHECK(!mac_retail_chicago_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
                CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(maps+0x54,saved,4);
                memcpy(saved,maps+0x6C,4);write32(maps+0x6C,UINT32_C(0x62616421));
                CHECK(!mac_retail_chicago_base_bitmap(&cache,tag.handle,&mutated,&mu,&mv));
                CHECK(mutated==0&&mu==0.0f&&mv==0.0f);memcpy(maps+0x6C,saved,4);
            }
            fprintf(stderr,"real Chicago first map: %s shader=%08lx maps=%lu bitmap=%08lx uv=%g,%g; later layers omitted\n",
                tag.name,(unsigned long)tag.handle,(unsigned long)chicago_count,
                (unsigned long)selected,(double)u,(double)v);
        }
    }
    CHECK(models>0);CHECK(supported>0);CHECK(bitmap!=0);
    CHECK(chicago_count==2&&chicago_bitmap==UINT32_C(0xE27100FD));
    printf("model base-map probe: soso=%lu supported=%lu Chicago maps=%lu base=%08lx checks=%u failures=%u; later Chicago layers/blending/lighting/animation remain unsupported\n",
        (unsigned long)models,(unsigned long)supported,(unsigned long)chicago_count,
        (unsigned long)chicago_bitmap,checks,failures);
    free(bytes);return failures?1:0;
}
