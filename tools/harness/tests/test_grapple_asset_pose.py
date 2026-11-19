"""Actual grapple payload + extracted engine bind-matrix multiplication."""
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def test_actual_grapple_bind_pose(tmp_path):
    addon = ROOT.parent / 'clmb-mutator-repo/grappling_hook/grappling_hook.ceweapon'
    if not addon.exists():
        import pytest
        pytest.skip('external grapple package unavailable')
    data = addon.read_bytes()
    u = lambda offset: struct.unpack_from('<I', data, offset)[0]
    tags, count, base, models = u(20), u(24), u(36), u(68)
    for i in range(count):
        group, _, _, name, root = struct.unpack_from('<5I', data, tags+i*20)
        label = data[base+name:].split(b'\0', 1)[0]
        if group == int.from_bytes(b'mod2', 'big') and label == b'altis\\weapons\\grappling_hook\\grappling_hook':
            break
    else:
        raise AssertionError('actual rope model missing')
    model = base+root
    assert u(model) == 0  # model-global skin indices, not a part-local palette
    assert u(model+0xb8) == 2
    nodes = base+u(model+0xbc)
    inverse = struct.unpack_from('<13f', data, nodes+0x9c+104)
    assert inverse[0] == 1
    assert abs(inverse[10]+0.1194727644) < 1e-7
    geometry = base+u(model+0xd4)
    part = base+u(geometry+0x28)
    vertices, offset = u(part+0x58), u(part+0x64)
    tip = next(struct.unpack_from('<3f', data, models+offset+i*68)
               for i in range(vertices)
               if struct.unpack_from('<h', data, models+offset+i*68+56)[0] == 1)
    source = (ROOT/'source/math/matrix_math.c').read_text()
    function = source[source.index('void matrix4x3_multiply('):source.index('\nreal\nmatrix3x3_determinant(')]
    c = '''#include <assert.h>
#include <math.h>
typedef float real;
#define long int
typedef struct { real scale; union { real n[4][3]; struct { real basis[9]; struct {real x,y,z;} position; }; }; } real_matrix4x3;
'''+function+'''
int main(void) {
real_matrix4x3 pose={.scale=1,.n={{1,0,0},{0,1,0},{0,0,1},{20,30,40}}};
real_matrix4x3 bind={'''+','.join(map(str,inverse))+'''}, result;
matrix4x3_multiply(&pose,&bind,&result);
assert(result.scale==1);
assert(fabsf(result.position.x-(20-0.1194727644f))<1e-5f);
assert(result.position.y==30 && result.position.z==40);
assert(fabsf(result.position.x+'''+str(tip[0])+'''-20)<0.01f);
return 0; }
'''
    path, executable = tmp_path/'pose.c', tmp_path/'pose'
    path.write_text(c)
    subprocess.run(['cc','-std=c11','-Wall','-Werror','-Wno-missing-braces',str(path),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
