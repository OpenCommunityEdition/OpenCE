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
struct render_frustum{real_rectangle2d bounds;float scale_x,scale_y;float unchanged[12];};
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
static void render_frustum_get_projection_bounds(const struct render_frustum*f,real_rectangle2d*b){*b=f->bounds;}
static void render_camera_build_frustum(const struct render_camera*c,const real_rectangle2d*b,struct render_frustum*f,int project){CHECK(project);f->bounds=*b;f->scale_y=1/tanf(c->vertical_field_of_view/2);f->scale_x=f->scale_y/c->aspect;builds++;}
static void rasterizer_set_frustum_z(float n,float f){CHECK(n==0&&f==0);uploaded_angle=global_window_parameters.camera.vertical_field_of_view;uploads++;}
'''
TESTS = r'''
static void set(double d){desired=d;generation++;}
int main(void){
 memset(&render,0,sizeof(render));memset(&global_window_parameters,0,sizeof(global_window_parameters));
 render.camera.vertical_field_of_view=.9f;render.camera.aspect=16.f/9;render.frustum.bounds=(real_rectangle2d){-.8f,-.7f,.9f,1};
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
  CHECK(!memcmp(&rf.bounds,&render.frustum.bounds,sizeof(rf.bounds)));CHECK(!memcmp(&gf.bounds,&global_window_parameters.frustum.bounds,sizeof(gf.bounds)));
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
    flags=['-std=gnu11','-O2','-fuse-ld=lld']+(['--target=i686-pc-windows-msvc'] if sys.platform=='win32' else ['-lm'])
    for path in ('source/rasterizer/xbox/rasterizer_xbox_models.c','source/rasterizer/rasterizer_transparent_geometry.c',
                 'source/render/render_sprite.c','source/rasterizer/rasterizer_lights.c'):
        s=(ROOT/path).read_text();assert 'viewmodel_projection_begin();' in s and 'viewmodel_projection_end();' in s
    with tempfile.TemporaryDirectory(prefix='halo-viewmodel-fov-') as directory:
        d=Path(directory);(d/'test.c').write_text(PRELUDE+source+TESTS)
        exe=d/'test.exe';subprocess.run([a.cc,*flags,str(d/'test.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=20)


if __name__=='__main__':main()
