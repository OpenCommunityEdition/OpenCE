/* Real-map adversarial checks for the uncompressed animation reader. Edits
   only this process's private copy and restores every changed field. */
#include "retail_animation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void put(unsigned char *p,uint32_t v,unsigned n)
{ unsigned i;for(i=0;i<n;++i)p[i]=(unsigned char)(v>>(8*i)); }
static void reject(struct mac_retail_cache *c,uint32_t h,unsigned char *p,
    uint32_t value,unsigned size,const char *name)
{
    unsigned char saved[4];
    struct mac_retail_animation a,empty;
    const char *error=NULL;
    int ok;
    memcpy(saved,p,size);put(p,value,size);
    memset(&empty,0,sizeof(empty));memset(&a,0xa5,sizeof(a));
    ok=mac_retail_animation_decode(c,h,0,&a,&error);
    ++checks;
    if(ok || !error || memcmp(&a,&empty,sizeof(a))) {
        ++failures;fprintf(stderr,"FAIL rejection: %s\n",name);
    }
    mac_retail_animation_free(&a);
    memcpy(p,saved,size);
}
int main(int argc,char **argv)
{
    FILE *f;long size;unsigned char *bytes,*g,*p;
    const unsigned char *span;
    struct mac_retail_cache c;struct mac_retail_tag tag;
    struct mac_retail_animation a;
    const char *error=NULL;uint32_t i,h=UINT32_MAX,n;
    if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000L || fseek(f,0,SEEK_SET))return 2;
    bytes=(unsigned char *)malloc(size);if(!bytes)return 2;
    if(fread(bytes,1,size,f)!=(size_t)size)return 2;fclose(f);
    if(!mac_retail_open(bytes,size,&c,&error))return 2;
    for(i=0;i<c.tag_count;++i)if(mac_retail_tag_get(&c,i,&tag) &&
        tag.group==UINT32_C(0x616e7472) && !strcmp(tag.name,"characters\\cyborg\\cyborg")) { h=tag.handle;break; }
    if(h==UINT32_MAX || !mac_retail_animation_count(&c,h,&n,&error) || !n)return 2;
    span=mac_retail_tag_span(&c,tag.address,128);if(!span)return 2;
    g=bytes+(span-c.file);
    span=mac_retail_tag_span(&c,u32(g+120),180);if(!span)return 2;
    p=bytes+(span-c.file);
    if(!mac_retail_animation_decode(&c,h,0,&a,&error)) { fprintf(stderr,"baseline: %s\n",error);return 1; }
    mac_retail_animation_free(&a);
#define BAD(off,val,len,name) reject(&c,h,p+(off),(val),(len),(name))
    BAD(32,3,2,"type");BAD(34,0,2,"zero frames");BAD(34,32767,2,"excess frames");
    BAD(36,65535,2,"negative stride");BAD(38,4,2,"frame-info type");
    BAD(44,65,2,"node limit");BAD(44,0,2,"zero nodes");BAD(46,65535,2,"negative loop frame");
    BAD(48,UINT32_C(0x7fc00001),4,"NaN weight");BAD(52,32767,2,"key frame");
    BAD(54,32767,2,"second key frame");BAD(56,n,2,"next index");
    BAD(58,1,2,"compressed flag");BAD(60,32767,2,"sound index");
    BAD(62,32767,2,"sound frame");BAD(66,n,2,"parent index");
    BAD(68,UINT32_C(0x7f800000),4,"infinite normalized weight");
    BAD(72,UINT32_MAX,4,"frame-info length");
#undef BAD
    reject(&c,h,p+92,u32(p+92)^1,4,"translation mask stride mismatch");
    reject(&c,h,p+108,u32(p+108)^1,4,"rotation mask stride mismatch");
    reject(&c,h,p+124,u32(p+124)^1,4,"scale mask stride mismatch");
    reject(&c,h,p+136,1,4,"compressed offset");
    reject(&c,h,p+140,UINT32_MAX,4,"default length");
    reject(&c,h,p+160,UINT32_MAX,4,"frame length");
    reject(&c,h,g+116,257,4,"graph count");
    reject(&c,h,g+120,UINT32_C(0xfffffffc),4,"graph address overflow");
    /* Payload pointers must be checked even when declared sizes are valid. */
    if(u32(p+72))reject(&c,h,p+84,UINT32_C(0xfffffffc),4,"frame info address");
    if(u32(p+140))reject(&c,h,p+152,UINT32_C(0xfffffffc),4,"default address");
    if(u32(p+160))reject(&c,h,p+172,UINT32_C(0xfffffffc),4,"frame address");
    if(u32(p+72)) {
        span=mac_retail_tag_span(&c,u32(p+84),4);
        if(!span)return 2;
        reject(&c,h,bytes+(span-c.file),UINT32_C(0x7fc00001),4,"nonfinite frame info payload");
    }
    ++checks;
    if(!mac_retail_animation_decode(&c,h,0,&a,&error))++failures;
    mac_retail_animation_free(&a);mac_retail_animation_free(&a);
    printf("animation private-map mutations: checks=%u failures=%u\n",checks,failures);
    free(bytes);return failures?1:0;
}
