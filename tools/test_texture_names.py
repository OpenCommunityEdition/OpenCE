"""Real batched name allocation: uniqueness, ownership, failure and GLES fallback."""
import os
from pathlib import Path
import shutil
import subprocess
import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('gles', [False, True])
def test_texture_name_ownership(tmp_path, gles):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    code = r'''
#include <assert.h>
#include <string.h>
typedef unsigned int GLuint;
typedef int GLsizei;
static unsigned next_name, calls, deleted, fail;
static unsigned char owned[4096], removed[4096];
static void glGenTextures(int count,GLuint *names) {
    int i;calls++;
    for(i=0;i<count;i++)names[i]=fail?0:++next_name;
}
static void glDeleteTextures(int count,GLuint const *names) {
    int i;
    for(i=0;i<count;i++) {
        assert(names[i] && !owned[names[i]] && !removed[names[i]]);
        removed[names[i]]=1;deleted++;
    }
}
'''
    code += (ROOT/'port/linux/src/xgpu_texture_names.c').read_text().replace('#include "xgpu.h"', '')
    code += r'''
int main(void) {
    unsigned i,name,previous;
    fail=1;assert(!xgpu_texture_name_new());fail=0;
    for(i=0;i<70;i++) {
        name=xgpu_texture_name_new();assert(name && !owned[name] && !removed[name]);owned[name]=1;
    }
#ifndef HALO_GLES
    assert(calls==3);
    xgpu_texture_names_prepare();assert(texture_name_count==1024 && calls==4);
    for(i=0;i<12;i++) {
        name=xgpu_texture_name_new();assert(name && !owned[name] && !removed[name]);owned[name]=1;
    }
    xgpu_texture_names_dispose();assert(deleted==1012 && texture_name_count==0);
    previous=deleted;xgpu_texture_names_dispose();assert(deleted==previous);
    name=xgpu_texture_name_new();assert(name && !owned[name] && !removed[name]);owned[name]=1;
    xgpu_texture_names_dispose();assert(deleted==previous+63);
#else
    assert(calls==71);xgpu_texture_names_prepare();xgpu_texture_names_dispose();assert(!deleted && calls==71);
#endif
    return 0;
}
'''
    path = tmp_path/'names.c';path.write_text(code)
    binary = tmp_path/('names.exe' if os.name == 'nt' else 'names')
    flags = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld'] if os.name == 'nt' else ['-m32']
    if gles: flags += ['-DHALO_GLES=1']
    result = subprocess.run([compiler, *flags, '-std=gnu89', str(path), '-o', str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(binary)], check=True)
