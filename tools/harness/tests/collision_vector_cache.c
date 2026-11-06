#include "harness.h"
#include <limits.h>
#include <math.h>
#define MAXIMUM_BSP3D_TRAVERSAL_DEPTH 128
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
enum { _contents_unknown, _contents_solid, _contents_empty, _contents_semi_empty };
enum { _collision_test_front_facing_surfaces_bit, _collision_test_back_facing_surfaces_bit,
 _collision_test_ignore_two_sided_surfaces_bit, _collision_test_ignore_invisible_surfaces_bit,
 _collision_test_ignore_breakable_surfaces_bit };
enum { _collision_leaf_contains_two_sided_bit, _collision_surface_invisible_bit, _collision_surface_breakable_bit };
enum { COLLISION_VECTOR_CACHE_NODES=512 };
typedef struct { real_vector3d n; real d; } real_plane3d;
struct bsp3d_node { long plane_designator, children[2]; };
struct block { long count; void *address; };
struct collision_bsp { struct { struct block nodes,planes; } bsp3d; struct block leaves,surfaces; };
struct collision_leaf { unsigned long flags; };
struct collision_surface { byte flags,breakable_surface_index; short material_index; long plane_designator; };
struct collision_bsp_test_vector_result { real t; const real_plane3d *plane; long surface_index,plane_designator;
 byte flags,breakable_surface_index; short material_index; long leaf_count,leaf_indices[256]; };
#include "cache.inc"
#include "context.inc"
static int distances, entries;
static real plane3d_distance_to_point(const real_plane3d *p,const real_point3d *v)
{ distances++;return p->n.i*v->x+p->n.j*v->y+p->n.k*v->z-p->d; }
static real dot_product3d(const real_vector3d *a,const real_vector3d *b)
{ return a->i*b->i+a->j*b->j+a->k*b->k; }
static boolean collision_bsp3d_enter_node(const struct collision_bsp *b,long n,short depth,long *left)
{ entries++;return n<b->bsp3d.nodes.count && depth<128 && --*left>=0; }
static boolean collision_bsp_valid_leaf(const struct collision_bsp *b,long n)
{ return n>=0 && n<b->leaves.count; }
static long collision_leaf_test_vector(const struct collision_bsp *b,short count,const byte *bits,
 const real_point3d *p,const real_vector3d *v,long leaf,long plane,real t,boolean surface)
{ return plane==NONE?NONE:0; }
#include "under_test.inc"
static struct {
 const struct collision_bsp *bsp;
 const real_point3d *point;
 real extent;
 struct collision_bsp_vector_cache cache[8];
} batch;
#include "prepare.inc"
int main(int argc,char **argv)
{
 struct bsp3d_node nodes[513];real_plane3d planes[3];struct collision_leaf leaves[2];
 struct collision_surface surface;struct collision_bsp bsp;struct collision_bsp_vector_cache cache;
 struct collision_bsp_test_vector_context c;struct collision_bsp_test_vector_result a,b;
 real_point3d p={.25f,.5f,.75f};real_vector3d v;int i,hit_a,hit_b,uncached=0,cached=0;
 memset(nodes,0,sizeof(nodes));memset(planes,0,sizeof(planes));memset(leaves,0,sizeof(leaves));
 memset(&surface,0,sizeof(surface));memset(&cache,0,sizeof(cache));memset(&c,0,sizeof(c));
 nodes[0].children[0]=512;nodes[0].children[1]=1;
 nodes[512].plane_designator=1;nodes[512].children[0]=LONG_MIN;nodes[512].children[1]=NONE;
 nodes[1].plane_designator=2;nodes[1].children[0]=LONG_MIN|1;nodes[1].children[1]=NONE;
 planes[0].n.i=planes[1].n.j=planes[2].n.k=1;
 bsp.bsp3d.nodes.count=513;bsp.bsp3d.nodes.address=nodes;bsp.bsp3d.planes.count=3;bsp.bsp3d.planes.address=planes;
 bsp.leaves.count=2;bsp.leaves.address=leaves;bsp.surfaces.count=1;bsp.surfaces.address=&surface;
 c.bsp=&bsp;c.point=&p;c.vector=&v;c.flags=3;
 for(i=0;i<1000;i++)
 {
  v.i=(i%17-8)*.4f;v.j=(i%13-6)*.35f;v.k=(i%11-5)*.3f;
  memset(&a,0,sizeof(a));a.t=1;c.result=&a;c.nodes_left=513;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=NULL;
  distances=0;hit_a=collision_bsp_test_vector_recursive(&c,0,0,1,0);uncached+=distances;
  memset(&b,0,sizeof(b));b.t=1;c.result=&b;c.nodes_left=513;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=&cache;
  distances=0;hit_b=collision_bsp_test_vector_recursive(&c,0,0,1,0);cached+=distances;
  CHECK(hit_a==hit_b && !memcmp(&a,&b,sizeof(a)),"cache preserves hit, leaf order and all result fields despite slot collisions");
 }
 CHECK(cached<uncached,"batch reuses work across rays");
 /* Branching trees with long one-sided chains, thin planes and slot aliases.
    Compare the complete result and traversal state, including exhausted
    budgets, rather than just the hit distance. */
 batch.bsp=&bsp;batch.point=&p;batch.extent=4;
 planes[0].d=0;planes[1].d=20;planes[2].d=-20;
 for(i=0;i<513;i++) {
  nodes[i].plane_designator=i%5==0?0:1+i%2;
  nodes[i].children[0]=2*i+1<513?2*i+1:LONG_MIN;
  nodes[i].children[1]=2*i+2<513?2*i+2:NONE;
 }
 uncached=cached=0;
 for(i=0;i<2000;i++) {
  int work=512,left_a,entries_a,plane_a,leaf_a,contents_a,budget=1+i%513;
  long target;short skipped;
  v.i=(i%17-8)*.4f;v.j=(i%13-6)*.35f;v.k=(i%11-5)*.3f;
  memset(&batch.cache,0,sizeof(batch.cache));trace_prepare(0,0,(v.i>=0)|((v.j>=0)<<1)|((v.k>=0)<<2),&work,&target,&skipped);
  memset(&a,0,sizeof(a));a.t=1;c.result=&a;c.nodes_left=budget;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=NULL;
  entries=0;hit_a=collision_bsp_test_vector_recursive(&c,0,0,1,0);
  left_a=c.nodes_left;entries_a=entries;plane_a=c.last_plane_index;leaf_a=c.last_leaf_index;contents_a=c.last_contents;
  memset(&b,0,sizeof(b));b.t=1;c.result=&b;c.nodes_left=budget;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=&batch.cache[(v.i>=0)|((v.j>=0)<<1)|((v.k>=0)<<2)];
  entries=0;hit_b=collision_bsp_test_vector_recursive(&c,0,0,1,0);
  CHECK(hit_a==hit_b && !memcmp(&a,&b,sizeof(a)),"prepared chains preserve all surface and ordered leaf results");
  CHECK(left_a==c.nodes_left && plane_a==c.last_plane_index && leaf_a==c.last_leaf_index && contents_a==c.last_contents,"prepared chains preserve budget and crossing state");
  uncached+=entries_a;cached+=entries;
 }
 CHECK(cached<uncached,"prepared chains actually skip internal visits");
 /* Replacing a prepared entry must not retain its old skip destination. */
 memset(&cache,0,sizeof(cache));cache.nodes[0].index_plus_one=1;
 cache.nodes[0].skip_count=8;cache.nodes[0].skip_node=NONE;
 nodes[512].plane_designator=0;nodes[512].children[0]=LONG_MIN;nodes[512].children[1]=NONE;
 v.i=-1;v.j=v.k=0;
 for(i=0;i<2;i++) {
  memset(&a,0,sizeof(a));a.t=1;c.result=&a;c.nodes_left=513;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=NULL;
  hit_a=collision_bsp_test_vector_recursive(&c,512,0,1,0);
  memset(&b,0,sizeof(b));b.t=1;c.result=&b;c.nodes_left=513;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=&cache;
  hit_b=collision_bsp_test_vector_recursive(&c,512,0,1,0);
  CHECK(hit_a==hit_b && !memcmp(&a,&b,sizeof(a)),"slot eviction cannot reuse another node's chain");
 }
 /* A cycle must still exhaust the original guard, with or without a cache. */
 nodes[0].plane_designator=1;nodes[0].children[0]=0;
 {
  int work=512;long target;short skipped;
  memset(&batch.cache,0,sizeof(batch.cache));trace_prepare(0,0,7,&work,&target,&skipped);
  memset(&a,0,sizeof(a));a.t=1;c.result=&a;c.nodes_left=513;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=NULL;
  hit_a=collision_bsp_test_vector_recursive(&c,0,0,1,0);uncached=c.nodes_left;
  memset(&b,0,sizeof(b));b.t=1;c.result=&b;c.nodes_left=513;c.last_leaf_index=c.last_plane_index=NONE;c.last_contents=_contents_unknown;c.cache=&batch.cache[7];
  hit_b=collision_bsp_test_vector_recursive(&c,0,0,1,0);
  CHECK(hit_a==hit_b && !memcmp(&a,&b,sizeof(a)) && uncached==c.nodes_left,"cyclic input keeps the original depth guard");
 }
 return 0;
}
