"""Fixture-free checks for the independently modeled January obstacle record."""
import struct

from tools.audit import path_obstacles_add_disc_runtime as oracle


def before(case):
    data = bytearray(b'\xA5' * 0x1000)
    struct.pack_into('<3h', data, oracle.OBSTACLES - oracle.OUTPUT,
        case['obstacle_count'], case['count'], case['optional_count'])
    return bytes(data)


def test_corpus_covers_capacity_short_flags_and_failure_contracts():
    cases = oracle.cases()
    assert len(cases) == len({case['name'] for case in cases}) == 84
    assert {case['object'] for case in cases} == {0xFFFFFFFF, 0x12345678}
    assert {case['flags'] for case in cases} == {0, 1, 2, 3, 0x8000, 0x8001, 0x7FFF, 0xFFFF}
    assert sum(case['assert_line'] is not None for case in cases) == 4
    assert sum(case['count'] == 128 for case in cases) == 16


def test_full_set_and_failed_assertions_write_nothing():
    for case in oracle.cases():
        if case['count'] == 128 or case['assert_line'] is not None:
            data = before(case)
            assert oracle.expected_output(case, data) == data


def test_last_disc_ends_at_the_authentic_24_byte_container_boundary():
    assert 8 + 128 * oracle.DISC_SIZE == oracle.CONTAINER_SIZE == 0xC08
    case = next(c for c in oracle.cases() if c['count'] == 127 and c['flags'] == 1)
    data = before(case)
    result = oracle.expected_output(case, data)
    end = oracle.OBSTACLES - oracle.OUTPUT + oracle.CONTAINER_SIZE
    assert result[end:] == data[end:]
    assert result[end - 4:end] == struct.pack('<I', case['center'][2])


def test_optional_flag_uses_only_bit_zero():
    for case in oracle.cases():
        if case['count'] != 1 or case['assert_line'] is not None:
            continue
        result = oracle.expected_output(case, before(case))
        optional = struct.unpack_from('<h', result, oracle.OBSTACLES - oracle.OUTPUT + 4)[0]
        assert optional == case['optional_count'] + bool(case['flags'] & 1)


def test_disc_fields_preserve_projected_xy_radius_and_separate_height():
    case = next(c for c in oracle.cases() if c['count'] == 1 and c['flags'] == 0x8001)
    data = before(case)
    result = oracle.expected_output(case, data)
    base = oracle.OBSTACLES - oracle.OUTPUT
    assert result[base:base + 2] == data[base:base + 2]
    assert result[base + 6:base + 8] == data[base + 6:base + 8]
    assert struct.unpack_from('<HHI4I', result, base + 8 + oracle.DISC_SIZE) == (
        case['flags'], 0xFFFF, case['object'], *case['center'][:2], case['radius'], case['center'][2])


def test_quiet_nan_and_signed_zero_are_not_cleaned_up_by_the_model():
    case = next(c for c in oracle.cases() if c['count'] == 0 and c['center'][0] == 0x80000000)
    result = oracle.expected_output(case, before(case))
    at = oracle.OBSTACLES - oracle.OUTPUT + 8
    assert result[at + 8:at + 24] == struct.pack('<4I', 0x80000000, 0x7FC12345, 0x80000000, 0x7F800000)


def test_assertions_name_the_actual_counter_contracts():
    assert oracle.LITERALS[oracle.DISC_ASSERT][1].endswith(b'disc_count<=MAXIMUM_DISC_COUNT\0')
    assert oracle.LITERALS[oracle.OBSTACLE_ASSERT][1].endswith(b'obstacle_count<=obstacles->disc_count\0')
