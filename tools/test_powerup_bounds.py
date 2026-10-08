"""Test production powerup bounds and frustum culling, without a game/profile.

--map additionally measures both actual Xbox powerup meshes. Assets stay local.
"""
import argparse
import re
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
ROOT=Path(__file__).resolve().parent.parent

def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

PRELUDE=r'''
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>
typedef unsigned char byte,boolean;
typedef unsigned short word;
typedef float real;
typedef union {struct {real x,y,z;};struct {real i,j,k;};real n[3];} real_point3d,real_vector3d;
typedef struct {real i,j;} real_vector2d;
typedef struct {real red,green,blue;} real_rgb_color;
typedef struct {real i,j,k,w;} real_quaternion;
typedef struct real_matrix4x3 {real scale;real_vector3d forward,left,up;real_point3d position;} real_matrix4x3;
typedef struct {real_vector3d n;real d;} real_plane3d;
typedef struct {real x0,x1,y0,y1,z0,z1;} real_rectangle3d;
struct render_frustum {real_rectangle3d world_bounds;real_plane3d world_planes[6];};
struct tag_block {long count;void *address,*definition;};
struct tag_reference {unsigned long group_tag;char *name;long name_length,index;};
struct tag_iterator {long absolute_index;};
#define TRUE 1
#define FALSE 0
#define NONE -1
#define FLAG(b) (1u<<(b))
#define TEST_FLAG(f,b) (((f)&FLAG(b))!=0)
#define MAX(a,b) ((a)>(b)?(a):(b))
#define TAG_STRING_LENGTH 31
#define UNSIGNED_SHORT_MAX 65535
#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t*)((byte*)(b)->address+(i)*sizeof(t)))
#define D3DLOCK_READONLY 128
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}}while(0)
#define match_assert(file,line,c) CHECK(c)
#define OBJECTS_H_FILE "test"
struct test_buffer {byte *data;};
static void IDirect3DVertexBuffer8_Lock(void *b,int offset,int size,byte **data,int flags){*data=b&&((struct test_buffer*)b)->data?((struct test_buffer*)b)->data+offset:NULL;}
static void IDirect3DVertexBuffer8_Unlock(void *b){}
static long rasterizer_geometry_get_vertex_size(short type){return type==5?32:68;}
'''
HARNESS=r'''
static struct model models[8];
static struct equipment_definition equipment[8];
static long equipment_count;
#define equipment_definition_get(i) (equipment+(i))
#define object_definition_get(i) ((struct object_definition*)(equipment+(i)))
#define model_definition_get(i) (models+(i))
static void tag_iterator_new(struct tag_iterator *i,long group){i->absolute_index=0;}
static long tag_iterator_next(struct tag_iterator *i){return i->absolute_index<equipment_count?i->absolute_index++:NONE;}
struct object_datum {long definition_index;struct {real_point3d bounding_sphere_center;real scale;} object;};
static struct object_datum object;
static struct object_datum *object_get(long i){return &object;}
'''
TESTS=r'''
static struct model_node node;
static struct model_geometry geometries[2];
static struct model_geometry_part parts[2];
static float vertices[2][3][17];
static struct test_buffer buffers[2];
static void initialize(void){
 memset(equipment,0,sizeof(equipment));memset(models,0,sizeof(models));memset(&node,0,sizeof(node));
 memset(geometries,0,sizeof(geometries));memset(parts,0,sizeof(parts));memset(vertices,0,sizeof(vertices));
 node.runtime_default_inverse_matrix=(real_matrix4x3){.scale=1,.forward={.x=1},.left={.y=1},.up={.z=1},.position={.z=-.1f}};
 for(int i=0;i<2;i++){
  vertices[i][0][0]=.15f+i*.05f;vertices[i][0][2]=.15f;vertices[i][1][1]=-.15f;vertices[i][2][2]=.3f+i*.1f;
  buffers[i].data=(byte*)vertices[i];parts[i].vertex_buffer=(struct vertex_buffer){4,0,3,0,NULL,buffers+i};
  geometries[i].parts=(struct tag_block){1,parts+i,NULL};
 }
 models[0].nodes=(struct tag_block){1,&node,NULL};models[0].geometries=(struct tag_block){2,geometries,NULL};
 equipment_count=3;
 for(int i=0;i<3;i++){
  equipment[i].object.bounding_radius=.1f;equipment[i].object.render_bounding_radius=i==2?10:.1f;
  equipment[i].object.animation_graph.index=NONE;equipment[i].object.model.index=0;
  equipment[i].equipment.powerup_type=i==1?_equipment_powerup_health:_equipment_powerup_overshield;
 }
}
static void tests(void){
 initialize();struct equipment_definition before[3];memcpy(before,equipment,sizeof(before));
 models_fix_powerup_render_bounds();CHECK(equipment[0].object.render_bounding_radius>.3f);
 before[0].object.render_bounding_radius=equipment[0].object.render_bounding_radius;CHECK(!memcmp(before,equipment,sizeof(before)));
 real grown=equipment[0].object.render_bounding_radius;models_fix_powerup_render_bounds();CHECK(grown==equipment[0].object.render_bounding_radius);
 /* Atomic fallback: a valid first LOD must not mask a bad later LOD. */
 for(int c=0;c<14;c++){
  initialize();switch(c){case 0:models[0].nodes.count=2;break;case 1:parts[1].vertex_buffer.type=0;break;
  case 2:parts[1].vertex_buffer.hardware_format=NULL;break;case 3:parts[1].vertex_buffer.count=0;break;
  case 4:parts[1].flags=2;break;case 5:vertices[1][0][0]=NAN;break;case 6:models[0].geometries.address=NULL;break;
  case 7:equipment[0].object.model.index=NONE;equipment[2].object.model.index=NONE;break;
  case 8:equipment[0].object.animation_graph.index=5;equipment[2].object.animation_graph.index=5;break;
  case 9:parts[1].vertex_buffer.offset=1;break;case 10:buffers[1].data=NULL;break;case 11:models[0].nodes.address=NULL;break;case 12:node.runtime_default_inverse_matrix.scale=NAN;break;case 13:models[0].geometries.count=257;break;}
  memcpy(before,equipment,sizeof(before));models_fix_powerup_render_bounds();CHECK(!memcmp(before,equipment,sizeof(before)));
 }
 initialize();equipment[0].object.bounding_offset=(real_point3d){.x=.3f,.z=.1f};
 real radius;CHECK(model_rigid_render_radius(models,&equipment[0].object.bounding_offset,&radius));CHECK(radius>.39f);
 parts[1].flags=1;CHECK(model_rigid_render_radius(models,&equipment[0].object.bounding_offset,&radius));
 /* compressed and uncompressed vertices have identical positions */
 float packed[3][8]={0};for(int i=0;i<3;i++)memcpy(packed[i],vertices[0][i],12);
 parts[0].vertex_buffer.type=5;buffers[0].data=(byte*)packed;
 CHECK(model_rigid_render_radius(models,&equipment[0].object.bounding_offset,&radius));
 object.definition_index=0;object.object.bounding_sphere_center=(real_point3d){.x=3,.y=4,.z=5};
 float scales[]={0,.1f,.5f,1,2,4};
 for(int i=0;i<6;i++){real_point3d center;object.object.scale=scales[i];object_get_render_bounding_sphere(0,&center,&radius);
 CHECK(!memcmp(&center,&object.object.bounding_sphere_center,sizeof(center)));
 CHECK(radius==equipment[0].object.render_bounding_radius*(scales[i]>1?scales[i]:1));}
 puts("PASS: all LODs, root transform/offset, both vertex formats, conservative/idempotent growth, 14 atomic fallback paths, scaled culling; only render radius changes");
}
static void frustum(struct render_frustum *f,float fov,float aspect){
 float tv=tanf(fov*.5f),th=tv*aspect,h=1/sqrtf(1+th*th),v=1/sqrtf(1+tv*tv);
 f->world_bounds=(real_rectangle3d){-100*th,100*th,-100*tv,100*tv,-100,0};
 f->world_planes[0]=(real_plane3d){{.x=-h,.z=th*h},0};f->world_planes[1]=(real_plane3d){{.x=h,.z=th*h},0};
 f->world_planes[2]=(real_plane3d){{.y=-v,.z=tv*v},0};f->world_planes[3]=(real_plane3d){{.y=v,.z=tv*v},0};
 f->world_planes[4]=(real_plane3d){{.z=1},-.05f};f->world_planes[5]=(real_plane3d){{.z=-1},100};
}
static void sweeps(struct model *model,struct equipment_definition *definition){
 real local_radius=definition->object.render_bounding_radius;int checks=0,legacy_failures=0,culled=0;
 for(int size=0;size<4;size++)for(int fov=0;fov<3;fov++)for(int viewport=0;viewport<4;viewport++)for(int rotation=0;rotation<4;rotation++)for(int edge=0;edge<4;edge++)for(int step=-20;step<=20;step++){
  float scales[]={.25f,1,2,4},fovs[]={.6f,1.2f,1.8f},aspects[]={16.f/9,4.f/3,32.f/9,8.f/9};
  float scale=scales[size],depth=3,angle=rotation*.6f,tv=tanf(fovs[fov]*.5f),th=tv*aspects[viewport];
  real_matrix4x3 world={.scale=scale,.forward={.x=cosf(angle),.z=sinf(angle)},.left={.y=1},.up={.x=-sinf(angle),.z=cosf(angle)},.position={.z=-depth}};
  if(edge<2)world.position.x=(edge?1:-1)*(depth*th+step*.1f*local_radius*scale*sqrtf(1+th*th));
  else world.position.y=(edge==3?1:-1)*(depth*tv+step*.1f*local_radius*scale*sqrtf(1+tv*tv));
  struct render_frustum f;frustum(&f,fovs[fov],aspects[viewport]);
  real_point3d center;matrix4x3_transform_point(&world,&definition->object.bounding_offset,&center);
  object.definition_index=0;equipment[0]=*definition;object.object.scale=scale;object.object.bounding_sphere_center=center;
  real radius;real_point3d queried;object_get_render_bounding_sphere(0,&queried,&radius);
  int visible=render_frustum_sphere_visible(&f,&queried,radius),has_vertex=0;
  for(int g=0;g<model->geometries.count;g++){
   struct model_geometry *geo=TAG_BLOCK_GET_ELEMENT(&model->geometries,g,struct model_geometry);
   for(int p=0;p<geo->parts.count;p++){
    struct vertex_buffer *buffer=&TAG_BLOCK_GET_ELEMENT(&geo->parts,p,struct model_geometry_part)->vertex_buffer;
    struct test_buffer *b=buffer->hardware_format;
    for(int i=0;i<buffer->count;i++){
     real_point3d root,point;matrix4x3_transform_point(&((struct model_node*)model->nodes.address)->runtime_default_inverse_matrix,(real_point3d*)(b->data+i*rasterizer_geometry_get_vertex_size(buffer->type)),&root);
     matrix4x3_transform_point(&world,&root,&point);CHECK(distance_squared3d(&point,&center)<=radius*radius*1.00001f);
     int inside=1;for(int plane=0;plane<6;plane++)inside&=plane3d_distance_to_point(&f.world_planes[plane],&point)<-.000001f;
     has_vertex|=inside;
    }
   }
  }
  if(has_vertex){CHECK(visible);legacy_failures+=!render_frustum_sphere_visible(&f,&center,.1f);}
  culled+=!visible;checks++;
 }
 CHECK(legacy_failures>0&&culled>0);printf("PASS: %d edge sweeps; %d premature legacy culls; mesh containment, four viewport ratios, rotations, FOVs and scales; fully offscreen culling retained\n",checks,legacy_failures);
}
static void asset_test(const char *path){
 FILE *f=fopen(path,"rb");CHECK(f);unsigned int count;CHECK(fread(&count,4,1,f)==1&&count==2);
 for(int entry=0;entry<count;entry++){
  struct equipment_definition definition;CHECK(sizeof(definition)==800);CHECK(fread(&definition,sizeof(definition),1,f)==1);
  struct model_node root={0};CHECK(fread(&root.runtime_default_inverse_matrix,52,1,f)==1);
  struct model model={0};model.nodes=(struct tag_block){1,&root,NULL};unsigned int n;CHECK(fread(&n,4,1,f)==1);
  struct model_geometry geometry={0};struct model_geometry_part *parts=calloc(n,sizeof(*parts));struct test_buffer *buffers=calloc(n,sizeof(*buffers));
  geometry.parts=(struct tag_block){n,parts,NULL};model.geometries=(struct tag_block){1,&geometry,NULL};
  for(int i=0;i<n;i++){
   unsigned int h[3];CHECK(fread(h,4,3,f)==3);parts[i].flags=h[0];parts[i].vertex_buffer=(struct vertex_buffer){h[1],0,h[2],0,NULL,buffers+i};
   int stride=rasterizer_geometry_get_vertex_size(h[1]);buffers[i].data=malloc(stride*h[2]);CHECK(fread(buffers[i].data,stride,h[2],f)==h[2]);
  }
  real radius;CHECK(model_rigid_render_radius(&model,&definition.object.bounding_offset,&radius));
  equipment_count=1;equipment[0]=definition;models[0]=model;equipment[0].object.model.index=0;
  struct equipment_definition before=equipment[0];models_fix_powerup_render_bounds();CHECK(equipment[0].object.render_bounding_radius>.1f);
  before.object.render_bounding_radius=equipment[0].object.render_bounding_radius;CHECK(!memcmp(&before,equipment,sizeof(before)));
  definition.object.render_bounding_radius=equipment[0].object.render_bounding_radius;
  printf("Actual powerup type %d: radius %.6f -> %.6f\n",definition.equipment.powerup_type,.1f,radius);sweeps(&model,&definition);
  for(int i=0;i<n;i++)free(buffers[i].data);free(parts);free(buffers);
 }
 fclose(f);
}
int main(int argc,char **argv){tests();initialize();models_fix_powerup_render_bounds();struct equipment_definition def=equipment[0];sweeps(models,&def);if(argc>1)asset_test(argv[1]);return 0;}
'''

def fixture(path):
    raw=path.read_bytes();data=raw if struct.unpack_from('<I',raw,8)[0]==len(raw) else raw[:2048]+zlib.decompressobj().decompress(raw[2048:])
    u=lambda p:struct.unpack_from('<I',data,p)[0]
    offset=u(16);base=u(offset)-36;off=lambda p:p-base+offset
    tags={};powerups=[]
    for i in range(u(offset+12)):
        o=offset+36+32*i;tags[u(o+12)]=off(u(o+20))
        if data[o:o+4]==b'piqe' and struct.unpack_from('<h',data,off(u(o+20))+776)[0] in (2,3):powerups.append(off(u(o+20)))
    assert len(powerups)==2
    result=struct.pack('<I',len(powerups))
    for o in powerups:
        model=tags[u(o+52)];assert u(model+184)==1
        node=off(u(model+188));result+=data[o:o+800]+data[node+104:node+156]
        parts=[]
        for g in range(u(model+208)):
            geo=off(u(model+212))+48*g
            parts.extend(off(u(geo+40))+104*p for p in range(u(geo+36)))
        result+=struct.pack('<I',len(parts))
        for p in parts:
            typ=struct.unpack_from('<h',data,p+84)[0];assert typ in (4,5)
            n=u(p+88);start=off(u(off(u(p+100))+4));stride=32 if typ==5 else 68
            result+=struct.pack('<III',u(p),typ,n)+data[start:start+stride*n]
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cc',default='clang');parser.add_argument('--map',type=Path);args=parser.parse_args()
    read=lambda p:(ROOT/p).read_text()
    model_source=read('source/models/models.c');mh=read('source/models/model_definitions.h');rh=read('source/rasterizer/rasterizer_geometry.h')
    harness=re.sub(r'(?<!\.)\bequipment\b','test_equipment',HARNESS)
    tests=re.sub(r'(?<!\.)\bequipment\b','test_equipment',TESTS)
    unit=PRELUDE+block(mh,'enum\n{\n\tMODELS_GROUP_TAG')+';\n'+block(model_source,'enum\n{\n\t_model_geometry_part_stripped_bit')+';\n'+block(rh,'enum')+';\n'
    for src,names in [(rh,['vertex_buffer','triangle_buffer']), (mh,['model_node','model']),
                      (model_source,['model_geometry','model_geometry_part']),
                      (read('source/objects/object_definitions.h'),['_object_definition','object_definition']),
                      (read('source/items/item_definitions.h'),['_item_definition']),
                      (read('source/items/equipment_definitions.h'),['_equipment_definition','equipment_definition'])]:
        for name in names:unit+=block(src,'struct '+name+'\n')+';\n'
    unit+=block(read('source/items/equipment_definitions.h'),'enum equipment_powerup_type')+';\n#define EQUIPMENT_DEFINITION_TAG 0\n'+harness
    math=read('source/math/real_math.h')
    for name in ['vector_from_points3d','magnitude_squared3d','distance_squared3d','dot_product3d','plane3d_distance_to_point']:
        marker=('__inline real_vector3d *' if name=='vector_from_points3d' else '__inline real ')+name+'('
        unit+=block(math,marker).replace('__inline','static inline')+'\n'
    unit+=block(read('source/math/matrix_math.c'),'real_point3d *matrix4x3_transform_point(')+'\n'
    unit+=block(model_source,'static boolean model_rigid_render_radius(')+'\n'+block(model_source,'void models_fix_powerup_render_bounds(')+'\n'
    unit+=block(read('source/objects/objects.h'),'__inline void object_get_render_bounding_sphere(').replace('__inline','static')+'\n'
    unit+=block(read('source/render/render_cameras.c'),'short render_frustum_sphere_visible(')+'\n'+tests
    compiler=[args.cc,'-std=gnu11','-O2','-fuse-ld=lld']+(['--target=i686-pc-windows-msvc'] if sys.platform=='win32' else ['-m32','-lm'])
    with tempfile.TemporaryDirectory(prefix='halo-powerup-bounds-') as directory:
        path=Path(directory);(path/'test.c').write_text(unit);command=[str(path/'test.exe')]
        if args.map:(path/'asset.bin').write_bytes(fixture(args.map));command.append(str(path/'asset.bin'))
        subprocess.run([*compiler,str(path/'test.c'),'-o',str(path/'test.exe')],check=True)
        result=subprocess.run(command,check=True,timeout=45,capture_output=True,text=True)
        print(result.stdout,end='')
        assert 'PASS: all LODs' in result.stdout and 'edge sweeps' in result.stdout, result.stderr

if __name__=='__main__':main()
