#include "harness.h"
#include <limits.h>
#include <math.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAXIMUM_OBJECTS_PER_MAP 1024
#define MAXIMUM_CLUSTERS_PER_STRUCTURE 4
#define MAXIMUM_BREAKABLE_SURFACES_PER_MAP 256
#define REAL_MAX 3.402823466e38F
enum { PLASMA_COLLISION_RAYS=1536, PLASMA_COLLISION_WORDS=48 };
#define MAXIMUM_BSP3D_TRAVERSAL_DEPTH 128
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
#define _object_invisible_bit 0
enum { _collision_test_structure_bit, _collision_test_objects_bit,
 _collision_test_front_facing_surfaces_bit, _collision_test_back_facing_surfaces_bit,
 _collision_test_objects_first_type_bit, _collision_test_objects_scenery_bit=_collision_test_objects_first_type_bit,
 _collision_test_objects_machines_bit, _collision_test_objects_vehicles_bit };
struct collision_model_instance { long object_index; };
struct collision_bsp_test_vector_result { real t; int leaf_count; long leaf_indices[256]; };
struct collision_model_test_vector_result { struct collision_bsp_test_vector_result bsp_result; };
struct collision_result { real t; };
struct structure_leaf { short cluster_index; };
struct structure_bsp { struct { long count; void *address; } leaves; };
typedef struct { real_vector3d n; real d; } real_plane3d;
struct bsp3d_node { long plane_designator, children[2]; };
struct collision_bsp { struct { struct { long count; void *address; } nodes, planes; } bsp3d; };
struct collision_bsp_vector_cache;
static struct collision_bsp bsp;
static struct bsp3d_node nodes[4];
static real_plane3d planes[4];
static long queried_node;
static short queried_depth;
static real plane3d_distance_to_point(const real_plane3d *p,const real_point3d *v)
{ return p->n.i*v->x+p->n.j*v->y+p->n.k*v->z-p->d; }
struct object_datum { struct { long next_object_index, first_child_object_index; real_point3d bounding_sphere_center;
 real bounding_sphere_radius; unsigned long flags; int type; } object; };
static struct object_datum objects[1024];
static struct structure_leaf leaves[2];
static struct structure_bsp structure;
static int roots[1024], root_count, stock_calls, reads, model_builds, cluster_reads;
static float object_depth[1024], structure_depth;
static int bad_leaf, debug_collision_skip_objects, debug_collision_skip_vectors;
static struct object_datum *object_get(long index) { reads++; return &objects[index]; }
static struct structure_bsp *global_structure_bsp_get(void) { return &structure; }
static struct collision_bsp *global_collision_bsp_get(void) { return &bsp; }
static void *breakable_surface_flags_get(void) { return NULL; }
static int collision_bsp_test_vector_from_node(unsigned long flags,const struct collision_bsp *bsp,int breaks,void *bits,
 const real_point3d *p,const real_vector3d *v,float limit,struct collision_bsp_test_vector_result *r,long start,short depth,struct collision_bsp_vector_cache *cache)
{ queried_node=start;queried_depth=depth;r->t=structure_depth;r->leaf_count=2;r->leaf_indices[0]=bad_leaf?9:0;r->leaf_indices[1]=1;return structure_depth<1; }
static long cluster_get_first_collideable_object(long *reference,short cluster)
{ cluster_reads++;*reference=0;return root_count?roots[0]:NONE; }
static long cluster_get_next_collideable_object(long *reference)
{ return ++*reference<root_count?roots[*reference]:NONE; }
static int collision_model_instance_new(struct collision_model_instance *m,long index)
{ model_builds++;m->object_index=index;return object_depth[index]>=0; }
static int collision_model_test_vector(const struct collision_model_instance *m,unsigned long flags,
 const real_point3d *p,const real_vector3d *v,struct collision_model_test_vector_result *r)
{ r->bsp_result.t=object_depth[m->object_index];return r->bsp_result.t<1; }
static int collision_test_vector(unsigned long flags,const real_point3d *p,const real_vector3d *v,long ignore,struct collision_result *r)
{ stock_calls++;r->t=.123f;return 1; }
static void plasma_collision_model_depths(const struct collision_model_instance *m,const real_point3d *p,
 const real_vector3d *v,int count,const unsigned int *mask,float *depths)
{ int i;for(i=0;i<count;i++)if(mask[i/32]&(1U<<(i%32))) { float d=object_depth[m->object_index];if(d>=0&&d<1&&d<depths[i])depths[i]=d; } }
void plasma_collision_end(void);
#include "under_test.inc"
static void setup(void)
{
 int i;memset(objects,0,sizeof(objects));memset(object_depth,0,sizeof(object_depth));
 for(i=0;i<1024;i++){objects[i].object.next_object_index=NONE;objects[i].object.first_child_object_index=NONE;
 objects[i].object.bounding_sphere_radius=1;objects[i].object.type=0;object_depth[i]=.8f;roots[i]=i;}
 root_count=3;leaves[0].cluster_index=0;leaves[1].cluster_index=1;structure.leaves.count=2;structure.leaves.address=leaves;
 reads=model_builds=cluster_reads=stock_calls=bad_leaf=debug_collision_skip_vectors=debug_collision_skip_objects=0;structure_depth=1;
 memset(&bsp,0,sizeof(bsp));memset(nodes,0,sizeof(nodes));memset(planes,0,sizeof(planes));
 bsp.bsp3d.nodes.count=bsp.bsp3d.planes.count=4;bsp.bsp3d.nodes.address=nodes;bsp.bsp3d.planes.address=planes;
 for(i=0;i<4;i++){nodes[i].plane_designator=i;nodes[i].children[0]=nodes[i].children[1]=NONE;planes[i].n.i=1;}
}
int main(int argc,char **argv)
{
 real_point3d p={0,0,0};real_vector3d v={1,0,0};int i;
 unsigned long flags=FLAG(_collision_test_structure_bit)|FLAG(_collision_test_objects_bit)|FLAG(_collision_test_front_facing_surfaces_bit)|FLAG(_collision_test_back_facing_surfaces_bit)|FLAG(_collision_test_objects_scenery_bit)|FLAG(_collision_test_objects_machines_bit);
 setup();object_depth[0]=.4f;objects[1].object.flags=1;object_depth[1]=.1f;objects[2].object.type=2;object_depth[2]=.2f;
 plasma_collision_begin(flags,&p,NONE);
 for(i=0;i<32;i++)CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.4f,"same closest eligible blocker");
 CHECK(reads==3 && model_builds==1 && cluster_reads==2,"one metadata/model read per batch and cluster");
 CHECK(!stock_calls,"ordinary case uses batch");plasma_collision_end();
 CHECK(!batch.active && !batch.point && !batch.structure && !batch.objects[0].model_state,"borrowed state cleared");
 setup();root_count=1;object_depth[0]=-1;objects[0].object.first_child_object_index=1;object_depth[1]=.25f;
 plasma_collision_begin(flags,&p,NONE);
 for(i=0;i<32;i++)CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.25f,"missing parent model still visits child");
 CHECK(model_builds==2,"failed and successful model setup cached within batch");plasma_collision_end();
 setup();object_depth[0]=.4f;objects[1].object.flags=1;object_depth[1]=.1f;objects[2].object.type=2;object_depth[2]=.2f;
 plasma_collision_begin(flags|FLAG(_collision_test_objects_vehicles_bit),&p,0);
 CHECK(plasma_collision_depth(flags|FLAG(_collision_test_objects_vehicles_bit),&p,&v,0)==.2f,"vehicle flag and ignored owner");plasma_collision_end();
 setup();root_count=1;objects[0].object.first_child_object_index=1;objects[1].object.next_object_index=2;object_depth[2]=.2f;
 plasma_collision_begin(flags,&p,NONE);CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.2f,"child sibling traversal");plasma_collision_end();
 objects[0].object.bounding_sphere_radius=0;plasma_collision_begin(flags,&p,NONE);
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==2,"parent sphere gates children");plasma_collision_end();
 objects[0].object.next_object_index=2;plasma_collision_begin(flags,&p,NONE);
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.2f,"ineligible parent still visits siblings");plasma_collision_end();
 setup();object_depth[0]=.5f;plasma_collision_begin(flags,&p,NONE);CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.5f,"first pose");plasma_collision_end();
 object_depth[0]=.3f;plasma_collision_begin(flags,&p,NONE);CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.3f,"next batch refreshes metadata");plasma_collision_end();
 setup();root_count=0;structure_depth=.65f;plasma_collision_begin(flags,&p,NONE);
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.65f,"structure depth");debug_collision_skip_vectors=1;
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==2,"debug skips vectors");plasma_collision_end();
 setup();debug_collision_skip_objects=1;plasma_collision_begin(flags,&p,NONE);
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==2,"debug skips objects");plasma_collision_end();
 setup();bad_leaf=1;plasma_collision_begin(flags,&p,NONE);CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.123f && stock_calls==1,"malformed leaf uses diagnostic stock path");plasma_collision_end();
 setup();root_count=600;plasma_collision_begin(flags,&p,NONE);CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.123f && stock_calls==1,"capacity overflow falls back");
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.123f && stock_calls==2,"overflow remains stock for batch");plasma_collision_end();
 setup();plasma_collision_begin(0,&p,NONE);CHECK(plasma_collision_depth(0,&p,&v,NONE)==.123f,"unsupported flags use stock");plasma_collision_end();
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.123f,"inactive query uses stock");
 setup();plasma_collision_begin(flags,&p,NONE);
 CHECK(plasma_collision_depth(flags,&p,&v,1)==.123f,"changed owner cannot use stale batch");
 CHECK(plasma_collision_depth(flags|FLAG(_collision_test_objects_vehicles_bit),&p,&v,NONE)==.123f,"changed flags cannot use stale batch");
 plasma_collision_end();
 setup();objects[0].object.bounding_sphere_center.y=100;object_depth[0]=.1f;
 plasma_collision_begin(flags,&p,NONE);
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.8f&&model_builds==2,"disjoint sphere is excluded before per-ray model work");
 plasma_collision_end();
 /* A whole ray cube follows two planes, but straddles the third. */
 setup();planes[0].d=-10;nodes[0].children[1]=1;planes[1].d=10;nodes[1].children[0]=2;
 plasma_collision_begin(flags,&p,NONE);plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==2 && queried_depth==2,"common prefix retains original traversal depth");
 v.i=20;
 CHECK(plasma_collision_depth(flags,&p,&v,NONE)==.123f&&stock_calls==1,"larger later ray uses stock instead of culled objects");v.i=1;plasma_collision_end();
 planes[0].d=-1;v.i=-1;plasma_collision_begin(flags,&p,NONE);plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==0,"octant touching a plane retains stock arithmetic");plasma_collision_end();v.i=1;
 setup();planes[0].d=-.1f;nodes[0].children[1]=1;planes[1].d=0;
 plasma_collision_begin(flags,&p,NONE);plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==1&&queried_depth==1,"positive rays skip a plane behind the origin");
 v.i=-1;plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==0&&queried_depth==0,"negative rays retain their independent root traversal");
 plasma_collision_end();v.i=1;

 setup();planes[0].d=-.1f;nodes[0].children[1]=1;
 planes[0].d=-10;nodes[0].children[1]=0;
 plasma_collision_begin(flags,&p,NONE);plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==0 && queried_depth==0,"cycle falls back without bypassing safety budget");plasma_collision_end();
 nodes[0].children[1]=99;plasma_collision_begin(flags,&p,NONE);plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==0 && queried_depth==0,"invalid descendant falls back");plasma_collision_end();
 nodes[0].children[1]=NONE;plasma_collision_begin(flags,&p,NONE);plasma_collision_depth(flags,&p,&v,NONE);
 CHECK(queried_node==NONE && queried_depth==1,"contained solid leaf preserves entry depth");plasma_collision_end();
 /* Compare the moved invariant arithmetic against the engine predicate,
    including inside starts, zero vectors, tangencies and large origins. */
 for(i=0;i<10000;i++)
 {
  struct trace_object *cached;
  setup();root_count=1;
  p.x=(i%13)*100.f;p.y=(i%17)*-.25f;p.z=(i%19)*.125f;
  objects[0].object.bounding_sphere_center.x=p.x+(i%23-11)*.125f;
  objects[0].object.bounding_sphere_center.y=p.y+(i%29-14)*.125f;
  objects[0].object.bounding_sphere_center.z=p.z+(i%31-15)*.125f;
  objects[0].object.bounding_sphere_radius=(i%7)*.25f;
  v.i=(i%11-5)*.5f;v.j=(i%13-6)*.25f;v.k=(i%17-8)*.125f;
  plasma_collision_begin(flags,&p,NONE);
  cached=trace_object(0);
  batch.vector_squared=v.i*v.i+v.j*v.j+v.k*v.k;
  CHECK(trace_sphere(cached,&v)==fast_vector_intersects_sphere(&p,&v,&cached->center,cached->radius),"cached sphere matches engine predicate");
  plasma_collision_end();
 }
 {
  real_vector3d vectors[PLASMA_COLLISION_RAYS+1];
  real expected[PLASMA_COLLISION_RAYS+1],actual[PLASMA_COLLISION_RAYS+1];
  setup();p.x=p.y=p.z=0;root_count=2;object_depth[0]=-1;
  objects[0].object.first_child_object_index=1;objects[1].object.next_object_index=2;
  object_depth[1]=.4f;object_depth[2]=.2f;objects[2].object.bounding_sphere_center.y=.8f;
  objects[2].object.bounding_sphere_radius=.3f;
  plasma_collision_begin(flags,&p,NONE);
  for(i=0;i<PLASMA_COLLISION_RAYS;i++)
  {
   vectors[i].i=1;vectors[i].j=(i%9-4)*.25f;vectors[i].k=(i%7-3)*.25f;
   expected[i]=plasma_collision_depth(flags,&p,&vectors[i],NONE);
  }
  plasma_collision_end();
  plasma_collision_depths(flags,&p,vectors,PLASMA_COLLISION_RAYS,NONE,actual);
  CHECK(!memcmp(expected,actual,PLASMA_COLLISION_RAYS*sizeof(real)),"model-major collection preserves every sphere/child/cluster gate");
  CHECK(!batch.active&&!batch.point&&!batch.objects[0].pending[0],"model-major completion clears borrowed state and pending rays");
  bad_leaf=1;plasma_collision_depths(flags,&p,vectors,PLASMA_COLLISION_RAYS,NONE,actual);
  for(i=0;i<PLASMA_COLLISION_RAYS;i++)CHECK(actual[i]==.123f,"invalid geometry falls back for the complete batch");
  setup();debug_collision_skip_vectors=1;
  plasma_collision_depths(flags,&p,vectors,PLASMA_COLLISION_RAYS,NONE,actual);
  for(i=0;i<PLASMA_COLLISION_RAYS;i++)CHECK(actual[i]==2,"vector debug skip remains clear");
  setup();vectors[PLASMA_COLLISION_RAYS]=vectors[0];
  plasma_collision_depths(flags,&p,vectors,PLASMA_COLLISION_RAYS+1,NONE,actual);
  for(i=0;i<=PLASMA_COLLISION_RAYS;i++)CHECK(actual[i]==.123f,"oversized batch uses scalar stock queries without bitset writes");
 }
 return 0;
}
