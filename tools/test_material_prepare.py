"""The real preparation gates: no pixel fallback, bounded metadata, inactive/GLES paths."""
import os
from pathlib import Path
import shutil
import subprocess
import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('gles', [False, True])
def test_material_preparation_gates(tmp_path, gles):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    source = (ROOT/'port/linux/game/material_prepare.c').read_text()
    state = source[source.index('#ifndef HALO_GLES'):source.index('/* Native draw-packet')]
    functions = source[source.index('int material_prepare_skip_shader'):source.index('void material_prepare_map')]
    code = r'''
#include <assert.h>
#include <stdlib.h>
typedef struct {unsigned Data;} D3DBaseTexture;
struct bitmap_data {int valid;};
struct shader {struct {int type;} base;};
static int calls,draws;
static int texture_cache_prepare_descriptor(struct bitmap_data *b,void *p) {
    calls++;((D3DBaseTexture *)p)->Data=0;return b->valid;
}
static void render_material_prepare_draw(void) {draws++;}
'''
    code += state+'\n#endif\n'+functions
    code += r'''
int main(void) {
    struct bitmap_data b[4097];struct shader shader;
    void *header=(void *)123;int i;
    assert(!material_prepare_bitmap(&b[0],&header) && header==(void *)123);
    assert(!material_prepare_draw() && !material_prepare_skip_shader(NULL));
#ifndef HALO_GLES
    bitmaps=calloc(MATERIAL_BITMAP_LIMIT,sizeof(*bitmaps));assert(bitmaps);preparing=1;
    b[0].valid=0;assert(material_prepare_bitmap(&b[0],&header) && !header && !bitmap_count);
    for(i=0;i<4097;i++)b[i].valid=1;
    assert(material_prepare_bitmap(&b[0],&header) && header && ((D3DBaseTexture *)header)->Data==1);
    assert(material_prepare_bitmap(&b[0],&header) && calls==2 && bitmap_count==1);
    for(i=1;i<4096;i++)assert(material_prepare_bitmap(&b[i],&header) && header);
    assert(material_prepare_bitmap(&b[4096],&header) && !header && bitmap_count==4096);
    assert(material_prepare_draw() && draws==1);
    for(i=0;i<MATERIAL_DRAW_LIMIT;i++)assert(material_prepare_draw());
    assert(draws==MATERIAL_DRAW_LIMIT);
    for(i=0;i<11;i++) {shader.base.type=i;assert(material_prepare_skip_shader(&shader)==(i!=5 && i!=6));}
    assert(material_prepare_skip_shader(NULL));
    preparing=0;free(bitmaps);bitmaps=NULL;bitmap_count=0;
    assert(!material_prepare_bitmap(&b[0],&header) && !material_prepare_draw());
#else
    assert(!calls && !draws);
#endif
    return 0;
}
'''
    path = tmp_path/'material.c';path.write_text(code)
    binary = tmp_path/('material.exe' if os.name=='nt' else 'material')
    flags = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld'] if os.name=='nt' else ['-m32']
    if gles: flags += ['-DHALO_GLES=1']
    result = subprocess.run([compiler,*flags,'-std=gnu89',str(path),'-o',str(binary)],capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    subprocess.run([str(binary)],check=True)
