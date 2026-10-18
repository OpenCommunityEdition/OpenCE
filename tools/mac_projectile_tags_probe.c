#include "cseries.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "effects/contrail_definitions.h"
#include "native_object_tags.h"
#include "native_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
static unsigned checks,failures,refs;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d failed: %s\n",__LINE__,#x); } } while (0)
static int visit(void *context,uint32_t group,uint32_t handle)
{ (void)context;(void)group;(void)handle;++refs;return 1; }
static void reject(const struct mac_retail_cache *cache,uint32_t handle,int contrail,unsigned char *field,uint32_t value)
{
    unsigned char old[4];const char *error;struct mac_native_object_tag t;int result;
    memcpy(old,field,4);field[0]=(unsigned char)value;field[1]=(unsigned char)(value>>8);field[2]=(unsigned char)(value>>16);field[3]=(unsigned char)(value>>24);
    result=contrail?mac_native_contrail_decode(cache,handle,&t,&error):mac_native_projectile_decode(cache,handle,&t,&error);
    CHECK(!result && !t.root && !t.owner);if(result)mac_native_object_tag_free(&t);memcpy(field,old,4);
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes;const char *error;
    struct mac_retail_cache cache;struct mac_retail_tag raw;
    struct mac_native_tags registry;
    struct mac_native_object_tag owned[8],weapon;
    uint32_t weapons[2]={UINT32_C(0xe25e00ea),UINT32_C(0xe34d01d9)};
    unsigned count=0,i,j,k;uint32_t digest=2166136261U;
    memset(owned,0,sizeof(owned));
    if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||fseek(f,0,SEEK_SET))return 2;
    bytes=malloc((size_t)length);if(!bytes||fread(bytes,1,(size_t)length,f)!=(size_t)length)return 2;fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)||!mac_native_tags_create(&cache,&registry,&error))return 2;
    for(i=0;i<2;++i) {
        struct weapon_definition *w;struct projectile_definition *p;struct weapon_trigger_definition *trigger;
        uint32_t projectile;
        if(!mac_native_weapon_decode(&cache,weapons[i],&weapon,&error))return 2;
        w=weapon.root;CHECK(w->weapon.triggers.count>0);trigger=w->weapon.triggers.address;projectile=(uint32_t)trigger->projectile.index;
        if(!mac_native_projectile_decode(&cache,projectile,&owned[count],&error)){fprintf(stderr,"projectile %08x: %s\n",projectile,error);return 1;}
        p=owned[count].root;CHECK(p->object.type==5);CHECK(p->projectile.material_responses.count==33);
        CHECK(mac_native_object_visit_references(&owned[count],visit,NULL));
        printf("projectile %08x: %lu responses %lu attachments digest %08x\n",projectile,(unsigned long)p->projectile.material_responses.count,(unsigned long)p->object.attachments.count,owned[count].field_digest);
        CHECK(mac_retail_tag_get(&cache,projectile&65535,&raw));
        {
            unsigned char *disk=(unsigned char *)mac_retail_tag_span(&cache,raw.address,owned[count].root_bytes);
            reject(&cache,projectile,0,disk+offsetof(struct projectile_definition,projectile.material_responses),34);
            reject(&cache,projectile,0,disk+offsetof(struct projectile_definition,projectile.material_responses)+4,0);
            reject(&cache,projectile,0,disk+offsetof(struct projectile_definition,projectile.initial_velocity),0x7fc00000U);
            reject(&cache,projectile,0,disk,0);
        }
        ++count;
        for(j=0;j<(unsigned)p->object.attachments.count;++j) {
            struct object_attachment_definition *a=(struct object_attachment_definition *)p->object.attachments.address+j;
            struct contrail_definition *c;uint32_t handle=(uint32_t)a->type.index;
            if(a->type.group_tag!=UINT32_C(0x636f6e74)||handle==UINT32_MAX)continue;
            for(k=0;k<count;++k)if(owned[k].handle==handle)break;
            if(k<count)continue;
            if(!mac_native_contrail_decode(&cache,handle,&owned[count],&error)){fprintf(stderr,"contrail %08x: %s\n",handle,error);return 1;}
            c=owned[count].root;CHECK(c->states.count>0);CHECK(c->bitmap.index!=NONE);
            CHECK(mac_native_object_visit_references(&owned[count],visit,NULL));
            printf("contrail %08x: %lu states bitmap %08lx digest %08x\n",handle,(unsigned long)c->states.count,(unsigned long)c->bitmap.index,owned[count].field_digest);
            CHECK(mac_retail_tag_get(&cache,handle&65535,&raw));
            {
                unsigned char *disk=(unsigned char *)mac_retail_tag_span(&cache,raw.address,owned[count].root_bytes);
                reject(&cache,handle,1,disk+offsetof(struct contrail_definition,states),17);
                reject(&cache,handle,1,disk+offsetof(struct contrail_definition,states)+4,0);
                reject(&cache,handle,1,disk+offsetof(struct contrail_definition,point_generation_rate),0x7f800000U);
                reject(&cache,handle,1,disk+offsetof(struct contrail_definition,bitmap)+12,0);
            }
            ++count;
        }
        mac_native_object_tag_free(&weapon);
    }
    CHECK(count>=3);
    for(i=0;i<count;++i) {
        CHECK(mac_native_tags_publish(&registry,owned[i].handle,owned[i].group,owned[i].root,owned[i].root_bytes,owned[i].bytes_owned,owned[i].owner,mac_native_object_owner_free,&error));
        owned[i].owner=NULL;digest=(digest^owned[i].field_digest)*16777619U;
    }
    free(bytes);
    for(i=0;i<count;++i) {
        CHECK(mac_native_tags_get(&registry,owned[i].group,owned[i].handle,owned[i].root_bytes,&error)==owned[i].root);
        CHECK(mac_native_object_visit_references(&owned[i],visit,NULL));
    }
    mac_native_tags_free(&registry);
    printf("native projectile/contrail: %u checks %u failures roots %u refs %u digest %08x\n",checks,failures,count,refs,digest);
    return failures?1:0;
}
