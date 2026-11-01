"""Exercise real recipe parsing, map ownership, fallback and bounded recording."""
from pathlib import Path
import re
import shutil
import subprocess
import sys
import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope='module')
def checker(tmp_path_factory):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    work = tmp_path_factory.mktemp('shader-prepare')
    code = (ROOT/'port/linux/src/shader_prepare.h').read_text()
    code += re.sub(r'^#include ["<](?:[^"\n]+\.h"|SDL3/SDL.h>)\s*$', '',
                   (ROOT/'port/linux/src/shader_prepare.c').read_text(), flags=re.M)
    (work/'under_test.inc').write_text(code)
    path = work/'check.c'
    path.write_text((ROOT/'tools/fixtures/shader_prepare_check.c').read_text())
    binary = work/('check.exe' if sys.platform=='win32' else 'check')
    command = [compiler, '-std=gnu99', '-O2', '-Wall', '-Wno-unused-function', '-Werror', str(path), '-o', str(binary)]
    if sys.platform=='win32':
        command[1:1] = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld', '-D_CRT_SECURE_NO_WARNINGS']
    subprocess.run(command, check=True, capture_output=True, text=True)
    return binary


@pytest.mark.parametrize('case', ['disabled','ownership','cached','failed','allocation','invalid','bounds','toggle',
    'malformed_length','malformed_hash','malformed_terminator','malformed_embedded_nul',
    'malformed_trailing','malformed_duplicate'])
def test_recipes(checker, tmp_path, case):
    result = subprocess.run([str(checker), case], cwd=tmp_path, capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout+result.stderr
