#include "cseries.h"
#include "tag_files/tag_files.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "native_collision_tags.h"
#include "retail_collision_model.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
/* TU-private engine records (damage.c and collision_bsp.c), independently
   witnessed. Other records use the actual source headers directly. */
struct native_damage_region { char name[32];unsigned long flags;long unused0;real threshold;long unused1[3];struct tag_reference effect;struct tag_block permutations; };
struct world_collision_leaf { word flags;short count;long first; };
struct world_collision_reference { long plane,root; };
#define WITNESS(name,test) typedef char name[(test)?1:-1]
WITNESS(coll_root_size,sizeof(struct collision_model)==0x298);
WITNESS(coll_resistance_size,sizeof(struct damage_resistance)==0x258);
WITNESS(coll_materials_offset,offsetof(struct collision_model,resistance.materials)==0x234);
WITNESS(coll_regions_offset,offsetof(struct collision_model,resistance.regions)==0x240);
WITNESS(coll_spheres_offset,offsetof(struct collision_model,pathfinding_spheres)==0x280);
WITNESS(coll_nodes_offset,offsetof(struct collision_model,nodes)==0x28c);
WITNESS(coll_node_size,sizeof(struct collision_node)==0x40);
WITNESS(coll_node_bsps,offsetof(struct collision_node,bsps)==0x34);
WITNESS(coll_region_size,sizeof(struct native_damage_region)==0x54);
WITNESS(coll_region_effect,offsetof(struct native_damage_region,effect)==0x38);
WITNESS(coll_region_permutations,offsetof(struct native_damage_region,permutations)==0x48);
WITNESS(coll_sphere_size,sizeof(struct pathfinding_sphere)==32);
WITNESS(coll_leaf_size,sizeof(struct world_collision_leaf)==8);
WITNESS(coll_reference_size,sizeof(struct world_collision_reference)==8);
WITNESS(coll_bsp_size,sizeof(struct collision_bsp)==96);
struct collision_allocation { void *data;struct collision_allocation *next; };
struct collision_owner { struct collision_allocation *head;size_t bytes; };
static long signed_word(uint32_t bits){long value;memcpy(&value,&bits,4);return value;}
static void *owned_alloc(struct collision_owner*owner,size_t size)
{
    struct collision_allocation*a;void*p;const size_t budget=64U*1024U*1024U;
    if(!size || size>budget-sizeof(*a) || owner->bytes>budget-size-sizeof(*a))return NULL;
    a=malloc(sizeof(*a));if(!a)return NULL;p=calloc(1,size);if(!p){free(a);return NULL;}
    a->data=p;a->next=owner->head;owner->head=a;owner->bytes+=size+sizeof(*a);return p;
}
static void *allocate_block(struct collision_owner*owner,struct tag_block*block,uint32_t count,size_t stride)
{
    void*p;if(!stride || count>UINT32_C(0x7fffffff)/stride)return NULL;
    p=owned_alloc(owner,(count?count:1)*stride);
    if(p){block->count=(long)count;block->address=p;block->definition=NULL;}return p;
}
void mac_native_collision_owner_free(void*context)
{
    struct collision_owner*o=context;struct collision_allocation*a,*next;if(!o)return;
    for(a=o->head;a;a=next){next=a->next;free(a->data);free(a);}free(o);
}
void mac_native_collision_tag_free(struct mac_native_collision_tag*t)
{if(t){mac_native_collision_owner_free(t->owner);memset(t,0,sizeof(*t));}}
static int bind_effect(const struct mac_retail_cache*c,struct collision_owner*owner,const struct mac_collision_effect*input,struct tag_reference*out)
{
    struct mac_retail_tag tag;const char*end;size_t size,available;
    out->group_tag=signed_word(input->group);out->index=signed_word(input->handle);out->name=NULL;out->name_length=0;
    if(input->handle==UINT32_MAX)return 1;
    if(!mac_retail_tag_get(c,input->handle&65535,&tag)||tag.handle!=input->handle||tag.group!=input->group)return 0;
    available=c->tag_size-(size_t)((const unsigned char*)tag.name-c->tags);if(available>256)available=256;
    end=memchr(tag.name,0,available);if(!end)return 0;size=(size_t)(end-tag.name)+1;
    out->name=owned_alloc(owner,size);if(!out->name)return 0;memcpy(out->name,tag.name,size);out->name_length=(long)(size-1);return 1;
}
static int bind_collision(struct collision_owner *owner,struct collision_bsp *output,const struct mac_retail_collision *input)
{
    struct bsp3d_node *nodes;
    real_plane3d *planes;
    struct world_collision_leaf *leaves;
    struct world_collision_reference *references;
    struct bsp2d_node *nodes2;
    struct collision_surface *surfaces;
    struct collision_edge *edges;
    struct collision_vertex *vertices;
    uint32_t i,j;
#define ALLOC(name,block,type,slot) name=(type *)allocate_block(owner,&(block),input->count[slot],sizeof(type));if(!name)return 0
    ALLOC(nodes,output->bsp3d.nodes,struct bsp3d_node,0);
    ALLOC(planes,output->bsp3d.planes,real_plane3d,1);
    ALLOC(leaves,output->leaves,struct world_collision_leaf,2);
    ALLOC(references,output->bsp2d_references,struct world_collision_reference,3);
    ALLOC(nodes2,output->bsp2d.nodes,struct bsp2d_node,4);
    ALLOC(surfaces,output->surfaces,struct collision_surface,5);
    ALLOC(edges,output->edges,struct collision_edge,6);
    ALLOC(vertices,output->vertices,struct collision_vertex,7);
#undef ALLOC
    for(i=0;i<input->count[0];++i) {
        nodes[i].plane_designator=signed_word(input->nodes[i].plane);
        for(j=0;j<2;++j)nodes[i].children[j]=signed_word(input->nodes[i].child[j]);
    }
    for(i=0;i<input->count[1];++i) {
        planes[i].n.i=input->planes[i].n[0];planes[i].n.j=input->planes[i].n[1];planes[i].n.k=input->planes[i].n[2];planes[i].d=input->planes[i].d;
    }
    for(i=0;i<input->count[2];++i) {
        leaves[i].flags=input->leaves[i].flags;leaves[i].count=input->leaves[i].count;leaves[i].first=signed_word(input->leaves[i].first);
    }
    for(i=0;i<input->count[3];++i) {
        references[i].plane=signed_word(input->references[i].plane);references[i].root=signed_word(input->references[i].root);
    }
    for(i=0;i<input->count[4];++i) {
        nodes2[i].plane.n.i=input->nodes2[i].plane[0];nodes2[i].plane.n.j=input->nodes2[i].plane[1];nodes2[i].plane.d=input->nodes2[i].plane[2];
        for(j=0;j<2;++j)nodes2[i].child_indices[j]=signed_word(input->nodes2[i].child[j]);
    }
    for(i=0;i<input->count[5];++i) {
        surfaces[i].plane_designator=signed_word(input->surfaces[i].plane);surfaces[i].first_edge_index=signed_word(input->surfaces[i].edge);
        surfaces[i].flags=input->surfaces[i].flags;surfaces[i].breakable_surface_index=input->surfaces[i].breakable;surfaces[i].material_index=input->surfaces[i].material;
    }
    for(i=0;i<input->count[6];++i)for(j=0;j<2;++j) {
        edges[i].vertex_indices[j]=signed_word(input->edges[i].vertex[j]);edges[i].edge_indices[j]=signed_word(input->edges[i].edge[j]);edges[i].surface_indices[j]=signed_word(input->edges[i].surface[j]);
    }
    for(i=0;i<input->count[7];++i) {
        vertices[i].point.x=input->vertices[i].p[0];vertices[i].point.y=input->vertices[i].p[1];vertices[i].point.z=input->vertices[i].p[2];vertices[i].first_edge_index=signed_word(input->vertices[i].edge);
    }
    return 1;
}
int mac_native_collision_decode(const struct mac_retail_cache*cache,uint32_t handle,
    struct mac_native_collision_tag*out,const char**error)
{
    struct mac_retail_collision_model*input;
    struct collision_owner*owner;struct collision_model*root;
    struct damage_resistance*r;const struct mac_collision_resistance*s;
    struct damage_resistance_material*materials;struct native_damage_region*regions;
    struct pathfinding_sphere*spheres;struct collision_node*nodes;
    struct tag_reference*effects[8];uint32_t i,j;
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));
    input=malloc(sizeof(*input));if(!input){if(error)*error="native collision staging allocation";return 0;}
    if(!mac_retail_collision_model_decode(cache,handle,input,error)){free(input);return 0;}
    owner=calloc(1,sizeof(*owner));if(!owner){mac_retail_collision_model_free(input);free(input);if(error)*error="native collision owner allocation";return 0;}
    owner->bytes=sizeof(*owner);
#define FAIL(message) do{if(error)*error=(message);mac_native_collision_owner_free(owner);mac_retail_collision_model_free(input);free(input);return 0;}while(0)
    root=owned_alloc(owner,sizeof(*root));if(!root)FAIL("native collision root allocation");
    r=&root->resistance;s=&input->resistance;
    r->flags=s->flags;r->indirect_damage_material_index=s->indirect_material;r->shield_material_type=s->shield_material;r->shield_failure_function=s->shield_failure_function;
    r->maximum_body_vitality=s->body_vitality;r->body_system_shock=s->body_system_shock;r->friendly_damage_resistance=s->friendly_resistance;
    r->area_damage_effect_threshold=s->area_threshold;r->body_damaged_effect_threshold=s->body_damaged_threshold;r->body_destroyed_threshold=s->body_destroyed_threshold;
    r->maximum_shield_vitality=s->shield_vitality;r->shield_failure_threshold=s->shield_failure_threshold;r->maximum_shield_failure=s->maximum_shield_failure;
    r->minimum_shield_stun_damage=s->minimum_shield_stun_damage;r->shield_stun_time=s->shield_stun_time;r->shield_recharge_time=s->shield_recharge_time;
    r->shield_damaged_effect_threshold=s->shield_damaged_threshold;r->runtime_shield_recharge_velocity=s->shield_recharge_velocity;
    effects[0]=&r->localized_damage_effect;effects[1]=&r->area_damage_effect;effects[2]=&r->body_damaged_effect;effects[3]=&r->body_depleted_effect;
    effects[4]=&r->body_destroyed_effect;effects[5]=&r->shield_damaged_effect;effects[6]=&r->shield_depleted_effect;effects[7]=&r->shield_recharging_effect;
    for(i=0;i<8;++i)if(!bind_effect(cache,owner,&s->effects[i],effects[i]))FAIL("native collision resistance effect");
    for(i=0;i<6;++i)root->pathfinding_box.n[i]=input->bounds[i];
    materials=allocate_block(owner,&r->materials,input->material_count,sizeof(*materials));
    regions=allocate_block(owner,&r->regions,input->region_count,sizeof(*regions));
    spheres=allocate_block(owner,&root->pathfinding_spheres,input->sphere_count,sizeof(*spheres));
    nodes=allocate_block(owner,&root->nodes,input->node_count,sizeof(*nodes));
    if(!materials||!regions||!spheres||!nodes)FAIL("native collision root blocks");
    for(i=0;i<input->material_count;++i){const struct mac_collision_material*m=&input->materials[i];
        memcpy(materials[i].name,m->name,32);materials[i].flags=m->flags;materials[i].material_type=m->type;
        materials[i].shield_leak_fraction=m->leak;materials[i].shield_damage_multiplier=m->shield_multiplier;materials[i].body_damage_multiplier=m->body_multiplier;
    }
    for(i=0;i<input->region_count;++i){const struct mac_collision_region*p=&input->regions[i];char(*permutations)[32];
        memcpy(regions[i].name,p->name,32);regions[i].flags=p->flags;regions[i].threshold=p->threshold;
        if(!bind_effect(cache,owner,&p->effect,&regions[i].effect))FAIL("native collision region effect");
        permutations=allocate_block(owner,&regions[i].permutations,p->permutation_count,32);if(!permutations)FAIL("native collision permutations");
        for(j=0;j<p->permutation_count;++j)memcpy(permutations[j],p->permutations[j],32);
    }
    for(i=0;i<input->sphere_count;++i){spheres[i].node_index=input->spheres[i].node;for(j=0;j<3;++j)spheres[i].center.n[j]=input->spheres[i].center[j];spheres[i].radius=input->spheres[i].radius;}
    for(i=0;i<input->node_count;++i){const struct mac_collision_model_node*n=&input->nodes[i];struct collision_bsp*bsps;
        memcpy(nodes[i].name,n->name,32);nodes[i].region_index=n->region;nodes[i].parent_node_index=n->parent;
        nodes[i].next_sibling_node_index=n->sibling;nodes[i].first_child_node_index=n->child;nodes[i].runtime_damage_part=n->runtime_damage_part;
        bsps=allocate_block(owner,&nodes[i].bsps,n->bsp_count,sizeof(*bsps));if(!bsps)FAIL("native collision BSP blocks");
        for(j=0;j<n->bsp_count;++j)if(!bind_collision(owner,&bsps[j],&n->bsps[j]))FAIL("native collision BSP arrays");
    }
    out->root=root;out->owner=owner;out->handle=handle;out->root_bytes=sizeof(*root);out->bytes_owned=owner->bytes;
    mac_retail_collision_model_free(input);free(input);return 1;
#undef FAIL
}
