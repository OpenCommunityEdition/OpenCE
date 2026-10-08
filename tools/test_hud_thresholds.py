"""Render the real NV2A meter shader and committed atlas in a hidden GL context.

Exercises all health levels, all four health sprites, several HUD scales and
fractional placement. Also compares continuous meters and the original HUD
shader path. No game assets or player profile are needed.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image

from hud_assets import add_meter_thresholds

ROOT = Path(__file__).resolve().parent.parent


def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PRELUDE = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
typedef unsigned long DWORD, D3DCOLOR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define __HALO_LINUX_PLATFORM_H
#define CHECK(c) do { if (!(c)) {fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);} } while(0)
'''

HARNESS = r'''
#include "xgpu.h"
#define GL_DEFINE(name) __typeof__(name) name;
GL_FUNCTIONS(GL_DEFINE)
#undef GL_DEFINE
int config_boolean(const char *name) {return 0;}
const char *config_string(const char *name) {return "";}
static DWORD real_alpha_to_pixel32(float a) {return (DWORD)(a*255.0f+0.5f)<<24;}
#define MAX(a,b) ((a)>(b)?(a):(b))
struct meter_parameters {
 DWORD gradient_min_color, gradient_max_color, flash_color, background_color, tint_color;
 float gradient; BOOL flash_color_is_negative;
};
static struct nv2a_pixel_shader_key meter_key(struct meter_parameters *meter, BOOL point) {
 struct nv2a_pixel_shader_key key={0};
 key.sampler_type[0]=_xgpu_sampler_2d;
 key.alpha_kill[0]=1; key.coverage_alpha=1; key.point_threshold=point;
 METER_SETUP
 return key;
}
static GLuint shader(GLenum type,const char *text) {
 GLuint s=glCreateShader(type);glShaderSource(s,1,&text,NULL);glCompileShader(s);
 GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[8192];glGetShaderInfoLog(s,sizeof(log),NULL,log);fprintf(stderr,"%s\n%s",log,text);exit(1);}return s;
}
static GLuint program(struct nv2a_pixel_shader_key *key) {
 const char *vs="#version 450 core\n"
 "uniform vec4 uv_rect; out vec4 xD0,xD1,xB0,xB1,xT0,xT1,xT2,xT3;out float xFog;"
 "void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);"
 "gl_Position=vec4(p*2.0-1.0,0,1);xT0=vec4(uv_rect.xy+p*uv_rect.zw,0,1);"
 "xD0=xD1=xB0=xB1=xT1=xT2=xT3=vec4(0);xFog=0;}";
 char *fs=nv2a_pixel_shader_to_glsl(key);GLuint v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);
 free(fs);GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
 GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);CHECK(ok);glDeleteShader(v);glDeleteShader(f);return p;
}
static void constants(GLuint p,struct nv2a_pixel_shader_key *key) {
 float c[8][4];
 for(int bank=0;bank<2;bank++) {
  for(int i=0;i<8;i++) {
   DWORD v=key->combiner_state[D3DRS_PSCONSTANT0_0+8*bank+i];
   c[i][0]=((v>>16)&255)/255.0f;c[i][1]=((v>>8)&255)/255.0f;c[i][2]=(v&255)/255.0f;c[i][3]=((v>>24)&255)/255.0f;
  }
  glUniform4fv(glGetUniformLocation(p,bank?"ps_c1":"ps_c0"),8,(float*)c);
 }
 float scale[4][4]={{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
 glUniform4fv(glGetUniformLocation(p,"texture_scale"),4,(float*)scale);
 glUniform1i(glGetUniformLocation(p,"tex0"),0);
}
static void draw(GLuint p,struct nv2a_pixel_shader_key *key,int *rect,int w,int h,float shift,unsigned char *out) {
 glUseProgram(p);constants(p,key);
 float uv[4]={(rect[0]+shift)/2048.0f,(rect[1]+shift)/2048.0f,
  (rect[2]-rect[0])/2048.0f,(rect[3]-rect[1])/2048.0f};
 glUniform4fv(glGetUniformLocation(p,"uv_rect"),1,uv);
 glViewport(0,0,w,h);glClearColor(32/255.0f,48/255.0f,64/255.0f,1);glClear(GL_COLOR_BUFFER_BIT);
 glEnable(GL_BLEND);glBlendColor(1,1,1,1);glBlendFunc(GL_CONSTANT_COLOR,GL_SRC_ALPHA);
 glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,out);
 CHECK(glGetError()==GL_NO_ERROR);
}
int main(int argc,char **argv) {
 CHECK(SDL_Init(SDL_INIT_VIDEO));SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,5);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 SDL_Window *window=SDL_CreateWindow("HUD regression",16,16,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);CHECK(window);
 SDL_GLContext context=SDL_GL_CreateContext(window);CHECK(context);
#define LOAD(name) name=(__typeof__(name))SDL_GL_GetProcAddress(#name);CHECK(name);
 GL_FUNCTIONS(LOAD)
#undef LOAD
 unsigned char *atlas=malloc(2048*2048*4),*out=malloc(1040*272*4),*reference=malloc(1040*272*4);
 FILE *input=fopen(argv[1],"rb");CHECK(input);CHECK(fread(atlas,1,2048*2048*4,input)==2048*2048*4);fclose(input);
 GLuint texture,target,fbo,vao;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2048,2048,0,GL_RGBA,GL_UNSIGNED_BYTE,atlas);glGenerateMipmap(GL_TEXTURE_2D);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1040,272,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
 glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);
 CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glBindTexture(GL_TEXTURE_2D,texture);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glDisable(GL_DITHER);
 struct meter_parameters meter={.gradient_min_color=0xffffffff,.gradient_max_color=0xffffff,
  .flash_color=0xff000000,.background_color=0xff000000,.tint_color=0xbfffffff,.gradient=1};
 struct nv2a_pixel_shader_key key=meter_key(&meter,TRUE),legacy=meter_key(&meter,FALSE);
 GLuint fixed=program(&key),old=program(&legacy);
 int rects[][4]={HEALTH_RECTS};int divisions[]={1,2,3,4,8};int renders=0,old_ghosts=0;
 for(int s=0;s<4;s++) for(int size=0;size<5;size++) for(int offset=0;offset<2;offset++) for(int bars=0;bars<=8;bars++) {
  int *r=rects[s],w=(r[2]-r[0])/divisions[size],h=(r[3]-r[1])/divisions[size];
  int q=bars?bars*30+15:31;float shift=offset?0.37f:0;
  meter.gradient_min_color=(q<<24)|0xffffff;meter.flash_color=q<<24;
  key=meter_key(&meter,TRUE);legacy=meter_key(&meter,FALSE);
  draw(fixed,&key,r,w,h,shift,out);draw(old,&legacy,r,w,h,shift,reference);
  int filled=0;
  for(int y=0;y<h;y++)for(int x=0;x<w;x++) {
   float px=r[0]+shift+(x+0.5f)*(r[2]-r[0])/w,py=r[1]+shift+(y+0.5f)*(r[3]-r[1])/h;
   int ax=(int)floorf(px),ay=(int)floorf(py);
   unsigned char threshold=atlas[4*(ay*2048+ax)];int at=4*(y*w+x);
   CHECK(threshold>0);
   /* Interpolation can land either side of an exact texel boundary.
      Verify only unambiguous categorical expectations at those ties. */
   int ambiguous=0;
   for(int dy=-1;dy<=1;dy+=2)for(int dx=-1;dx<=1;dx+=2) {
    int tx=(int)floorf(px+dx*0.001f),ty=(int)floorf(py+dy*0.001f);
    if((atlas[4*(ty*2048+tx)]>=q)!=(threshold>=q))ambiguous=1;
   }
   if(ambiguous)continue;
   if(threshold>=q) {
    if(out[at]!=32 || out[at+1]!=48 || out[at+2]!=64) fprintf(stderr,"sprite=%d division=%d shift=%f bars=%d x=%d y=%d atlas=%d,%d threshold=%d q=%d rgb=%d,%d,%d\n",s,divisions[size],shift,bars,x,y,ax,ay,threshold,q,out[at],out[at+1],out[at+2]);
    CHECK(out[at]==32 && out[at+1]==48 && out[at+2]==64);
    if(reference[at]!=32 || reference[at+1]!=48 || reference[at+2]!=64)old_ghosts++;
   } else if(out[at]>32)filled++;
  }
  CHECK(filled>0);renders++;
 }
 CHECK(old_ghosts>0);
 /* Red zero in continuous sprites: new sampling must produce identical output. */
 int continuous[][4]={{0,272,976,520},{1040,0,2016,248},{0,632,1040,712},{976,384,2016,464}};
 for(int s=0;s<4;s++)for(int q=32;q<256;q+=31) {
  int *r=continuous[s],w=(r[2]-r[0])/4,h=(r[3]-r[1])/4;
  meter.gradient_min_color=(q<<24)|0x509f;meter.gradient_max_color=0x2896ff;meter.flash_color=q<<24;
  key=meter_key(&meter,TRUE);legacy=meter_key(&meter,FALSE);
  draw(fixed,&key,r,w,h,0.37f,out);draw(old,&legacy,r,w,h,0.37f,reference);
  CHECK(!memcmp(out,reference,w*h*4));
 }
 /* The unmodified Xbox/disabled-hires shader has no extra fetch. */
 char *text=nv2a_pixel_shader_to_glsl(&legacy);CHECK(!strstr(text,"texelFetch"));free(text);
 printf("PASS: %d real GL health renders, levels 0-8, full/split HUDs, five scales, fractional placement; %d legacy ghost samples eliminated; continuous meters identical\n",renders,old_ghosts);
 glDeleteTextures(1,&texture);glDeleteTextures(1,&target);glDeleteFramebuffers(1,&fbo);
 free(atlas);free(out);free(reference);SDL_GL_DestroyContext(context);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    args = parser.parse_args()
    entry = next(e for e in json.loads((ROOT / 'port/assets/hud/layout.json').read_text())['assets']
                 if e['name'] == 'hud_unit_meters__0')
    atlas = np.asarray(Image.open(ROOT / 'port/assets/hud/hud_unit_meters__0.png')).copy()
    rebuilt = add_meter_thresholds(atlas.copy(), entry)
    assert np.array_equal(atlas, rebuilt), 'committed thresholds must match the generator'
    rects = [[value * entry['scale'] for value in c['xbox']] for c in entry['cells'] if c.get('thresholds')]
    assert len(rects) == 4
    for rect in rects:
        x, y, right, bottom = rect
        assert set(np.unique(atlas[y:bottom, x:right, 0])) == set(range(30, 241, 30))
    for c in entry['cells']:
        if not c.get('thresholds'):
            x, y, right, bottom = [value * entry['scale'] for value in c['xbox']]
            assert not atlas[y:bottom, x:right, 0].any()
    # Use the game's actual combiner setup, rather than a substitute shader.
    source = (ROOT / 'source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c').read_text()
    setup = source[source.index('pixel_shader.texture_modes = 1;'):source.index('\n\t}\n\telse if (parameters->map[0])')]
    fields = {'texture_modes': 'key.texture_modes', 'combiner_count': 'key.combiner_state[D3DRS_PSCOMBINERCOUNT]',
              'final_combiner_inputs_abcd': 'key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD]',
              'final_combiner_inputs_efg': 'key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG]'}
    for name, index in {'constant_0': 'PSCONSTANT0_0', 'constant_1': 'PSCONSTANT1_0',
                        'alpha_inputs': 'PSALPHAINPUTS0', 'rgb_inputs': 'PSRGBINPUTS0',
                        'alpha_outputs': 'PSALPHAOUTPUTS0', 'rgb_outputs': 'PSRGBOUTPUTS0'}.items():
        setup = re.sub(r'pixel_shader\.' + name + r'\[(\d+)\]', r'key.combiner_state[D3DRS_' + index + r'+\1]', setup)
    for name, expression in fields.items():
        setup = setup.replace('pixel_shader.' + name, expression)
    assert 'pixel_shader.' not in setup
    enums = (ROOT / 'port/include/xdk/xdk_pdb.h').read_text()
    unit = PRELUDE + '\n'.join(block(enums, 'enum ' + name + ' {') + ';' for name in
                              ['_D3DRENDERSTATETYPE', '_D3DCMPFUNC', '_D3DFOGMODE'])
    unit += HARNESS.replace('METER_SETUP', setup).replace('HEALTH_RECTS', ','.join('{' + ','.join(map(str, r)) + '}' for r in rects))
    # Compile the production translator and string builder with only platform/config stubs.
    unit += '\n' + (ROOT / 'port/linux/src/xgpu_text.c').read_text()
    unit += '\n' + (ROOT / 'port/linux/src/nv2a_psh.c').read_text()
    compiler = [args.cc, '-std=gnu11', '-O2', '-fuse-ld=lld', '-I' + str(ROOT / 'port/linux/src')]
    env = dict(os.environ)
    if sys.platform == 'win32':
        sdl = next((ROOT / 'build/windows/third_party').glob('SDL3-*/include')).parent
        compiler += ['--target=i686-pc-windows-msvc', '-I' + str(sdl / 'include')]
        libraries = [str(sdl / 'lib/x86/SDL3.lib')]
        env['PATH'] = str(sdl / 'lib/x86') + os.pathsep + env['PATH']
    else:
        compiler += subprocess.check_output(['pkg-config', '--cflags', 'sdl3'], text=True).split()
        libraries = subprocess.check_output(['pkg-config', '--libs', 'sdl3'], text=True).split() + ['-lm']
    with tempfile.TemporaryDirectory(prefix='halo-hud-test-') as directory:
        path = Path(directory)
        (path / 'hud.c').write_text(unit)
        (path / 'atlas.rgba').write_bytes(atlas.tobytes())
        subprocess.run([*compiler, str(path / 'hud.c'), *libraries, '-o', str(path / 'hud.exe')], check=True)
        subprocess.run([str(path / 'hud.exe'), str(path / 'atlas.rgba')], env=env, check=True, timeout=45)


if __name__ == '__main__':
    main()
