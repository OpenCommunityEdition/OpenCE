#include "cseries.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_model_types.h"
#include "native_model_tags.h"
#include "native_model_buffers.h"
#include "engine_tags.h"
#include "retail_object_spawn.h"
#undef malloc
#undef free
#undef memcmp
#undef memcpy
#undef memset
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct probe_part {unsigned long flags;short shader;char previous,next;short primary,secondary;float weights[2];real_point3d centroid;struct tag_block uncompressed,compressed,triangles;struct triangle_buffer triangle;struct vertex_buffer vertex;};
struct probe_geometry {byte reserved[36];struct tag_block parts;};
struct resource_check {struct probe_part *parts[8192];uint32_t count;};
#include "mac_resource_release_probe.h"
static unsigned checks,failures;static uint32_t digest=2166136261U;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
#define NEAR(a,b) (((a)>(b)?(a)-(b):(b)-(a))<0.0001f)
static void number(uint32_t n){digest=(digest^n)*16777619U;}
static int streams(void*ctx,const struct model_vertex_uncompressed*v,uint32_t count,const uint16_t*indices,uint32_t triangles)
{
    uint32_t i,j,w;unsigned*parts=ctx;++*parts;number(count);number(triangles);
    for(i=0;i<count;++i){const unsigned char*p=(const unsigned char*)(v+i);for(j=0;j<14;++j){memcpy(&w,p+j*4,4);CHECK((w&0x7fffffffU)<0x7f800000U);number(w);}for(j=0;j<2;++j){number((uint16_t)v[i].nodes[j]);memcpy(&w,&v[i].node_weights[j],4);number(w);}}
    for(i=0;i<triangles+2;++i){CHECK(indices[i]<count);number(indices[i]);}return 1;
}
static int resources(void*context,const struct model_vertex_uncompressed*v,uint32_t n,const uint16_t*strip,uint32_t triangles)
{
    struct resource_check*c=context;struct probe_part*p;D3DVertexBuffer*vr;D3DIndexBuffer*ir;struct model_vertex_compressed*expected;uint32_t i;
    CHECK(c->count>0);if(!c->count)return 0;p=c->parts[--c->count];vr=p->vertex.hardware_format;ir=p->triangle.hardware_format;CHECK(vr&&ir);if(!vr||!ir)return 0;
    CHECK(p->vertex.type==5&&p->vertex.count==(long)n&&p->vertex.offset==0);CHECK(p->triangle.type==1&&p->triangle.count==(long)triangles);
    CHECK((vr->Common&D3DCOMMON_TYPE_MASK)==D3DCOMMON_TYPE_VERTEXBUFFER);CHECK((ir->Common&D3DCOMMON_TYPE_MASK)==D3DCOMMON_TYPE_INDEXBUFFER);
    CHECK(!(vr->Common&D3DCOMMON_D3DCREATED));CHECK(!(ir->Common&D3DCOMMON_D3DCREATED));
    CHECK(mac_shared_resource_release((D3DResource*)vr)==0);CHECK(mac_shared_resource_release((D3DResource*)ir)==0);
    CHECK((void*)(vr->Data|0x80000000UL)==p->vertex.base_address);CHECK((void*)ir->Data==p->triangle.base_address);
    expected=malloc(n*sizeof(*expected));if(!expected)return 0;rasterizer_geometry_compress_vertices(4,n,expected,n*sizeof(*expected),(void*)v,n*sizeof(*v));
    CHECK(!memcmp(expected,p->vertex.base_address,n*sizeof(*expected)));CHECK(!memcmp(strip,p->triangle.base_address,(triangles+2)*2));
    for(i=0;i<n;++i){
        /* The actual retail model streams exercise the source 32-byte
           compressed layout. Check the fields consumed by the indexed GPU
           adapter against the engine's decompressed model vertices. */
        CHECK(expected[i].position.x==v[i].position.x&&expected[i].position.y==v[i].position.y&&expected[i].position.z==v[i].position.z);
        CHECK(NEAR(v[i].texcoord.x,((real)expected[i].texcoord.x*2.0f+1.0f)*(1.0f/65535.0f)));
        CHECK(NEAR(v[i].texcoord.y,((real)expected[i].texcoord.y*2.0f+1.0f)*(1.0f/65535.0f)));
        CHECK(expected[i].nodes[0]%3==0&&expected[i].nodes[1]%3==0);
        CHECK(v[i].nodes[0]==expected[i].nodes[0]/3&&v[i].nodes[1]==expected[i].nodes[1]/3);
        number(expected[i].normal);number(expected[i].binormal);number(expected[i].tangent);number(expected[i].nodes[0]);number(expected[i].nodes[1]);number((uint16_t)expected[i].node_weight);
    }
    free(expected);return 1;
}
static int denied(void*ctx,struct vertex_buffer*v,struct triangle_buffer*t,const struct model_vertex_uncompressed*p,uint32_t n,const uint16_t*i,uint32_t k)
{(void)v;(void)t;(void)p;(void)n;(void)i;(void)k;++*(unsigned*)ctx;return 0;}
static size_t no_bytes(void*ctx,const struct vertex_buffer*v,const struct triangle_buffer*t){(void)ctx;(void)v;(void)t;return 0;}
static void destroy(void*ctx,struct vertex_buffer*v,struct triangle_buffer*t){(void)ctx;(void)v;(void)t;CHECK(0);}
static uint32_t read32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void reject_word(struct mac_retail_cache*c,uint32_t h,unsigned char*p,uint32_t value)
{unsigned char saved[4];struct mac_native_model_tag bad;const char*error;memcpy(saved,p,4);p[0]=value;p[1]=value>>8;p[2]=value>>16;p[3]=value>>24;CHECK(!mac_native_model_decode(c,h,&bad,&error));CHECK(!bad.owner&&!bad.root);memcpy(p,saved,4);mac_native_model_tag_free(&bad);}
int main(int argc,char**argv)
{
    FILE*f;long length;unsigned char*bytes;struct mac_retail_cache c;struct mac_retail_object_spawn spawn;struct mac_native_model_tag models[3],bad;struct mac_native_model_buffers callbacks;const char*error=NULL;uint32_t handles[3],i;unsigned parts,calls=0;const struct model*m;struct resource_check resource_check;struct mac_native_tags registry;
    if(argc!=2){fprintf(stderr,"usage: %s bloodgulch.map\n",argv[0]);return 2;}f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);length=ftell(f);rewind(f);if(length<=0){fclose(f);return 2;}bytes=malloc((size_t)length);if(!bytes||fread(bytes,1,(size_t)length,f)!=(size_t)length){fclose(f);free(bytes);return 2;}fclose(f);
    if(!mac_retail_open(bytes,length,&c,&error)||!mac_retail_object_spawn_decode(&c,&spawn,&error)){fprintf(stderr,"map: %s\n",error);free(bytes);return 2;}
    if(!mac_native_tags_create(&c,&registry,&error))return 2;CHECK(mac_engine_tags_bind(&registry));
    handles[0]=spawn.biped.refs[0].handle;handles[1]=spawn.weapons[0].object.refs[0].handle;handles[2]=spawn.weapons[1].object.refs[0].handle;
    memset(models,0,sizeof(models));
    for(i=0;i<3;++i){if(!mac_native_model_decode(&c,handles[i],models+i,&error)){fprintf(stderr,"model %08lx: %s\n",(unsigned long)handles[i],error);return 1;}CHECK(!models[i].root);m=mac_native_model_staging(models+i);CHECK(m&&m->nodes.count>0);parts=0;CHECK(mac_native_model_visit_streams(models+i,streams,&parts));CHECK(parts==models[i].part_count);printf("native model CPU %08lx nodes=%ld regions=%ld markers=%ld geometries=%lu parts=%lu vertices=%lu indices=%lu bytes=%lu\n",(unsigned long)handles[i],m->nodes.count,m->regions.count,m->markers.count,(unsigned long)models[i].geometry_count,(unsigned long)models[i].part_count,(unsigned long)models[i].vertex_count,(unsigned long)models[i].index_count,(unsigned long)models[i].bytes_owned);}
    CHECK(!mac_native_model_decode(&c,handles[0]^0x10000,&bad,&error));CHECK(!bad.owner&&!bad.root);
    CHECK(!mac_native_model_decode(&c,spawn.biped.handle,&bad,&error));CHECK(!bad.owner&&!bad.root);
    {struct mac_retail_tag tag;unsigned char*r,*g,*p,*v,*idx;uint32_t offset;
    CHECK(mac_retail_tag_get(&c,handles[0]&65535,&tag));r=(unsigned char*)mac_retail_tag_span(&c,tag.address,232);
    reject_word(&c,handles[0],r+172,257);reject_word(&c,handles[0],r+8,0x7fc00000);reject_word(&c,handles[0],r+212,0xfffffff0);
    g=(unsigned char*)mac_retail_tag_span(&c,read32(r+212),48);p=(unsigned char*)mac_retail_tag_span(&c,read32(g+40),132);
    reject_word(&c,handles[0],p+88,0xffffffff);reject_word(&c,handles[0],p+72,0xffffffff);reject_word(&c,handles[0],p+32,1);
    offset=read32(p+100);v=bytes+c.model_offset+offset;reject_word(&c,handles[0],v,0x7f800000);
    idx=bytes+c.model_offset+c.model_indices_offset+read32(p+76);reject_word(&c,handles[0],idx,0xffffffff);}
    memset(&callbacks,0,sizeof(callbacks));callbacks.context=&calls;callbacks.create=denied;callbacks.destroy=destroy;callbacks.bytes_owned=no_bytes;
    CHECK(!mac_native_model_finalize(models,&callbacks,&error));CHECK(calls==1&&!models[0].root);
    mac_native_model_buffer_callbacks(&callbacks);
    for(i=0;i<3;++i){size_t before=models[i].bytes_owned;CHECK(mac_native_model_finalize(models+i,&callbacks,&error));if(!models[i].root){fprintf(stderr,"finalize: %s\n",error);return 1;}CHECK(models[i].bytes_owned>before);
        {uint32_t g,j;struct probe_geometry*geometries=models[i].root->geometries.address;resource_check.count=0;
        for(g=0;g<(uint32_t)models[i].root->geometries.count;++g)for(j=0;j<(uint32_t)geometries[g].parts.count;++j)resource_check.parts[resource_check.count++]=(struct probe_part*)geometries[g].parts.address+j;
        CHECK(mac_native_model_visit_streams(models+i,resources,&resource_check));CHECK(!resource_check.count);}
    }
    free(bytes);for(i=0;i<3;++i){parts=0;CHECK(mac_native_model_visit_streams(models+i,streams,&parts));CHECK(mac_native_tags_publish_model(&registry,models[i].handle,models[i].root,models[i].root_bytes,models[i].bytes_owned,models[i].owner,mac_native_model_owner_free,&error));
        CHECK(model_definition_get((long)models[i].handle)==models[i].root);CHECK(!mac_native_tags_get(&registry,0x6d6f6432,models[i].handle,1,&error));
        models[i].owner=NULL;models[i].root=NULL;}
    mac_engine_tags_unbind();mac_native_tags_free(&registry);
    printf("native model complete publication: %u checks %u failures digest=%08lx; genuine CPU resource finalization exercised, GPU drawing not tested\n",checks,failures,(unsigned long)digest);return failures?1:0;
}
