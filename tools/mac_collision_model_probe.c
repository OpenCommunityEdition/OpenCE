#include "retail_collision_model.h"
#include "retail_object_spawn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;static uint32_t digest=UINT32_C(2166136261);
static void check(int ok){++checks;if(!ok)++failures;}
static void put(unsigned char*p,uint32_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static void put16(unsigned char*p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void word(uint32_t n){digest=(digest^n)*UINT32_C(16777619);}
static void realword(float f){uint32_t n;memcpy(&n,&f,4);word(n);}
static void nameword(const char*p){unsigned i;for(i=0;i<32;++i)word((unsigned char)p[i]);}
static void effectword(struct mac_collision_effect e){word(e.group);word(e.handle);}
static void synthetic(void)
{
    unsigned char bytes[2048],saved[2048];struct mac_retail_cache c;struct mac_retail_collision_model*out;const char*e;unsigned i;
    static const unsigned effects[8]={112,132,152,168,188,392,408,424};
    out=malloc(sizeof(*out));if(!out){check(0);return;}memset(bytes,0,sizeof(bytes));memset(&c,0,sizeof(c));c.tags=bytes;c.tags_end=32;c.tag_count=1;c.tag_size=sizeof(bytes);
    put(bytes,UINT32_C(0x636f6c6c));put(bytes+12,UINT32_C(0x12340000));put(bytes+16,MAC_RETAIL_BASE+64);put(bytes+20,MAC_RETAIL_BASE+128);
    put16(bytes+132,65535);for(i=0;i<8;++i)put(bytes+128+effects[i]+12,UINT32_MAX);
    put(bytes+128+564,1);put(bytes+128+568,MAC_RETAIL_BASE+900);
    put(bytes+128+640,1);put(bytes+128+644,MAC_RETAIL_BASE+1500);put(bytes+1528,UINT32_C(0x3f800000));
    put(bytes+128+652,1);put(bytes+128+656,MAC_RETAIL_BASE+1024);
    for(i=0;i<4;++i)put16(bytes+1024+32+i*2,65535);put(bytes+1024+52,1);put(bytes+1024+56,MAC_RETAIL_BASE+1100);
    put(bytes+1100,1);put(bytes+1104,MAC_RETAIL_BASE+1600);put(bytes+1112,1);put(bytes+1116,MAC_RETAIL_BASE+1620);
    put(bytes+1604,UINT32_MAX);put(bytes+1608,UINT32_MAX);put(bytes+1628,UINT32_C(0x3f800000));
    memcpy(saved,bytes,sizeof(bytes));check(mac_retail_collision_model_decode(&c,UINT32_C(0x12340000),out,&e));
    check(out->node_count==1&&out->nodes[0].bsp_count==1&&out->nodes[0].bsps[0].planes[0].n[2]==1.f&&out->sphere_count==1&&out->spheres[0].radius==1.f);
    mac_retail_collision_model_free(out);check(!out->node_count&&!out->nodes[0].bsps);
#define BAD32(at,value) do{put(bytes+(at),(value));check(!mac_retail_collision_model_decode(&c,UINT32_C(0x12340000),out,&e));check(!out->node_count&&!out->nodes[0].bsps);memcpy(bytes,saved,sizeof(bytes));}while(0)
    BAD32(128+588,1);BAD32(128+564,33);BAD32(128+652,65);BAD32(128+656,MAC_RETAIL_BASE+2040);
    BAD32(128+8,UINT32_C(0x7fc00000));BAD32(900+40,UINT32_C(0x7f800000));BAD32(1500+28,UINT32_C(0xbf800000));
    BAD32(1024+52,33);BAD32(1100,131073);BAD32(1104,MAC_RETAIL_BASE+2044);BAD32(1600,1);BAD32(1604,0);BAD32(1628,UINT32_C(0x7f800000));
#undef BAD32
#define BAD16(at,value) do{put16(bytes+(at),(value));check(!mac_retail_collision_model_decode(&c,UINT32_C(0x12340000),out,&e));memcpy(bytes,saved,sizeof(bytes));}while(0)
    BAD16(1024+34,0);BAD16(1024+36,0);BAD16(1024+38,0);BAD16(1500,64);BAD16(900+36,33);BAD16(132,1);
#undef BAD16
    check(!mac_retail_collision_model_decode(&c,UINT32_C(0x99990000),out,&e));free(out);
}
static void bsp_digest(const struct mac_retail_collision*b)
{
    uint32_t i,j;for(i=0;i<8;++i)word(b->count[i]);
    for(i=0;i<b->count[0];++i){word(b->nodes[i].plane);word(b->nodes[i].child[0]);word(b->nodes[i].child[1]);}
    for(i=0;i<b->count[1];++i){for(j=0;j<3;++j)realword(b->planes[i].n[j]);realword(b->planes[i].d);}
    for(i=0;i<b->count[2];++i){word(b->leaves[i].flags);word(b->leaves[i].count);word(b->leaves[i].first);}
    for(i=0;i<b->count[3];++i){word(b->references[i].plane);word(b->references[i].root);}
    for(i=0;i<b->count[4];++i){for(j=0;j<3;++j)realword(b->nodes2[i].plane[j]);word(b->nodes2[i].child[0]);word(b->nodes2[i].child[1]);}
    for(i=0;i<b->count[5];++i){word(b->surfaces[i].plane);word(b->surfaces[i].edge);word(b->surfaces[i].flags);word(b->surfaces[i].breakable);word((uint32_t)(int32_t)b->surfaces[i].material);}
    for(i=0;i<b->count[6];++i)for(j=0;j<2;++j){word(b->edges[i].vertex[j]);word(b->edges[i].edge[j]);word(b->edges[i].surface[j]);}
    for(i=0;i<b->count[7];++i){for(j=0;j<3;++j)realword(b->vertices[i].p[j]);word(b->vertices[i].edge);}
}
static void model_digest(const struct mac_retail_collision_model*m)
{
    const struct mac_collision_resistance*r=&m->resistance;uint32_t i,j,bsps=0,surfaces=0,vertices=0;
    word(m->handle);word(r->flags);word((uint32_t)(int32_t)r->indirect_material);word((uint32_t)(int32_t)r->shield_material);word((uint32_t)(int32_t)r->shield_failure_function);
    realword(r->body_vitality);realword(r->body_system_shock);realword(r->friendly_resistance);realword(r->area_threshold);realword(r->body_damaged_threshold);realword(r->body_destroyed_threshold);
    realword(r->shield_vitality);realword(r->shield_failure_threshold);realword(r->maximum_shield_failure);realword(r->minimum_shield_stun_damage);realword(r->shield_stun_time);realword(r->shield_recharge_time);realword(r->shield_damaged_threshold);realword(r->shield_recharge_velocity);
    for(i=0;i<8;++i)effectword(r->effects[i]);for(i=0;i<6;++i)realword(m->bounds[i]);
    word(m->material_count);word(m->region_count);word(m->sphere_count);word(m->node_count);
    for(i=0;i<m->material_count;++i){const struct mac_collision_material*v=&m->materials[i];nameword(v->name);word(v->flags);word((uint32_t)(int32_t)v->type);realword(v->leak);realword(v->shield_multiplier);realword(v->body_multiplier);}
    for(i=0;i<m->region_count;++i){const struct mac_collision_region*v=&m->regions[i];nameword(v->name);word(v->flags);realword(v->threshold);effectword(v->effect);word(v->permutation_count);for(j=0;j<v->permutation_count;++j)nameword(v->permutations[j]);}
    for(i=0;i<m->sphere_count;++i){word((uint32_t)(int32_t)m->spheres[i].node);for(j=0;j<3;++j)realword(m->spheres[i].center[j]);realword(m->spheres[i].radius);}
    for(i=0;i<m->node_count;++i){const struct mac_collision_model_node*v=&m->nodes[i];nameword(v->name);word((uint32_t)(int32_t)v->region);word((uint32_t)(int32_t)v->parent);word((uint32_t)(int32_t)v->sibling);word((uint32_t)(int32_t)v->child);word((uint32_t)(int32_t)v->runtime_damage_part);word(v->bsp_count);
        for(j=0;j<v->bsp_count;++j){++bsps;surfaces+=v->bsps[j].count[5];vertices+=v->bsps[j].count[7];bsp_digest(&v->bsps[j]);}
    }
    printf("coll=%08lx materials=%lu regions=%lu spheres=%lu nodes=%lu BSPs=%lu surfaces=%lu vertices=%lu body=%.6f shield=%.6f\n",(unsigned long)m->handle,(unsigned long)m->material_count,(unsigned long)m->region_count,(unsigned long)m->sphere_count,(unsigned long)m->node_count,(unsigned long)bsps,(unsigned long)surfaces,(unsigned long)vertices,r->body_vitality,r->shield_vitality);
}
int main(int argc,char**argv)
{
    FILE*f;long length;unsigned char*bytes;struct mac_retail_cache c;struct mac_retail_object_spawn spawn;struct mac_retail_collision_model*m;const char*e;uint32_t i,handles[5],n=0;
    synthetic();printf("collision model synthetic checks=%u failures=%u\n",checks,failures);if(failures)return 1;if(argc==1)return 0;if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(f);return 2;}if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&c,&e)||!mac_retail_object_spawn_decode(&c,&spawn,&e)){fprintf(stderr,"collision setup: %s\n",e);free(bytes);return 1;}
    handles[n++]=spawn.biped.refs[2].handle;for(i=0;i<spawn.weapon_count;++i)if(spawn.weapons[i].object.handle!=UINT32_MAX)handles[n++]=spawn.weapons[i].object.refs[2].handle;
    m=malloc(sizeof(*m));if(!m){free(bytes);return 2;}
    for(i=0;i<n;++i){if(handles[i]==UINT32_MAX)continue;if(!mac_retail_collision_model_decode(&c,handles[i],m,&e)){fprintf(stderr,"coll %08lx: %s\n",(unsigned long)handles[i],e);free(m);free(bytes);return 1;}model_digest(m);mac_retail_collision_model_free(m);}
    printf("collision model native digest=%08lx\n",(unsigned long)digest);free(m);free(bytes);return 0;
}
