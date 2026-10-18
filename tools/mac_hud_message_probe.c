#include "cseries.h"
#include "native_hud_messages.h"
#include "native_hud_message_layout.h"
#include "native_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
static unsigned checks,failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static uint32_t rd32(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void wr8(unsigned char*p,uint8_t v){p[0]=v;}
static void wr32(unsigned char*p,uint32_t v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
static void wr16(unsigned char*p,uint16_t v){p[0]=v;p[1]=v>>8;}
static uint32_t hash_word(uint32_t h,uint32_t value){return (h^value)*16777619U;}
static uint32_t message_hash(const struct mac_native_hud_message_text *root)
{
    uint32_t h=2166136261U,i;
    for(i=0;i<(uint32_t)root->text_data.size/2;++i)
        h=hash_word(h,((uint16_t *)root->text_data.address)[i]);
    h=hash_word(h,(uint32_t)root->elements.count);
    h=hash_word(h,(uint32_t)root->messages.count);
    return h;
}
static int reject_at(const unsigned char *source,size_t length,uint32_t handle,
    size_t offset,unsigned width,uint32_t value)
{
    unsigned char *copy=malloc(length);struct mac_retail_cache cache;
    struct mac_native_object_tag tag={0};const char *error=NULL;int rejected;
    if(!copy)return 0;
    memcpy(copy,source,length);
    if(!mac_retail_open(copy,length,&cache,&error)){free(copy);return 0;}
    if(width==1)wr8(copy+offset,(uint8_t)value);
    else if(width==2)wr16(copy+offset,(uint16_t)value);else wr32(copy+offset,value);
    rejected=!mac_native_hud_message_text_decode(&cache,handle,&tag,&error);
    if(!rejected)free(tag.owner);
    free(copy);return rejected&&error!=NULL;
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes,*copy;struct mac_retail_cache cache,mutated;
    struct mac_native_tags tags={0};
    const struct mac_native_hud_message_text *retained=NULL;struct mac_retail_tag first;
    uint32_t i,count=0,digest=2166136261U,retained_digest=0;const char *error=NULL;
    if(argc!=2||!(f=fopen(argv[1],"rb")))return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||fseek(f,0,SEEK_SET))return 2;
    bytes=malloc(length);if(!bytes||fread(bytes,1,length,f)!=(size_t)length)return 2;fclose(f);
    if(!mac_retail_open(bytes,length,&cache,&error))return 2;
    CHECK(mac_native_tags_create(&cache,&tags,&error));
    memset(&first,0,sizeof(first));
    for(i=0;i<cache.tag_count;++i) {
        struct mac_retail_tag source;struct mac_native_object_tag out={0};
        const struct mac_native_hud_message_text *root;uint32_t j;
        if(!mac_retail_tag_get(&cache,i,&source))return 2;
        if(source.group!=UINT32_C(0x686d7420))continue;
        {int ok=mac_native_hud_message_text_decode(&cache,source.handle,&out,&error);
         CHECK(ok);if(!ok){const unsigned char *d=mac_retail_tag_span(&cache,source.address,44);
            fprintf(stderr,"hmt handle=%08lx name=%s error=%s size=%lu elements=%lu@%08lx messages=%lu@%08lx\n",
                (unsigned long)source.handle,source.name,error?error:"none",
                (unsigned long)rd32(d),(unsigned long)rd32(d+20),(unsigned long)rd32(d+24),
                (unsigned long)rd32(d+32),(unsigned long)rd32(d+36));continue;}}
        {int published=mac_native_tags_publish(&tags,source.handle,source.group,out.root,out.root_bytes,
            out.bytes_owned,out.owner,mac_native_hud_message_text_owner_free,&error);
         CHECK(published);if(!published){mac_native_hud_message_text_tag_free(&out);continue;}}
        out.owner=NULL;
        root=mac_native_tags_get(&tags,source.group,source.handle,sizeof(*root),&error);
        CHECK(root!=NULL);if(!root)continue;
        ++count;if(!first.handle)first=source;
        CHECK(out.root_bytes==sizeof(*root));
        CHECK(root->text_data.size>=0&&!(root->text_data.size&1));
        CHECK(root->elements.count>=0&&root->elements.count<=8192);
        CHECK(root->messages.count>=0&&root->messages.count<=1024);
        if(root->messages.count) {
            CHECK(root->messages.address!=NULL);
            CHECK(((uintptr_t)root->messages.address&3U)==0);
        }
        if(root->elements.count)CHECK(root->elements.address!=NULL);
        if(root->text_data.size)CHECK(root->text_data.address!=NULL);
        for(j=0;j<(uint32_t)root->messages.count;++j) {
            const struct mac_native_hud_state_message *m=(const struct mac_native_hud_state_message *)root->messages.address+j;
            CHECK(m->element_count<=8);
            CHECK((uint32_t)m->element_start_index+m->element_count<=(uint32_t)root->elements.count);
            if(root->text_data.size)CHECK(m->text_start_index<(uint32_t)root->text_data.size/2);
        }
        for(j=0;j<(uint32_t)root->text_data.size/2;++j)
            digest=hash_word(digest,((uint16_t *)root->text_data.address)[j]);
        digest=hash_word(digest,(uint32_t)root->elements.count);
        digest=hash_word(digest,(uint32_t)root->messages.count);
        if(!retained){retained=root;retained_digest=message_hash(root);}
        else free(out.owner);
    }
    CHECK(count>0&&first.handle!=0);
    CHECK(mac_native_tags_owned_bytes(&tags)>0);
    CHECK(mac_native_tags_get(&tags,UINT32_C(0x686d7420),first.handle,sizeof(*retained),&error)==retained);
    copy=first.handle?malloc(length):NULL;CHECK(copy!=NULL);
    if(copy) {
        uint32_t root_offset,element_count,message_count,element_address,message_address,j;
        memcpy(copy,bytes,length);CHECK(mac_retail_open(copy,length,&mutated,&error));
        root_offset=mutated.tag_offset+(first.address-UINT32_C(0x40440000));
        CHECK(reject_at(bytes,(size_t)length,first.handle,root_offset+20,4,UINT32_MAX));
        element_count=rd32(bytes+root_offset+20);
        message_count=rd32(bytes+root_offset+32);
        element_address=rd32(bytes+root_offset+24);
        message_address=rd32(bytes+root_offset+36);
        if(message_count) {
            uint32_t message_offset=mutated.tag_offset+(message_address-UINT32_C(0x40440000));
            CHECK(reject_at(bytes,(size_t)length,first.handle,message_offset+0x24,1,9));
            if(element_count) {
                const unsigned char *messages=mac_retail_tag_span(&mutated,message_address,message_count*0x40);
                const unsigned char *elements=mac_retail_tag_span(&mutated,element_address,element_count*2);
                int tested=0;
                for(j=0;j<message_count&&!tested;++j) {
                    const unsigned char *m=messages+j*0x40;
                    uint32_t start=rd32(m+0x22)&65535U,amount=m[0x24],position=rd32(m+0x20)&65535U,k;
                    for(k=0;k<amount;++k) {
                        const unsigned char *e=elements+(start+k)*2;
                        if(e[0]==0&&e[1]) {
                            uint32_t text_address=rd32(bytes+root_offset+12);
                            size_t text_offset=mutated.tag_offset+(text_address-UINT32_C(0x40440000))+
                                (position+e[1]-1)*2;
                            CHECK(reject_at(bytes,(size_t)length,first.handle,text_offset,2,1));
                            tested=1;break;
                        }
                        if(e[0]==0)position+=e[1];
                    }
                }
                CHECK(tested);
            }
        }
        free(copy);
    }
    free(bytes);
    if(retained) {
        CHECK(sizeof(*retained)==0x80);
        CHECK(retained->messages.count<=1024);
        CHECK(message_hash(retained)==retained_digest);
    }
    mac_native_tags_free(&tags);CHECK(!tags.state);
    printf("native HUD messages=%lu digest=%08lx checks=%u failures=%u\n",
        (unsigned long)count,(unsigned long)digest,checks,failures);
    return failures?1:0;
}
