"""Fixture-free contracts for the bounded 2D rendering runtime oracle."""
import struct

from tools.audit import render_debug_2d_runtime as oracle
from tools.audit import render_debug_three_runtime as original


def test_base_oracle_is_not_mutated():
    assert oracle.b is not original
    assert oracle.PROJECT not in original.FUNCTION_ADDRESSES
    assert oracle.POINT2D not in original.FUNCTION_ADDRESSES
    assert original.input_image is not oracle.input_image


def test_corpus_has_complete_small_wrapper_grid_and_assertions():
    corpus = oracle.cases()
    assert len(corpus) == len({case['name'] for case in corpus}) == 55
    nulls = [case for case in corpus if case.get('null')]
    assert len(nulls) == 11
    assert {oracle.b.ASSERTIONS[(c['function'], c['null'])][1] for c in nulls} == {
        219, 220, 221, 242, 243, 244, 245, 269, 270, 271, 272}
    for function in oracle.WRAPPERS:
        grid = { (c['projection'], c['sign'], c['immediate']) for c in corpus
            if c['function'] == function and c['name'].count(':') == 3 }
        assert len(grid) == 12


def test_public_argument_counts_and_null_slots():
    for case in oracle.cases():
        _, _, locations = oracle.input_image(case)
        payload = oracle.public_arguments(case, locations)
        words = struct.unpack('<' + 'I' * (len(payload) // 4), payload)
        assert len(words) == (9 if case['function'] == oracle.VECTOR2D else 8)
        assert words[0] == int(case['immediate'])
        assert words[2:4] == (case['projection'], int(case['sign']))
        slots = {'plane': 1, 'point': 4, 'p0': 4, 'p1': 5, 'vector': 5,
            'color': 7 if case['function'] == oracle.VECTOR2D else 6}
        if case.get('null'):
            assert words[slots[case['null']]] == 0


def test_projected_coordinates_follow_axis_and_sign_mapping():
    for axis in range(3):
        for sign in (False, True):
            raw = oracle.projected((2.0, -1.0), (1.0, 1.0, 1.0, 3.0), axis, sign, 0.5)
            result = struct.unpack('<3f', raw)
            x, y = oracle.MAPPINGS[axis][int(sign)]
            assert result[x] == 2.0 and result[y] == -1.0
            assert result[axis] == (2.5 if sign else 1.5)


def test_zero_normal_branch_and_negative_offset():
    assert struct.unpack('<3f', oracle.projected((2.0, -1.0), (0.0, 1.0, 1.0, 3.0),
        0, True, -0.5)) == (-0.5, 2.0, -1.0)


def test_real_callee_trace_is_required_in_success_model():
    for case in oracle.cases():
        if case.get('null'):
            continue
        _, blocks, _ = oracle.input_image(case)
        _, _, trace, _ = oracle.expected_success(case, oracle.b.initial_cache(case), blocks)
        assert trace[0] == case['function']
        assert trace.count(oracle.PROJECT) == (1 if case['function'] == oracle.POINT2D else 2)
        assert oracle.b.POINT in trace or oracle.b.LINE in trace
        assert (oracle.b.CACHE_ENTRY in trace) == (not case['immediate'])


def test_mapping_bytes_have_expected_signed_short_layout():
    assert oracle.TABLE.hex() == '020001000100020000000200020000000100000000000100'
