"""Render real MCC sprite batches across the native eight-bitmap boundary."""
from pathlib import Path
import os
import shutil
import subprocess
import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_mcc_sprite_batches(tmp_path):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang is required')
    native = (ROOT / 'source/render/render_sprite.c').read_text()
    header = (ROOT / 'source/render/render_sprite.h').read_text()
    header = '\n'.join(line for line in header.splitlines() if not line.startswith(('#include', '#pragma')))
    helper = (ROOT / 'port/linux/game/mcc_sprites.c').read_text()
    helper = '\n'.join(line for line in helper.splitlines() if not line.startswith(('#include', '#pragma')))
    fixture = (ROOT / 'tools/harness/tests/mcc_sprites.c').read_text()
    source = fixture.replace('/* HEADER */', header).replace('/* IMPLEMENTATION */',
        helper + '\n' + '\n'.join(function(native, name) for name in
        ('build_sprites_begin', 'build_sprites_end', 'build_sprite_get_group')))
    path = tmp_path / 'sprites.c'
    path.write_text(source)
    binary = tmp_path / ('sprites.exe' if os.name == 'nt' else 'sprites')
    target = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld'] if os.name == 'nt' else ['-m32']
    result = subprocess.run([compiler, *target, '-std=gnu89', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=undefined', '-fsanitize-trap=undefined', str(path), '-o', str(binary)],
        capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
