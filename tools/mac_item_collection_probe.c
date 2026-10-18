#include "cseries.h"
#include "items/item_definitions.h"
#include "items/equipment_definitions.h"
#include "native_object_tags.h"
#undef malloc
#undef free
#undef memcpy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures,references;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static uint32_t read32(const unsigned char *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int visit(void *context,uint32_t group,uint32_t handle)
{(void)context;CHECK(group==UINT32_C(0x6974656d)||group==UINT32_C(0x77656170)||group==UINT32_C(0x65716970));CHECK(handle!=UINT32_MAX);++references;return 1;}
static void reject(const struct mac_retail_cache *cache,uint32_t handle,unsigned char *field,uint32_t value)
{
    struct mac_native_object_tag invalid={0};const char *error;unsigned char saved[4];unsigned j;
    memcpy(saved,field,4);for(j=0;j<4;++j)field[j]=(unsigned char)(value>>(8*j));
    CHECK(!mac_native_item_collection_decode(cache,handle,&invalid,&error));CHECK(!invalid.root&&!invalid.owner);
    mac_native_object_tag_free(&invalid);memcpy(field,saved,4);
}
static unsigned equipment_count;
static void equipment_check(struct mac_retail_cache *cache,const struct mac_retail_tag *tag)
{
    struct mac_native_object_tag native={0},invalid={0};struct equipment_definition *definition;
    unsigned char *disk,*powerup;unsigned char saved[4];const char *error;uint32_t bits;
    disk=(unsigned char *)mac_retail_tag_span(cache,tag->address,sizeof(struct equipment_definition));
    CHECK(disk!=NULL);if(!disk)return;
    CHECK(mac_native_equipment_decode(cache,tag->handle,&native,&error));
    if(!native.root){fprintf(stderr,"equipment %s: %s\n",tag->name,error);return;}
    definition=native.root;powerup=disk+offsetof(struct equipment_definition,equipment);
    CHECK(definition->object.type==3);
    CHECK((unsigned short)definition->equipment.powerup_type==(unsigned short)(powerup[0]|powerup[1]<<8));
    CHECK((unsigned short)definition->equipment.grenade_type==(unsigned short)(powerup[2]|powerup[3]<<8));
    memcpy(&bits,&definition->equipment.powerup_time,4);CHECK(bits==read32(powerup+4));
    CHECK((uint32_t)definition->equipment.pickup_sound.index==read32(powerup+20));
    memcpy(saved,powerup,4);powerup[0]=7;powerup[1]=0;
    CHECK(!mac_native_equipment_decode(cache,tag->handle,&invalid,&error));CHECK(!invalid.root);memcpy(powerup,saved,4);
    powerup[2]=2;powerup[3]=0;
    CHECK(!mac_native_equipment_decode(cache,tag->handle,&invalid,&error));CHECK(!invalid.root);memcpy(powerup,saved,4);
    printf("equipment %08lx digest=%08lx\n",(unsigned long)tag->handle,(unsigned long)native.field_digest);
    mac_native_object_tag_free(&native);++equipment_count;
}
int main(int argc,char **argv)
{
    FILE *file;long length;unsigned char *raw;struct mac_retail_cache cache;struct mac_retail_tag tag;
    struct mac_native_object_tag *owned;const char *error;unsigned i,count=0,permutations=0;uint32_t digest=2166136261UL;
    if(argc!=2)return 2;file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<2048||length>0x30000000L||fseek(file,0,SEEK_SET)){fclose(file);return 2;}
    raw=malloc(length);if(!raw){fclose(file);return 2;}
    if(fread(raw,1,length,file)!=(size_t)length){fclose(file);free(raw);return 2;}fclose(file);
    if(!mac_retail_open(raw,length,&cache,&error)){free(raw);return 1;}
    owned=calloc(cache.tag_count,sizeof(*owned));if(!owned){free(raw);return 2;}
    for(i=0;i<cache.tag_count;++i) {
        unsigned char *disk,*rows;struct item_collection_definition *definition;struct item_permutation_definition *native;long j;
        if(!mac_retail_tag_get(&cache,i,&tag))continue;
        if(tag.group==UINT32_C(0x65716970)){equipment_check(&cache,&tag);continue;}
        if(tag.group!=UINT32_C(0x69746d63))continue;
        disk=(unsigned char *)mac_retail_tag_span(&cache,tag.address,0x5c);CHECK(disk!=NULL);if(!disk)continue;
        CHECK(mac_native_item_collection_decode(&cache,tag.handle,owned+count,&error));
        if(!owned[count].root){fprintf(stderr,"collection %s: %s\n",tag.name,error);continue;}
        definition=owned[count].root;native=definition->permutations.address;
        CHECK((uint32_t)definition->permutations.count==read32(disk));
        CHECK((unsigned short)definition->spawn_time==(unsigned short)(disk[12]|disk[13]<<8));
        rows=(unsigned char *)mac_retail_tag_span(&cache,read32(disk+4),read32(disk)*0x54);CHECK(rows!=NULL);
        if(rows)for(j=0;j<definition->permutations.count;++j) {
            uint32_t bits;memcpy(&bits,&native[j].weight,4);CHECK(bits==read32(rows+j*0x54+32));
            CHECK((uint32_t)native[j].item.index==read32(rows+j*0x54+48));
            CHECK(native[j].item.name && native[j].item.name[0]);++permutations;
        }
        reject(&cache,tag.handle,disk,32768);reject(&cache,tag.handle,disk+4,0xffffffff);
        if(rows) {
            reject(&cache,tag.handle,rows+32,0x7fc00000);reject(&cache,tag.handle,rows+32,0xbf800000);
            reject(&cache,tag.handle,rows+32,0x47000000);reject(&cache,tag.handle,rows+36,0x736e6421);
            reject(&cache,tag.handle,rows+48,0xffffffff);
        }
        digest=(digest^owned[count].field_digest)*16777619UL;++count;
    }
    free(raw);raw=NULL;CHECK(count>0);CHECK(permutations>0);
    for(i=0;i<count;++i){CHECK(mac_native_object_visit_references(owned+i,visit,NULL));mac_native_object_tag_free(owned+i);CHECK(!owned[i].owner&&!owned[i].root);}
    CHECK(references==permutations);CHECK(equipment_count>0);free(owned);
    printf("native item collections: %u checks, %u failures; collections=%u permutations=%u equipment=%u digest=%08lx\n",checks,failures,count,permutations,equipment_count,(unsigned long)digest);
    return failures?1:0;
}
