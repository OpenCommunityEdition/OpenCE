"""MCC selection clearing uses the native operation without widening map or client policy."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, constant, enum_with, function, mutated, read, run, structure


def declaration(source, name):
    match = re.search(r'^static[^;]*\b' + re.escape(name) + r'\[\]\s*=\s*\{.*?^};', source, re.M | re.S)
    assert match, name
    return match.group(0) + '\n'


def generated(fault=None):
    hs = read('source/hs/hs.c')
    ai = read('source/ai/ai_debug.c')
    mcc = read('port/linux/game/mcc_scripts.c')
    types = enum_with(read('source/hs/hs.h'), '_hs_type_void') + '\n'
    types += enum_with(hs, '_hs_syntax_node_primitive_bit') + '\n'
    types += enum_with(hs, '_hs_node_refusal_none') + '\n'
    types += structure(read('source/hs/hs_scenario_definitions.h'), 'hs_syntax_node') + '\n'
    types += '#define NUMBER_OF_AI_DEBUG_ACTOR_RECORDS %d\n' % constant(
        read('source/ai/ai_debug.h'), 'NUMBER_OF_AI_DEBUG_ACTOR_RECORDS')
    policy = declaration(hs, 'hs_function_allowed_in_maps')
    # Derive function indices from the actual policy table, including unnamed entries.
    for name in ('ai_select', 'ai_deselect'):
        before = policy[:policy.index('/* ' + name + ':')]
        types += '#define NATIVE_%s %d\n' % (name.upper(), len(re.findall(r'\b(?:TRUE|FALSE)\s*,', before)) - 1)
    code = function(ai, 'ai_debug_idle_look_clear') + '\n'
    code += function(ai, 'ai_debug_select_encounter') + '\n'
    code += function(ai, 'ai_debug_select_actor') + '\n'
    code += function(read('source/ai/ai_script.c'), 'ai_scripting_deselect') + '\n'
    adapter = function(mcc, 'mcc_ai_deselect_evaluate')
    if fault == 'no-clear':
        adapter = mutated(adapter, 'ai_scripting_deselect();', '')
    if fault == 'ignore-initialization':
        code = mutated(code, 'if (ai_globals->ai_initialized_for_map)', 'if (TRUE)')
    code += adapter + '\n'
    if fault == 'legacy-admission':
        policy = mutated(policy, 'FALSE, /* ai_deselect:', 'TRUE, /* ai_deselect:')
    code += policy
    code += function(hs, 'hs_syntax_try_get') + '\n'
    code += function(hs, 'hs_syntax_node_check') + '\n'
    code += function(hs, 'hs_syntax_node_refusal') + '\n'
    code += function(hs, 'hs_expression_changes_no_game') + '\n'
    return (('types.inc', types), ('under_test.inc', code))


@pytest.mark.parametrize('case', ['clear', 'inactive', 'already-clear', 'actor-only', 'legacy-policy', 'client-policy'])
def test_mcc_ai_deselect_semantics_and_boundaries(case):
    assert run(build('mcc_ai_deselect', generated()), case) == (0, '')


@pytest.mark.parametrize('fault,case', [
    ('no-clear', 'clear'), ('ignore-initialization', 'inactive'), ('legacy-admission', 'legacy-policy'),
])
def test_mcc_ai_deselect_controls_detect_faults(fault, case):
    status, output = run(build('mcc_ai_deselect', generated(fault)), case)
    assert status == CHECK_FAILED, output
