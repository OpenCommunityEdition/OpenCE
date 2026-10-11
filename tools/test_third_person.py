"""Exercise the actual shoulder camera in a fake world, without game assets."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("fault", [None, "zoom", "collision", "shoulder", "reticle"])
def test_shoulder_camera(tmp_path, fault):
    compiler = shutil.which("clang") or shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required")
    code = (ROOT / "port/linux/game/third_person.c").read_text()
    code = re.sub(r'^#include[^\n]*', '', code, flags=re.M)
    if fault == "zoom":
        code = code.replace("player_control_get_zoom_level(local_player_index) != NONE", "0")
    elif fault == "collision":
        code = code.replace("fraction * length < 0.20f", "0")
    elif fault == "shoulder":
        code = code.replace("+ 0.24f * right.", "- 0.24f * right.")
    elif fault == "reticle":
        code = code.replace("screen.x - center_x", "0")
    stub = r'''
#include <math.h>
#include <stddef.h>
typedef int boolean;
typedef float real;
typedef struct {real i,j,k;} real_vector3d;
typedef struct {real x,y,z;} real_point3d;
typedef struct {real x,y;} real_point2d;
typedef struct {short x,y;} point2d;
typedef struct {short x0,y0,x1,y1;} rectangle2d;
#define TRUE 1
#define FALSE 0
#define NONE -1
#define MAXIMUM_LOCAL_PLAYERS 4
#define FLAG(n) (1u<<(n))
#define TEST_FLAG(v,n) ((v)&FLAG(n))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(v,a,b) MIN(MAX(v,a),b)
enum {_director_perspective_first_person, _object_dead_bit=0,
_collision_test_front_facing_surfaces_bit, _collision_test_back_facing_surfaces_bit,
_collision_test_structure_bit, _collision_test_objects_bit,
_collision_test_objects_sight_blocking_flags=64, _collision_test_for_projectiles_flags=1024};
struct unit_datum {
 struct {long parent_object_index; unsigned damage_flags;} object;
 struct {int current_zoom_level, desired_zoom_level; real_vector3d aiming_vector;} unit;
};
struct render_camera {real_point3d position; real_vector3d forward,up; rectangle2d window_bounds;};
struct collision_result {real t; real_point3d point;};
static struct {short local_player_index; struct render_camera camera; int frustum;} render;
static struct unit_datum body={{NONE,0},{NONE,NONE,{1,0,0}}};
static unsigned long changes;
static int enabled, cinematic, scripted, perspective, inhibited, zoom=NONE, unit_id=1;
static int *director_camera_scripted=&scripted;
static real wall=1, prop=1;
static real_point3d eye={0,0,0};
static int on_screen=1;
int config_boolean(char const *name){(void)name;return enabled;}
unsigned long config_changes(void){return changes;}
static int cinematic_in_progress(void){return cinematic;}
static int director_get_perspective(short p){(void)p;return perspective;}
static int director_inhibited_facing(short p){(void)p;return inhibited;}
static int player_control_get_zoom_level(short p){(void)p;return zoom;}
static long player_control_get_unit_index(short p){(void)p;return unit_id;}
static struct unit_datum *unit_try_and_get(long i){(void)i;return &body;}
static struct unit_datum *unit_get(long i){(void)i;return &body;}
static void unit_get_camera_position(long i,real_point3d *p){(void)i;*p=eye;}
static void cross_product3d(real_vector3d const *a,real_vector3d const *b,real_vector3d *v){
 v->i=a->j*b->k-a->k*b->j;v->j=a->k*b->i-a->i*b->k;v->k=a->i*b->j-a->j*b->i;
}
static real magnitude3d(real_vector3d const *v){return sqrtf(v->i*v->i+v->j*v->j+v->k*v->k);}
static void normalize3d(real_vector3d *v){real l=magnitude3d(v);v->i/=l;v->j/=l;v->k/=l;}
static void scale_vector3d(real_vector3d const *v,real s,real_vector3d *o){o->i=v->i*s;o->j=v->j*s;o->k=v->k*s;}
static void point_from_line3d(real_point3d const *p,real_vector3d const *v,real s,real_point3d *o){
 o->x=p->x+v->i*s;o->y=p->y+v->j*s;o->z=p->z+v->k*s;
}
static int collision_test_pill_new(unsigned long f,real_point3d const *p,real_vector3d const *v,real r,long i,struct collision_result *c){
 (void)f;(void)p;(void)v;(void)r;(void)i;c->t=wall;return wall<1;
}
static int collision_test_vector(unsigned long f,real_point3d const *p,real_vector3d const *v,long i,struct collision_result *c){
 (void)i;c->t=f==1024?.01f:prop;point_from_line3d(p,v,c->t,&c->point);return c->t<1;
}
static int render_camera_world_to_screen(struct render_camera const *c,int const *f,real_point3d const *p,real_point2d *s){
 (void)c;(void)f;s->x=350;s->y=260;return on_screen && p->x==10;
}
'''
    checks = r'''
#define CHECK(c) do {if(!(c))return __LINE__;}while(0)
static struct render_camera fresh(void){struct render_camera c={{0,0,0},{1,0,0},{0,0,1},{0,0,640,480}};return c;}
int main(void){
 struct render_camera c=fresh();point2d p={320,240};
 third_person_camera(0,&c);CHECK(!third_person_active(0)&&c.position.x==0);
 enabled=1;changes++;third_person_camera(0,&c);
 CHECK(third_person_active(0)&&c.position.x<-.8f&&c.position.y<-.2f&&c.position.z>.1f);
 CHECK(body.unit.aiming_vector.i==1&&eye.x==0);
 render.camera=c;CHECK(third_person_reticle(&p,.5f));CHECK(p.x==380&&p.y==280);
 on_screen=0;CHECK(!third_person_reticle(&p,1));on_screen=1;
 zoom=0;c=fresh();third_person_camera(0,&c);CHECK(!third_person_active(0)&&c.position.x==0);zoom=NONE;
 body.unit.current_zoom_level=0;third_person_camera(0,&c);CHECK(!third_person_active(0));body.unit.current_zoom_level=NONE;
 body.unit.desired_zoom_level=0;third_person_camera(0,&c);CHECK(!third_person_active(0));body.unit.desired_zoom_level=NONE;
 body.object.parent_object_index=2;third_person_camera(0,&c);CHECK(!third_person_active(0));body.object.parent_object_index=NONE;
 body.object.damage_flags=1;third_person_camera(0,&c);CHECK(!third_person_active(0));body.object.damage_flags=0;
 cinematic=1;third_person_camera(0,&c);CHECK(!third_person_active(0));cinematic=0;
 scripted=1;third_person_camera(0,&c);CHECK(!third_person_active(0));scripted=0;
 perspective=1;third_person_camera(0,&c);CHECK(!third_person_active(0));perspective=0;
 unit_id=NONE;third_person_camera(0,&c);CHECK(!third_person_active(0));unit_id=1;
 wall=.5f;c=fresh();third_person_camera(0,&c);CHECK(third_person_active(0)&&c.position.x>-.45f&&c.position.x<-.35f);
 wall=.05f;c=fresh();third_person_camera(0,&c);CHECK(!third_person_active(0)&&c.position.x==0);wall=1;
 prop=.4f;c=fresh();third_person_camera(0,&c);CHECK(third_person_active(0)&&c.position.x>-.3f);prop=1;
 c=fresh();third_person_camera(1,&c);CHECK(third_person_active(1));
 third_person_camera(0,NULL);CHECK(!third_person_active(0)&&third_person_active(1));
 CHECK(!third_person_active(-1)&&!third_person_active(4));
 enabled=0;changes++;c=fresh();third_person_camera(1,&c);CHECK(!third_person_active(1));
 return 0;
}
'''
    source = tmp_path / "shoulder.c"
    source.write_text(stub + code + checks)
    exe = tmp_path / ("shoulder.exe" if sys.platform == "win32" else "shoulder")
    flags = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld"] if sys.platform == "win32" else ["-lm"]
    compiled = subprocess.run([compiler, str(source), *flags, "-o", str(exe)], capture_output=True, text=True)
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run([str(exe)], timeout=10)
    assert (ran.returncode != 0) == bool(fault), ran.returncode
