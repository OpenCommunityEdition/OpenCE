#include "cseries.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "native_model_tags.h"
#include "retail_skeleton.h"
#undef malloc
#undef free
#undef memcpy
#undef memset
#include <stdlib.h>
#include <string.h>
/* Private layouts witnessed against models.c and tag_schema_models.c. */
struct model_geometry { byte reserved[36];struct tag_block parts; };
struct model_geometry_part {
    unsigned long flags;short shader_index;char previous_part_index,next_part_index;
    short centroid_primary_node_index,centroid_secondary_node_index;
    real centroid_primary_node_weight,centroid_secondary_node_weight;real_point3d centroid;
    struct tag_block uncompressed_vertices,compressed_vertices,triangles;
    struct triangle_buffer triangle_buffer;struct vertex_buffer vertex_buffer;
};
struct model_shader_reference {struct tag_reference shader;short permutation_index;word pad;long unused[3];};
struct model_region_permutation_marker {char name[32];short node_index;word pad;real_quaternion rotation;real_point3d translation;byte unused[16];};
#define VERIFY(n,e) typedef char n[(e)?1:-1]
VERIFY(model_root_size,sizeof(struct model)==232);
VERIFY(model_node_size,sizeof(struct model_node)==156);
VERIFY(model_marker_size,sizeof(struct model_marker)==64);
VERIFY(model_instance_size,sizeof(struct model_marker_instance)==32);
VERIFY(model_region_size,sizeof(struct model_region)==76);
VERIFY(model_permutation_size,sizeof(struct model_region_permutation)==88);
VERIFY(model_part_size,sizeof(struct model_geometry_part)==104);
VERIFY(model_geometry_size,sizeof(struct model_geometry)==48);
VERIFY(model_shader_size,sizeof(struct model_shader_reference)==32);
VERIFY(model_perm_marker_size,sizeof(struct model_region_permutation_marker)==80);
struct allocation {void *p;struct allocation *next;};
struct stream {struct model_geometry_part *part;struct model_vertex_uncompressed *vertices;uint16_t *indices;uint32_t vertices_count,triangles_count;struct stream *next;};
struct model_owner {struct allocation *allocations;size_t bytes;struct model *model;struct stream *streams;struct mac_native_model_buffers buffers;int finalized;};
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int16_t s16(const unsigned char*p){int32_t n=p[0]|p[1]<<8;return (int16_t)(n<32768?n:n-65536);}
static long word32(uint32_t w){long v;memcpy(&v,&w,4);return v;}
static int floats(void*out,const unsigned char*p,uint32_t n){uint32_t i,w;for(i=0;i<n;++i){w=u32(p+4*i);if((w&0x7fffffffU)>=0x7f800000U)return 0;memcpy((unsigned char*)out+4*i,&w,4);}return 1;}
static void *allocate(struct model_owner*o,size_t size){struct allocation*a;void*p;size_t cap=64U*1024U*1024U;if(!size)size=1;if(size>cap-sizeof(*a)||o->bytes>cap-size-sizeof(*a))return NULL;a=malloc(sizeof(*a));if(!a)return NULL;p=calloc(1,size);if(!p){free(a);return NULL;}a->p=p;a->next=o->allocations;o->allocations=a;o->bytes+=size+sizeof(*a);return p;}
static int name(char*out,const unsigned char*p){if(!memchr(p,0,32))return 0;memcpy(out,p,32);return 1;}
static int index_ok(int n,uint32_t count,int none){return (none&&n==-1)||(n>=0&&(uint32_t)n<count);}
static const unsigned char *block(const struct mac_retail_cache*c,struct model_owner*o,const unsigned char*p,uint32_t max,uint32_t disk,size_t native,struct tag_block*out)
{
    uint32_t n=u32(p),addr=u32(p+4);const unsigned char*raw;
    if(n>max || (n&&((addr&3)||n>UINT32_MAX/disk)))return NULL;
    raw=n?mac_retail_tag_span(c,addr,n*disk):p;if(!raw)return NULL;
    out->address=allocate(o,(n?n:1)*native);if(!out->address)return NULL;out->count=n;return raw;
}
static int shader(const struct mac_retail_cache*c,struct model_owner*o,const unsigned char*p,struct tag_reference*r)
{
    struct mac_retail_tag t;uint32_t h=u32(p+12),g=u32(p);const char*end;size_t n;
    /* Canonical shader families, never trust arbitrary parent metadata. */
    if(h==UINT32_MAX||!mac_retail_tag_get(c,h&65535,&t)||t.handle!=h||t.group!=g)return 0;
    switch(g){case 0x73656e76:case 0x736f736f:case 0x736f7472:case 0x73636869:case 0x73636578:case 0x73776174:case 0x73676c61:case 0x736d6574:case 0x73706c61:break;default:return 0;}
    n=c->tag_size-(size_t)((const unsigned char*)t.name-c->tags);if(n>256)n=256;end=memchr(t.name,0,n);if(!end)return 0;n=end-t.name+1;r->name=allocate(o,n);if(!r->name)return 0;memcpy(r->name,t.name,n);r->name_length=n-1;r->group_tag=word32(g);r->index=word32(h);return 1;
}
void mac_native_model_owner_free(void*ptr){struct model_owner*o=ptr;struct allocation*a,*next;struct stream*s;if(!o)return;if(o->buffers.destroy)for(s=o->streams;s;s=s->next)if(s->part->vertex_buffer.hardware_format||s->part->triangle_buffer.hardware_format)o->buffers.destroy(o->buffers.context,&s->part->vertex_buffer,&s->part->triangle_buffer);for(a=o->allocations;a;a=next){next=a->next;free(a->p);free(a);}free(o);}
void mac_native_model_tag_free(struct mac_native_model_tag*t){if(t){mac_native_model_owner_free(t->owner);memset(t,0,sizeof(*t));}}
const struct model *mac_native_model_staging(const struct mac_native_model_tag*t){return t&&t->owner?((struct model_owner*)t->owner)->model:NULL;}
static int geometry(const struct mac_retail_cache*c,struct model_owner*o,const unsigned char*raw,struct model*model,struct mac_native_model_tag*out)
{
    uint32_t i,j,k,z,vc,tc,vo,so,local,nodes=model->nodes.count;const unsigned char*p,*parts,*v,*indices;struct model_geometry*g=model->geometries.address;
    int local_nodes=!!(model->flags&2);struct model_geometry_part*part;struct stream*s;
    for(i=0;i<(uint32_t)model->geometries.count;++i){
        parts=block(c,o,raw+i*48+36,32,132,sizeof(*part),&g[i].parts);if(!parts)return 0;
        for(j=0;j<(uint32_t)g[i].parts.count;++j){
            p=parts+j*132;part=(struct model_geometry_part*)g[i].parts.address+j;
            /* Cached stream descriptors supersede the empty editor blocks. */
            if(u32(p+32)||u32(p+44)||u32(p+56))return 0;
            vc=u32(p+88);tc=u32(p+72);vo=u32(p+100);so=u32(p+76);local=p[107];
            if(s16(p+68)!=1||s16(p+84)!=4||!vc||vc>65535||!tc||tc>65535||local>22||
                c->model_indices_offset>c->model_size||vo>c->model_indices_offset||vc>(c->model_indices_offset-vo)/68||
                so>c->model_size-c->model_indices_offset||(tc+2)>(c->model_size-c->model_indices_offset-so)/2||
                (vo&3)||((c->model_indices_offset+so)&1)||(local_nodes&&!local))return 0;
            if(c->model_offset>c->file_size||c->model_size>c->file_size-c->model_offset)return 0;
            for(k=0;k<local;++k)if(p[108+k]>=nodes)return 0;
            part->flags=u32(p)&~2U;part->shader_index=s16(p+4);
            part->previous_part_index=(char)p[6];part->next_part_index=(char)p[7];
            if(!index_ok(part->shader_index,model->shaders.count,0)||
                !index_ok((signed char)p[6],g[i].parts.count,1)||!index_ok((signed char)p[7],g[i].parts.count,1))return 0;
            part->centroid_primary_node_index=s16(p+8);part->centroid_secondary_node_index=s16(p+10);
            if(!index_ok(part->centroid_primary_node_index,local_nodes?local:nodes,0)||!index_ok(part->centroid_secondary_node_index,local_nodes?local:nodes,0))return 0;
            if(local_nodes){part->centroid_primary_node_index=p[108+part->centroid_primary_node_index];part->centroid_secondary_node_index=p[108+part->centroid_secondary_node_index];}
            if(!floats(&part->centroid_primary_node_weight,p+12,2)||!floats(&part->centroid,p+20,3))return 0;
            s=allocate(o,sizeof(*s));if(!s)return 0;s->next=o->streams;o->streams=s;s->part=part;s->vertices_count=vc;s->triangles_count=tc;
            s->vertices=allocate(o,vc*sizeof(*s->vertices));s->indices=allocate(o,(tc+2)*sizeof(*s->indices));if(!s->vertices||!s->indices)return 0;
            v=c->file+c->model_offset+vo;indices=c->file+c->model_offset+c->model_indices_offset+so;
            for(k=0;k<vc;++k){struct model_vertex_uncompressed*dst=s->vertices+k;const unsigned char*src=v+k*68;
                if(!floats(&dst->position,src,14)||!floats(dst->node_weights,src+60,2))return 0;
                for(z=0;z<2;++z){int n=s16(src+56+2*z);if(!index_ok(n,local_nodes?local:nodes,0))return 0;dst->nodes[z]=local_nodes?p[108+n]:n;}
                for(z=0;z<3;++z)if(dst->normal.n[z]<-1.005f||dst->normal.n[z]>1.005f||dst->binormal.n[z]<-1.005f||dst->binormal.n[z]>1.005f||dst->tangent.n[z]<-1.005f||dst->tangent.n[z]>1.005f)return 0;
                if(dst->node_weights[0]<0||dst->node_weights[0]>1||dst->node_weights[1]<0||dst->node_weights[1]>1)return 0;
            }
            for(k=0;k<tc+2;++k){uint32_t index=indices[k*2]|indices[k*2+1]<<8;if(index>=vc)return 0;s->indices[k]=(uint16_t)index;}
            ++out->part_count;out->vertex_count+=vc;out->index_count+=tc+2;
        }
    }
    model->flags&=~2U;return 1;
}
int mac_native_model_decode(const struct mac_retail_cache*c,uint32_t handle,struct mac_native_model_tag*out,const char**error)
{
    struct model_owner*o=NULL;struct model*m;struct mac_retail_skeleton sk;struct mac_retail_tag tag;
    const unsigned char*root,*raw,*p,*sub,*q,*geometry_raw;uint32_t i,j,k;struct model_node*nodes;struct model_region*regions;
    struct model_region_permutation*perm;struct model_region_permutation_marker*pm;struct model_marker*markers;struct model_marker_instance*mi;struct model_shader_reference*shaders;
    if(error)*error=NULL;if(!out)return 0;memset(out,0,sizeof(*out));
#define FAIL(s) do{if(error)*error=s;goto fail;}while(0)
    if(!c||!mac_retail_skeleton_decode(c,handle,&sk,error))return 0;
    if(sk.count>=44)FAIL("native model needs renderer palette bridge");
    if(!mac_retail_tag_get(c,handle&65535,&tag)||(root=mac_retail_tag_span(c,tag.address,232))==NULL)FAIL("native model root span");
    o=calloc(1,sizeof(*o));if(!o)FAIL("native model owner allocation");m=allocate(o,sizeof(*m));if(!m)FAIL("native model root allocation");o->model=m;
    m->flags=u32(root);m->node_list_checksum=sk.checksum;
    if(!floats(m->detail_cutoff_pixels,root+8,5)||!floats(&m->base_map_scale,root+48,2))FAIL("native model root float");
    for(i=0;i<5;++i){m->node_counts[i]=s16(root+28+2*i);if(m->node_counts[i]<0||m->node_counts[i]>(short)sk.count)FAIL("native model LOD node count");}
    raw=block(c,o,root+184,64,156,sizeof(*nodes),&m->nodes);if(!raw)FAIL("native model nodes");nodes=m->nodes.address;
    for(i=0;i<sk.count;++i){struct mac_retail_model_node*n=sk.nodes+i;memcpy(nodes[i].name,n->name,32);nodes[i].next_sibling_node_index=n->sibling;nodes[i].first_child_node_index=n->child;nodes[i].parent_node_index=n->parent;memcpy(&nodes[i].default_translation,n->translation,12);memcpy(&nodes[i].default_rotation,n->rotation,16);nodes[i].node_distance_from_parent=n->distance;memcpy(&nodes[i].runtime_default_inverse_matrix,n->inverse,52);}
    geometry_raw=block(c,o,root+208,256,48,sizeof(struct model_geometry),&m->geometries);if(!geometry_raw)FAIL("native model geometries");
    raw=block(c,o,root+196,32,76,sizeof(*regions),&m->regions);if(!raw)FAIL("native model regions");regions=m->regions.address;
    for(i=0;i<(uint32_t)m->regions.count;++i){p=raw+i*76;if(!name(regions[i].name,p))FAIL("native model region name");sub=block(c,o,p+64,32,88,sizeof(*perm),&regions[i].permutations);if(!sub)FAIL("native model permutations");perm=regions[i].permutations.address;
        for(j=0;j<(uint32_t)regions[i].permutations.count;++j){q=sub+j*88;if(!name(perm[j].name,q))FAIL("native model permutation name");perm[j].flags=word32(u32(q+32));perm[j].variant_number=s16(q+36);
            for(k=0;k<5;++k){perm[j].geometry_indices[k]=s16(q+64+2*k);if(!index_ok(perm[j].geometry_indices[k],m->geometries.count,1))FAIL("native model geometry index");}
            p=block(c,o,q+76,64,80,sizeof(*pm),&perm[j].markers);if(!p)FAIL("native model permutation markers");pm=perm[j].markers.address;
            for(k=0;k<(uint32_t)perm[j].markers.count;++k){const unsigned char*mr=p+k*80;if(!name(pm[k].name,mr))FAIL("native model permutation marker name");pm[k].node_index=s16(mr+32);if(!index_ok(pm[k].node_index,sk.count,0)||!floats(&pm[k].rotation,mr+36,4)||!floats(&pm[k].translation,mr+52,3))FAIL("native model permutation marker value");}
        }
    }
    raw=block(c,o,root+172,256,64,sizeof(*markers),&m->markers);if(!raw)FAIL("native model markers");markers=m->markers.address;
    for(i=0;i<(uint32_t)m->markers.count;++i){p=raw+i*64;if(!name(markers[i].name,p))FAIL("native model marker name");sub=block(c,o,p+52,32,32,sizeof(*mi),&markers[i].instances);if(!sub)FAIL("native model marker instances");mi=markers[i].instances.address;
        for(j=0;j<(uint32_t)markers[i].instances.count;++j){q=sub+j*32;mi[j].region_index=q[0];mi[j].permutation_index=q[1];mi[j].node_index=q[2];
            if(q[2]>=sk.count||q[0]>=m->regions.count||q[1]>=regions[q[0]].permutations.count||!floats(&mi[j].translation,q+4,3)||!floats(&mi[j].rotation,q+16,4))FAIL("native model marker instance value");}
    }
    raw=block(c,o,root+220,32,32,sizeof(*shaders),&m->shaders);if(!raw)FAIL("native model shaders");shaders=m->shaders.address;
    for(i=0;i<(uint32_t)m->shaders.count;++i){p=raw+i*32;if(!shader(c,o,p,&shaders[i].shader))FAIL("native model shader reference");shaders[i].permutation_index=s16(p+16);}
    if(!geometry(c,o,geometry_raw,m,out))FAIL("native model geometry streams or part fields");
    if(o->bytes>64U*1024U*1024U-sizeof(*o))FAIL("native model allocation budget");
    out->owner=o;out->handle=handle;out->geometry_count=m->geometries.count;out->root_bytes=sizeof(*m);out->bytes_owned=o->bytes+sizeof(*o);return 1;
fail:mac_native_model_owner_free(o);memset(out,0,sizeof(*out));return 0;
#undef FAIL
}
int mac_native_model_finalize(struct mac_native_model_tag*t,const struct mac_native_model_buffers*b,const char**error)
{
    struct model_owner*o;struct stream*s;size_t resources=0,bytes;if(error)*error=NULL;
    if(!t||!(o=t->owner)||o->finalized||!b||!b->create||!b->destroy||!b->bytes_owned){if(error)*error="native model finalization arguments";return 0;}
    o->buffers=*b;
    for(s=o->streams;s;s=s->next){
        if(!b->create(b->context,&s->part->vertex_buffer,&s->part->triangle_buffer,s->vertices,s->vertices_count,s->indices,s->triangles_count)||
            !s->part->vertex_buffer.hardware_format||!s->part->triangle_buffer.hardware_format||
            s->part->vertex_buffer.type!=_rasterizer_vertex_type_model_compressed||s->part->vertex_buffer.count!=(long)s->vertices_count||
            s->part->triangle_buffer.type!=_triangle_buffer_type_precompiled_strip||s->part->triangle_buffer.count!=(long)s->triangles_count ||
            !(bytes=b->bytes_owned(b->context,&s->part->vertex_buffer,&s->part->triangle_buffer)) ||
            bytes>64U*1024U*1024U-t->bytes_owned-resources){
            struct stream*cleanup;for(cleanup=o->streams;cleanup;cleanup=cleanup->next){
                if(cleanup->part->vertex_buffer.hardware_format||cleanup->part->triangle_buffer.hardware_format)b->destroy(b->context,&cleanup->part->vertex_buffer,&cleanup->part->triangle_buffer);
                memset(&cleanup->part->vertex_buffer,0,sizeof(cleanup->part->vertex_buffer));memset(&cleanup->part->triangle_buffer,0,sizeof(cleanup->part->triangle_buffer));}
            memset(&o->buffers,0,sizeof(o->buffers));if(error)*error="native model hardware buffer creation";return 0;
        }
        resources+=bytes;
    }
    o->finalized=1;t->bytes_owned+=resources;t->root=o->model;return 1;
}

int mac_native_model_visit_streams(const struct mac_native_model_tag*t,mac_native_model_stream_visitor visit,void*ctx)
{struct model_owner*o;struct stream*s;if(!t||!(o=t->owner)||!visit)return 0;for(s=o->streams;s;s=s->next)if(!visit(ctx,s->vertices,s->vertices_count,s->indices,s->triangles_count))return 0;return 1;}

int mac_native_model_visit_shaders(const struct model *model,
    mac_native_model_shader_visitor visitor,void *context)
{
    const struct model_shader_reference *references;
    long i;
    if(!model || !visitor || model->shaders.count<0 || model->shaders.count>32 ||
       (model->shaders.count && !model->shaders.address))return 0;
    references=model->shaders.address;
    for(i=0;i<model->shaders.count;++i)
        if(!visitor(context,(uint32_t)references[i].shader.group_tag,
            (uint32_t)references[i].shader.index))return 0;
    return 1;
}
