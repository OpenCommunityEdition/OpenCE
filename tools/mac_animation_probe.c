#include "retail_animation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t digest=UINT32_C(2166136261);
static void word(uint32_t v) { digest=(digest^v)*UINT32_C(16777619); }
static void stream_digest(const struct mac_retail_animation *a,const unsigned char *p,int animated)
{
    const uint32_t *masks[3];
    static const unsigned sizes[3]={8,12,4};
    unsigned i,j,k,offset=0;uint16_t half;uint32_t bits;
    masks[0]=a->rotation_flags;masks[1]=a->translation_flags;masks[2]=a->scale_flags;
    for(i=0;i<(unsigned)a->node_count;++i)for(j=0;j<3;++j) {
        if((int)((masks[j][i/32]>>(i%32))&1)!=animated)continue;
        if(!j)for(k=0;k<8;k+=2){memcpy(&half,p+offset+k,2);word(half);}
        else for(k=0;k<sizes[j];k+=4){memcpy(&bits,p+offset+k,4);word(bits);}
        offset+=sizes[j];
    }
}
static void put32(unsigned char *p,uint32_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24); }
static void put16(unsigned char *p,unsigned v) { p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8); }
static int synthetic(void)
{
    unsigned char bytes[640];struct mac_retail_cache c;struct mac_retail_animation a;
    const char *error;unsigned char *p=bytes+200;uint32_t bits;int16_t q;unsigned failures=0;
    memset(bytes,0,sizeof(bytes));memset(&c,0,sizeof(c));
    c.tags=bytes;c.tag_size=sizeof(bytes);c.tag_count=1;c.tags_end=32;
    put32(bytes,UINT32_C(0x616e7472));put32(bytes+12,UINT32_C(0x11110000));
    put32(bytes+16,MAC_RETAIL_BASE+64);put32(bytes+20,MAC_RETAIL_BASE+64);
    put32(bytes+64+104,1);put32(bytes+64+116,1);put32(bytes+64+120,MAC_RETAIL_BASE+200);
    memcpy(p,"mixed streams",14);put16(p+34,2);put16(p+36,12);put16(p+38,1);put16(p+44,1);
    put16(p+56,65535);put16(p+60,65535);put16(p+66,65535);
    put32(p+108,1);put32(p+124,1);
    put32(p+72,16);put32(p+84,MAC_RETAIL_BASE+500);
    put32(p+140,12);put32(p+152,MAC_RETAIL_BASE+520);
    put32(p+160,24);put32(p+172,MAC_RETAIL_BASE+544);
    put32(bytes+520,UINT32_C(0x40400000));put32(bytes+524,UINT32_C(0x40800000));put32(bytes+528,UINT32_C(0x40a00000));
    put16(bytes+544,0xc000);put16(bytes+548,0x4000);put16(bytes+550,0x7fff);put32(bytes+552,UINT32_C(0x3f800000));
    put16(bytes+562,0x7fff);put32(bytes+564,UINT32_C(0x40000000));
    if(!mac_retail_animation_decode(&c,UINT32_C(0x11110000),0,&a,&error)) {
        fprintf(stderr,"synthetic animation: %s\n",error);return 0;
    }
    memcpy(&q,a.data,2);if(q!=-16384)++failures;
    memcpy(&q,a.data+4,2);if(q!=16384)++failures;
    memcpy(&bits,a.data+8,4);if(bits!=UINT32_C(0x3f800000))++failures;
    memcpy(&bits,a.defaults+8,4);if(bits!=UINT32_C(0x40a00000))++failures;
    mac_retail_animation_free(&a);mac_retail_animation_free(&a);
    put16(p+58,1);
    if(mac_retail_animation_decode(&c,UINT32_C(0x11110000),0,&a,&error)) { ++failures;mac_retail_animation_free(&a); }
    if(a.data || a.defaults || a.frame_info)++failures;
    put16(p+58,0);put32(bytes+520,UINT32_C(0x7fc00000));
    if(mac_retail_animation_decode(&c,UINT32_C(0x11110000),0,&a,&error)) { ++failures;mac_retail_animation_free(&a); }
    if(a.data || a.defaults || a.frame_info)++failures;
    printf("animation synthetic mixed quaternion/float checks=8 failures=%u\n",failures);
    return !failures;
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;
    struct mac_retail_cache cache;struct mac_retail_animation a;
    struct mac_retail_tag tag;const char *error;
    uint32_t h=0,count=0,i,j,bits,frames=0,defaults=0,data=0,info=0;
    if(!synthetic())return 1;
    if(argc!=2)return argc==1?0:2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END) || (length=ftell(f))<2048 || length>0x30000000L || fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){fprintf(stderr,"map: %s\n",error);free(bytes);return 1;}
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&tag) && tag.group==UINT32_C(0x616e7472) &&
        !strcmp(tag.name,"characters\\cyborg\\cyborg")){h=tag.handle;break;}
    if(!h || !mac_retail_animation_count(&cache,h,&count,&error)){fprintf(stderr,"cyborg animation graph unavailable\n");free(bytes);return 1;}
    for(i=0;i<count;++i) {
        if(!mac_retail_animation_decode(&cache,h,i,&a,&error)){fprintf(stderr,"animation %lu: %s\n",(unsigned long)i,error);free(bytes);return 1;}
        word(a.node_list_checksum);word((uint32_t)a.frame_count);word((uint32_t)a.node_count);word((uint32_t)a.frame_size);
        for(j=0;j<2;++j){word(a.rotation_flags[j]);word(a.translation_flags[j]);word(a.scale_flags[j]);}
        stream_digest(&a,a.defaults,0);
        if(a.frame_size)for(j=0;j<(uint32_t)a.frame_count;++j)stream_digest(&a,a.data+j*a.frame_size,1);
        for(j=0;j<a.frame_info_size;j+=4){memcpy(&bits,a.frame_info+j,4);word(bits);}
        frames+=a.frame_count;defaults+=a.default_size;data+=a.data_size;info+=a.frame_info_size;
        mac_retail_animation_free(&a);mac_retail_animation_free(&a);
    }
    printf("native cyborg animations=%lu frames=%lu frame_info=%lu defaults=%lu data=%lu digest=%08lx\n",
        (unsigned long)count,(unsigned long)frames,(unsigned long)info,(unsigned long)defaults,(unsigned long)data,(unsigned long)digest);
    free(bytes);return 0;
}
