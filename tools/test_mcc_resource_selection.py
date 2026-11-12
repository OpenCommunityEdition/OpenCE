"""Local resource choice retains full hashes, package precedence and stamp checks."""
from pathlib import Path
import struct
import subprocess
import pytest
from harness import function
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'port/linux/game'


@pytest.fixture(scope='module')
def resource_tool(tmp_path_factory):
    work = tmp_path_factory.mktemp('resource-choice')
    game = GAME
    for name in ('mcc_resources.h', 'mcc_package.h', 'mcc_validation.h', 'mcc_resource_check.inl', 'mcc_validation.inl'):
        (work / name).write_text((game / name).read_text())
    (work / 'cseries.h').write_text('typedef int boolean;\ntypedef unsigned char byte;\n')
    (work / 'errors.h').write_text('#define _error_silent 0\n#define error(...) ((void)0)\n')
    (work / 'xtl.h').write_text('''
#ifndef _WIN32
#define __stdcall
#endif
#define TRUE 1
#define FALSE 0
#define WAIT_OBJECT_0 0
#define INFINITE (~0u)
#define CreateEventA(a,b,c,d) NULL
#define CreateThread(a,b,c,d,e,f) ((void)(c),NULL)
#define SetEvent(h) ((void)(h))
#define WaitForSingleObject(h,t) ((void)(h),(void)(t),WAIT_OBJECT_0)
#define CloseHandle(h) ((void)(h))
''')
    resources = (game / 'mcc_resources.c').read_text().replace('../../third_party/zlib/zlib_prefixed.h',
        (ROOT / 'port/third_party/zlib/zlib_prefixed.h').as_posix())
    planner = (GAME / 'mcc_share_package.c').read_text()
    code = (ROOT / 'tools/tests/mcc_resource_selection.c').read_text()
    code = code.replace('/* RESOURCES */', resources).replace('/* PLANNER */',
        '\n'.join(function(planner, n) for n in ('base_resource_copy', 'base_resources')))
    (work / 'driver.c').write_text(code)
    (work / 'crc.c').write_text('#define Z_PREFIX\n#include "' + (ROOT / 'port/third_party/zlib/crc32.c').as_posix() + '"\n')
    return compile_tool(work, [work / 'driver.c', game / 'mcc_package.c', GAME / 'mcc_sha256.c',
                              work / 'crc.c'])


@pytest.mark.parametrize('case', ['match', 'mismatch', 'override', 'unused', 'changed', 'cache', 'stage', 'both'])
def test_local_resources(resource_tool, tmp_path, case):
    def resource(value, kind=2):
        names = b'sound\\test__0__0\0' if kind == 2 else b'sound\\test_0\0'
        return struct.pack('<4I', kind, 20, 20+len(names), 1) + value + names + struct.pack('<3I', 0, 4, 16)
    for folder in ('p', '_shared/singleplayer', '_shared/multiplayer'):
        (tmp_path / 'mcc_maps' / folder).mkdir(parents=True, exist_ok=True)
    (tmp_path / 'mcc_maps/p/a.map').write_bytes(bytes(2048))
    (tmp_path / 'mcc_maps/sounds.map').write_bytes(resource(b'old!'))
    (tmp_path / 'mcc_maps/_shared/singleplayer/sounds.map').write_bytes(b'bad')
    (tmp_path / 'mcc_maps/_shared/multiplayer/sounds.map').write_bytes(resource(b'new!'))
    (tmp_path / 'mcc_staging/p').mkdir(parents=True)
    (tmp_path / 'mcc_staging/p/a.map').write_bytes(bytes(2048))
    (tmp_path / 'mcc_maps/bitmaps.map').write_bytes(resource(b'old!', 1))
    (tmp_path / 'mcc_maps/_shared/singleplayer/bitmaps.map').write_bytes(resource(b'new!', 1))
    result = subprocess.run([str(resource_tool), case], cwd=tmp_path, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
