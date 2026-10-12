#include "harness.h"
#include <math.h>
#include <stddef.h>

#define HALO_PORT_MAXIMUM_NETWORK_PLAYERS 128
#define MAXIMUM_OBJECTS_PER_MAP 8192
#define MAXIMUM_MASS_POINTS_PER_PHYSICS 32
#define NUMBEROF(a) (sizeof(a)/sizeof(*(a)))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) MIN(MAX(x,a),b)
#define csmemcpy memcpy
#define csmemset memset
#define _game_connection_network_server 2
enum { _object_dead_bit, _object_connected_to_map_bit, _object_outside_of_map_bit,
       _unit_possessed_by_recording_bit };
enum { _collision_test_structure_bit, _collision_test_objects_bit, _collision_test_front_facing_surfaces_bit,
       _collision_test_back_facing_surfaces_bit };
#define _collision_test_objects_sight_blocking_flags 0
#define _object_mask_all 0xffffffffUL
#define _object_mask_projectile 4
struct scenario_object_datum {int unused;};
struct player_datum {long unit_index;};
struct location {long leaf_index; short cluster_index;};
struct test_object {
 long definition_index;
 struct {long parent_object_index; unsigned long flags,damage_flags; real_point3d position,bounding_sphere_center;
 real_vector3d forward,up,translational_velocity; float bounding_sphere_radius; short variant_number;
 float base_change_colors[12];} object;
 struct {unsigned long flags; long driver_object_index,player_index;} unit;
 struct {byte airborne_ticks;} vehicle;
};
#define vehicle_datum test_object
#define unit_datum test_object
#define object_datum test_object
struct object_placement_data {long definition_index; short variant_number; real_point3d position;
 real_vector3d forward,up; float change_colors[12];};
struct unit_definition {byte pad[0x2f4];short type;struct {struct {long index;} physics;float bounding_radius;} object;};
struct mass_point_definition {real_point3d position;float radius;};
struct tag_block {long count;void *address;};
struct physics_definition {struct tag_block mass_points;};
struct collision_model_instance {int unused;};
struct collision_result {struct {real_vector3d n;} plane;};
struct network_game {int players[128];};
static struct network_game game;
static struct player_datum player;
static struct test_object objects[1024];
static long identifiers[1024];
static struct unit_definition definitions[8];
static struct mass_point_definition mass_point;
static struct physics_definition physics;
static struct scenario_object_datum sources[4];
static struct {long actual_count;} headers,*object_header_data=&headers;
static short global_structure_bsp_index;
static int enabled,connection,coop,percent,multiplier,blocked,sphere_blocked,floor_present,allocation_failed,nearby;
static long now,new_count,last_copy;
static boolean collision_model_instance_new(struct collision_model_instance *p,long index) {(void)p;(void)index;return TRUE;}
static boolean collision_model_test_sphere(struct collision_model_instance const *p,real_point3d const *c,float r) {(void)p;(void)c;(void)r;return blocked;}

static char const *mode="per_player";
static char const *config_string(char const *name) {(void)name;return enabled?mode:"none";}
static long config_integer(char const *name) {return !strcmp(name,"network.coop_enemies")?percent:multiplier;}
static int game_connection(void) {return connection;}
static boolean network_coop_active(void) {return coop;}
static long game_time_get(void) {return now;}
static struct network_game *network_game_get_game(void) {return &game;}
static int network_player_is_valid(int const *p) {return *p;}
static struct test_object *get(long index) {int n=index&65535;return index!=NONE&&n<1024&&identifiers[n]==index?&objects[n]:NULL;}
#define vehicle_try_and_get get
#define vehicle_datum_try_and_get get
#define unit_try_and_get get
#define object_get get
static struct player_datum *player_try_and_get(long index) {return index==0?&player:NULL;}
static struct unit_definition *vehicle_definition_get(long index) {return &definitions[index];}
static struct physics_definition *physics_definition_get(long index) {(void)index;return &physics;}
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
static void object_placement_data_new(struct object_placement_data *p,long def,long owner) {
 (void)owner;memset(p,0,sizeof(*p));p->definition_index=def;
}
static boolean collision_test_vector(unsigned long f,real_point3d const *p,real_vector3d const *v,long ignored,struct collision_result *hit) {
 (void)f;(void)p;(void)v;(void)ignored;hit->plane.n.k=1;return floor_present;
}
boolean collision_test_sphere(real_point3d const *p,float r,long ignored) {(void)p;(void)r;(void)ignored;return sphere_blocked;}
static void scenario_location_from_point(struct location *l,real_point3d const *p) {(void)p;l->leaf_index=0;l->cluster_index=0;}
static short objects_in_sphere(int f,unsigned long m,struct location const *l,real_point3d const *p,float r,long *indices,int max) {
 (void)f;(void)m;(void)l;(void)p;(void)r;(void)max;indices[0]=identifiers[9];return blocked || nearby;
}
static void cross_product3d(real_vector3d const *a,real_vector3d const *b,real_vector3d *c) {
 c->i=a->j*b->k-a->k*b->j;c->j=a->k*b->i-a->i*b->k;c->k=a->i*b->j-a->j*b->i;
}
static long make(int slot,int definition) {
 struct test_object *v=&objects[slot];memset(v,0,sizeof(*v));
 identifiers[slot]=0x10000+slot;v->definition_index=definition;v->object.parent_object_index=NONE;
 v->object.flags=FLAG(_object_connected_to_map_bit);v->object.forward.i=1;v->object.up.k=1;
 v->unit.driver_object_index=NONE;v->unit.player_index=NONE;v->object.bounding_sphere_radius=.7f;return identifiers[slot];
}
static long object_new(struct object_placement_data const *p) {
 long index;
 if(allocation_failed)return NONE;
 index=make(20+new_count++,p->definition_index);last_copy=index;
 objects[index&65535].object.position=p->position;
 objects[index&65535].object.position.z+=definitions[p->definition_index].object.bounding_radius*.5f;
 return index;
}

#include "under_test.inc"

static long reset(void) {
 int i;long index;
 memset(objects,0,sizeof(objects));memset(identifiers,0,sizeof(identifiers));memset(&game,0,sizeof(game));
 memset(definitions,0,sizeof(definitions));
 for(i=0;i<8;i++){definitions[i].type=i;definitions[i].object.physics.index=0;definitions[i].object.bounding_radius=1;}
 enabled=coop=floor_present=1;connection=2;percent=50;multiplier=2;mode="per_player";blocked=sphere_blocked=allocation_failed=nearby=0;
 headers.actual_count=1;global_structure_bsp_index=0;now=new_count=0;for(i=0;i<5;i++)game.players[i]=1;
 physics.mass_points.count=1;physics.mass_points.address=&mass_point;mass_point.radius=.2f;
 index=make(0,1);make(9,1);coop_vehicles_new_game();coop_vehicles_register(index,&sources[0]);return index;
}
static void settle(void) {int i;for(i=0;i<VEHICLE_SETTLE_TICKS;i++)coop_vehicles_update();}
static void drive(long index,float metres) {
 struct test_object *v=get(index);long driver=make(1,0);
 player.unit_index=driver;objects[1].unit.player_index=0;objects[1].object.parent_object_index=index;
 v->unit.driver_object_index=driver;coop_vehicles_update();v->object.position.x+=metres/3.048f;v->object.bounding_sphere_center.x+=metres/3.048f;coop_vehicles_update();
}
static void counts(void) {
 int p,x,m;reset();
 for(p=1;p<=128;p++)for(x=25;x<=200;x+=25)
  CHECK(vehicles_extra_count(p,x,0)==((p-1)*x)/200,"players %d percentage %d",p,x);
 CHECK(vehicles_extra_count(3,50,0)==0&&vehicles_extra_count(4,50,0)==0&&vehicles_extra_count(5,50,0)==1,"round down per vehicle");
 for(m=2;m<=32;m*=2)for(p=1;p<=128;p++)
  CHECK(vehicles_extra_count(p,50,m)==(m-1)/2,"static vehicle count depends on players");
 mode="multiplier";multiplier=4;coop_vehicles_new_game();
 CHECK(vehicles_multiplier==4&&vehicles_enabled,"static config");
 mode="invalid";coop_vehicles_new_game();CHECK(!vehicles_enabled,"unknown mode enabled");
}
static void authority(void) {
 long index=reset();connection=1;coop_vehicles_new_game();coop_vehicles_register(index,&sources[0]);CHECK(!coop_vehicle_homes.count,"client registered");
 connection=2;enabled=0;coop_vehicles_new_game();coop_vehicles_register(index,&sources[0]);CHECK(!coop_vehicle_homes.count,"disabled registered");
 enabled=1;coop=0;coop_vehicles_new_game();coop_vehicles_register(index,&sources[0]);CHECK(!coop_vehicle_homes.count,"non-coop registered");
 coop=1;coop_vehicles_new_game();coop_vehicles_register(make(2,3),&sources[0]);coop_vehicles_register(make(3,6),&sources[1]);CHECK(!coop_vehicle_homes.count,"dropship/turret registered");
}
static void driving(void) {
 long index;int i;
 float radii[]={.2f,.7f,2.f,8.f};
 for(i=0;i<4;i++){
  float threshold=2*radii[i]*1.15f;
  index=reset();get(index)->object.bounding_sphere_radius=radii[i];settle();
  /* An offset model origin must not affect sphere separation. */
  get(index)->object.position.x=100;get(index)->object.bounding_sphere_center.x=100;coop_vehicles_update();CHECK(!new_count,"unoccupied movement copied");
  get(index)->object.position.x=0;get(index)->object.bounding_sphere_center.x=0;
  drive(index,(threshold-.01f)*3.048f);CHECK(!new_count,"before buffered clearance radius %f",radii[i]);
  get(index)->object.bounding_sphere_center.x=threshold+.01f;
  coop_vehicles_update();CHECK(new_count==1,"after buffered clearance radius %f",radii[i]);
 }
 reset();settle();identifiers[0]+=0x10000;coop_vehicles_update();CHECK(!new_count,"reused slot accepted");
}
static void delivery(void) {
 long index=reset();get(index)->object.parent_object_index=9;get(index)->object.position.z=100;settle();CHECK(!coop_vehicle_homes.homes[0].anchored,"cargo anchored");
 get(index)->object.parent_object_index=NONE;get(index)->object.position.z=.5f;floor_present=0;settle();CHECK(!coop_vehicle_homes.homes[0].anchored,"airborne anchored");
 floor_present=1;get(index)->vehicle.airborne_ticks=10;settle();CHECK(!coop_vehicle_homes.homes[0].anchored,"no ground contact");
 get(index)->vehicle.airborne_ticks=0;settle();CHECK(coop_vehicle_homes.homes[0].anchored&&coop_vehicle_homes.homes[0].placement.position.z==.5f,"unloaded anchor");
 drive(index,11);CHECK(new_count==1&&get(last_copy)->object.position.z==.5f,"copy not on ground pad");
}
static void chain(void) {
 long index=reset();game.players[5]=game.players[6]=game.players[7]=game.players[8]=1;settle();drive(index,11);CHECK(new_count==1,"first copy");
 drive(last_copy,11);CHECK(new_count==2,"second copy");drive(last_copy,11);CHECK(new_count==2,"budget amplified");
 index=reset();mode="multiplier";multiplier=4;coop_vehicles_new_game();coop_vehicles_register(index,&sources[0]);
 memset(&game,0,sizeof(game));game.players[0]=1;settle();
 drive(index,11);CHECK(new_count==1,"halved static copies missing");
 drive(last_copy,11);CHECK(new_count==1,"static budget amplified");
}
static void blocked_test(void) {
 long index=reset();settle();blocked=1;drive(index,11);CHECK(!new_count,"occupied pad");
 blocked=0;sphere_blocked=1;coop_vehicles_update();CHECK(!new_count,"wall");
 sphere_blocked=0;global_structure_bsp_index=1;coop_vehicles_update();CHECK(!new_count,"wrong BSP");
 global_structure_bsp_index=0;headers.actual_count=MAXIMUM_OBJECTS_PER_MAP-512;coop_vehicles_update();CHECK(!new_count,"capacity");
 headers.actual_count=1;allocation_failed=1;coop_vehicles_update();CHECK(!new_count&&!coop_vehicle_homes.homes[0].copies,"failed allocation consumed budget");
 allocation_failed=0;nearby=1;coop_vehicles_update();CHECK(new_count==1,"unblocked pad never retried");
}
static void checkpoint(void) {
 long index=reset();settle();now=102;coop_vehicles_before_save();drive(index,11);CHECK(coop_vehicle_homes.homes[0].copies==1,"copy");
 coop_vehicles_after_load();CHECK(!coop_vehicle_homes.homes[0].copies&&coop_vehicle_homes.homes[0].current==index,"budget not restored");
 coop_vehicle_homes.homes[0].copies=1;coop_vehicles_before_save();coop_vehicle_homes.homes[0].copies=9;coop_vehicles_after_load();CHECK(coop_vehicle_homes.homes[0].copies==1,"consumed budget not restored");
 now=103;coop_vehicles_after_load();CHECK(!coop_vehicle_homes.count,"unrelated core retained handles");
}
static void recreated(void) {
 long index=reset();settle();drive(index,11);coop_vehicles_register(make(2,1),&sources[0]);CHECK(coop_vehicle_homes.count==1&&coop_vehicle_homes.homes[0].copies==1,"script recreation reset budget");
 identifiers[last_copy&65535]=0;coop_vehicles_register(make(3,1),&sources[0]);CHECK(coop_vehicle_homes.count==1&&coop_vehicle_homes.homes[0].copies==1,"destroyed copy reset budget");
}
int main(int argc,char **argv) {
 if(argc!=2)return 2;
 if(!strcmp(argv[1],"counts"))counts();else if(!strcmp(argv[1],"authority"))authority();
 else if(!strcmp(argv[1],"driving"))driving();else if(!strcmp(argv[1],"delivery"))delivery();
 else if(!strcmp(argv[1],"chain"))chain();else if(!strcmp(argv[1],"blocked"))blocked_test();
 else if(!strcmp(argv[1],"checkpoint"))checkpoint();else if(!strcmp(argv[1],"recreated"))recreated();else return 2;
 return 0;
}
