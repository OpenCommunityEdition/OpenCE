#include "cseries.h"
#include "tag_files/tag_files.h"
#include "physics/collision_model_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "native_collision_tags.h"
#include "retail_collision_model.h"
#include "retail_object_spawn.h"
#include "engine_tags.h"
#undef malloc
#undef free
#undef memcpy
#undef memcmp
#undef strcmp
#undef strlen
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct probe_region {char name[32];unsigned long flags;long unused;real threshold;long unused1[3];struct tag_reference effect;struct tag_block permutations;};
struct probe_leaf {word flags;short count;long first;};
struct probe_reference {long plane,root;};
static unsigned long checks,failures;static unsigned released;static uint32_t digest=UINT32_C(2166136261);
#define CHECK(x) do{++checks;if(!(x)){if(failures<12)fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);++failures;}}while(0)
static void number(uint32_t actual,uint32_t expected){CHECK(actual==expected);digest=(digest^actual)*UINT32_C(16777619);}
static void realvalue(float a,float b){uint32_t x,y;memcpy(&x,&a,4);memcpy(&y,&b,4);number(x,y);}
static void effect(const struct mac_retail_cache*c,const struct tag_reference*n,const struct mac_collision_effect*p)
{
    struct mac_retail_tag t;number((uint32_t)n->group_tag,p->group);number((uint32_t)n->index,p->handle);
    if(p->handle==UINT32_MAX){CHECK(!n->name&&!n->name_length);return;}
    CHECK(mac_retail_tag_get(c,p->handle&65535,&t));CHECK(n->name&&n->name!=t.name);if(n->name){CHECK(!strcmp(n->name,t.name));CHECK(n->name_length==(long)strlen(n->name));}
}
static void bsp(const struct collision_bsp*n,const struct mac_retail_collision*p)
{
    const struct tag_block*blocks[8];uint32_t i,j;
    const struct bsp3d_node*nodes=n->bsp3d.nodes.address;const real_plane3d*planes=n->bsp3d.planes.address;
    const struct probe_leaf*leaves=n->leaves.address;const struct probe_reference*refs=n->bsp2d_references.address;
    const struct bsp2d_node*nodes2=n->bsp2d.nodes.address;const struct collision_surface*surfaces=n->surfaces.address;
    const struct collision_edge*edges=n->edges.address;const struct collision_vertex*vertices=n->vertices.address;
    blocks[0]=&n->bsp3d.nodes;blocks[1]=&n->bsp3d.planes;blocks[2]=&n->leaves;blocks[3]=&n->bsp2d_references;
    blocks[4]=&n->bsp2d.nodes;blocks[5]=&n->surfaces;blocks[6]=&n->edges;blocks[7]=&n->vertices;
    for(i=0;i<8;++i){number(blocks[i]->count,p->count[i]);CHECK(!blocks[i]->definition);if(p->count[i])CHECK(blocks[i]->address!=NULL);}
    CHECK((const void*)nodes!=(const void*)p->nodes);
    for(i=0;i<p->count[0];++i){number(nodes[i].plane_designator,p->nodes[i].plane);for(j=0;j<2;++j)number(nodes[i].children[j],p->nodes[i].child[j]);}
    for(i=0;i<p->count[1];++i){for(j=0;j<3;++j)realvalue(planes[i].n.n[j],p->planes[i].n[j]);realvalue(planes[i].d,p->planes[i].d);}
    for(i=0;i<p->count[2];++i){number(leaves[i].flags,p->leaves[i].flags);number(leaves[i].count,p->leaves[i].count);number(leaves[i].first,p->leaves[i].first);}
    for(i=0;i<p->count[3];++i){number(refs[i].plane,p->references[i].plane);number(refs[i].root,p->references[i].root);}
    for(i=0;i<p->count[4];++i){realvalue(nodes2[i].plane.n.i,p->nodes2[i].plane[0]);realvalue(nodes2[i].plane.n.j,p->nodes2[i].plane[1]);realvalue(nodes2[i].plane.d,p->nodes2[i].plane[2]);for(j=0;j<2;++j)number(nodes2[i].child_indices[j],p->nodes2[i].child[j]);}
    for(i=0;i<p->count[5];++i){number(surfaces[i].plane_designator,p->surfaces[i].plane);number(surfaces[i].first_edge_index,p->surfaces[i].edge);number(surfaces[i].flags,p->surfaces[i].flags);number(surfaces[i].breakable_surface_index,p->surfaces[i].breakable);number(surfaces[i].material_index,p->surfaces[i].material);}
    for(i=0;i<p->count[6];++i)for(j=0;j<2;++j){number(edges[i].vertex_indices[j],p->edges[i].vertex[j]);number(edges[i].edge_indices[j],p->edges[i].edge[j]);number(edges[i].surface_indices[j],p->edges[i].surface[j]);}
    for(i=0;i<p->count[7];++i){for(j=0;j<3;++j)realvalue(vertices[i].point.n[j],p->vertices[i].p[j]);number(vertices[i].first_edge_index,p->vertices[i].edge);}
}
static void model(const struct mac_retail_cache*c,const struct collision_model*n,const struct mac_retail_collision_model*p)
{
    const struct damage_resistance*r=&n->resistance;const struct mac_collision_resistance*s=&p->resistance;uint32_t i,j;
    const struct damage_resistance_material*materials=r->materials.address;const struct probe_region*regions=r->regions.address;
    const struct pathfinding_sphere*spheres=n->pathfinding_spheres.address;const struct collision_node*nodes=n->nodes.address;
#define V(a,b) realvalue(r->a,s->b)
    number(r->flags,s->flags);number(r->indirect_damage_material_index,s->indirect_material);number(r->shield_material_type,s->shield_material);number(r->shield_failure_function,s->shield_failure_function);
    V(maximum_body_vitality,body_vitality);V(body_system_shock,body_system_shock);V(friendly_damage_resistance,friendly_resistance);V(area_damage_effect_threshold,area_threshold);V(body_damaged_effect_threshold,body_damaged_threshold);V(body_destroyed_threshold,body_destroyed_threshold);
    V(maximum_shield_vitality,shield_vitality);V(shield_failure_threshold,shield_failure_threshold);V(maximum_shield_failure,maximum_shield_failure);V(minimum_shield_stun_damage,minimum_shield_stun_damage);V(shield_stun_time,shield_stun_time);V(shield_recharge_time,shield_recharge_time);V(shield_damaged_effect_threshold,shield_damaged_threshold);V(runtime_shield_recharge_velocity,shield_recharge_velocity);
#undef V
    effect(c,&r->localized_damage_effect,&s->effects[0]);effect(c,&r->area_damage_effect,&s->effects[1]);effect(c,&r->body_damaged_effect,&s->effects[2]);effect(c,&r->body_depleted_effect,&s->effects[3]);effect(c,&r->body_destroyed_effect,&s->effects[4]);effect(c,&r->shield_damaged_effect,&s->effects[5]);effect(c,&r->shield_depleted_effect,&s->effects[6]);effect(c,&r->shield_recharging_effect,&s->effects[7]);
    CHECK(!r->modifiers.count&&!r->modifiers.address&&!r->modifiers.definition);
    number(r->materials.count,p->material_count);number(r->regions.count,p->region_count);number(n->pathfinding_spheres.count,p->sphere_count);number(n->nodes.count,p->node_count);
    for(i=0;i<6;++i)realvalue(n->pathfinding_box.n[i],p->bounds[i]);
    for(i=0;i<p->material_count;++i){const struct mac_collision_material*v=&p->materials[i];CHECK(!memcmp(materials[i].name,v->name,32));number(materials[i].flags,v->flags);number(materials[i].material_type,v->type);realvalue(materials[i].shield_leak_fraction,v->leak);realvalue(materials[i].shield_damage_multiplier,v->shield_multiplier);realvalue(materials[i].body_damage_multiplier,v->body_multiplier);}
    for(i=0;i<p->region_count;++i){const struct mac_collision_region*v=&p->regions[i];CHECK(!memcmp(regions[i].name,v->name,32));number(regions[i].flags,v->flags);realvalue(regions[i].threshold,v->threshold);effect(c,&regions[i].effect,&v->effect);number(regions[i].permutations.count,v->permutation_count);CHECK(!memcmp(regions[i].permutations.address,v->permutations,v->permutation_count*32));}
    for(i=0;i<p->sphere_count;++i){number(spheres[i].node_index,p->spheres[i].node);for(j=0;j<3;++j)realvalue(spheres[i].center.n[j],p->spheres[i].center[j]);realvalue(spheres[i].radius,p->spheres[i].radius);}
    for(i=0;i<p->node_count;++i){const struct mac_collision_model_node*v=&p->nodes[i];const struct collision_bsp*bsps=nodes[i].bsps.address;
        CHECK(!memcmp(nodes[i].name,v->name,32));number(nodes[i].region_index,v->region);number(nodes[i].parent_node_index,v->parent);number(nodes[i].next_sibling_node_index,v->sibling);number(nodes[i].first_child_node_index,v->child);number(nodes[i].runtime_damage_part,v->runtime_damage_part);number(nodes[i].bsps.count,v->bsp_count);
        for(j=0;j<v->bsp_count;++j)bsp(&bsps[j],&v->bsps[j]);
    }
}
static void release(void*p){++released;mac_native_collision_owner_free(p);}
int main(int argc,char**argv)
{
    FILE*f;long length;unsigned char*bytes;struct mac_retail_cache c;struct mac_retail_object_spawn spawn;
    struct mac_native_tags registry={0};struct mac_native_collision_tag tag;struct mac_retail_collision_model*p;
    const char*error;uint32_t handles[5],n=0,i;void*roots[5];unsigned published=0;
    if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(f);return 2;}if(fread(bytes,1,(size_t)length,f)!=(size_t)length){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&c,&error)||!mac_retail_object_spawn_decode(&c,&spawn,&error)||!mac_native_tags_create(&c,&registry,&error)){fprintf(stderr,"native coll setup: %s\n",error);free(bytes);return 1;}
    handles[n++]=spawn.biped.refs[2].handle;for(i=0;i<spawn.weapon_count;++i)if(spawn.weapons[i].object.refs[2].handle!=UINT32_MAX)handles[n++]=spawn.weapons[i].object.refs[2].handle;
    CHECK(mac_engine_tags_bind(&registry));p=malloc(sizeof(*p));if(!p)return 2;
    CHECK(!mac_native_collision_decode(&c,spawn.biped.handle,&tag,&error));CHECK(!tag.root&&!tag.owner);
    CHECK(!mac_native_collision_decode(&c,handles[0]^65536,&tag,&error));CHECK(!tag.root&&!tag.owner);
    for(i=0;i<n;++i){
        if(!mac_retail_collision_model_decode(&c,handles[i],p,&error)||!mac_native_collision_decode(&c,handles[i],&tag,&error)){fprintf(stderr,"native coll: %s\n",error);return 1;}
        CHECK(tag.root_bytes==sizeof(struct collision_model));CHECK(tag.bytes_owned>=tag.root_bytes);model(&c,tag.root,p);mac_retail_collision_model_free(p);
        roots[i]=tag.root;CHECK(mac_native_tags_publish(&registry,handles[i],UINT32_C(0x636f6c6c),tag.root,tag.root_bytes,tag.bytes_owned,tag.owner,release,&error));++published;
        CHECK(collision_model_definition_get((long)handles[i])==tag.root);
        printf("native coll %08lx root_bytes=%lu owned_bytes=%lu\n",(unsigned long)handles[i],(unsigned long)tag.root_bytes,(unsigned long)tag.bytes_owned);
    }
    free(p);free(bytes); /* Both portable staging and original map can now die. */
    for(i=0;i<n;++i){struct collision_model*m=collision_model_definition_get((long)handles[i]);CHECK(m==roots[i]);CHECK(m->nodes.count>0);CHECK(((struct collision_node*)m->nodes.address)[0].name[0]!=0);}
    mac_engine_tags_unbind();mac_native_tags_free(&registry);CHECK(released==published);
    printf("native collision publication: %lu checks %lu failures digest=%08lx owners_released=%u\n",checks,failures,(unsigned long)digest,released);return failures?1:0;
}
