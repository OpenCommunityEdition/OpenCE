"""Bounded actual-byte 2D wrapper replay using the existing 3D/cache oracle.

The target executes January's selected Decals projection helper; first and
ordinary candidates execute their own emitted helper. Real downstream point,
line and cache bodies execute. Only the existing genuine external API models
are reused. Finite binary-fraction geometry and eleven NULL exits are covered,
not arbitrary IEEE equivalence: the inherited projection helper has a separately
confirmed NaN difference. Void return-register values and x87 condition-code
bits are not treated as contractual outputs; ABI, TOP, tags and FPCW are checked.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'tools/coff_compare.py').is_file())
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

# Private module instance: adapting its constants/functions must not alter the
# original oracle imported by another test in the same interpreter.
BASE_PATH = ROOT / 'tools/audit/render_debug_three_runtime.py'
spec = importlib.util.spec_from_file_location('_render_debug_2d_private_base', BASE_PATH)
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)

POINT2D, LINE2D, VECTOR2D = ('_render_debug_point2d', '_render_debug_line2d', '_render_debug_vector2d')
PROJECT = '_project_point2d'
WRAPPERS = (POINT2D, LINE2D, VECTOR2D)
PATHS = {
    'target': ROOT / 'scratch/render-debug-2d-before-20260906/files/build/split/source/render/render_debug.obj',
    'first': ROOT / 'scratch/render-debug-2d-first-20260906.obj',
    'actual': ROOT / 'scratch/render-debug-2d-first-20260906/source/render/render_debug.obj',
    'target_helper': ROOT / 'scratch/render-debug-2d-decals-target-20260906.obj',
    'mapping_owner': ROOT / 'build/split/source/math/real_math.obj',
    'source': ROOT / 'scratch/render-debug-2d-first-20260906/files/source/render/render_debug.c',
    'header': ROOT / 'scratch/render-debug-2d-first-20260906/files/source/render/render_debug_geometry.h',
}
PINS = {
    'target': 'd3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0',
    'first': 'b1c86df56a33151eccc6bcdef73adbb3ded3db3da32a3778c501e59c1781f5b3',
    'actual': '646a3fc5c62175c283765117075d4c0c2544ff9ec4079062b2fab76fe87c6323',
    'target_helper': 'f4de9ec343021d668ea367880bb8c65e982b9b60b03c2a23014616872f305083',
    'mapping_owner': '93e4b179d3e52fe3d62a998ba3951a42eb57dddd2790713dee08e1de91780460',
    'source': '21ca2552e1e890c2b58495bfc85016efba15c7ae817f85f39a5b80c1dd8245e0',
    'header': '85a9beb3378eddb29abea3c58e8dc747b153fcbb257f751524e62ccf0f3507b2',
}
BASE_PIN = 'fd66d5fd9df8df4f79d0ff98c526470c42d4c286f023545a8c0cdd6eda084a08'
MAPPINGS = (((2, 1), (1, 2)), ((0, 2), (2, 0)), ((1, 0), (0, 1)))
TABLE = struct.pack('<12h', *(v for axis in MAPPINGS for pair in axis for v in pair))
b.FUNCTIONS += WRAPPERS + (PROJECT,)
b.FUNCTION_ADDRESSES = {name: b.CODE + index * 0x1000 for index, name in enumerate(b.FUNCTIONS)}
b.LITERALS.update({
    '_global_projection3d_mappings': TABLE,
    '__real@3f1a36e2e0000000': bytes.fromhex('000000e0e2361a3f'),
    '__real@00000000': b'\0' * 4,
    '??_C@_0BE@DKJODJEH@?4?4?2math?2real_math?4h?$AA@': b'..\\math\\real_math.h\0',
    '??_C@_0CB@FEGCJDLB@projection?$DO?$DN_x?5?$CG?$CG?5projection?$DM?$DN_z@': b'projection>=_x && projection<=_z\0',
    '??_C@_0L@LKENDBEE@?$HO?$CIsign?$CG?$HO1?$CJ?$AA@': b'~(sign&~1)\0',
    '??_C@_05MAIECAHP@plane?$AA@': b'plane\0',
    '??_C@_02ILAJHIEO@p0?$AA@': b'p0\0',
    '??_C@_02JCBCEJAP@p1?$AA@': b'p1\0',
    '??_C@_06NBFPPGEG@vector?$AA@': b'vector\0',
})
b.LITERAL_ADDRESSES = {name: b.LITERAL + index * 0x400 for index, name in enumerate(sorted(b.LITERALS))}
_input_image, _public_arguments, _expected_success = b.input_image, b.public_arguments, b.expected_success


def cases():
    result = []
    for function in WRAPPERS:
        for axis in range(3):
            for sign in (False, True):
                for immediate in (False, True):
                    result.append({'name': f'{function}:{axis}:{int(sign)}:{int(immediate)}',
                        'function': function, 'projection': axis, 'sign': sign,
                        'immediate': immediate, 'offset': 0.75, 'size': 2.0, 'count': 3})
        assertion_names = ('plane', 'point', 'color') if function == POINT2D else (
            ('plane', 'p0', 'p1', 'color') if function == LINE2D else ('plane', 'point', 'vector', 'color'))
        start = {POINT2D: 219, LINE2D: 242, VECTOR2D: 269}[function]
        for index, name in enumerate(assertion_names):
            b.ASSERTIONS[(function, name)] = (name, start + index)
            result.append({'name': f'{function}:null:{name}', 'function': function,
                'projection': 0, 'sign': False, 'immediate': True, 'null': name})
        result.append({'name': f'{function}:zero_normal', 'function': function,
            'projection': 2, 'sign': True, 'immediate': True, 'zero_normal': True,
            'offset': -0.5, 'size': -2.0})
        result.append({'name': f'{function}:zero_offset', 'function': function,
            'projection': 1, 'sign': False, 'immediate': False, 'offset': 0.0, 'size': 0.0})
    result += [
        {'name': 'line2d:cache_overflow', 'function': LINE2D, 'projection': 1,
         'sign': False, 'immediate': False, 'count': 512},
        {'name': 'vector2d:new_tick', 'function': VECTOR2D, 'projection': 2,
         'sign': True, 'immediate': False, 'cache_time': 9, 'game_time': 10,
         'second_time': 11, 'count': 511, 'pool': b'stale\0'},
    ]
    return result


def input_image(case):
    raw, blocks, locations = _input_image(case)
    if case['function'] not in WRAPPERS:
        return raw, blocks, locations
    raw = bytearray(raw)
    plane = [0.5, -0.25, 2.0, 3.0]
    if case.get('zero_normal'):
        plane[case['projection']] = 0.0
    blocks.update({'point': struct.pack('<2f', 1.25, -2.5),
        'p0': struct.pack('<2f', 1.25, -2.5), 'p1': struct.pack('<2f', -4.5, 5.25),
        'vector': struct.pack('<2f', 0.5, -1.0), 'plane': struct.pack('<4f', *plane)})
    locations = {name: b.INPUT + index * 0x100 for index, name in enumerate(blocks)}
    for name, payload in blocks.items():
        at = locations[name] - b.INPUT
        raw[at:at + len(payload)] = payload
    return bytes(raw), blocks, locations


def public_arguments(case, locations):
    if case['function'] not in WRAPPERS:
        return _public_arguments(case, locations)
    address = lambda name: b.p32(0 if case.get('null') == name else locations[name])
    args = b.p32(int(case['immediate'])) + address('plane') + b.p32(case['projection']) + b.p32(int(case['sign']))
    if case['function'] == LINE2D:
        args += address('p0') + address('p1')
    else:
        args += address('point')
        if case['function'] == VECTOR2D:
            args += address('vector')
        args += b.f32(case.get('size', 4.0))
    return args + address('color') + b.f32(case.get('offset', 0.75))


def projected(point, plane, axis, sign, offset):
    x, y = MAPPINGS[axis][int(sign)]
    result = [0.0, 0.0, 0.0]
    result[x], result[y] = point
    # Test geometry is exactly representable; no host-double IEEE proof is
    # inferred for arbitrary values or NaNs.
    if abs(plane[axis]) >= struct.unpack('<d', bytes.fromhex('000000e0e2361a3f'))[0]:
        result[axis] = b.f32_value(((plane[3] - point[0] * plane[x]) - point[1] * plane[y]) / plane[axis])
    result[axis] = b.f32_value(result[axis] + (offset if sign else -offset))
    return b.point_bytes(result)


def expected_success(case, before, blocks):
    if case['function'] not in WRAPPERS:
        return _expected_success(case, before, blocks)
    function = case['function']
    plane = struct.unpack('<4f', blocks['plane'])
    project = lambda point: projected(point, plane, case['projection'], case['sign'], case.get('offset', 0.75))
    new_blocks = dict(blocks)
    new_case = dict(case, function=b.POINT if function == POINT2D else b.LINE)
    if function == POINT2D:
        new_blocks['point'] = project(struct.unpack('<2f', blocks['point']))
        prefix = [POINT2D, PROJECT]
    else:
        if function == LINE2D:
            p0, p1 = struct.unpack('<2f', blocks['p0']), struct.unpack('<2f', blocks['p1'])
            prefix = [LINE2D, PROJECT, PROJECT]
        else:
            p0 = struct.unpack('<2f', blocks['point'])
            vector = struct.unpack('<2f', blocks['vector'])
            p1 = tuple(b.f32_value(p + case.get('size', 4.0) * v) for p, v in zip(p0, vector))
            prefix = [VECTOR2D, LINE2D, PROJECT, PROJECT]
        new_blocks['point0'], new_blocks['point1'] = project(p0), project(p1)
    cache, events, trace, index = _expected_success(new_case, before, new_blocks)
    return cache, events, prefix + trace, index


b.input_image, b.public_arguments, b.expected_success = input_image, public_arguments, expected_success


class Loader(b.Loader):
    def __init__(self, label, path, helper_path, patches=None):
        self.helper_obj = cc.load(helper_path)
        self.helper_sections = b.function_sections(self.helper_obj)
        super().__init__(label, path, patches)
        self.uc.mem_protect(b.CODE, 0x10000, b.u.UC_PROT_ALL)
        for name in WRAPPERS:
            self.function(name)
        self.uc.mem_protect(b.CODE, 0x10000, b.u.UC_PROT_READ | b.u.UC_PROT_EXEC)

    def function(self, name):
        if name != PROJECT:
            return super().function(name)
        old_obj, old_sections = self.obj, self.sections
        self.obj, self.sections = self.helper_obj, self.helper_sections
        try:
            return super().function(name)
        finally:
            self.obj, self.sections = old_obj, old_sections

    def run(self, case, control):
        outcome = super().run(case, control)
        outcome['abi']['esp_final'] = self.uc.reg_read(b.UC_X86_REG_ESP)
        outcome['volatile_eax_diagnostic_only'] = self.uc.reg_read(b.UC_X86_REG_EAX)
        return outcome


def comparable(outcome):
    return {key: value for key, value in b.comparable(outcome).items()
        if key != 'volatile_eax_diagnostic_only'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'scratch/render-debug-2d-runtime-report.json')
    args = parser.parse_args()
    assert not args.output.exists(), 'preserve prior evidence; choose a new output path'
    assert b.digest(BASE_PATH.read_bytes()) == BASE_PIN
    for role, path in PATHS.items():
        assert b.digest(path.read_bytes()) == PINS[role], role
    owner = cc.load(PATHS['mapping_owner'])
    symbol = cc.symbol(owner, '_global_projection3d_mappings')
    payload = cc._section_bytes(owner, owner['sections'][symbol['section'] - 1])[symbol['value']:symbol['value'] + 24]
    assert bytes(payload) == TABLE
    corpus = cases()
    loaders = {role: Loader(role, PATHS[role], PATHS['target_helper'] if role == 'target' else PATHS[role])
        for role in ('target', 'first', 'actual')}
    results = []
    for case in corpus:
        for control in b.FPCW_MODES:
            outcomes = {role: loader.run(case, control) for role, loader in loaders.items()}
            assert comparable(outcomes['target']) == comparable(outcomes['first']) == comparable(outcomes['actual']), case['name']
            results.append({'case': case['name'], 'control': control, 'target': outcomes['target'],
                'first_equal': True, 'actual_equal': True})
    # Real target bytes: fchs -> fabs keeps execution valid but reverses the
    # sign-selected lift for a false sign and positive offset.
    negative = Loader('wrong_offset_sign', PATHS['target'], PATHS['target_helper'],
        {POINT2D: [(0x9F, 0xE0, 0xE1)]})
    control_case = next(c for c in corpus if c['function'] == POINT2D and c['immediate'] and not c['sign'] and not c.get('null'))
    try:
        negative.run(control_case, 0x037F)
    except AssertionError as error:
        rejection = str(error)
        detail = error.args[0]
        assert isinstance(detail, tuple) and len(detail) == 4, rejection
        assert detail[:2] == ('wrong_offset_sign', control_case['name']), rejection
        assert isinstance(detail[2], list) and isinstance(detail[3], list) and detail[2] != detail[3], rejection
    else:
        raise AssertionError('wrong offset-sign mutation was not rejected')
    report = {'scope': __doc__, 'harness_sha256': b.digest(Path(__file__).read_bytes()),
        'base_sha256': BASE_PIN, 'pins': PINS, 'scenario_count': len(corpus),
        'execution_count': len(results) * len(loaders), 'fpcw_modes': list(b.FPCW_MODES),
        'all_pass': True, 'actual_byte_negative': {'detected': True, 'function': POINT2D,
            'kind': 'host copy of genuine target bytes modified before emulator mapping; on-disk object unchanged',
            'offset': 0x9F, 'expected': 0xE0, 'replacement': 0xE1, 'rejection': rejection},
        'projection_probe': {'path': 'tools/audit/project_point2d_runtime.py',
            'sha256': b.digest((ROOT / 'tools/audit/project_point2d_runtime.py').read_bytes()),
            'scope': 'independent helper-only replay; finite agreement and known NaN difference, not helper exact credit'},
        'ownership_limitation': 'Historical split Decals is NODUP; all five current base providers are SELECTANY. Current base/base diagnostic links have no duplicate errors. Mixed base/January-owner links do; neither is a complete executable link.',
        'results': results}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: report[key] for key in ('scenario_count', 'execution_count', 'all_pass', 'actual_byte_negative')}))


if __name__ == '__main__':
    main()
