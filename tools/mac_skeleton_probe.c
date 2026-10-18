#include "retail_skeleton.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;
static uint32_t digest=UINT32_C(2166136261);
static void check(int ok){++checks;if(!ok)++failures;}
static void word(uint32_t n){digest=(digest^n)*UINT32_C(16777619);}
static void put(unsigned char *p,uint32_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static void put16(unsigned char *p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void synthetic(void)
{
    unsigned char bytes[4096],saved[4096];
    struct mac_retail_cache c;struct mac_retail_skeleton s;
    uint32_t biped,model,i;const char *error;
    static const uint32_t groups[3]={UINT32_C(0x6d617467),UINT32_C(0x62697064),UINT32_C(0x6d6f6432)};
    static const uint32_t addresses[3]={128,800,2200};
    memset(&c,0,sizeof(c));memset(bytes,0,sizeof(bytes));
    c.tags=bytes;c.tag_size=sizeof(bytes);c.tags_end=96;c.tag_count=3;
    for(i=0;i<3;++i){put(bytes+i*32,groups[i]);put(bytes+i*32+12,UINT32_C(0x12340000)+i);put(bytes+i*32+16,MAC_RETAIL_BASE+100);put(bytes+i*32+20,MAC_RETAIL_BASE+addresses[i]);}
    put(bytes+128+0x164,1);put(bytes+128+0x168,MAC_RETAIL_BASE+600);
    put(bytes+616,groups[1]);put(bytes+628,UINT32_C(0x12340001));
    put(bytes+800+0x28,groups[2]);put(bytes+800+0x34,UINT32_C(0x12340002));
    put(bytes+2200+4,UINT32_C(0x89abcdef));put(bytes+2200+0xb8,2);put(bytes+2200+0xbc,MAC_RETAIL_BASE+2500);
    for(i=0;i<2;++i){unsigned char *p=bytes+2500+i*156;strcpy((char *)p,i?"child":"root");put16(p+32,65535);put16(p+34,i?65535:1);put16(p+36,i?0:65535);put(p+0x28,UINT32_C(0x3fa00000));put(p+0x40,UINT32_C(0x3f800000));put(p+0x68,UINT32_C(0x3f800000));}
    memcpy(saved,bytes,sizeof(bytes));
    check(mac_retail_multiplayer_model(&c,&biped,&model,&error));check(biped==UINT32_C(0x12340001)&&model==UINT32_C(0x12340002));
    check(mac_retail_skeleton_decode(&c,model,&s,&error));check(s.count==2 && s.nodes[0].child==1 && s.nodes[1].parent==0 && s.nodes[0].translation[0]==1.25f && s.nodes[1].rotation[3]==1 && s.checksum==UINT32_C(0x89abcdef));
    check(!mac_retail_skeleton_decode(&c,model^65536,&s,&error));check(!s.count && !s.nodes[0].name[0]);
#define REJECT_AT(offset,value) do{put(bytes+(offset),(value));check(!mac_retail_skeleton_decode(&c,model,&s,&error));check(!s.count);memcpy(bytes,saved,sizeof(bytes));}while(0)
    REJECT_AT(2200+0xb8,65);REJECT_AT(2200+0xb8,0);REJECT_AT(2200+0xbc,MAC_RETAIL_BASE+4092);
    REJECT_AT(2500+0x28,UINT32_C(0x7f800000));REJECT_AT(2500+0x34,UINT32_C(0x7fc00000));REJECT_AT(2500+0x68,UINT32_C(0xff800000));
#undef REJECT_AT
#define REJECT_SHORT(offset,value) do{put16(bytes+(offset),(value));check(!mac_retail_skeleton_decode(&c,model,&s,&error));memcpy(bytes,saved,sizeof(bytes));}while(0)
    REJECT_SHORT(2500+32,0);REJECT_SHORT(2500+36,0);REJECT_SHORT(2500+34,2);
    REJECT_SHORT(2500+34,65535);REJECT_SHORT(2500+156+36,65535);REJECT_SHORT(2500+156+32,0);
#undef REJECT_SHORT
    memset(bytes+2500,'x',32);check(!mac_retail_skeleton_decode(&c,model,&s,&error));memcpy(bytes,saved,sizeof(bytes));
    put(bytes+628,UINT32_C(0x99990001));check(!mac_retail_multiplayer_model(&c,&biped,&model,&error));check(biped==UINT32_MAX && model==UINT32_MAX);memcpy(bytes,saved,sizeof(bytes));
    put(bytes+128+0x164,2);check(!mac_retail_multiplayer_model(&c,&biped,&model,&error));
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;uint32_t biped,model,i,j,w;
    struct mac_retail_cache c;struct mac_retail_skeleton s;const char *error;
    synthetic();printf("skeleton synthetic checks=%u failures=%u\n",checks,failures);
    if(failures)return 1;if(argc==1)return 0;if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&c,&error)||!mac_retail_multiplayer_model(&c,&biped,&model,&error)||!mac_retail_skeleton_decode(&c,model,&s,&error)){
        fprintf(stderr,"skeleton: %s\n",error);free(bytes);return 1;
    }
    word(s.model_handle);word(s.checksum);word(s.count);
    for(i=0;i<s.count;++i){
        const struct mac_retail_model_node *n=&s.nodes[i];
        for(j=0;j<32;++j)word((unsigned char)n->name[j]);
        word((uint32_t)(int32_t)n->parent);word((uint32_t)(int32_t)n->child);word((uint32_t)(int32_t)n->sibling);
        for(j=0;j<3;++j){memcpy(&w,n->translation+j,4);word(w);}
        for(j=0;j<4;++j){memcpy(&w,n->rotation+j,4);word(w);}
        memcpy(&w,&n->distance,4);word(w);for(j=0;j<13;++j){memcpy(&w,n->inverse+j,4);word(w);}
    }
    printf("multiplayer biped=%08lx model=%08lx nodes=%lu checksum=%08lx native_digest=%08lx\n",(unsigned long)biped,(unsigned long)model,(unsigned long)s.count,(unsigned long)s.checksum,(unsigned long)digest);
    printf("root=%s child=%d parent=%d translation=%.9f %.9f %.9f\n",s.nodes[0].name,(int)s.nodes[0].child,(int)s.nodes[0].parent,s.nodes[0].translation[0],s.nodes[0].translation[1],s.nodes[0].translation[2]);
    free(bytes);return 0;
}
