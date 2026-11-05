"""Exercise the actual visibility field with synthetic collision geometry."""
import re
import sys
import pytest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, read, run


def test_visibility_field():
    code = re.sub(r'^#include.*$', '', read('port/linux/game/ammo_counter_falloff.c') + '\n' + read('port/linux/game/plasma_occlusion.c'), flags=re.M)
    status, output = run(build('plasma_occlusion', (('under_test.inc', code),)), 'visibility')
    assert status == 0, output


def test_stale_pass_upload_is_detected():
    code = re.sub(r'^#include.*$', '', read('port/linux/game/ammo_counter_falloff.c') + '\n' + read('port/linux/game/plasma_occlusion.c'), flags=re.M)
    assert code.count('field->texture_mask = 0;') == 2
    code = code.replace('field->texture_mask = 0;', '(void)0;')
    status, output = run(build('plasma_occlusion', (('under_test.inc', code),)), 'visibility')
    assert status == CHECK_FAILED, output


def test_directional_mask_is_required():
    code = re.sub(r'^#include.*$', '', read('port/linux/game/ammo_counter_falloff.c') + '\n' + read('port/linux/game/plasma_occlusion.c'), flags=re.M)
    code = code.replace('return t*t*(3.0f - 2.0f*t);', '(void)t; return 1.0f;')
    status, output = run(build('plasma_occlusion', (('under_test.inc', code),)), 'visibility')
    assert status == CHECK_FAILED, output


def test_needler_filtered_foot_pool_is_detected():
    code = re.sub(r'^#include.*$', '', read('port/linux/game/ammo_counter_falloff.c') + '\n' + read('port/linux/game/plasma_occlusion.c'), flags=re.M)
    # The previous long fade must no longer satisfy the lower-spill contract.
    old = code.replace('(-height - 0.06f) / 0.06f', '(-height - 0.06f) / 0.18f')
    status, output = run(build('plasma_occlusion', (('under_test.inc', old),)), 'visibility')
    assert status == CHECK_FAILED, output


def test_stationary_history_matches_reprojection():
    code = re.sub(r'^#include.*$', '', read('port/linux/game/ammo_counter_falloff.c') + '\n' + read('port/linux/game/plasma_occlusion.c'), flags=re.M)
    # Compare every updated cell, including moving lights, door changes, changed
    # extents and the 30/60/120 Hz sequences exercised by the fixture.
    marker = '\tfield->valid = 1;'
    assert code.count(marker) == 1
    code = code.replace(marker, marker + '\n\tfor (i = 0; i < OCC_COUNT; i++) printf("CELL %.9g\\n", field->visibility[i]);')
    samples = []
    reference = code.replace('if (stationary)', 'if (stationary && FALSE)')
    # The reference also needs coordinates for stationary fields now that
    # reprojection prepares its separable axes before visiting the cells.
    reference = reference.replace('if (!stationary && field->valid', 'if (field->valid')
    for version in (code, reference):
        status, output = run(build('plasma_occlusion', (('under_test.inc', version),)), 'visibility')
        assert status == 0, output[-2000:]
        samples.append([float(x) for x in re.findall(r'CELL (\S+)', output)])
    assert len(samples[0]) > 4096
    assert samples[0] == pytest.approx(samples[1], abs=0.00001)


def test_precomputed_history_matches_cell_coordinates():
    code = re.sub(r'^#include.*$', '', read('port/linux/game/ammo_counter_falloff.c') + '\n' + read('port/linux/game/plasma_occlusion.c'), flags=re.M)
    fast = 'history = sample_axes(previous, &history_axes[0][x], &history_axes[1][y], &history_axes[2][z]);'
    assert code.count(fast) == 1
    # Check the actual fast result against the original per-cell coordinate
    # arithmetic throughout movement, extent changes and all three rates.
    reference = '''sample_field(previous,
        0.5f + (light->position.x + ray.i - field->center[0]) / (2 * field->extent),
        0.5f + (light->position.y + ray.j - field->center[1]) / (2 * field->extent),
        0.5f + (light->position.z + ray.k - field->center[2]) / (2 * field->extent))'''
    code = code.replace(fast, '{ ' + fast + '\nCHECK(history == ' + reference + ', "precomputed coordinates preserve every sample"); }')
    status, output = run(build('plasma_occlusion', (('under_test.inc', code),)), 'visibility')
    assert status == 0, output
