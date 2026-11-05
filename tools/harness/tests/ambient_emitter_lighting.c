#include "harness.h"
#include <string.h>
#define NUMBEROF(a) ((int)(sizeof(a)/sizeof((a)[0])))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(a,b,c) MIN(MAX(a,b),c)
typedef struct { real red,green,blue; } real_rgb_color;
typedef struct { real_point3d position; } real_matrix4x3;
struct object_marker { real_matrix4x3 matrix; };
enum { _object_invisible_bit=0, _object_mask_projectile=16, _object_mask_biped=1, _object_mask_vehicle=2, _object_mask_weapon=4, _object_mask_equipment=8, _object_mask_scenery=64 };
struct object_datum { long definition_index; struct {
 long flags,owner_object_index,parent_object_index;
 struct { int size; } node_matrices;
 real maximum_body_vitality,body_vitality,outgoing_function_values[4];
 real_point3d position;
 } object; };
static struct object_datum objects[40];
static struct { struct { struct { long index; } model; } object; } definition;
#define object_definition_get(i) (&definition)
static int vehicle,world,projectile,lookups,iteration_count,marker_count=8,generation;
struct object_iterator { long index; };
static struct object_datum *object_try_and_get(long i) { return i>=0&&i<40?objects+i:NULL; }
static void object_iterator_new(struct object_iterator *it,int mask,int flags) { it->index=19; }
static struct object_datum *object_iterator_next(struct object_iterator *it) { return ++it->index<20+iteration_count?objects+it->index:NULL; }
static void object_get_origin(long i,real_point3d *p) { *p=objects[i].object.position; }
void render_interpolation_object_point(long i,real_point3d *p) { p->x+=.5f; }
static int object_get_marker_by_name(long i,const char *name,struct object_marker *out,int max)
{ int n;CHECK(!strcmp(name,"hover thrusters"),"only authored thrusters");for(n=0;n<marker_count&&n<max;n++)out[n].matrix.position.x=(real)n;return MIN(marker_count,max); }
int lighting_config_boolean(const char *name)
{ return !strcmp(name,"display.vehicle_lighting")?vehicle:!strcmp(name,"display.world_lighting")?world:projectile; }
static long tag_loaded(long group,const char *name)
{
 lookups++;
 if(group=='proj')return generation+10;
 if(group=='vehi')return generation+1;
 if(group=='scen')return generation+4;
 return generation+(!strcmp(name,"powerups\\over shield")?2:3);
}
#include "under_test.inc"
int main(int argc,char **argv)
{
 struct emitter_sample sample;int i,old;
 objects[0].definition_index=2;objects[0].object.parent_object_index=NONE;
 objects[0].object.node_matrices.size=sizeof(real_matrix4x3);
 CHECK(!ambient_emitter_lighting_begin(),"off scans no object types");CHECK(!lookups&&!ambient_emitter_lighting_sample(0,0,&sample),"off has no discovery or samples");
 world=1;ambient_emitter_lighting_begin();old=lookups;
 CHECK(ambient_emitter_lighting_sample(0,0,&sample)&&sample.kind==EMITTER_WORLD&&sample.color.red==1,"overshield pickup red");
 CHECK(sample.position.x==.5f,"render interpolation applied");
 CHECK(!ambient_emitter_lighting_sample(0,1,&sample),"one light per pickup");
 objects[0].object.parent_object_index=1;CHECK(!ambient_emitter_lighting_sample(0,0,&sample),"collected pickup has no light");objects[0].object.parent_object_index=NONE;
 objects[0].object.flags=1;CHECK(!ambient_emitter_lighting_sample(0,0,&sample),"hidden pickup has no light");objects[0].object.flags=0;
 objects[0].definition_index=3;CHECK(ambient_emitter_lighting_sample(0,0,&sample)&&sample.color.blue==1,"camo pickup blue");
 objects[0].definition_index=4;CHECK(ambient_emitter_lighting_sample(0,0,&sample)&&sample.color.green==1,"portal surface green");
 objects[0].definition_index=99;CHECK(!ambient_emitter_lighting_sample(0,0,&sample),"unknown object not inferred from name fragment");
 objects[0].definition_index=2;world=0;CHECK(!ambient_emitter_lighting_sample(0,0,&sample),"world disable immediate");
 vehicle=1;ambient_emitter_lighting_begin();CHECK(lookups==old,"no repeated tag searches");
 objects[0].definition_index=1;objects[0].object.outgoing_function_values[2]=.5f;
 CHECK(ambient_emitter_lighting_sample(0,0,&sample)&&sample.position.x==0&&sample.strength>.53f&&sample.strength<.55f,"first thruster follows engine function");
 CHECK(ambient_emitter_lighting_sample(0,1,&sample)&&sample.position.x==4,"second thruster separated");
 marker_count=1;CHECK(!ambient_emitter_lighting_sample(0,1,&sample),"short marker list bounded");marker_count=8;
 objects[0].object.maximum_body_vitality=1;CHECK(ambient_emitter_lighting_sample(0,0,&sample)&&sample.strength==0,"destroyed engine dark");
 projectile=1;iteration_count=12;definition.object.model.index=NONE;
 for(i=20;i<32;i++){objects[i].definition_index=10;objects[i].object.flags=1;objects[i].object.owner_object_index=1;objects[i].object.position.x=(real)(i-20);}
 ambient_emitter_lighting_begin();CHECK(flame_count==1&&flames[0].count==12,"many particles share one stream");
 CHECK(ambient_emitter_lighting_sample(1,0,&sample)&&sample.kind==EMITTER_FLAME&&sample.position.x==6&&sample.strength==1,"stream centroid and intensity bounded");
 for(i=20;i<32;i++)objects[i].object.owner_object_index=i-19;
 ambient_emitter_lighting_begin();CHECK(flame_count==8,"many owners bounded");
 iteration_count=0;vehicle=0;CHECK(!ambient_emitter_lighting_begin(),"no live stream skips owner walk");CHECK(!ambient_emitter_lighting_sample(1,0,&sample),"expired stream disappears");
 ambient_emitter_lighting_forget();generation=100;world=1;ambient_emitter_lighting_begin();objects[0].definition_index=2;
 CHECK(!ambient_emitter_lighting_sample(0,0,&sample),"stale map identity cannot match");
 objects[0].definition_index=102;CHECK(ambient_emitter_lighting_sample(0,0,&sample),"new map identity resolved");
 return 0;
}
