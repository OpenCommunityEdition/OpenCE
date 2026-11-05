#include "harness.h"
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#define _contents_unknown 0
typedef struct { long long QuadPart; } LARGE_INTEGER;
struct collision_bsp { struct { struct { long count; } nodes; } bsp3d; };
struct collision_bsp_vector_cache { int unused; };
struct collision_bsp_test_vector_result { real t; long leaf_count; };
static struct collision_bsp *global_collision_bsp;
static struct { LARGE_INTEGER vector; } collision_bsp_usage_times;
static long seen_node, seen_budget;
static short seen_depth;
static real seen_t;
static int calls, logs;
static void collision_log_usage(short function) { logs++; }
static void collision_log_start_time(LARGE_INTEGER *time) { time->QuadPart=0; }
static void collision_log_end_time(short function,long long time) {}
#include "context.inc"
static boolean collision_bsp_test_vector_recursive(struct collision_bsp_test_vector_context *c,
 long node,real t0,real t1,short depth)
{
 CHECK(c->last_leaf_index==NONE && c->last_plane_index==NONE && c->last_contents==_contents_unknown,
  "entry preserves initial traversal history");
 CHECK(c->result->leaf_count==0,"entry resets leaf collection");
 calls++;seen_node=node;seen_depth=depth;seen_budget=c->nodes_left;seen_t=t1;
 return TRUE;
}
#include "under_test.inc"
int main(int argc,char **argv)
{
 struct collision_bsp bsp;
 struct collision_bsp_test_vector_result result;
 real_point3d point={0,0,0};real_vector3d vector={1,0,0};
 bsp.bsp3d.nodes.count=100;
 CHECK(collision_bsp_test_vector(3,&bsp,0,NULL,&point,&vector,10,&result),"stock return value");
 CHECK(seen_node==0 && seen_depth==0 && seen_budget==100 && seen_t==1 && result.t==10,
  "stock calls retain root, full budget and original maximum-t handling");
 collision_bsp_test_vector(3,&bsp,0,NULL,&point,&vector,-2,&result);
 CHECK(seen_t==0 && result.t==0,"negative range preserves stock clamping");
 collision_bsp_test_vector_from_node(3,&bsp,0,NULL,&point,&vector,.7f,&result,18,6,NULL);
 CHECK(seen_node==18 && seen_depth==6 && seen_budget==94 && seen_t==.7f,
  "skipped prefix consumes its node and depth budgets");
 CHECK(logs==3 && calls==3,"one traversal and diagnostic accounting per query");
 return 0;
}
