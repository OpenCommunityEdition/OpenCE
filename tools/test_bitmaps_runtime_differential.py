"""Fixture-free contract and negative checks for the Bitmaps runtime oracle."""
import copy
import struct

import pytest

pytest.importorskip("unicorn")
from tools.audit import bitmaps_runtime_differential as audit


def test_bounded_case_population():
    cases = audit.all_cases()
    assert len(cases) == 34
    assert {case['function'] for case in cases} == set(audit.FUNCTIONS)
    assert len({(case['function'], case['name']) for case in cases}) == 34
    assert sum(case['function'].endswith('_new') for case in cases) == 14


@pytest.mark.parametrize('name', audit.FUNCTIONS)
def test_section_guard_checks_pinned_function(name):
    spec = audit.FUNCTIONS[name]
    prepared = {'name': name, 'symbol': spec['symbol'], 'info': {
        field: spec[field] for field in ('size', 'relocation_count', 'normalized_sha256')
    }}
    assert all(audit.verify_section(prepared).values())
    for field, wrong in (('size', 1), ('relocation_count', 999),
                         ('normalized_sha256', '0' * 64)):
        altered = copy.deepcopy(prepared)
        altered['info'][field] = wrong
        with pytest.raises(AssertionError, match='section guard'):
            audit.verify_section(altered)


@pytest.mark.parametrize('kind', (audit.TYPE_2D, audit.TYPE_3D, audit.TYPE_CUBE))
def test_constructor_models_complete_record_and_pixel_allocation_failure(kind):
    name = {audit.TYPE_2D: 'bitmap_2d_new', audit.TYPE_3D: 'bitmap_3d_new',
            audit.TYPE_CUBE: 'bitmap_cube_map_new'}[kind]
    cases = audit.constructor_cases(name, kind)
    success = audit.constructor_expected_bitmap(cases[0])
    failure = audit.constructor_expected_bitmap(cases[3])
    assert len(success) == len(failure) == 0x30
    assert struct.unpack_from('<I', success)[0] == audit.BITMAP_GROUP_TAG
    assert struct.unpack_from('<H', success, 10)[0] == kind
    assert success[:44] == failure[:44]
    assert struct.unpack_from('<I', success, 44)[0] == audit.PIXELS
    assert failure[44:] == b'\0' * 4
    assert success[22:44] == b'\0' * 22


@pytest.mark.parametrize('index,count', ((0, 4), (1, 5), (2, 5)))
def test_dispatcher_has_real_public_cdecl_arguments(index, count):
    case = audit.dispatcher_cases()[index]
    event, = audit.expected_event_core(case)
    assert event['api'] == case['dispatch_api']
    assert len(event['args']) == count
    assert event['args'] == ['bitmap'] + [0] * (count - 2) + [2]


def returning_address_outcome():
    case = audit.address_cases()[0]
    frame = audit.p32(audit.STOP) + b''.join(audit.p32(arg) for arg in case['args'])
    state = {'input_bitmap': audit.make_bitmap(case),
             'allocated_bitmap': b'A' * audit.BITMAP_SIZE,
             'pixel_prefix': b'P' * 256}
    outcome = {
        'hook_errors': [], 'execution_error': None, 'terminal': 'return',
        'events': [{**item, 'stack_depth': 32} for item in audit.expected_event_core(case)],
        'state': copy.deepcopy(state), 'initial': copy.deepcopy(state),
        'eax': case['expected_eax'], 'eip': audit.STOP,
        'esp': audit.ENTRY_SP + 4, 'eflags': 0x202,
        'registers': {audit.SAVED_REGISTER_NAMES[reg]: value
                      for reg, value in audit.SAVED_REGISTERS.items()},
        'caller_frame': frame, 'expected_caller_frame': frame,
    }
    return case, outcome


def test_argument_slots_are_not_falsely_abi_preserved():
    case, outcome = returning_address_outcome()
    assert audit.validate_semantics(case, outcome) == []
    baseline = audit.canonical_outcome(outcome)
    outcome['caller_frame'] = outcome['caller_frame'][:4] + b'Q' * (len(outcome['caller_frame']) - 4)
    assert audit.validate_semantics(case, outcome) == []
    assert 'caller_frame_sha256' in audit.concise_difference(
        audit.canonical_outcome(outcome), baseline)['differing_components']


@pytest.mark.parametrize('field', ('eax', 'esp', 'callee_saved', 'return_address', 'bitmap', 'df'))
def test_semantic_validator_rejects_abi_and_state_corruption(field):
    case, outcome = returning_address_outcome()
    if field == 'callee_saved':
        outcome['registers']['esi'] = 0
    elif field == 'return_address':
        outcome['caller_frame'] = audit.p32(0) + outcome['caller_frame'][4:]
    elif field == 'bitmap':
        outcome['state']['input_bitmap'] = b'\0' * audit.BITMAP_SIZE
    elif field == 'df':
        outcome['eflags'] |= 0x400
    else:
        outcome[field] ^= 4
    assert audit.validate_semantics(case, outcome)


def test_five_target_comparison_negative_controls():
    case, outcome = returning_address_outcome()
    result = audit.run_negative_controls({case['function']: {
        case['name']: audit.canonical_outcome(outcome)}})
    assert result['pass']
    assert len(result['controls']) == 5
    assert all(result['controls'].values())
