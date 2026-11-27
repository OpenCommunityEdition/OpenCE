"""Camera-relative H1A rays, exercised without maps or a renderer."""
from pathlib import Path
import subprocess

import pytest

from harness import function
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]


def compile_tool(work, sources):
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("clang required")
    exe = work / ("test.exe" if sys.platform == "win32" else "test")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if sys.platform == "win32" else []
    result = subprocess.run([clang, *flags, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
                             "-I", str(work), "-I", str(ROOT / "port/linux/game"),
                             *map(str, sources), *([] if sys.platform == "win32" else ["-lm"]), "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return exe


@pytest.fixture(scope="module")
def ray_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-projectiles")
    implementation = "\n".join(line for line in
        (ROOT / "port/linux/game/mcc_projectiles.c").read_text().splitlines()
        if not line.startswith("#include"))
    source = work / "test.c"
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define FLAG(x) (1UL << (x))
#define TEST_FLAG(v,b) ((v)&FLAG(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
typedef int boolean;
typedef float real;
typedef struct {real x,y,z;} real_point3d;
typedef struct {real i,j,k;} real_vector3d;
enum {_weapon_trigger_uses_weapon_origin_bit=5,_weapon_trigger_projectiles_cannot_be_aimed_bit=11,
      _object_dead_bit=0,_unit_fires_from_camera_bit=1,_collision_test_for_projectiles_flags=123};
struct weapon_datum {long definition_index;};
struct weapon_trigger_definition {unsigned long flags; real_point3d first_person_weapon_offset;};
struct block {int count; void *address;};
struct weapon_definition {struct {struct block triggers;} weapon;};
struct unit_datum {long definition_index; struct {long parent_object_index;unsigned long damage_flags;} object;
    struct {long player_index,gunner_object_index;real_vector3d desired_aiming_vector;} unit;};
struct unit_definition {struct {unsigned long flags;} unit;};
struct player_datum {long aim_assist_unit_index,aim_assist_timestamp;};
struct collision_result {real t; real_point3d point;};
static struct weapon_datum weapon;
static struct weapon_definition definition;
static struct weapon_trigger_definition trigger;
static struct unit_datum unit;
static struct unit_definition unit_definition;
static struct player_datum player;
static real_point3d eye;
static real_vector3d motion, plane_normal;
static real plane_distance;
static int loaded, following, valid_weapon, valid_unit, calls, assisted, wall;
static real_vector3d z_axis={0,0,1}, y_axis={0,1,0};
#define global_up3d (&z_axis)
#define global_left3d (&y_axis)
static int mcc_cache_tags_loaded(void) {return loaded;}
static int mcc_camera_weapon_following(long u) {CHECK(u==1);return following;}
static struct weapon_datum *weapon_try_and_get(long w) {CHECK(w==2);return valid_weapon?&weapon:NULL;}
static struct weapon_definition *weapon_definition_get(long d) {(void)d;return &definition;}
static struct unit_datum *unit_try_and_get(long u) {CHECK(u==1);return valid_unit?&unit:NULL;}
static struct unit_definition *unit_definition_get(long d) {(void)d;return &unit_definition;}
static struct player_datum *player_get(long p) {CHECK(p==3);return &player;}
static long game_time_get(void) {return 42;}
static void unit_get_camera_position(long u,real_point3d *p) {CHECK(u==1);*p=eye;}
static void object_get_velocities(long u,real_vector3d *v,void *a) {CHECK(u==1 && !a);*v=motion;}
static real dot_product3d(real_vector3d const *a,real_vector3d const *b) {return a->i*b->i+a->j*b->j+a->k*b->k;}
static real magnitude3d(real_vector3d const *v) {return (real)sqrt(dot_product3d(v,v));}
static real normalize3d(real_vector3d *v) {real l=magnitude3d(v);if(l){v->i/=l;v->j/=l;v->k/=l;}return l;}
static void cross_product3d(real_vector3d const *a,real_vector3d const *b,real_vector3d *v) {
 v->i=a->j*b->k-a->k*b->j;v->j=a->k*b->i-a->i*b->k;v->k=a->i*b->j-a->j*b->i;}
static void point_from_line3d(real_point3d const *p,real_vector3d const *v,real t,real_point3d *o) {
 o->x=p->x+v->i*t;o->y=p->y+v->j*t;o->z=p->z+v->k*t;}
static void vector_from_points3d(real_point3d const *a,real_point3d const *b,real_vector3d *v) {
 v->i=b->x-a->x;v->j=b->y-a->y;v->k=b->z-a->z;}
static void scale_vector3d(real_vector3d const *a,real s,real_vector3d *v) {v->i=a->i*s;v->j=a->j*s;v->k=a->k*s;}
static void fast_normals_interpolate(real_vector3d const *a,real_vector3d const *b,real t,real_vector3d *v) {
 v->i=a->i*(1-t)+b->i*t;v->j=a->j*(1-t)+b->j*t;v->k=a->k*(1-t)+b->k*t;normalize3d(v);}
static int collision_test_vector(int flags,real_point3d const *p,real_vector3d const *v,long ignore,struct collision_result *c) {
 real denom=dot_product3d(&plane_normal,v),t=2.0f,d;
 CHECK(flags==123 && ignore==1);calls++;
 if(denom!=0) {d=(plane_distance-plane_normal.i*p->x-plane_normal.j*p->y-plane_normal.k*p->z)/denom;if(d>=0 && d<=1)t=d;}
 if(wall && v->j!=0) {d=(-0.05f-p->y)/v->j;if(d>=0 && d<t)t=d;}
 c->t=t;point_from_line3d(p,v,t,&c->point);return t<=1;
}
static long player_get_aim_assist_target(long p,real_point3d const *pos,real_vector3d const *dir,real_point3d *target,real *level) {
 CHECK(p==3);point_from_line3d(pos,dir,2,target);target->y+=0.05f;*level=assisted?1.0f:0.0f;return assisted?7:NONE;
}
''' + implementation + r'''
static void reset(void) {
 memset(&unit,0,sizeof(unit));memset(&trigger,0,sizeof(trigger));memset(&motion,0,sizeof(motion));
 eye.x=3;eye.y=0;eye.z=1;loaded=valid_weapon=valid_unit=1;following=calls=assisted=wall=0;
 unit.object.parent_object_index=unit.unit.gunner_object_index=NONE;unit.unit.player_index=3;
 unit.unit.desired_aiming_vector.i=1;unit.unit.desired_aiming_vector.j=unit.unit.desired_aiming_vector.k=0;
 unit_definition.unit.flags=FLAG(_unit_fires_from_camera_bit);definition.weapon.triggers.count=1;
 definition.weapon.triggers.address=&trigger;trigger.first_person_weapon_offset.x=0.15f;
 trigger.first_person_weapon_offset.y=trigger.first_person_weapon_offset.z=-0.15f;
 plane_normal=unit.unit.desired_aiming_vector;plane_distance=5;player.aim_assist_unit_index=99;player.aim_assist_timestamp=98;
}
static void rejected(short index,long owner) {
 real_point3d p={11,12,13};real_vector3d d={14,15,16};real v=17;long t=18;
 CHECK(!mcc_projectile_ray(2,owner,index,TRUE,&p,&d,&v,&t));
 CHECK(p.x==11 && p.y==12 && p.z==13 && d.i==14 && d.j==15 && d.k==16 && v==17 && t==18 && calls==0);
}
int main(int argc,char **argv) {
 real_point3d p, hit;real_vector3d d,delta;real v;long target;int i,j;real angles[]={0,-45,-85.5f,85.5f,90};
 CHECK(argc==2);reset();
 if(!strcmp(argv[1],"guards")) {
  loaded=0;rejected(0,1);reset();trigger.flags=0x4000;rejected(0,1);
  reset();trigger.flags=FLAG(_weapon_trigger_uses_weapon_origin_bit);rejected(0,1);
  reset();trigger.flags=FLAG(_weapon_trigger_projectiles_cannot_be_aimed_bit);rejected(0,1);
  reset();unit.unit.player_index=NONE;rejected(0,1);reset();unit.object.parent_object_index=8;rejected(0,1);
  reset();unit.unit.gunner_object_index=8;rejected(0,1);reset();unit.object.damage_flags=1;rejected(0,1);
  reset();unit_definition.unit.flags=0;rejected(0,1);reset();following=1;rejected(0,1);
  reset();valid_weapon=0;rejected(0,1);reset();valid_unit=0;rejected(0,1);reset();rejected(-1,1);rejected(1,1);rejected(0,NONE);
  reset();memset(&unit.unit.desired_aiming_vector,0,sizeof(d));rejected(0,1);
 } else if(!strcmp(argv[1],"aim")) {
  for(i=0;i<5;i++)for(j=0;j<2;j++) {
   real a=angles[i]*0.01745329252f;
   reset();unit.unit.desired_aiming_vector.i=j?0:(real)cos(a);unit.unit.desired_aiming_vector.j=j?(real)cos(a):0;
   unit.unit.desired_aiming_vector.k=(real)sin(a);plane_normal=unit.unit.desired_aiming_vector;
   plane_distance=plane_normal.i*eye.x+plane_normal.j*eye.y+plane_normal.k*eye.z+2;
   CHECK(mcc_projectile_ray(2,1,0,FALSE,&p,&d,&v,&target));CHECK(calls==2 && target==NONE);
   point_from_line3d(&eye,&plane_normal,2,&hit);vector_from_points3d(&p,&hit,&delta);normalize3d(&delta);
   CHECK(dot_product3d(&d,&delta)>0.99999f);CHECK(player.aim_assist_unit_index==99 && player.aim_assist_timestamp==98);
   /* Authored offsets are based at the camera, independent of a world marker. */
   vector_from_points3d(&eye,&p,&delta);CHECK(fabs(dot_product3d(&delta,&plane_normal)-0.15f)<0.00001f);
  }
 } else if(!strcmp(argv[1],"cover")) {
  wall=1;CHECK(mcc_projectile_ray(2,1,0,FALSE,&p,&d,&v,&target));CHECK(p.y > -0.05f && p.y < -0.049f);
  CHECK(d.i>0 && d.j>0);
  reset();plane_distance=eye.x+0.02f;CHECK(mcc_projectile_ray(2,1,0,FALSE,&p,&d,&v,&target));
  CHECK(p.x<plane_distance && d.i>0); /* cover closer than the forward offset */
  reset();plane_distance=eye.x;CHECK(mcc_projectile_ray(2,1,0,FALSE,&p,&d,&v,&target));CHECK(d.i>0);
 } else if(!strcmp(argv[1],"assist")) {
  assisted=1;motion.i=0.2f;motion.j=0.1f;
  CHECK(mcc_projectile_ray(2,1,0,TRUE,&p,&d,&v,&target));CHECK(target==7 && player.aim_assist_unit_index==7 && player.aim_assist_timestamp==42);
  CHECK(fabs(v-dot_product3d(&unit.unit.desired_aiming_vector,&motion))<0.00001f);CHECK(d.j>0);
 } else if(!strcmp(argv[1],"transition")) {
  CHECK(mcc_projectile_ray(2,1,0,FALSE,&p,&d,&v,&target));loaded=0;calls=0;rejected(0,1);
  loaded=1;CHECK(mcc_projectile_ray(2,1,0,FALSE,&p,&d,&v,&target)); /* no retained ray or tag pointers */
 } else return 2;
 return 0;
}
''')
    return compile_tool(work, [source])


@pytest.mark.parametrize("case", ["guards", "aim", "cover", "assist", "transition"])
def test_mcc_projectile_ray(ray_tool, case):
    subprocess.run([str(ray_tool), case], check=True)


def test_aim_assist_query_preserves_authored_zoom_rules(tmp_path):
    text = (ROOT / "source/game/aim_assist.c").read_text()
    source = tmp_path / "query.c"
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x))exit(__LINE__);}while(0)
#define NONE (-1)
#define TRUE 1
#define FALSE 0
#define MAX(a,b) ((a)>(b)?(a):(b))
#define TEST_FLAG(v,b) ((v)&(1U<<(b)))
#define _weapon_aim_assists_only_when_zoomed_bit 0
typedef int boolean;typedef float real;
typedef struct {real x,y,z;} real_point3d;
typedef struct {real i,j,k;} real_vector3d;
struct aim_assist_parameters {real autoaim_angle,autoaim_distance,magnetism_angle,magnetism_distance,deviation_angle,unused;};
struct aim_assist_target {long object_index;real_point3d position;real autoaim_level;};
struct player_datum {long unit_index;short team_index;};
struct unit_datum {struct {short current_weapon_index;} unit;};
struct weapon_datum {long definition_index;};
struct weapon_definition {struct {unsigned flags;struct aim_assist_parameters aim_assist_parameters;} weapon;};
static struct player_datum p={1,2};static struct unit_datum u;static struct weapon_datum w;static struct weapon_definition wd;
static short zoom=NONE;static int calls;static real magnification=1;
static struct player_datum *player_get(long index) {CHECK(index==3);return &p;}
static long unit_get_aiming_unit_index(long index) {return index;}
static short unit_get_zoom_level(long index) {CHECK(index==1);return zoom;}
static struct unit_datum *unit_get(long index) {CHECK(index==1);return &u;}
static long unit_inventory_get_weapon(long index,short slot) {CHECK(index==1 && slot==0);return 4;}
static struct weapon_datum *weapon_get(long index) {CHECK(index==4);return &w;}
static struct weapon_definition *weapon_definition_get(long index) {(void)index;return &wd;}
static real weapon_get_zoom_magnification(long index,short z) {CHECK(index==4 && z==zoom);return magnification;}
static int aim_assist(struct aim_assist_parameters const *a,real_point3d const *pos,real_vector3d const *dir,long ignore,short team,struct aim_assist_target *t) {
 CHECK(ignore==1 && team==2 && pos->x==11 && dir->k==1);calls++;
 CHECK(a->autoaim_angle==0.25f && a->autoaim_distance==40 && a->deviation_angle==0.25f);
 t->object_index=9;t->position=*pos;t->autoaim_level=0.75f;return TRUE;
}
''' + function(text, "unit_get_aim_assist_parameters") + "\n" +
        function(text, "player_get_aim_assist_target") + r'''
int main(void) {
 real_point3d pos={11,12,13},target;real_vector3d dir={0,0,1};real level=99;
 wd.weapon.flags=1;wd.weapon.aim_assist_parameters.autoaim_angle=0.5f;
 wd.weapon.aim_assist_parameters.autoaim_distance=20;
 CHECK(player_get_aim_assist_target(3,&pos,&dir,&target,&level)==NONE && level==0 && calls==0);
 zoom=0;magnification=2;
 CHECK(player_get_aim_assist_target(3,&pos,&dir,&target,&level)==9 && level==0.75f && target.x==11 && calls==1);
 return 0;
}
''')
    exe = compile_tool(tmp_path, [source])
    subprocess.run([str(exe)], check=True)
