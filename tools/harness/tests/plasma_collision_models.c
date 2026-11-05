#include "harness.h"
#include <float.h>
#define REAL_MAX FLT_MAX
#define MAXIMUM_REGIONS_PER_OBJECT 4
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(a,b,c) MIN(MAX(a,b),c)
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
enum { PLASMA_COLLISION_RAYS=1536, PLASMA_COLLISION_WORDS=48,
 _collision_test_front_facing_surfaces_bit=0, _collision_test_back_facing_surfaces_bit=1, _error_silent=0 };
typedef struct { long long QuadPart; } LARGE_INTEGER;
struct block { long count; void *address; };
struct collision_bsp { struct { struct block nodes; } bsp3d; int axis; float boundary; };
struct collision_node { short region_index; struct block bsps; };
struct collision_model { struct block nodes; };
struct collision_bsp_vector_cache { int unused; };
struct collision_bsp_test_vector_result { real t; };
struct collision_model_test_vector_result { struct collision_bsp_test_vector_result bsp_result; short node_index,region_index,bsp_index; };
static int inversions, diagnostics, warned_about_collision_model_regions;
static void error(int level,const char *format,...) { diagnostics++; }
static void collision_log_usage(int n) {}
static void collision_log_start_time(LARGE_INTEGER *t) { t->QuadPart=0; }
static void collision_log_end_time(int n,long long t) {}
static int collision_bsp_test_vector_from_node(unsigned long flags,const struct collision_bsp *b,int breaks,const byte *bits,
 const real_point3d *p,const real_vector3d *v,real limit,struct collision_bsp_test_vector_result *r,long node,short depth,struct collision_bsp_vector_cache *cache)
{
 real a=b->axis==0?p->x:b->axis==1?p->y:p->z;
 real d=b->axis==0?v->i:b->axis==1?v->j:v->k;
 real t;
 CHECK(flags==3 && !breaks && !bits && !node && !depth,"model BSP call retains original flags/root contract");
 r->t=limit;if(d==0)return 0;t=(b->boundary-a)/d;
 if(t>=0&&t<=1&&t<limit){r->t=t;return 1;}return 0;
}
static int collision_bsp_test_vector(unsigned long flags,const struct collision_bsp *b,int breaks,const byte *bits,
 const real_point3d *p,const real_vector3d *v,real limit,struct collision_bsp_test_vector_result *r)
{ return collision_bsp_test_vector_from_node(flags,b,breaks,bits,p,v,limit,r,0,0,NULL); }
#include "under_test.inc"
int main(int argc,char **argv)
{
 struct collision_model model;struct collision_node nodes[4];struct collision_bsp bsps[4][2];
 real_matrix4x3 matrices[4];struct collision_model_instance instance;
 real_point3d point={.1f,-.2f,.3f};real_vector3d vectors[PLASMA_COLLISION_RAYS];
 real actual[PLASMA_COLLISION_RAYS+2],expected[PLASMA_COLLISION_RAYS+2];
 unsigned int pending[PLASMA_COLLISION_WORDS];byte permutations[4]={0,255,1,0};
 int trial,i,n,count,old_inversions;
 memset(matrices,0,sizeof(matrices));memset(bsps,0,sizeof(bsps));
 model.nodes.count=4;model.nodes.address=nodes;
 instance.model=&model;instance.node_count=4;instance.region_permutation_indices=permutations;instance.matrices=matrices;
 for(n=0;n<4;n++)
 {
  nodes[n].region_index=n;nodes[n].bsps.count=2;nodes[n].bsps.address=bsps[n];
  matrices[n].scale=1;matrices[n].forward.i=matrices[n].left.j=matrices[n].up.k=1;
  matrices[n].position.x=n*.13f;matrices[n].position.y=-n*.17f;
  for(i=0;i<2;i++){bsps[n][i].bsp3d.nodes.count=1;bsps[n][i].axis=(n+i)%3;bsps[n][i].boundary=.35f+n*.09f+i*.2f;}
 }
 for(trial=0;trial<64;trial++)
 {
  count=trial&1?96:PLASMA_COLLISION_RAYS;
  matrices[1].forward.i=trial%2?.6f:1;matrices[1].forward.j=trial%2?.8f:0;
  matrices[1].left.i=-matrices[1].forward.j;matrices[1].left.j=matrices[1].forward.i;
  memset(pending,0,sizeof(pending));
  nodes[2].region_index=trial%5==0?NONE:2;
  nodes[3].bsps.count=trial%7==0?0:2;
  matrices[0].scale=trial%4==0?0:trial%4==1?.5f:trial%4==2?-2.f:1.f;
  for(i=0;i<PLASMA_COLLISION_RAYS+2;i++)actual[i]=expected[i]=2;
  for(i=0;i<count;i++)
  {
   vectors[i].i=((i%17)-8)*.23f;vectors[i].j=((i%19)-9)*.19f;vectors[i].k=((i%23)-11)*.21f;
   if(i%3!=trial%3)pending[i/32]|=1U<<(i%32);
   actual[i]=expected[i]=i%7==0?.15f:2;
  }
  inversions=0;
  for(i=0;i<count;i++)if(pending[i/32]&(1U<<(i%32)))
  {
   struct collision_model_test_vector_result result;
   if(collision_model_test_vector(&instance,3,&point,&vectors[i],&result)&&result.bsp_result.t<1&&result.bsp_result.t<expected[i])expected[i]=result.bsp_result.t;
  }
  old_inversions=inversions;inversions=0;
  plasma_collision_model_depths(&instance,&point,vectors,count,pending,actual);
  CHECK(!memcmp(actual,expected,sizeof(actual)),"model-major results exactly match every original model query and preserve unselected rays");
  CHECK(inversions<=4 && old_inversions>inversions*10,"node transforms are shared across the entire ray batch");
 }
 nodes[0].region_index=5;diagnostics=0;warned_about_collision_model_regions=0;
 for(i=0;i<count;i++)actual[i]=expected[i]=2;
 for(i=0;i<count;i++)if(pending[i/32]&(1U<<(i%32)))
 {
  struct collision_model_test_vector_result r;
  if(collision_model_test_vector(&instance,3,&point,&vectors[i],&r)&&r.bsp_result.t<1)expected[i]=r.bsp_result.t;
 }
 CHECK(diagnostics==1,"reference diagnoses malformed region");
 diagnostics=0;warned_about_collision_model_regions=0;
 plasma_collision_model_depths(&instance,&point,vectors,count,pending,actual);
 CHECK(diagnostics==1&&!memcmp(actual,expected,sizeof(actual)),"malformed region retains stock diagnostics and fallback depths");
 return 0;
}
