"""Check production dropped-Needler LOD policy and platform exclusions.

No game/profile is opened. --map optionally validates a local Xbox asset;
the asset is read only and is never part of the test or PR.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]


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
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef float real;
#define NONE (-1)
#define REAL_MAX FLT_MAX
#define NUMBER_OF_DETAIL_LEVELS_PER_MODEL 5
#define TEST_FLAG(v,b) (((v)&(1u<<(b)))!=0)
#define ABS(x) fabsf(x)
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#define csstrcmp strcmp
#define _render_model_shadow_bit 1
#define _render_model_first_person_bit 3
#define _object_type_weapon 2
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
struct model {real detail_cutoff_pixels[5];struct {int count;}nodes,geometries;};
struct object_datum {long definition_index;struct {short type;long parent_object_index;}object;};
struct object_definition {struct {struct {long index;}model;}object;};
typedef struct {float x,y,z;} real_point3d;
struct vector{float i,j,k;};
struct render_frustum {struct {struct vector forward,left,up;real_point3d position;}world_to_view;struct vector projection_world_to_screen;};
static struct model model={{0,25,50,50,100},{1},{3}};
static struct object_datum object={2,{2,NONE}};
static struct object_definition definition={{{1}}};
static struct {short debug_model_lod;} rasterizer_debug_options={NONE};
static const char *model_name="weapons\\needler\\needler",*weapon_name="weapons\\needler\\needler";
static int reads;
static const char *tag_get_name(long i){reads++;CHECK(i==1||i==2);return i==1?model_name:weapon_name;}
static struct object_datum *object_try_and_get(long i){reads++;return i==7?&object:NULL;}
static struct object_definition *object_definition_get(long i){CHECK(i==2);return &definition;}
'''

TESTS = r'''
static short legacy(real pixels){short i=4;while(i>0&&pixels<model.detail_cutoff_pixels[i])i--;return i;}
static void unchanged(real pixels,long id,unsigned flags){
 real result=model_detail_selection_pixels(1,&model,id,flags,pixels);
 CHECK(!memcmp(&pixels,&result,sizeof(pixels)));
}
int main(void){
 struct model original_model=model;struct object_datum original_object=object;
 struct object_definition original_definition=definition;
 unsigned checks=0;
 for(int n=0;n<=800;n++){
  real p=n*.25f;
  CHECK(select_detail(p,7,0)==legacy(p*(EXPECT_DESKTOP?5:1)));
  CHECK(select_detail(p,7,1u<<_render_model_shadow_bit)==legacy(p));
  CHECK(select_detail(p,7,1u<<_render_model_first_person_bit)==legacy(p));
  object.object.parent_object_index=42;CHECK(select_detail(p,7,0)==legacy(p));object=original_object;
  checks+=4;
 }
 if(EXPECT_DESKTOP){CHECK(select_detail(20,7,0)==4);CHECK(select_detail(10,7,0)==3);CHECK(select_detail(9.99f,7,0)==1);}
 else CHECK(reads==0);
 unchanged(20,NONE,0);unchanged(20,123,0);
 object.object.type=0;unchanged(20,7,0);object=original_object;
 definition.object.model.index=3;unchanged(20,7,0);definition=original_definition;
 model_name="weapons\\needler\\fp\\fp";unchanged(20,7,0);model_name="weapons\\needler\\needler";
 weapon_name="weapons\\plasma rifle\\plasma rifle";unchanged(20,7,0);weapon_name="weapons\\needler\\needler";
 for(int i=0;i<5;i++){model.detail_cutoff_pixels[i]+=1;unchanged(20,7,0);model=original_model;}
 model.nodes.count=2;unchanged(20,7,0);model=original_model;
 model.geometries.count=4;unchanged(20,7,0);model=original_model;
 real invalid[]={0,-0.f,-1,NAN,INFINITY,-INFINITY,FLT_MAX};
 for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++)unchanged(invalid[i],7,0);
 for(int i=0;i<5;i++){rasterizer_debug_options.debug_model_lod=i;CHECK(select_detail(20,7,0)==i);}
 rasterizer_debug_options.debug_model_lod=NONE;
 /* A model hidden by its authored visibility cutoff stays hidden. */
 model.detail_cutoff_pixels[0]=30;CHECK(select_detail(20,7,0)==NONE);model=original_model;
 CHECK(select_detail(-1,7,0)==NONE);
 for(int view=0;view<4;view++)for(int fov=30;fov<=150;fov+=30)for(int d=1;d<=256;d++){
  struct render_frustum f={0};real_point3d near_point={0,0,(float)d},far_point={0,0,d*5.f};
  f.world_to_view.up.k=1;f.projection_world_to_screen.j=(240.f/(view+1))/tanf(fov*3.14159265358979323846f/360.f);
  real near_pixels=render_frustum_sphere_diameter_in_pixels(&f,&near_point,.3f);
  real far_pixels=render_frustum_sphere_diameter_in_pixels(&f,&far_point,.3f);
  CHECK(select_detail(far_pixels,7,0)==legacy(EXPECT_DESKTOP?near_pixels:far_pixels));checks++;
 }
 CHECK(!memcmp(&model,&original_model,sizeof(model)));
 CHECK(!memcmp(&object,&original_object,sizeof(object)));
 CHECK(!memcmp(&definition,&original_definition,sizeof(definition)));
 printf("PASS: %u LOD probes; desktop=%d, fivefold range, dropped/held, first-person/shadows, custom tags/thresholds, invalid handles/values, unchanged culling/debug override and object/model data\n",checks,EXPECT_DESKTOP);
}
'''


def check_asset(path):
    raw = path.read_bytes()
    data = raw if struct.unpack_from('<I', raw, 8)[0] == len(raw) else raw[:2048] + zlib.decompressobj().decompress(raw[2048:])
    u = lambda p: struct.unpack_from('<I', data, p)[0]
    offset = u(16)
    base = u(offset) - 36
    off = lambda p: p - base + offset
    for i in range(u(offset + 12)):
        tag = offset + 36 + 32*i
        name = off(u(tag + 16))
        if data[name:data.index(b'\0', name)] != b'weapons\\needler\\needler' or data[tag:tag+4] not in (b'edom', b'2dom'):
            continue
        model = off(u(tag + 20))
        assert struct.unpack_from('<5f', data, model + 8) == (0, 25, 50, 50, 100)
        assert u(model + 184) == 1 and u(model + 208) == 3
        assert u(model + 196) == 1
        region = off(u(model + 200))
        assert u(region + 64) == 1
        permutation = off(u(region + 68))
        assert struct.unpack_from('<5h', data, permutation + 64) == (2, 2, 2, 1, 0)
        print('PASS: local world Needler asset has the guarded cutoffs and low/medium/high geometries')
        return
    raise AssertionError('Needler model not found')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--map', type=Path)
    args = parser.parse_args()
    source = (ROOT/'source/models/models.c').read_text()
    helper = block(source, 'static real model_detail_selection_pixels(')
    body = block(source, 'void render_model(')
    gate = 'if (level_of_detail_pixels>=model->detail_cutoff_pixels[0] || TEST_FLAG(flags, _render_model_shadow_bit))'
    assert body.index(gate) < body.index('real geometry_detail_pixels = model_detail_selection_pixels(')
    assert 'model_index, model, unique_identifier, flags, level_of_detail_pixels' in body
    assert body.count('geometry_detail_pixels') == 2
    start = body.index('geometry_detail_level_index = NUMBER_OF_DETAIL_LEVELS_PER_MODEL-1;')
    end = body.index('\n\t\tmatch_assert(', start)
    selector = '''static short select_detail(real level_of_detail_pixels,long object_index,unsigned flags){
        struct model *value=&model;short geometry_detail_level_index;
        GATE {
            real geometry_detail_pixels=model_detail_selection_pixels(1,value,object_index,flags,level_of_detail_pixels);
            SELECTION
            return geometry_detail_level_index;
        }return NONE;
    }
    '''.replace('GATE', gate.replace('model->', 'value->')).replace('SELECTION', body[start:end].replace('model->', 'value->'))
    cameras = (ROOT/'source/render/render_cameras.c').read_text()
    unit = PRELUDE + helper + '\n' + block(cameras, 'real render_frustum_sphere_diameter_in_pixels(') + '\n' + selector + TESTS
    flags = ['-std=gnu11', '-O2', '-fuse-ld=lld'] + (['--target=i686-pc-windows-msvc'] if sys.platform == 'win32' else ['-m32', '-lm'])
    modes = [('Windows', ['-U__linux__', '-DHALO_WINDOWS=1'], 1),
             ('Linux', ['-D__linux__=1'], 1),
             ('Android', ['-D__linux__=1', '-DHALO_ANDROID=1'], 0),
             ('Other', ['-U__linux__'], 0)]
    with tempfile.TemporaryDirectory(prefix='halo-needler-lod-') as directory:
        path = Path(directory)
        (path/'test.c').write_text(unit)
        for name, defines, enabled in modes:
            exe = path/(name+'.exe')
            subprocess.run([args.cc, *flags, *defines, '-DEXPECT_DESKTOP='+str(enabled), str(path/'test.c'), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=15)
    if args.map:
        check_asset(args.map)


if __name__ == '__main__':
    main()
