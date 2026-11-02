"""Real translator variants and the independent omission experiment."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import pytest
from harness import enum_with, function, structure
from render_stutter_build import omit_honeycomb
from render_stutter_run import compare_player_traces, summarize

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('platform', ['desktop', 'android', 'web'])
@pytest.mark.parametrize('omit', [False, True])
def test_honeycomb_translation(tmp_path, platform, omit):
    cc = shutil.which('clang')
    if not cc:
        pytest.skip('clang required')
    header = (ROOT/'port/linux/src/xgpu.h').read_text()
    pdb = (ROOT/'port/include/xdk/xdk_pdb.h').read_text()
    code = '#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <stdarg.h>\n#include <assert.h>\n'
    code += 'typedef unsigned long DWORD;typedef int BOOL;\n#define TRUE 1\n#define FALSE 0\nstatic BOOL xgpu_reversed_depth;\n'
    if platform != 'desktop': code += '#define HALO_GLES 1\n'
    if platform == 'android': code += '#define HALO_ANDROID 1\n'
    if platform == 'web': code += '#define HALO_WEB 1\n'
    code += 'static struct {const char *shading_language;} xgpu_capabilities={"300 es"};\n'
    code += '\n'.join('#define '+name+' '+value for name,value in re.findall(r'\b(D3D\w+)\s*=\s*(0x[\da-fA-F]+|\d+)\s*,',pdb))+'\n'
    code += '\n'.join(structure(header,name) for name in ['xgpu_text','nv2a_vertex_lighting','nv2a_pixel_shader_key'])+'\n'
    code += 'void xgpu_text_append(struct xgpu_text *,char const *,...);\n'
    code += re.search(r'^#define XGPU_VERTEX_CONSTANT_BIAS .*$',header,re.M).group()+'\n'
    code += enum_with(header,'_xgpu_sampler_none')+'\n'
    code += header[header.index('#ifdef HALO_GLES\n/* ES samplers'):header.index('/* ---------- textures */')]
    code += 'static char const *debug_expression="";\nstatic char const *config_string(char const *s){(void)s;return debug_expression;}\nstatic int config_boolean(char const *s){(void)s;return 0;}\n'
    code += (ROOT/'port/linux/src/shield_honeycomb_gl.h').read_text()
    code += function((ROOT/'port/linux/src/mcc_environment_shader_gl.c').read_text(),'mcc_environment_shader_emit')
    for name in ['xgpu_text','plasma_model_shader','nv2a_psh']:
        source=(ROOT/f'port/linux/src/{name}.c').read_text()
        if omit and name=='nv2a_psh': source=omit_honeycomb(source)
        code += re.sub(r'^#include.*$', '', source, flags=re.M)+'\n'
    code += r'''
int main(void) {
    struct nv2a_pixel_shader_key key={0};
    char *plain=nv2a_pixel_shader_to_glsl(&key),*shield,*again;
    assert(!strstr(plain,"shield_honeycomb_surface") && !strstr(plain,"shield_hex"));
    key.shield_honeycomb=1;shield=nv2a_pixel_shader_to_glsl(&key);
    assert((strstr(shield,"result = shield_honeycomb_surface(result)")!=NULL)==EXPECT_SURFACE);
    key.shield_honeycomb=0;again=nv2a_pixel_shader_to_glsl(&key);
    assert(!strcmp(plain,again));
    free(plain);free(shield);free(again);
    /* A dead r0.a write must produce the same desktop source as discard.
     * The GLES source stays unchanged; later inputs/mux/final reads stay live. */
    memset(&key,0,sizeof(key));
    key.combiner_state[D3DRS_PSCOMBINERCOUNT]=8;
    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD]=0x08000000;
    key.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG]=0x00001800;
    plain=nv2a_pixel_shader_to_glsl(&key);
    key.combiner_state[D3DRS_PSALPHAINPUTS3]=0x1c140000;
    key.combiner_state[D3DRS_PSALPHAOUTPUTS3]=0xc00;
    again=nv2a_pixel_shader_to_glsl(&key);
#ifndef HALO_GLES
    assert(!strcmp(plain,again));
    {
        unsigned test;
        for(test=0;test<7;test++) {
            struct nv2a_pixel_shader_key live=key;
            if(test==0)live.combiner_state[D3DRS_PSRGBINPUTS4]=0x1c000000;
            if(test==1)live.combiner_state[D3DRS_PSALPHAINPUTS4]=0x001c0000;
            if(test==2)live.combiner_state[D3DRS_PSRGBOUTPUTS4]=0x4000;
            if(test==3)live.combiner_state[D3DRS_PSALPHAOUTPUTS4]=0x4000;
            if(test==4)live.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG]=0x00001c00;
            if(test==5)live.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD]=live.combiner_state[D3DRS_PSFINALCOMBINERINPUTSEFG]=0;
            if(test==6)live.combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD]=0x001c0000;
            discard_dead_r0_alpha(live.combiner_state);
            assert(live.combiner_state[D3DRS_PSALPHAOUTPUTS3]==0xc00);
        }
        key.combiner_state[D3DRS_PSALPHAOUTPUTS3]=0xd00;
        discard_dead_r0_alpha(key.combiner_state);
        assert(key.combiner_state[D3DRS_PSALPHAOUTPUTS3]==0xd00);
    }
#else
    assert(strcmp(plain,again));
#endif
    free(plain);free(again);
    key.combiner_state[D3DRS_PSALPHAOUTPUTS3]=0xc00;
    debug_expression="r0.a";again=nv2a_pixel_shader_to_glsl(&key);
    assert(strstr(again,"r0.a = aSUM;"));free(again);return 0;
}
'''
    path=tmp_path/'translation.c';path.write_text(code)
    exe=tmp_path/('translation.exe' if os.name=='nt' else 'translation')
    flags=['--target=i686-pc-windows-msvc','-fuse-ld=lld','-D_CRT_SECURE_NO_WARNINGS'] if os.name=='nt' else ['-m32']
    result=subprocess.run([cc,*flags,'-std=gnu89',f'-DEXPECT_SURFACE={int(not omit)}',str(path),'-o',str(exe)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    subprocess.run([str(exe)],check=True)


def test_settled_window_does_not_hide_long_frames():
    rows=[dict(ns=ns,tick=tick,playing=1,link_ns=0,link_calls=0)
          for ns,tick in [(0,599),(8_000_000,600),(108_000_000,601),(116_000_000,1201)]]
    report=summarize(rows)
    assert report['frames']==2 and report['worst_ms']==100 and report['over50']==1


def test_player_trace_comparison_preserves_gameplay_fields():
    log='network test: tick 30 player 0: (1 2 3) h1/1 m65 | items 4\n'
    assert compare_player_traces(log, log.replace('m65', 'm113'))['matched']
    assert not compare_player_traces(log, log.replace('(1 2 3)', '(1 2 4)'))['matched']
    assert not compare_player_traces(log, log.replace('tick 30', 'tick 60'))['matched']
    assert not compare_player_traces('', '')['matched']
