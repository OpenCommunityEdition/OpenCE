"""The graphics LOD switch must not accelerate static lighting refreshes."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run

def executable(fault=False):
    source = read('source/render/render_objects.c')
    code = enum_with(source, 'OBJECT_RENDER_STATE_LARGE_INTERVAL')
    for name in ('OBJECT_RENDER_STATE_LARGE_PIXELS','OBJECT_RENDER_STATE_SMALL_PIXELS'):
        code += '\n#define ' + name + ' ' + str(constant(source, name))
    code += '\n' + function(source, 'object_render_state_refresh')
    if fault:
        code = mutated(code, 'if (level_of_detail_pixels > OBJECT_RENDER_STATE_LARGE_PIXELS)',
                       'if (render_lod_disabled() || level_of_detail_pixels > OBJECT_RENDER_STATE_LARGE_PIXELS)')
    return build('render_lod_lighting', (('under_test.inc', code),))

def test_lighting_cadence():
    code, out = run(executable(), 'cadence')
    assert code == 0, out

def test_forced_lighting_negative_control():
    code, out = run(executable(True), 'cadence')
    assert code == CHECK_FAILED, out
