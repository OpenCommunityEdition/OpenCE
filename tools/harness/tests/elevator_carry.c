#include "harness.h"
#include <math.h>
enum {_machine_is_elevator_bit=2, _object_mask_biped=1};
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
struct fake_object {
    struct { real_point3d position, bounding_sphere_center; int location; real bounding_sphere_radius; } object;
    struct { real_point3d elevator_position; } machine;
    struct { long elevator_object_index; } biped;
};
struct fake_definition {struct {unsigned flags; short elevator_node_index;} machine;};
struct fake_matrix {real_point3d position;};
#define machine_datum fake_object
#define biped_datum fake_object
#define machine_definition fake_definition
#define real_matrix4x3 struct fake_matrix
static struct fake_object platform, rider, stranger;
static struct fake_definition definition;
static struct fake_matrix node;
static struct fake_object *machine_get(long i) {return &platform;}
static struct fake_definition *machine_definition_get(long i) {return &definition;}
/* The production function reads the definition index too. */
#define definition_index object.location
static struct fake_matrix *object_get_node_matrix(long i, int n) {return &node;}
static short objects_in_sphere(long a,long b,void *l,void *p,real r,long *out,int count) {out[0]=1;out[1]=3;return 2;}
static struct fake_object *biped_get(long i) {return i==1 ? &rider : &stranger;}
static void add_vectors3d(real_vector3d const *a,real_vector3d const *b,real_vector3d *c) {c->i=a->i+b->i;c->j=a->j+b->j;c->k=a->k+b->k;}
static void object_translate(long i,real_point3d const *p,void *unused) {biped_get(i)->object.position=*p;}
#include "under_test.inc"
int main(int argc,char **argv) {
    char const *case_name=argc>1?argv[1]:"";
    real_point3d before={0,0,10};
    real old_step=.1f,new_step=.1f;
    definition.machine.flags=FLAG(_machine_is_elevator_bit);definition.machine.elevator_node_index=0;
    rider.biped.elevator_object_index=2;stranger.biped.elevator_object_index=4;
    rider.object.position.z=stranger.object.position.z=10;
    if (!strcmp(case_name,"down")) old_step=new_step=-.1f;
    if (!strcmp(case_name,"start")) {old_step=0;new_step=.1f;}
    if (!strcmp(case_name,"stop")) new_step=0;
    if (!strcmp(case_name,"reverse")) {old_step=-.1f;new_step=.1f;}
    if (!strcmp(case_name,"snap")) {old_step=.1f;new_step=-4;}
    if (!strcmp(case_name,"door")) definition.machine.flags=0;
    platform.machine.elevator_position.z=before.z-old_step;
    node.position.z=before.z+new_step;
    machine_move_elevator_riders(2,&before);
    if (!strcmp(case_name,"door")) CHECK(rider.object.position.z==10,"door carried a rider");
    else CHECK(fabsf(rider.object.position.z-node.position.z)<.00001f,"%s: rider %f floor %f",case_name,rider.object.position.z,node.position.z);
    CHECK(stranger.object.position.z==10,"another elevator's rider was moved");
    return 0;
}
