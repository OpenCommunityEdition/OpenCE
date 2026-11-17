"""A deleted object reaches every handler the walk of every object reached; units' label tests are remembered per map."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, mutated, read, run

CASES = ['walk', 'labels', 'new-map']


def generated(fault=None):
    object_types = read('source/objects/object_types.c')
    objects = read('source/objects/objects.c')
    units = read('source/units/units.c')
    label_tests = re.search(r'^#define UNIT_PORT_LABEL_TEST_COUNT.*?^static struct unit_port_label_test unit_port_label_tests\[.*?;$',
                            units, re.S | re.M).group(0)
    code = '\n\n'.join([
        function(object_types, 'object_type_handle_deleted_object'),
        function(object_types, 'object_type_port_handles_deleted_objects'),
        function(objects, 'objects_fix_for_deleted_object'),
        label_tests,
        function(units, 'units_initialize_for_new_map'),
        function(units, 'unit_can_use_weapon'),
    ])
    if fault:
        code = mutated(code, *fault)
    return (('under_test.inc', code + '\n'),)


@pytest.mark.parametrize('case', CASES)
def test_case(case):
    status, output = run(build('deleted_object_walk', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [
    # only a type's first part looked at for a handler
    (('for (i = 0; definition->part_definitions[i]; i++)\n\t\t{\n\t\t\tif (definition->part_definitions[i]->handle_deleted_object)',
      'for (i = 0; definition->part_definitions[i] && i < 1; i++)\n\t\t{\n\t\t\tif (definition->part_definitions[i]->handle_deleted_object)'),
     'walk'),
    # a remembered test found by its seat label alone
    (('\t\t\t\tentry->seat_label == seat_label &&\n\t\t\t\tentry->weapon_label == weapon_label))',
      '\t\t\t\tentry->seat_label == seat_label))'), 'labels'),
    # the remembered tests kept from the last map
    (('\tmemset(unit_port_label_tests, 0, sizeof(unit_port_label_tests));\n', '\n'), 'new-map'),
])
def test_negative_control(fault, case):
    status, output = run(build('deleted_object_walk', generated(fault)), case)
    assert status == CHECK_FAILED, output
