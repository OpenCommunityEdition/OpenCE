"""MCC signed object colors reach the native post-darkening clamp."""
from pathlib import Path
import os
import shutil
import subprocess
import pytest
from harness import function

ROOT = Path(__file__).resolve().parents[1]


def test_mcc_signed_change_colors(tmp_path):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang is required')
    helper = function((ROOT / 'port/linux/game/mcc_objects.c').read_text(), 'mcc_object_color_extrapolate')
    native = function((ROOT / 'source/objects/objects.c').read_text(), 'object_compute_change_colors')
    fixture = (ROOT / 'tools/harness/tests/mcc_change_colors.c').read_text()
    source = tmp_path / 'colors.c'
    source.write_text(fixture.replace('/* IMPLEMENTATION */', helper + '\n' + native))
    binary = tmp_path / ('colors.exe' if os.name == 'nt' else 'colors')
    target = ['--target=i686-pc-windows-msvc', '-fuse-ld=lld'] if os.name == 'nt' else ['-m32']
    result = subprocess.run([compiler, *target, '-std=gnu89', '-Wall', '-Wextra', '-Werror',
        '-ffp-contract=off', '-fsanitize=undefined', '-fsanitize-trap=undefined',
        str(source), '-o', str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
