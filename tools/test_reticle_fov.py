"""Exercise production camera ratios and HUD bitmap vertices, without game assets."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

import test_render_fov as fov

ROOT = Path(__file__).resolve().parent.parent
PRELUDE = fov.PRELUDE.replace(
    'boolean mirrored;real z_near,z_far;};',
    'boolean mirrored;real z_near,z_far;union rectangle2d {struct {short y0,x0,y1,x1;};short v[4];} window_bounds,viewport_bounds;};')
PRELUDE += r'''
typedef unsigned pixel32;
typedef struct {short x,y;} point2d;
typedef struct {real x,y;} real_point2d;
typedef struct {real i,j;} real_vector2d;
typedef struct {real x0,y0,x1,y1;} real_rectangle2d;
struct bitmap_data {short width,height;point2d registration_point;};
struct hud_absolute_placement_definition {short corner;};
struct hud_placement_definition {point2d offset;real_vector2d scale;short multiplayer_scaling_flags;};
struct dynamic_screen_vertex {real_point2d position,texture_coordinates;pixel32 color;};
struct rasterizer_dynamic_screen_geometry_parameters {
 real_vector2d map_texture_scale[1],map_scale[1];void *meter_parameters;
 int point_sampled,framebuffer_blend_function;struct bitmap_data *map[1];
};
static struct {short local_player_index;struct render_camera camera;} render;
static struct dynamic_screen_vertex drawn[4];
static struct rasterizer_dynamic_screen_geometry_parameters parameters_drawn;
enum {_hud_anchor_top_left,_hud_anchor_top_right,_hud_anchor_bottom_left,_hud_anchor_bottom_right,_hud_anchor_center};
#define _hud_anchor_right_bit 0
#define _hud_anchor_bottom_bit 1
#define _hud_dont_scale_offset_bit 0
#define _shader_framebuffer_blend_function_alpha_multiply_add 1
#define FLAG(n) (1u<<(n))
#define STACK_BUFFER_LENGTH 128
#define csmemset memset
#define match_assert(file,line,c) CHECK(c)
#define match_assert_stack_frame(file,line) ((void)0)
static long get_return_eip(void){return 0;}
static long fast_ftol(real v){return (long)lrintf(v);}
static int players=1;
static int local_player_count(void){return players;}
static real hud_globals_get_scale(boolean multi){return multi?.5f:1.f;}
static void rasterizer_psuedo_dynamic_screen_quad_draw(
 struct rasterizer_dynamic_screen_geometry_parameters *p,struct dynamic_screen_vertex *v){
 memcpy(drawn,v,sizeof(drawn));parameters_drawn=*p;
}
'''
TESTS = r'''
static unsigned probes;
static float native(float degrees){return 2*atan2f(.75f*render_camera_get_adjusted_field_of_view_tangent(DEGREES_TO_RADIANS(degrees)),1.f);}
static void reset(void){
 cinematic=scripted=0;
 for(int i=0;i<4;i++){
  memset(&units[i],0,sizeof(units[i]));units[i].definition_index=i;units[i].object.parent_object_index=NONE;
  unit_indices[i]=weapon_indices[i]=i;perspectives[i]=inhibited[i]=0;magnifications[i]=2;
  definitions[i].unit.camera_field_of_view=DEGREES_TO_RADIANS(70);
 }
}
static void set(double value){setting=value;changes++;}
static void draw(int eligible,int corner,int interface_bitmap,int multi,float theta){
 struct bitmap_data bitmap={48,32,{0,0}};
 struct hud_absolute_placement_definition anchor={corner};
 struct hud_placement_definition placement={{13,-9},{1.25f,.75f},0};
 real_rectangle2d clip=interface_bitmap?(real_rectangle2d){4,3,40,25}:(real_rectangle2d){.125f,.25f,.875f,.75f};
 players=multi?2:1;
 hud_draw_bitmap_with_meter(NULL,&bitmap,&anchor,&placement,&clip,multi?.5f:1.f,theta,0x87654321,multi,interface_bitmap,eligible);
}
static void geometry(float ratio){
 float aspects[]={4.f/3,16.f/9,21.f/9,8.f/9,16.f/27};
 for(int a=0;a<5;a++)for(int multi=0;multi<2;multi++)for(int linear=0;linear<2;linear++)for(int rotation=0;rotation<2;rotation++){
  int width=(int)(480*aspects[a]);
  render.camera.window_bounds=(union rectangle2d){{21,37,501,(short)(37+width)}};
  render.camera.viewport_bounds=(union rectangle2d){{7,11,527,(short)(51+width)}};
  float cx=(render.camera.window_bounds.x0+render.camera.window_bounds.x1)/2-render.camera.viewport_bounds.x0;
  float cy=(render.camera.window_bounds.y0+render.camera.window_bounds.y1)/2-render.camera.viewport_bounds.y0;
  float theta=rotation?.37f:0;
  draw(0,_hud_anchor_center,linear,multi,theta);
  struct dynamic_screen_vertex baseline[4];memcpy(baseline,drawn,sizeof(drawn));
  draw(1,_hud_anchor_center,linear,multi,theta);
  for(int v=0;v<4;v++){
   CHECK(fabsf(drawn[v].position.x-(cx+(baseline[v].position.x-cx)*ratio))<.0002f);
   CHECK(fabsf(drawn[v].position.y-(cy+(baseline[v].position.y-cy)*ratio))<.0002f);
   CHECK(!memcmp(&drawn[v].texture_coordinates,&baseline[v].texture_coordinates,sizeof(real_point2d)));
   CHECK(drawn[v].color==baseline[v].color);
  }
  if(ratio==1)CHECK(!memcmp(baseline,drawn,sizeof(drawn)));
  /* Corners are HUD layout, not the aiming anchor. */
  for(int corner=0;corner<4;corner++){
   draw(0,corner,linear,multi,theta);memcpy(baseline,drawn,sizeof(drawn));
   draw(1,corner,linear,multi,theta);CHECK(!memcmp(baseline,drawn,sizeof(drawn)));
  }
  probes++;
 }
}
static void ratios(void){
 reset();
 CHECK(render_fov_reticle_scale(-1)==1&&render_fov_reticle_scale(4)==1);
 for(int i=0;i<4;i++)CHECK(render_fov_reticle_scale(i)==1);
 for(int degrees=20;degrees<=150;degrees+=5)for(int p=0;p<4;p++)for(int mag=2;mag<=10;mag+=2){
  set(degrees+.125*(degrees!=150));magnifications[p]=mag;render.local_player_index=p;
  float b=native(70),s=native(70.f/mag);
  for(int step=0;step<=20;step++){
   float stock=s+(b-s)*step/20.f;
   float adjusted=render_fov_vertical(p,stock),ratio=render_fov_reticle_scale(p);
   double expected=tan((double)stock/2)/tan((double)adjusted/2);
   CHECK(isfinite(ratio)&&ratio>0&&fabs(ratio-expected)<.00001);
   /* A ray's on-screen distance must change by the same amount. */
   double ray=.03125;
   CHECK(fabs((ray/tan(stock/2))*ratio-ray/tan(adjusted/2))<.00001);
   if(adjusted==stock)CHECK(ratio==1);
   if(step==0||step==20)geometry(ratio);
   probes++;
  }
 }
 /* Each player retains their own last rendered projection. */
 reset();float saved[4];
 for(int p=0;p<4;p++){set(80+20*p);render_fov_vertical(p,native(70));saved[p]=render_fov_reticle_scale(p);}
 for(int p=0;p<4;p++)CHECK(saved[p]==render_fov_reticle_scale(p));
 /* A config edit alone cannot change the HUD before its camera is rebuilt. */
 set(20);for(int p=0;p<4;p++)CHECK(saved[p]==render_fov_reticle_scale(p));
 /* Native, invalid and excluded views reset a previous custom ratio. */
 for(int mode=0;mode<10;mode++){
  reset();set(100);render_fov_vertical(0,native(70));CHECK(render_fov_reticle_scale(0)<1);
  switch(mode){case 0:set(0);break;case 1:cinematic=1;break;case 2:scripted=1;break;
   case 3:perspectives[0]=1;break;case 4:inhibited[0]=1;break;case 5:units[0].object.parent_object_index=1;break;
   case 6:units[0].object.damage_flags=1;break;case 7:unit_indices[0]=NONE;break;case 8:set(NAN);break;case 9:set(INFINITY);break;}
  render_fov_vertical(0,native(70));CHECK(render_fov_reticle_scale(0)==1);
 }
 reset();set(100);float invalid[]={0,NAN,INFINITY,-INFINITY,_pi};
 for(int i=0;i<5;i++){render_fov_vertical(0,invalid[i]);CHECK(render_fov_reticle_scale(0)==1);}
 set(0);render_fov_vertical(0,native(70));render.local_player_index=0;geometry(1);
}
int main(void){ratios();printf("PASS: %u production reticle projection/vertex probes; 20-150, zoom, four local views, sprite/linear bitmaps, multipart offsets, split-screen, rotation, exact native/HUD fallbacks and stable per-view cache\n",probes);}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cc', default='clang')
    args = parser.parse_args()
    camera = (ROOT/'port/linux/game/render_fov.c').read_text()
    hud = (ROOT/'source/interface/hud_draw.c').read_text()
    source = PRELUDE + fov.block((ROOT/'source/render/render_cameras.c').read_text(), 'real render_camera_get_adjusted_field_of_view_tangent(')
    source += '\n' + fov.block((ROOT/'source/items/weapons.c').read_text(), 'real weapon_get_field_of_view(')
    source += '\nstatic real reticle_scales[MAXIMUM_LOCAL_PLAYERS];\n'
    for marker in ('static float render_fov_adjust(', 'float render_fov_vertical(', 'float render_fov_reticle_scale('):
        source += fov.block(camera, marker) + '\n'
    for marker in ('void hud_calculate_point(', 'static void hud_calculate_bitmap_bounds(', 'static void hud_draw_bitmap_internal(', 'static void hud_draw_bitmap_with_meter('):
        # Skip file-local forward declarations before extracting definitions.
        source += fov.block(hud[hud.index('/* ---------- public code */'):], marker) + '\n'
    source += TESTS
    flags = ['-std=gnu11', '-O2', '-fno-builtin', '-fuse-ld=lld']
    flags += ['--target=i686-pc-windows-msvc'] if sys.platform == 'win32' else ['-m32', '-lm']
    with tempfile.TemporaryDirectory(prefix='halo-reticle-fov-') as directory:
        root = Path(directory)
        (root/'test.c').write_text(source)
        exe = root/'test.exe'
        subprocess.run([args.cc, *flags, str(root/'test.c'), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=30)


if __name__ == '__main__':
    main()
