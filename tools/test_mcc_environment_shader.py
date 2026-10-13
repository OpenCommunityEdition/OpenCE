"""Exercise the real combiner generator, including its untouched legacy path."""
from pathlib import Path
import os
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


def test_mcc_bump_combiner(tmp_path):
    from harness import function
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    source = (ROOT / "port/linux/src/nv2a_psh.c").read_text()
    prefix = source[source.index("enum\n"):source.index("/* ---------- texture stages */")]
    emit = function((ROOT / "port/linux/src/mcc_environment_shader_gl.c").read_text(),
                    "mcc_environment_shader_emit")
    code = r'''
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
typedef unsigned int DWORD;
typedef int BOOL;
#define FALSE 0
#define TRUE 1
enum { D3DRS_PSCOMBINERCOUNT, D3DRS_PSALPHAINPUTS0=8, D3DRS_PSRGBINPUTS0=16,
       D3DRS_PSALPHAOUTPUTS0=24, D3DRS_PSRGBOUTPUTS0=32 };
struct xgpu_text {char data[16000];};
static void xgpu_text_append(struct xgpu_text *t,const char *format,...) {
 va_list args;size_t n=strlen(t->data);va_start(args,format);
 vsnprintf(t->data+n,sizeof(t->data)-n,format,args);va_end(args);
}
''' + emit + prefix + r'''
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}} while (0)
int main(void) {
 DWORD state[64]={0};struct xgpu_text legacy={{0}},mcc={{0}},other={{0}};
 state[D3DRS_PSRGBINPUTS0]=0x484B0A01;state[D3DRS_PSRGBOUTPUTS0]=0x2000;
 combiner_stage(&legacy,state,0,FALSE);combiner_stage(&mcc,state,0,TRUE);
 CHECK(strstr(legacy.data,"vec3 cAB = vec3(dot(cA, cB));"));
 CHECK(!strstr(legacy.data,"cB.z"));CHECK(strstr(mcc.data,"1.0 + dot(cA, cB) - cB.z"));
 CHECK(strstr(mcc.data,"vec3 cA = (2.0 * max(t0.rgb, 0.0) - 1.0)"));
 CHECK(strstr(mcc.data,"vec3 cB = (2.0 * max(t3.rgb, 0.0) - 1.0)"));
 CHECK(strstr(mcc.data,"float aAB = aA * aB;"));
 state[D3DRS_PSRGBINPUTS0]=0x484A0A01;
 legacy.data[0]=0;combiner_stage(&legacy,state,0,FALSE);combiner_stage(&other,state,0,TRUE);
 CHECK(!strcmp(legacy.data,other.data));
 state[D3DRS_PSRGBOUTPUTS0]=0;legacy.data[0]=other.data[0]=0;
 combiner_stage(&legacy,state,0,FALSE);combiner_stage(&other,state,0,TRUE);
 CHECK(!strcmp(legacy.data,other.data));
 return 0;
}
'''
    path = tmp_path / "shader.c"
    path.write_text(code)
    binary = tmp_path / ("shader.exe" if os.name == "nt" else "shader")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if os.name == "nt" else []
    result = subprocess.run([compiler, *target, "-std=gnu89", str(path), "-o", str(binary)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
