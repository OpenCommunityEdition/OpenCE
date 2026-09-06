"""Fixture-free checks for the bounded public Path 3D runtime contract."""
from fractions import Fraction
import struct

from tools.audit import path_3d_runtime as oracle


def test_corpus_covers_branches_unused_bits_and_optional_outputs():
    cases = oracle.cases()
    assert len(cases) == len({case['name'] for case in cases}) == 175
    assert sum(case['entry'] == oracle.AVAILABLE for case in cases) == 140
    assert sum(case['entry'] == oracle.BUILD for case in cases) == 35
    assert {case['avoidance'] for case in cases} == {0, 0x80000000, 0x3F800000, 0x7FC00000, 0xDEADBEEF}
    for bits in {case['t_bits'] for case in cases}:
        assert {case['outputs'] for case in cases if case['t_bits'] == bits and case['entry'] == oracle.AVAILABLE} == {0, 1, 2, 3}


def test_available_null_outputs_preserve_memory():
    before = b'\xA5' * 0x1000
    case = {'entry': oracle.AVAILABLE, 'outputs': 0, 'available': False}
    assert oracle.expected_output(case, before) == before


def test_available_endpoint_is_written_even_when_blocked():
    before = b'\xA5' * 0x1000
    case = {'entry': oracle.AVAILABLE, 'outputs': 3, 'available': False}
    result = oracle.expected_output(case, before)
    assert result[oracle.FINISH - oracle.OUTPUT] == 0
    assert result[oracle.ENDPOINT - oracle.OUTPUT:oracle.ENDPOINT - oracle.OUTPUT + 12] == oracle.END_BYTES
    assert sum(a != b for a, b in zip(before, result)) <= 13


def test_build_failure_zeros_only_the_complete_path_result():
    before = b'\xA5' * 0x1000
    result = oracle.expected_output({'entry': oracle.BUILD, 'available': False}, before)
    at = oracle.PATH - oracle.OUTPUT
    assert result[at:at + 0x5C] == b'\0' * 0x5C
    assert result[:at] == before[:at] and result[at + 0x5C:] == before[at + 0x5C:]


def test_build_success_exact_typed_layout():
    result = oracle.expected_output({'entry': oracle.BUILD, 'available': True}, b'\xA5' * 0x1000)
    at = oracle.PATH - oracle.OUTPUT
    path = result[at:at + 0x5C]
    assert path[:4] == b'\x01\0\0\0'
    assert path[4:16] == path[0x20:0x2C] == oracle.END_BYTES
    assert path[0x10:0x14] == path[0x1C:0x20] == b'\xFF' * 4
    assert path[0x14:0x18] == b'\0' * 4
    assert path[0x18:0x1C] == b'\x01\x01\0\0'
    assert path[0x2C:] == b'\0' * 0x30


def test_adjacent_threshold_probes_straddle_the_real_constant():
    real = lambda bits: Fraction(struct.unpack('<f', struct.pack('<I', bits))[0])
    threshold = real(0x3DCCCCCD)
    assert (1 - real(0x3F2F0BB2)) ** 2 >= threshold
    assert (1 - real(0x3F2F0BB3)) ** 2 < threshold
    outcomes = {case['t_bits']: case['available'] for case in oracle.cases() if case['hit']}
    assert outcomes[0x3F2F0BB2] is False and outcomes[0x3F2F0BB3] is True


def test_no_hit_nan_does_not_make_nan_hit_successful():
    cases = [case for case in oracle.cases() if case['t_bits'] == 0x7FC00000]
    assert all(case['available'] == (not case['hit']) for case in cases)
