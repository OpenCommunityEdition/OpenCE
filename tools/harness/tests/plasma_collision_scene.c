#include "harness.h"
#include <math.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define _object_mask_scenery 1
#define _object_mask_machine 2
#define _object_mask_vehicle 4
#define _object_invisible_bit 0
#define _object_has_collision_model_bit 1
#define MAXIMUM_BREAKABLE_SURFACES_PER_MAP 256
static boolean debug_collision_skip_vectors, debug_collision_skip_objects;
static long global_scenario_index, global_structure_bsp_index;
static unsigned char broken[32];
static float matrices[3][8192];
struct object_datum { long definition_index; struct {
 unsigned long flags; short type; long first_cluster_reference_index, next_object_index,
 first_child_object_index, parent_object_index; real_point3d bounding_sphere_center;
 real bounding_sphere_radius; unsigned char region_permutations[8];
 struct { short size; } node_matrices;
} object; } objects[3];
struct object_iterator { long index; unsigned long mask; };
struct object_cluster_iterator { int visited; };
static short membership[3];
static short object_get_first_cluster(struct object_cluster_iterator *it,long i) {it->visited=1;return membership[i];}
static short object_get_next_cluster(struct object_cluster_iterator *it,long i) {return NONE;}
static int count=3, reads;
static void object_iterator_new(struct object_iterator *i,unsigned long mask,int unused) { i->index=-1;i->mask=mask; }
static struct object_datum *object_iterator_next(struct object_iterator *i)
{
 while(++i->index<count) if(i->mask & (1<<i->index)) {reads++;return &objects[i->index];}
 return NULL;
}
static void *object_get_node_matrices(long i) { return matrices[i]; }
typedef struct {float values[12];} real_matrix4x3;
struct collision_model_instance {short node_count; const real_matrix4x3 *matrices;};
static int collision_model_instance_new(struct collision_model_instance *m,long i)
{m->node_count=objects[i].object.node_matrices.size<0?-1:objects[i].object.node_matrices.size/48;m->matrices=(const real_matrix4x3 *)matrices[i];return 1;}
static void *breakable_surface_flags_get(void) { return broken; }
#include "under_test.inc"
static struct plasma_collision_scene scenes[2];
static real_point3d point;
static unsigned long next(int vehicles) { return plasma_collision_scene_revision(&scenes[vehicles],&point,1,vehicles); }
int main(int argc,char **argv)
{
 unsigned long a,b;int i,offset;
 for(i=0;i<3;i++) objects[i].object.node_matrices.size=48;
 a=next(0);CHECK(a && reads==2,"ordinary field snapshots scenery and machines only");
 CHECK(next(0)==a,"equal inputs reuse revision");
 matrices[2][0]=2;CHECK(next(0)==a,"ordinary fields ignore vehicle motion");
 b=next(1);matrices[2][0]=3;CHECK(next(1)!=b,"vehicle blockers track their rendered pose");
#define CHANGE(value) value++; b=next(0);CHECK(b!=a,"changed collision input invalidates");a=b
 CHANGE(matrices[0][0]); CHANGE(matrices[1][0]);
 CHANGE(objects[0].object.region_permutations[0]);
 CHANGE(objects[0].object.bounding_sphere_center.x);
 CHANGE(objects[0].object.bounding_sphere_radius);
 CHANGE(objects[0].object.next_object_index);
 CHANGE(objects[0].object.first_child_object_index);
 CHANGE(objects[0].object.parent_object_index);
 CHANGE(objects[0].definition_index);
 CHANGE(objects[0].object.flags);
 CHANGE(membership[0]);
 CHANGE(global_structure_bsp_index); CHANGE(global_scenario_index);
 CHANGE(broken[0]); CHANGE(debug_collision_skip_vectors); CHANGE(debug_collision_skip_objects);
#undef CHANGE
 count=1;b=next(0);CHECK(b!=a,"removed blocker invalidates");a=b;
 count=3;b=next(0);CHECK(b!=a,"new blocker invalidates");a=b;
 objects[0].object.node_matrices.size=-1;b=next(0);CHECK(b!=a && next(0)!=b,"invalid input never certifies reuse");
 objects[0].object.node_matrices.size=48;
 memset(scenes,0,sizeof(scenes));a=next(0);CHECK(a && next(0)==a,"field reset starts new snapshots");
 objects[0].object.first_cluster_reference_index++;CHECK(next(0)==a,"partition allocation handles do not describe geometry");
 objects[1].object.bounding_sphere_center.x=100;a=next(0);matrices[1][0]++;CHECK(next(0)==a,"distant door motion does not invalidate local fields");
 objects[1].object.bounding_sphere_center.x=0;CHECK(next(0)!=a,"blocker entering the light volume invalidates");
 offset=PLASMA_COLLISION_SCENE_BYTES;CHECK(scene_append(&scenes[0],&offset,broken,1) && offset==-1,"overflow refuses partial comparison");
 return 0;
}
