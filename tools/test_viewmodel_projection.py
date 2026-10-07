"""Exercise production viewmodel projection scopes with a fake renderer.

The fake frustum builder isolates scope/restore behavior; full game startup
and rendered checks additionally validate the native renderer integration.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRELUDE = r'''
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float real;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define MAXIMUM_LOCAL_PLAYERS 4
#define _director_perspective_first_person 0
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {float x0,y0,x1,y1;} real_rectangle2d;
struct render_camera{float vertical_field_of_view,aspect;float unchanged[12];};
struct render_frustum{real_rectangle2d frustum_bounds;float scale_x,scale_y;float unchanged[12];float projection_matrix[4][4];int projection_valid;};
static struct {short local_player_index;struct render_camera camera;struct render_frustum frustum;}render;
static struct {short rasterizer_target;struct render_camera camera;struct render_frustum frustum;}global_window_parameters;
static int cinematic,scripted,perspective,inhibited,uploads,builds;
static boolean *director_camera_scripted=&scripted;
static unsigned long generation;
static double desired;
static int config_reads;
static float uploaded_angle;
static double config_real(const char*n){CHECK(!strcmp(n,"display.viewmodel_fov"));config_reads++;return desired;}
static unsigned long config_changes(void){return generation;}
static int cinematic_in_progress(void){return cinematic;}
static int director_get_perspective(short p){CHECK(p>=0&&p<4);return perspective;}
static int director_inhibited_facing(short p){(void)p;return inhibited;}
#define match_assert(file,line,condition) CHECK(condition)
static void render_camera_build_frustum(const struct render_camera*c,const real_rectangle2d*b,struct render_frustum*f,int project){
 CHECK(project);f->frustum_bounds=*b;
 /* The production builder's projection scale/centre arithmetic. These are
 normalized crop bounds, distinct from get_projection_bounds' ray slopes. */
 float half_x=(b->x1-b->x0)*.5f,half_y=(b->y1-b->y0)*.5f,t=tanf(c->vertical_field_of_view*.5f);
 f->scale_x=1/(half_x*c->aspect*t);f->scale_y=1/(half_y*t);
 f->projection_matrix[0][0]=f->scale_x;f->projection_matrix[1][1]=f->scale_y;
 f->projection_matrix[2][0]=-(b->x0+b->x1)/(2*half_x);f->projection_matrix[2][1]=-(b->y0+b->y1)/(2*half_y);
 f->projection_valid=1;builds++;
}
static void rasterizer_set_frustum_z(float n,float f){CHECK(n==0&&f==0);uploaded_angle=global_window_parameters.camera.vertical_field_of_view;uploads++;}
'''
TESTS = r'''
static void set(double d){desired=d;generation++;}
int main(void){
 memset(&render,0,sizeof(render));memset(&global_window_parameters,0,sizeof(global_window_parameters));
 render.camera.vertical_field_of_view=.9f;render.camera.aspect=16.f/9;render.frustum.frustum_bounds=(real_rectangle2d){-.8f,-.7f,.9f,1};
 render_camera_build_frustum(&render.camera,&render.frustum.frustum_bounds,&render.frustum,1);
 global_window_parameters.camera=render.camera;global_window_parameters.frustum=render.frustum;
 struct render_camera rc=render.camera,gc=global_window_parameters.camera;
 struct render_frustum rf=render.frustum,gf=global_window_parameters.frustum;
 double bad[]={0,-1,19.99,150.01,NAN,INFINITY};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){set(bad[i]);viewmodel_projection_begin();viewmodel_projection_end();CHECK(!memcmp(&rc,&render.camera,sizeof(rc)));CHECK(!uploads);}
 set(100);
 for(int excluded=0;excluded<7;excluded++){
  cinematic=excluded==0;scripted=excluded==1;perspective=excluded==2;inhibited=excluded==3;
  global_window_parameters.rasterizer_target=excluded==4;render.local_player_index=excluded==5?-1:excluded==6?4:0;
  viewmodel_projection_begin();viewmodel_projection_end();CHECK(!uploads);CHECK(!memcmp(&rc,&render.camera,sizeof(rc)));
 }
 cinematic=scripted=perspective=inhibited=0;global_window_parameters.rasterizer_target=0;
 unsigned probes=0;
 float aspects[]={4.f/3,16.f/9,21.f/9,1.f,3.f/4};
 for(int p=0;p<4;p++)for(int a=0;a<5;a++)for(int degrees=20;degrees<=150;degrees++){
  render.local_player_index=p;rc.aspect=gc.aspect=aspects[a];render.camera=rc;global_window_parameters.camera=gc;
  set(degrees);int old_uploads=uploads,old_reads=config_reads;
  viewmodel_projection_begin();float angle=global_window_parameters.camera.vertical_field_of_view;
  CHECK(isfinite(angle)&&angle>0&&angle<3.141593f);CHECK(fabsf(2*atanf(tanf(angle/2)*(16.f/9))*180/3.141593f-degrees)<.0001f);
  CHECK(render.camera.vertical_field_of_view==angle);CHECK(config_reads==old_reads+1);CHECK(uploads==old_uploads+1&&uploaded_angle==angle);
  CHECK(!memcmp(&rf.frustum_bounds,&render.frustum.frustum_bounds,sizeof(rf.frustum_bounds)));CHECK(!memcmp(&gf.frustum_bounds,&global_window_parameters.frustum.frustum_bounds,sizeof(gf.frustum_bounds)));
  CHECK(render.frustum.scale_x>0&&render.frustum.scale_y>0);
  CHECK(fabsf(render.frustum.scale_x-1/(((rf.frustum_bounds.x1-rf.frustum_bounds.x0)*.5f)*rc.aspect*tanf(angle*.5f)))<.00001f);
  CHECK(!memcmp(rc.unchanged,render.camera.unchanged,sizeof(rc.unchanged)));
  viewmodel_projection_begin();CHECK(config_reads==old_reads+1);CHECK(uploads==old_uploads+1);
  viewmodel_projection_end();CHECK(global_window_parameters.camera.vertical_field_of_view==angle);
  viewmodel_projection_end();CHECK(uploads==old_uploads+2);CHECK(uploaded_angle==gc.vertical_field_of_view);
  CHECK(!memcmp(&rc,&render.camera,sizeof(rc)));CHECK(!memcmp(&gc,&global_window_parameters.camera,sizeof(gc)));
  CHECK(!memcmp(&rf,&render.frustum,sizeof(rf)));CHECK(!memcmp(&gf,&global_window_parameters.frustum,sizeof(gf)));probes++;
 }
 int old_uploads=uploads;viewmodel_projection_end();CHECK(uploads==old_uploads);
 printf("PASS: %u production viewmodel scope probes; four views/five aspects, 20-150, exact restore, cropped bounds, nesting, live config, shader uploads and excluded cameras\n",probes);
}
'''


def main():
    p=argparse.ArgumentParser();p.add_argument('--cc',default='clang');a=p.parse_args()
    source=(ROOT/'port/linux/game/viewmodel_fov.c').read_text()
    source=re.sub(r'^#include.*\n','',source,flags=re.M)
    cameras=(ROOT/'source/render/render_cameras.c').read_text()
    start=cameras.index('void render_frustum_get_projection_bounds(')
    end=cameras.index('\nvoid render_camera_screen_to_world(',start)
    getter=cameras[start:end]
    flags=['-std=gnu11','-O2','-fuse-ld=lld']+(['--target=i686-pc-windows-msvc'] if sys.platform=='win32' else ['-lm'])
    for path in ('source/rasterizer/xbox/rasterizer_xbox_models.c','source/rasterizer/rasterizer_transparent_geometry.c',
                 'source/render/render_sprite.c','source/rasterizer/rasterizer_lights.c'):
        s=(ROOT/path).read_text();assert 'viewmodel_projection_begin();' in s and 'viewmodel_projection_end();' in s
    with tempfile.TemporaryDirectory(prefix='halo-viewmodel-fov-') as directory:
        d=Path(directory);(d/'test.c').write_text(PRELUDE+getter+source+TESTS)
        exe=d/'test.exe';subprocess.run([a.cc,*flags,str(d/'test.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=20)


if __name__=='__main__':main()
