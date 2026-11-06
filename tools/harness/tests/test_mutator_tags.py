"""The mutators' tag edits (mutator_tags.c), made from mutator.toml (port_mutators.c) in a fake map's tags, without assets."""
import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness import CHECK_FAILED, build, enum_with, mutated, read, run
from test_port_mutators import CHECK_KEY, STUBS, sha256_source, stock_mutators


def stripped(path):
    """a source without its #includes (the test's fakes stand in)"""
    return re.sub(r'^#include [<"][^>"\n]+[>"]\n', '', read(path), flags=re.M)


def generated(fault=None):
    tags = stripped('port/linux/game/mutator_tags.c')
    if fault == 'range':
        tags = mutated(tags, 'if (edit->number < field->minimum || edit->number > field->maximum ||', 'if (FALSE ||')
    if fault == 'element':
        tags = mutated(tags, 'if (index >= block->count || !block->address)', 'if (!block->address)')
    if fault == 'undo':
        tags = mutated(tags, '\t\tcsmemcpy(undo->address, undo->bytes, undo->size);\n', '')
    if fault == 'whole':
        tags = mutated(tags, '\t/* (all of them checked before any is made) */\n\twhile (offset < size)',
                       '\twhile (FALSE)')
    if fault == 'runtime':
        tags = mutated(tags, '\t\t*(real *)(address - field->offset + field->runtime_offset) = value;\n', '')
    if fault == 'speed':
        tags = mutated(tags, 'value = edit->number / (real)TICKS_PER_SECOND;', 'value = edit->number;')
    if fault == 'stock':
        tags = mutated(tags, 'return index != NONE ? index : stock_tags_find(group_tag, name);', 'return index;')
    if fault == 'stock_first':
        tags = mutated(tags, 'return index != NONE ? index : stock_tags_find(group_tag, name);',
                       'stock_tags_find(group_tag, name); return index;')
    if fault == 'pose_range':
        tags = mutated(tags, 'edit->number < OBJECT_POSE_MINIMUM_SCALE || edit->number > OBJECT_POSE_MAXIMUM_SCALE)',
                       'FALSE)')
    if fault == 'pose_clear':
        tags = mutated(tags, '\tmutator_tags.element_bytes_used = 0;\n\tobject_pose_clear();\n\tobject_death_clear();\n\texplosion_decal_clear();\n}\n\nboolean',
                       '\tmutator_tags.element_bytes_used = 0;\n\tobject_death_clear();\n\texplosion_decal_clear();\n}\n\nboolean')
    if fault == 'death_clear':
        tags = mutated(tags, '\tmutator_tags.element_bytes_used = 0;\n\tobject_pose_clear();\n\tobject_death_clear();\n\texplosion_decal_clear();\n}\n\nboolean',
                       '\tmutator_tags.element_bytes_used = 0;\n\tobject_pose_clear();\n\texplosion_decal_clear();\n}\n\nboolean')
    if fault == 'where':
        tags = mutated(tags, '\t\t\tif (value < 0 || value >= where->name_count || !(bits & (1UL << value)))\n\t\t\t\tcontinue;\n', '')
    if fault == 'every':
        tags = mutated(tags, '\t\tif (mutator_tags_make_elements(edit, tag_index, reason, size))\n\t\t\tmade++;',
                       '\t\tif (mutator_tags_make_elements(edit, tag_index, reason, size))\n\t\t{\n\t\t\tmade++;\n\t\t\tbreak;\n\t\t}')
    if fault == 'fired':
        tags = mutated(tags, '\t\tif (!where && names && !mutator_tags_fired(tag_index))\n\t\t\tcontinue;\n', '')
    if fault == 'all':
        tags = mutated(tags, 'for (index = 0; mutator_tags_path_each(edit->path, index, path, sizeof(path)); index++)',
                       'for (index = 0; index < 1 && mutator_tags_path_each(edit->path, index, path, sizeof(path)); index++)')
    if fault == 'append':
        tags = mutated(tags, 'count = (edit->append ? block->count : 0) +', 'count =')
    ported = read('port/linux/src/port_mutators.c')
    if fault == 'scaled':
        ported = mutated(ported, 'edit->number * option_value(mutator, &mutator->options[edit->option]) / 100.0',
                         'edit->number')
    if fault == 'extremes':
        ported = mutated(ported, 'for (index = 0; index < 2; index++)\n\t{\n\t\tedit_number_write',
                         'for (index = 0; index < 0; index++)\n\t{\n\t\tedit_number_write')
    source = sha256_source(ported).replace(
        'int mutators_key_check(const char *key, double value, char *reason, unsigned long size);', CHECK_KEY)
    enums = '\n'.join((enum_with(read('source/game/game_globals.h'), '_material_dirt = 0'),
                       enum_with(read('source/items/projectile_definitions.h'), '_projectile_material_response_disappear')))
    return STUBS + (('port_mutators.h', read('port/linux/src/port_mutators.h')),
                    ('mutator_files.h', stock_mutators()),
                    ('tomlc17.h', read('port/third_party/tomlc17/tomlc17.h')),
                    ('tomlc17.inc', read('port/third_party/tomlc17/tomlc17.c')),
                    ('mutators_header.inc', read('port/linux/game/mutators.h')),
                    ('object_pose_header.inc', '\n'.join(
                        line for line in read('port/linux/game/object_pose.h').splitlines()
                        if line.startswith('#define OBJECT_POSE_'))),
                    ('mutator_tags.inc', tags), ('enums.inc', enums),
                    ('port_mutators_source.inc', source))


@pytest.mark.parametrize('case', ['ma5b', 'plasma', 'magazine', 'missing', 'stock', 'refused', 'pose',
                                  'paintball', 'bytes'])
def test_case(case):
    status, output = run(build('mutator_tags', generated()), case)
    assert status == 0, output


@pytest.mark.parametrize('fault,case', [('range', 'refused'), ('element', 'missing'), ('undo', 'ma5b'),
                                        ('whole', 'bytes'), ('append', 'ma5b'), ('runtime', 'plasma'),
                                        ('speed', 'plasma'), ('element', 'magazine'), ('stock', 'stock'),
                                        ('stock_first', 'stock'), ('pose_range', 'pose'),
                                        ('pose_clear', 'pose'), ('death_clear', 'paintball'),
                                        ('all', 'paintball'), ('scaled', 'paintball'), ('extremes', 'paintball'),
                                        ('where', 'paintball'), ('every', 'paintball'),
                                        ('fired', 'paintball')])
def test_negative(fault, case):
    status, output = run(build('mutator_tags', generated(fault)), case)
    assert status == CHECK_FAILED, output


def test_names():
    """the names a mutator gives materials and crosshair types are the game's enums', in their order"""
    tags = read('port/linux/game/mutator_tags.c')
    materials = re.findall(r'_material_(\w+)', enum_with(read('source/game/game_globals.h'), '_material_dirt = 0'))
    listed = re.findall(r'"(\w+)"', tags[tags.index('mutator_material_names['):tags.index('};', tags.index(
        'mutator_material_names['))])
    assert listed == materials[:materials.index('hunter_shield') + 1]
    hud = read('source/interface/hud_weapon.c')
    states = re.findall(r'_crosshair_state_(\w+)', enum_with(hud, '_crosshair_state_aim = 0'))
    crosshairs = re.findall(r'"(\w+)"', tags[tags.index('mutator_crosshair_type_names[]'):tags.index('};', tags.index(
        'mutator_crosshair_type_names[]'))])
    assert crosshairs == states
    responses = re.findall(r'_projectile_material_response_(\w+)',
                           enum_with(read('source/items/projectile_definitions.h'),
                                     '_projectile_material_response_disappear'))
    assert re.findall(r'"(\w+)"', tags[tags.index('mutator_projectile_response_names['):tags.index('};', tags.index(
        'mutator_projectile_response_names['))]) == responses
