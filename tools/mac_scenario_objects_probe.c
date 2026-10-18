#include "native_scenario_objects.h"
#include "native_scenario_blocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp
static unsigned checks,failures;
int mac_scenario_blocks_source_index_witness(const struct mac_native_scenario_blocks *);
static uint32_t u32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void put32(unsigned char *p,uint32_t v)
{ p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24); }
static void check(int ok,const char *name)
{ ++checks;if(!ok){++failures;fprintf(stderr,"FAIL %s\n",name);} }
static int rejected(const struct mac_retail_cache *c)
{
    struct mac_native_scenario_objects out,zero;const char *error=NULL;int ok;
    memset(&out,0xa5,sizeof(out));memset(&zero,0,sizeof(zero));
    ok=!mac_native_scenario_objects_decode(c,&out,&error) && !memcmp(&out,&zero,sizeof(out));
    mac_native_scenario_objects_free(&out);return ok;
}
static void named_fixture(const struct mac_retail_cache *cache)
{
    struct mac_retail_cache fixture=*cache;struct mac_native_scenario_blocks blocks;
    unsigned char *tag_copy,*scenario,*placement,*name;uint32_t old_size;
    const char *error=NULL;
    old_size=cache->tag_size;tag_copy=(unsigned char *)malloc((size_t)old_size+36);
    if(!tag_copy){check(0,"named fixture allocation");return;}
    memcpy(tag_copy,cache->tags,old_size);memset(tag_copy+old_size,0,36);
    fixture.tags=tag_copy;fixture.tag_size=old_size+36;
    scenario=tag_copy+fixture.scenario_offset;name=tag_copy+old_size;
    memcpy(name,"native placement name",21);put32(scenario+0x204,1);
    put32(scenario+0x208,UINT32_C(0x40440000)+old_size);
    placement=(unsigned char *)mac_retail_tag_span(&fixture,u32(scenario+0x214),40);
    if(!placement){check(0,"named fixture placement span");free(tag_copy);return;}
    placement[2]=0;placement[3]=0;
    /* Cache runtime fields are outputs of object_names_postprocess; seed
       nonsense and verify the adapter derives the native mapping from placement. */
    name[32]=0x7f;name[33]=0x7f;name[34]=0x7f;name[35]=0x7f;
    if(!mac_native_scenario_blocks_decode(&fixture,&blocks,&error)) {
        fprintf(stderr,"named fixture: %s\n",error?error:"decode failed");
        check(0,"named fixture native conversion");free(tag_copy);return;
    }
    check(blocks.object_name_count==1,"named fixture object name count");
    check(blocks.object_names[0].runtime_object_type==6 &&
        blocks.object_names[0].runtime_scenario_datum_index==0,
        "runtime object-name type/index derived from scenery placement");
    check(!strcmp(blocks.object_names[0].name,"native placement name"),"native object name text");
    mac_native_scenario_blocks_free(&blocks);free(tag_copy);
}
static void mutation(const struct mac_retail_cache *c,unsigned char *p,uint32_t value,const char *label)
{
    unsigned char save[4];memcpy(save,p,4);put32(p,value);check(rejected(c),label);memcpy(p,save,4);
}
int main(int argc,char **argv)
{
    FILE *f;long length;unsigned char *bytes,*scenario,*block;struct mac_retail_cache cache;
    struct mac_native_scenario_objects view;struct mac_native_scenario_blocks blocks,bad_blocks;
    const char *error=NULL;uint32_t i;
    static const uint32_t expected_place[MAC_SCENARIO_FAMILY_COUNT]={47,0,28,0,0,0,0,0,2};
    static const uint32_t expected_palette[MAC_SCENARIO_FAMILY_COUNT]={19,1,5,0,0,0,0,0,1};
    if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)length);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&cache,&error)){fprintf(stderr,"map: %s\n",error);free(bytes);return 1;}
    if(!mac_native_scenario_objects_decode(&cache,&view,&error)){fprintf(stderr,"scenario objects: %s\n",error);free(bytes);return 1;}
    check(view.name_count==0,"actual map has empty object-name block");
    for(i=0;i<MAC_SCENARIO_FAMILY_COUNT;++i) {
        check(view.families[i].placement_count==expected_place[i],"actual placement family count");
        check(view.families[i].palette_count==expected_palette[i],"actual palette family count");
    }
    check(view.families[MAC_SCENARIO_SCENERY].placements[0].palette_entry_index>=0,"native scenery placement fields");
    check(view.families[MAC_SCENARIO_VEHICLE].placements[0].palette_entry_index>=0,"native vehicle placement fields");
    check(view.families[MAC_SCENARIO_VEHICLE].placements[0].has_permutation,"vehicle scenario permutation decoded");
    check(view.families[MAC_SCENARIO_VEHICLE].placements[0].has_unit_state,"vehicle scenario unit state decoded");
    check(view.families[MAC_SCENARIO_SCENERY].placements[0].has_permutation,"scenery scenario permutation decoded");
    check(view.families[MAC_SCENARIO_SOUND_SCENERY].palette[0].handle!=UINT32_MAX,"sound-scenery palette handle");
    check(view.bytes_owned>0,"owned conversion allocations recorded");
    scenario=(unsigned char *)cache.tags+cache.scenario_offset;
    if(!mac_native_scenario_blocks_decode(&cache,&blocks,&error)) {
        fprintf(stderr,"scenario native blocks: %s\n",error?error:"decode failed");
        mac_native_scenario_objects_free(&view);free(bytes);return 1;
    }
    check(blocks.families[MAC_SCENARIO_SCENERY].placement_count==47,"native scenery engine block count");
    check(blocks.families[MAC_SCENARIO_SCENERY].palette_count==19,"native scenery engine palette count");
    check(blocks.families[MAC_SCENARIO_VEHICLE].placement_count==28,"native vehicle engine block count");
    check(blocks.families[MAC_SCENARIO_VEHICLE].palette_count==5,"native vehicle engine palette count");
    check(blocks.families[MAC_SCENARIO_SOUND_SCENERY].placement_count==2,"native sound-scenery block count");
    check(blocks.families[MAC_SCENARIO_BIPED].placement_count==0 && blocks.families[MAC_SCENARIO_BIPED].palette_count==1,"native empty biped placements and palette");
    check(blocks.families[MAC_SCENARIO_SCENERY].placements!=NULL && blocks.families[MAC_SCENARIO_VEHICLE].placements!=NULL,"exact-stride engine placements allocated");
    check(blocks.families[MAC_SCENARIO_VEHICLE].palette[0].reference.name!=NULL,"native palette reference owns accessible name");
    { uint32_t bits;
      memcpy(&bits,&blocks.families[MAC_SCENARIO_SCENERY].palette[1].reference.index,4);
      check(bits==view.families[MAC_SCENARIO_SCENERY].palette[1].handle,
          "scenery palette reference index one preserves salted handle");
      memcpy(&bits,&blocks.families[MAC_SCENARIO_SCENERY].palette[18].reference.index,4);
      check(bits==view.families[MAC_SCENARIO_SCENERY].palette[18].handle,
          "scenery palette last reference preserves salted handle");
      memcpy(&bits,&blocks.families[MAC_SCENARIO_VEHICLE].palette[1].reference.index,4);
      check(bits==view.families[MAC_SCENARIO_VEHICLE].palette[1].handle,
          "vehicle palette reference index one preserves salted handle");
      memcpy(&bits,&blocks.families[MAC_SCENARIO_VEHICLE].palette[4].reference.index,4);
      check(bits==view.families[MAC_SCENARIO_VEHICLE].palette[4].handle,
          "vehicle palette last reference preserves salted handle"); }
    check(mac_scenario_blocks_source_index_witness(&blocks),
        "source TAG_BLOCK_GET_ELEMENT indexes 48-byte palette entries");
    check(blocks.bytes_owned>view.bytes_owned,"native engine arrays included in ownership");
    { unsigned char *vehicle;unsigned char saved[4];uint32_t address=u32(scenario+0x240+4);
      vehicle=(unsigned char *)mac_retail_tag_span(&cache,address+0x58,4);
      if(!vehicle){free(bytes);return 1;}
      memcpy(saved,vehicle,4);vehicle[0]=0x7e;vehicle[2]=0x5a;vehicle[3]=0xa5;
      if(mac_native_scenario_blocks_decode(&cache,&bad_blocks,&error)) {
          const struct mac_native_scenario_vehicle_record *r=bad_blocks.families[MAC_SCENARIO_VEHICLE].placements;
          check(r[0].multiplayer_spawn_flags==UINT16_C(0xa55a),"vehicle CE spawn flags preserve nonzero native endian value");
          check(r[0].multiplayer_team_index==0x7e,"vehicle CE team index preserves nonzero fixture");
          mac_native_scenario_blocks_free(&bad_blocks);
      } else check(0,"vehicle CE state fixture decodes");
      memcpy(vehicle,saved,4);
    }
    /* Scenario object_names block, scenery placement and scenery palette. */
    block=(unsigned char *)mac_retail_tag_span(&cache,u32(scenario+0x210+4),0x48);
    if(!block){free(bytes);mac_native_scenario_objects_free(&view);return 1;}
    mutation(&cache,block+8,UINT32_C(0x7fc00000),"NaN placement rejected");
    { unsigned char save[2];memcpy(save,block,2);block[0]=0xff;block[1]=0x7f;
      check(rejected(&cache),"out-of-range palette index rejected");memcpy(block,save,2); }
    block=(unsigned char *)mac_retail_tag_span(&cache,u32(scenario+0x21c+4),48);
    if(!block){free(bytes);mac_native_scenario_objects_free(&view);return 1;}
    mutation(&cache,block,UINT32_C(0x62697064),"wrong palette group rejected");
    mutation(&cache,block+12,u32(block+12)^UINT32_C(0x00010000),"wrong salted palette handle rejected");
    { uint32_t saved=u32(scenario+0x204);put32(scenario+0x204,1);
      check(rejected(&cache),"nonempty object-name block with null address rejected");
      put32(scenario+0x204,saved); }
    { unsigned char header[8],saved_name[32];uint32_t name_address=u32(scenario+0x210+4);
      memcpy(header,scenario+0x204,8);block=(unsigned char *)mac_retail_tag_span(&cache,name_address,36);
      if(!block){free(bytes);mac_native_scenario_objects_free(&view);return 1;}
      memcpy(saved_name,block,32);memset(block,'x',32);put32(scenario+0x204,1);put32(scenario+0x208,name_address);
      check(rejected(&cache),"unterminated object name rejected");memcpy(scenario+0x204,header,8);memcpy(block,saved_name,32); }
    /* Create a bounded, structurally valid nonempty equipment fixture by
       retagging one existing reference; the engine-block adapter must refuse
       its unsupported placement family rather than silently dropping it. */
    { unsigned char *palette;struct mac_native_scenario_objects fixture_view;
      uint32_t scen_place_count=u32(scenario+0x210),scen_place_address=u32(scenario+0x214);
      uint32_t scen_pal_count=u32(scenario+0x21c),scen_pal_address=u32(scenario+0x220);
      uint32_t fixture_place_count=u32(scenario+0x2c4),fixture_place_address=u32(scenario+0x2c8);
      uint32_t fixture_palette_count=u32(scenario+0x2d0),fixture_palette_address=u32(scenario+0x2d4);
      uint32_t old_group,handle,tag_index,tag_group;unsigned char *tag_row;
      palette=(unsigned char *)mac_retail_tag_span(&cache,scen_pal_address,48);
      if(!palette){free(bytes);mac_native_scenario_blocks_free(&blocks);mac_native_scenario_objects_free(&view);return 1;}
      old_group=u32(palette);handle=u32(palette+12);tag_index=handle&65535;
      tag_row=(unsigned char *)cache.tags+cache.instances_offset+tag_index*32;tag_group=u32(tag_row);
      put32(palette,UINT32_C(0x6c696669));put32(tag_row,UINT32_C(0x6c696669));
      put32(scenario+0x210,0);put32(scenario+0x21c,0);
      put32(scenario+0x2c4,1);put32(scenario+0x2c8,scen_place_address);
      put32(scenario+0x2d0,1);put32(scenario+0x2d4,scen_pal_address);
      check(mac_native_scenario_objects_decode(&cache,&fixture_view,&error),"light-fixture layout fixture decodes");
      if(fixture_view.owner) {
          check(!fixture_view.families[MAC_SCENARIO_LIGHT_FIXTURE].placements[0].has_permutation,
              "light-fixture tail is not misread as a scenario permutation");
          mac_native_scenario_objects_free(&fixture_view);
      }
      check(!mac_native_scenario_blocks_decode(&cache,&bad_blocks,&error),"unsupported nonempty light-fixture family rejected");
      check(!bad_blocks.owner && !bad_blocks.decoded.owner,"unsupported family leaves zero output");
      put32(palette,old_group);put32(tag_row,tag_group);
      put32(scenario+0x210,scen_place_count);put32(scenario+0x214,scen_place_address);
      put32(scenario+0x21c,scen_pal_count);put32(scenario+0x220,scen_pal_address);
      put32(scenario+0x2c4,fixture_place_count);put32(scenario+0x2c8,fixture_place_address);
      put32(scenario+0x2d0,fixture_palette_count);put32(scenario+0x2d4,fixture_palette_address);
    }
    named_fixture(&cache);
    /* The public view owns both names and converted placement values. */
    { char first_name[33];strcpy(first_name,view.families[MAC_SCENARIO_SCENERY].palette[0].name);
      free(bytes);bytes=NULL;
      check(!strcmp(first_name,view.families[MAC_SCENARIO_SCENERY].palette[0].name),"palette name survives map unmap");
      check(view.families[MAC_SCENARIO_SCENERY].placements[0].position[0]==view.families[MAC_SCENARIO_SCENERY].placements[0].position[0],"placement survives map unmap"); }
    mac_native_scenario_blocks_free(&blocks);
    mac_native_scenario_objects_free(&view);
    check(!view.owner && !view.names,"owned scenario view frees cleanly");
    printf("scenario object checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
