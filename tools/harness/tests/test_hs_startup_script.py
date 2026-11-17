"""Only the executing script and an enabled scenario boolean count for a map."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run

CASES = ['startup', 'dormant', 'console', 'initializer', 'idle', 'global-boolean',
         'global-absent', 'global-other-type', 'global-disabled', 'global-engine']
CONTROLS = {
    'any-thread-type': ('thread->type!=_hs_thread_type_script', 'FALSE', 'console'),
    'uninitialized-read': ('!hs_runtime_globals.initialized || ', '', 'idle'),
    'any-type': ('hs_global_get_type(designator)!=_hs_type_boolean || ', '', 'global-other-type'),
    'disabled-read': ('hs_scenario_global_initializer_disabled(designator)', 'FALSE', 'global-disabled'),
    'engine-read': ('((word)designator & 0x8000)', 'FALSE', 'global-engine'),
}


def generated(fault=None):
    runtime = read('source/hs/hs_runtime.c')
    code = '\n'.join(function(runtime, name) for name in
                     ('hs_runtime_port_executing_script_type', 'hs_runtime_port_scenario_boolean'))
    if fault:
        before, after, _ = CONTROLS[fault]
        code = mutated(code, before, after)
    section = runtime[runtime.index("/* ---------- constants */"):]
    enums = enum_with(section, '_hs_thread_type_script') + '\n'
    enums += enum_with(read('source/hs/hs_scenario_definitions.h'), '_hs_script_startup') + '\n'
    enums += enum_with(read('source/hs/hs.h'), '_hs_type_boolean') + '\n'
    return (('enums.inc', enums), ('under_test.inc', code))


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('hs_startup_script', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault', sorted(CONTROLS))
def test_negative_control(fault):
    status, output = run(build('hs_startup_script', generated(fault)), CONTROLS[fault][2])
    assert status == CHECK_FAILED, output
