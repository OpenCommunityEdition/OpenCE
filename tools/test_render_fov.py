"""Exercise the production FOV adjustment, camera handoff and menu.

Run with the local compiler/SDK environment. No game or profile is opened.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import port_settings


def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

PRELUDE = r'''
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float real;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define MAXIMUM_LOCAL_PLAYERS 4
#define _pi 3.14159265358979323846f
#define tangent tanf
#define arctangent(y,x) atan2f(y,x)
#define DEGREES_TO_RADIANS(x) ((x)*(_pi/180.0f))
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#define TEST_FLAG(v,b) (((v)&(1u<<(b)))!=0)
#define _object_dead_bit 0
#define _director_perspective_first_person 0
#define _director_perspective_neutral 3
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
struct point { real x,y,z; };
struct vector { real i,j,k; };
struct plane { struct vector normal; real distance; };
struct bounds {real x0,y0,z0,x1,y1,z1;};
struct render_frustum {struct bounds world_bounds;struct plane world_planes[6];};
typedef struct point real_point3d;
struct render_camera {struct point position;struct vector forward,up;real vertical_field_of_view;boolean mirrored;real z_near,z_far;};
struct render_window {short local_player_index;struct render_camera rasterizer_camera,render_camera;};
struct observer_result {struct point position;struct vector forward,up;real field_of_view;};
typedef struct {struct point position;struct vector forward,up;} real_matrix4x3;
struct unit_datum {long definition_index;struct {long parent_object_index;unsigned damage_flags;}object;struct{short current_weapon_index;}unit;};
struct unit_definition {struct {real camera_field_of_view;}unit;};
static struct unit_datum units[4];
static struct unit_definition definitions[4];
static long unit_indices[4],weapon_indices[4];
static real magnifications[4];
static int perspectives[4],inhibited[4],cinematic,scripted;
static boolean *director_camera_scripted=&scripted;
static struct {real near_clip_distance,far_clip_distance;} rasterizer_globals={.01f,1000};
static struct point origin;
static struct vector zero,forward={1,0,0},up={0,0,1};
static struct point *global_origin3d=&origin;
static struct vector *global_zero_vector3d=&zero,*global_forward3d=&forward,*global_up3d=&up;
static boolean debug_render_freeze;
static double setting;
static unsigned long changes;
static unsigned config_reads,camera_queries;
static unsigned long config_changes(void){return changes;}
static double config_real(const char*n){CHECK(!strcmp(n,"display.fov"));config_reads++;return setting;}
static int cinematic_in_progress(void){camera_queries++;return cinematic;}
static int director_get_perspective(short i){CHECK(i>=0&&i<4);camera_queries++;return perspectives[i];}
static int director_inhibited_facing(short i){return inhibited[i];}
static long player_control_get_unit_index(short i){return unit_indices[i];}
static struct unit_datum*unit_try_and_get(long i){return i>=0&&i<4?&units[i]:NULL;}
static struct unit_definition*unit_definition_get(long i){CHECK(i>=0&&i<4);return &definitions[i];}
static long unit_inventory_get_weapon(long i,short j){(void)j;return weapon_indices[i];}
static real weapon_get_zoom_magnification(long i,short z){return z<0?1:magnifications[i];}
static int console_is_active(void){return 0;}
static int game_time_get_paused(void){return 0;}
static void player_effect_get_camera_effect_matrix(short i,real_matrix4x3*m){(void)i;memset(m,0,sizeof(*m));}
static void matrix4x3_from_point_and_vectors(real_matrix4x3*m,struct point const*p,struct vector const*f,struct vector const*u){m->position=*p;m->forward=*f;m->up=*u;}
static void matrix4x3_multiply(real_matrix4x3*a,real_matrix4x3*b,real_matrix4x3*out){(void)b;*out=*a;}
static void matrix4x3_to_point_and_vectors(real_matrix4x3*m,struct point*p,struct vector*f,struct vector*u){*p=m->position;*f=m->forward;*u=m->up;}
static real plane3d_distance_to_point(struct plane const*p,struct point const*v){return p->normal.i*v->x+p->normal.j*v->y+p->normal.k*v->z-p->distance;}
'''
TESTS = r'''
static unsigned probes;
static void set(double value){setting=value;changes++;}
/* Match the production camera's atan2(y, 1) operation exactly: libm need not
round atanf(y) and atan2f(y, 1) to the same last bit. */
static float native(float degrees){return 2*atan2f(.75f*render_camera_get_adjusted_field_of_view_tangent(DEGREES_TO_RADIANS(degrees)),1.f);}
static void reset(void){
 cinematic=scripted=debug_render_freeze=0;
 for(int i=0;i<4;i++){
  memset(&units[i],0,sizeof(units[i]));units[i].definition_index=i;units[i].object.parent_object_index=NONE;
  unit_indices[i]=weapon_indices[i]=i;perspectives[i]=0;inhibited[i]=0;magnifications[i]=2;
  definitions[i].unit.camera_field_of_view=DEGREES_TO_RADIANS(70);
 }
}
static void unchanged(float angle){float result=render_fov_vertical(0,angle);CHECK((isnan(angle)&&isnan(result))||result==angle);}
static void unchanged_cases(void){
 float angle=native(70);
 float stock_horizontal=2*atanf(tanf(angle/2)*(16.f/9))*(180.f/_pi);
 CHECK(stock_horizontal>75 && stock_horizontal<=80); /* next menu increment rounds up */
 reset();set(0);camera_queries=0;unchanged(angle);CHECK(camera_queries==0);
 double invalid[]={-1,19.99,150.01,NAN,INFINITY,-INFINITY};
 for(int i=0;i<6;i++){set(invalid[i]);unchanged(angle);}
 set(100);CHECK(render_fov_vertical(-1,angle)==angle);CHECK(render_fov_vertical(4,angle)==angle);
 cinematic=1;unchanged(angle);cinematic=0;scripted=1;unchanged(angle);scripted=0;
 for(int p=1;p<=3;p++){perspectives[0]=p;unchanged(angle);}perspectives[0]=0;
 inhibited[0]=1;unchanged(angle);inhibited[0]=0;
 unit_indices[0]=NONE;unchanged(angle);unit_indices[0]=5;unchanged(angle);unit_indices[0]=0;
 units[0].object.parent_object_index=7;unchanged(angle);units[0].object.parent_object_index=NONE;
 units[0].object.damage_flags=1;unchanged(angle);units[0].object.damage_flags=0;
 float bad[]={0,NAN,INFINITY,DEGREES_TO_RADIANS(91)};
 for(int i=0;i<4;i++){definitions[0].unit.camera_field_of_view=bad[i];unchanged(angle);}
 definitions[0].unit.camera_field_of_view=DEGREES_TO_RADIANS(70);
 unchanged(NAN);
}
static void make_frustum(struct render_frustum*f,float vertical,float aspect){
 float tv=tanf(vertical/2),th=tv*aspect;
 memset(f,0,sizeof(*f));f->world_bounds=(struct bounds){-1000,-1000,.01f,1000,1000,100};
 float ch=1/sqrtf(1+th*th),cv=1/sqrtf(1+tv*tv);
 f->world_planes[0]=(struct plane){{ch,0,-th*ch},0};f->world_planes[1]=(struct plane){{-ch,0,-th*ch},0};
 f->world_planes[2]=(struct plane){{0,cv,-tv*cv},0};f->world_planes[3]=(struct plane){{0,-cv,-tv*cv},0};
 f->world_planes[4]=(struct plane){{0,0,-1},-.01f};f->world_planes[5]=(struct plane){{0,0,1},100};
}
static void projections(void){
 reset();float aspects[]={4.f/3,16.f/9,21.f/9,32.f/9,8.f/9,16.f/27};
 struct observer_result observer={{2,3,4},{1,0,0},{0,0,1},DEGREES_TO_RADIANS(70)};
 struct unit_datum old_units[4];struct unit_definition old_definitions[4];
 memcpy(old_units,units,sizeof(units));memcpy(old_definitions,definitions,sizeof(definitions));
 for(int degrees=20;degrees<=150;degrees++){
  set(degrees);unsigned reads=config_reads;
  for(int player=0;player<4;player++){
   struct render_window window;memset(&window,0,sizeof(window));window.local_player_index=player;
   set_window_camera_values(&window,&observer);
   CHECK(!memcmp(&window.render_camera,&window.rasterizer_camera,sizeof(window.render_camera)));
   CHECK(!memcmp(&window.render_camera.position,&observer.position,sizeof(observer.position)));
   CHECK(!memcmp(&window.render_camera.forward,&observer.forward,sizeof(observer.forward)));
   float v=window.render_camera.vertical_field_of_view;
   CHECK(isfinite(v)&&v>0&&v<_pi);
   float reference=2*atanf(tanf(v/2)*(16.f/9))*180/_pi;CHECK(fabsf(reference-degrees)<.0001f);
   for(int a=0;a<6;a++){
    struct render_frustum f;make_frustum(&f,v,aspects[a]);
    float xmax=10*tanf(v/2)*aspects[a],ymax=10*tanf(v/2);
    struct point inside={xmax*.999f,0,10},outside={xmax*1.001f,0,10};
    CHECK(render_frustum_sphere_visible(&f,&inside,0));CHECK(!render_frustum_sphere_visible(&f,&outside,0));
    inside=(struct point){0,ymax*.999f,10};outside=(struct point){0,ymax*1.001f,10};
    CHECK(render_frustum_sphere_visible(&f,&inside,0));CHECK(!render_frustum_sphere_visible(&f,&outside,0));probes++;
   }
  }
  CHECK(config_reads==reads+1); /* same local setting shared by all views, reread after change */
 }
 CHECK(!memcmp(old_units,units,sizeof(units)));CHECK(!memcmp(old_definitions,definitions,sizeof(definitions)));
 set(0);struct render_window window={0};set_window_camera_values(&window,&observer);CHECK(window.render_camera.vertical_field_of_view==native(70));
 window.render_camera.vertical_field_of_view=123;debug_render_freeze=1;set(100);set_window_camera_values(&window,&observer);CHECK(window.render_camera.vertical_field_of_view==123);debug_render_freeze=0;
 window.local_player_index=NONE;set_window_camera_values(&window,&observer);CHECK(window.render_camera.vertical_field_of_view==native(70));
 set_window_camera_values(&window,NULL);CHECK(window.render_camera.vertical_field_of_view==native(80));
}
static void zoom_and_maps(void){
 for(int base=40;base<=85;base+=5){
  reset();float b=native(base);definitions[0].unit.camera_field_of_view=DEGREES_TO_RADIANS(base);
  for(int mag=2;mag<=10;mag++){
   magnifications[0]=mag;float s=native((float)base/mag);
   for(int degrees=20;degrees<=150;degrees+=5){
    set(degrees);float unzoomed=render_fov_vertical(0,b),zoomed=render_fov_vertical(0,s);
    CHECK(zoomed<unzoomed);
    if(unzoomed>=b){CHECK(zoomed==s);CHECK(render_fov_vertical(0,native((float)base/(mag*2)))==native((float)base/(mag*2)));}
    else{CHECK(zoomed<s);CHECK(fabsf(tanf(zoomed/2)/tanf(s/2)-tanf(unzoomed/2)/tanf(b/2))<.00001f);}
    float last=zoomed;
    for(int step=1;step<=100;step++){
     float stock=s+(b-s)*step/100.f,value=render_fov_vertical(0,stock);
     CHECK(value>=last-.000001f);CHECK(value<=unzoomed+.000001f);CHECK(isfinite(value));last=value;probes++;
    }
   }
  }
 }
 reset();weapon_indices[0]=NONE;set(100);CHECK(fabsf(render_fov_vertical(0,native(70))-2*atanf(tanf(DEGREES_TO_RADIANS(100)/2)*(9.f/16)))<.000001f);
 reset();magnifications[0]=1;CHECK(fabsf(render_fov_vertical(0,native(70))-2*atanf(tanf(DEGREES_TO_RADIANS(100)/2)*(9.f/16)))<.000001f);
 float bad[]={NAN,INFINITY,-INFINITY,0,_pi};
 for(int i=0;i<5;i++){float r=render_fov_vertical(0,bad[i]);CHECK((isnan(bad[i])&&isnan(r))||r==bad[i]);}
}
int main(void){unchanged_cases();projections();zoom_and_maps();printf("PASS: %u FOV/frustum/zoom probes; all four players, live config cache, exact Default/scoped/special-camera fallbacks, narrow zoom monotonicity and camera copies\n",probes);}
'''

def menu():
    path = ROOT/'port/assets/menus/ce/main_menu.settings_select.player_setup.player_profile_edit.video_settings.xml'
    assert path.read_text() == '\n'.join(port_settings.settings_files()[path.name])
    root = ET.parse(path).getroot()
    options = {w.get('setting'): w for w in root.findall('widget') if w.get('setting')}
    for name, zero in (('display.fov', 'DEFAULT'), ('display.viewmodel_fov', 'SAME')):
        widget = options[name]
        assert widget.get('strings').split('|') == [zero] + [str(n) for n in range(80, 151, 5)]
        assert widget.get('values').split('|') == ['0'] + [str(n) for n in range(80, 151, 5)]
        assert [e.get('event') for e in widget.findall('on')] == ['created']
    assert any(w.get('name').endswith('/settings_next_page') for w in root.findall('widget'))
    print('PASS: paginated Video Setup exposes FOV Default=0 and Viewmodel FOV Same=0, followed by 80-150; console config keys and pending-edit lifecycle preserved')


def main():
    p=argparse.ArgumentParser();p.add_argument('--cc',default='clang');a=p.parse_args()
    menu()
    s=(ROOT/'port/linux/game/render_fov.c').read_text()
    source=PRELUDE+block((ROOT/'source/render/render_cameras.c').read_text(),'real render_camera_get_adjusted_field_of_view_tangent(')+'\n'
    source+=block((ROOT/'source/items/weapons.c').read_text(),'real weapon_get_field_of_view(')+'\n'
    source+='static real reticle_scales[MAXIMUM_LOCAL_PLAYERS];\n'
    source+=block(s,'static float render_fov_adjust(')+'\n'
    source+=block(s,'float render_fov_vertical(')+'\n'
    source+=block(s,'float render_fov_reticle_scale(')+'\n'
    source+=block((ROOT/'source/main/main.c').read_text(),'void set_window_camera_values(')+'\n'
    source+=block((ROOT/'source/render/render_cameras.c').read_text(),'short render_frustum_sphere_visible(')+'\n'+TESTS
    # Production calls halo_* math functions, which cannot be folded to the
    # compiler host's libm. Keep the harness's host math calls opaque too.
    flags=['-std=gnu11','-O2','-fno-builtin','-fuse-ld=lld']+(['--target=i686-pc-windows-msvc'] if sys.platform=='win32' else ['-m32','-lm'])
    with tempfile.TemporaryDirectory(prefix='halo-fov-') as d:
        path=Path(d);(path/'fov.c').write_text(source)
        subprocess.run([a.cc,*flags,str(path/'fov.c'),'-o',str(path/'fov.exe')],check=True)
        subprocess.run([str(path/'fov.exe')],check=True,timeout=15)

if __name__=='__main__': main()
