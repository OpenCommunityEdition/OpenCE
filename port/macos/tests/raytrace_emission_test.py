#!/usr/bin/env python3
"""Execute production energy extraction against small authored shader fixtures.

The Jackal generic shader and Shade beam both use RGB input 13 constants,
with A-out animation. These numerical fixtures reproduce that structure;
no game asset data is bundled. Native attached projectile lights are separate.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
source = (ROOT / 'port/linux/game/raytrace_world.c').read_text()

def function(name):
    match = re.search(r'static boolean ' + name + r'\([^;]*?\)\s*\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

code = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;
typedef int boolean;
#define FALSE 0
#define TRUE 1
#define MAX(a,b) ((a)>(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
struct tag_block { int count; void *address; void *definition; };
struct object_datum { struct { float outgoing_function_values[4];
 struct {float n[3];} outgoing_change_colors[4]; } object; };
static struct { float game_time_sec; } global_frame_parameters;
static float periodic_function_evaluate(short function, float time) { return time; }
''' + source[source.index('struct ray_generic_energy_stage'):source.index('static boolean generic_energy_color(')] + function('generic_energy_color') + '\n' + function('plasma_energy_color') + r'''
static void f(byte *data, int offset, float value) { memcpy(data+offset,&value,4); }
static void h(byte *data, int offset, short value) { memcpy(data+offset,&value,2); }
static void near(float a,float b) { assert(fabsf(a-b)<1e-6f); }
int main(void) {
 _Alignas(16) byte shader[512]={0}, stages[7*112]={0};
 struct object_datum object={0}; float color[3];
 struct tag_block *block=(struct tag_block *)(shader+0x60);
 block->count=7; block->address=stages; h(shader,0x2C,3);
 byte *shield=stages+3*112;
 h(shield,0,6);h(shield,4,3);f(shield,8,1);h(shield,0x40,13);
 for(int k=0;k<3;k++) {f(shield,16+k*4,1);f(shield,32+k*4,1);}
 f(shield,28,1);
 object.outgoing_change_colors[2].n[0]=.1f;
 object.outgoing_change_colors[2].n[1]=.5f;
 object.outgoing_change_colors[2].n[2]=1;
 assert(!generic_energy_color(shader,&object,color));
 object.object.outgoing_function_values[0]=1;
 assert(generic_energy_color(shader,&object,color)); near(color[0],.1f);near(color[1],.5f);near(color[2],1);
 object.object.outgoing_function_values[0]=.5f;
 assert(generic_energy_color(shader,&object,color));near(color[0],.05f);near(color[2],.5f);
 h(shader,0x2C,0);assert(!generic_energy_color(shader,&object,color));h(shader,0x2C,3);
 memset(stages,0,sizeof(stages));byte *beam=stages+6*112;
 h(beam,0,4);f(beam,8,1);h(beam,0x44,13);
 f(beam,20,.329412f);f(beam,24,.423529f);
 for(int k=0;k<3;k++)f(beam,32+k*4,1);
 object.object.outgoing_function_values[0]=0;
 assert(generic_energy_color(shader,&object,color));near(color[0],0);near(color[1],.329412f);near(color[2],.423529f);
 object.object.outgoing_function_values[0]=1;
 assert(generic_energy_color(shader,&object,color));near(color[0],1);near(color[2],1);
 object.object.outgoing_function_values[0]=NAN;assert(!generic_energy_color(shader,&object,color));
 memset(shader,0,sizeof(shader));h(shader,0x2C,2);f(shader,0x30,2);h(shader,0xA8,3);
 f(shader,0x60,1);f(shader,0x64,1);f(shader,0x68,1);f(shader,0x6C,1);
 object.object.outgoing_function_values[1]=.5f;
 assert(plasma_energy_color(shader,&object,color));near(color[0],.025f);near(color[1],.125f);near(color[2],.25f);
 object.object.outgoing_function_values[1]=0;assert(!plasma_energy_color(shader,&object,color));
 object.object.outgoing_function_values[1]=NAN;assert(!plasma_energy_color(shader,&object,color));
 puts("PASS authored additive shield/beam activation and tint; plasma runtime power and finite bounds");
}
'''
# Retain this exact production insertion block to check a late close emitter
# can displace an earlier distant one without writing past a full budget.
start=source.index('\t\t{\n\t\t\tlong insertion = 0;', source.index('long halo_ray_tracing_emitters('))
end=source.index('\n\t\tout[0] = at->x;',start)
insertion=source[start:end]
code=code.replace('int main(void) {', r'''
static void sorted(float *emitters,long maximum,const float *camera,float dx,float dy,float dz,long *size) {
 long count=*size;float *out; do {
''' + insertion + r'''
 out[0]=camera[0]+dx;out[1]=camera[1]+dy;out[2]=camera[2]+dz;
 } while(0);*size=count;
}
int main(void) {
 float records[18]={0};long size=0;const float camera[3]={0};records[16]=123;records[17]=456;
 sorted(records,2,camera,20,0,0,&size);sorted(records,2,camera,10,0,0,&size);
 sorted(records,2,camera,1,0,0,&size);assert(size==2);near(records[0],1);near(records[8],10);
 sorted(records,2,camera,30,0,0,&size);near(records[0],1);near(records[8],10);
 near(records[16],123);near(records[17],456);
''')
# The fixture uses the real datum's nested object members.
code=code.replace(' object.outgoing_change_colors',' object.object.outgoing_change_colors')
with tempfile.TemporaryDirectory() as directory:
    source_path=Path(directory)/'emission.c'; binary=Path(directory)/'emission'
    source_path.write_text(code)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-parameter',str(source_path),'-lm','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
