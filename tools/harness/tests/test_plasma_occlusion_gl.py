"""Validate program-owned locations and unchanged sampling without a GL driver."""
import re
import sys
import pytest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, read, run


@pytest.mark.parametrize('gles', [False, True])
def test_model_occlusion_binding_ownership(gles):
    header = read('port/linux/include/plasma_occlusion.h')
    uniforms = re.search(r'struct plasma_occlusion_uniforms\s*\{[^}]+\};', header)[0]
    code = re.sub(r'^#include.*$', '', read('port/linux/src/plasma_occlusion_gl.c'), flags=re.M)
    status, output = run(build('plasma_occlusion_gl', (('under_test.inc', ('#define HALO_GLES\n' if gles else '')+uniforms+'\n'+code),)), 'binding')
    assert status == 0, output
