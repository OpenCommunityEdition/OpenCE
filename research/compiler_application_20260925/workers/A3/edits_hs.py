"""A3 edit groups for source/hs/hs_runtime.c (cards H-1..H-4). Used by tools/mkvar.py."""
import os
import re

_WT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
_SRC = open(os.path.join(_WT, 'source', 'hs', 'hs_runtime.c'), 'rb').read().decode('latin-1').replace('\r\n', '\n')

EDITS = {}

# H-1: authentic stringified enum_value assertion in hs_inspect_enum
EDITS['H1'] = [(
    '\tstruct hs_enum_definition *enum_definition;\n'
    '\n'
    '\tenum_definition = &hs_enum_table[type-_hs_type_enum_game_difficulty];\n'
    '\tmatch_assert("c:\\\\halo\\\\source\\\\hs\\\\hs_library_internal_runtime.h", 0x27b,\n'
    '\t\tHS_TYPE_IS_ENUM(type));\n'
    '\tmatch_vassert("c:\\\\halo\\\\source\\\\hs\\\\hs_library_internal_runtime.h", 0x27c,\n'
    '\t\tvalue.short_integer>=0 && value.short_integer<enum_definition->count,\n'
    '\t\t"enum_value>=0 && enum_value<enum_definition->count");\n'
    '\n'
    '\tsprintf(result, "%s", enum_definition->values[value.short_integer]);\n',
    '\tstruct hs_enum_definition *enum_definition;\n'
    '\tshort enum_value= value.short_integer;\n'
    '\n'
    '\tenum_definition = &hs_enum_table[type-_hs_type_enum_game_difficulty];\n'
    '\tmatch_assert("c:\\\\halo\\\\source\\\\hs\\\\hs_library_internal_runtime.h", 0x27b,\n'
    '\t\tHS_TYPE_IS_ENUM(type));\n'
    '\tmatch_assert("c:\\\\halo\\\\source\\\\hs\\\\hs_library_internal_runtime.h", 0x27c,\n'
    '\t\tenum_value>=0 && enum_value<enum_definition->count);\n'
    '\n'
    '\tsprintf(result, "%s", enum_definition->values[enum_value]);\n',
)]

# H-2: const-qualify extern declarations whose definitions are const (hs.c, hs_globals_external.c)
_h2 = []
for m in re.finditer(r'^extern (short|long|boolean|real) (_hs_type_\w+_default|hs_type_sizes\[NUMBER_OF_HS_TYPES\]|hs_external_global_count);$',
                     _SRC, flags=re.M):
    _h2.append((m.group(0), 'extern %s const %s;' % (m.group(1), m.group(2))))
EDITS['H2'] = _h2

# H-3: HCEX capacity constants (MAXIMUM_NUMBER_OF_HS_GLOBALS is also named by January's diagnostic string)
EDITS['H3'] = [
    ('enum\n{\n\tHS_THREAD_STACK_SIZE = 0x200\n};\n',
     'enum\n{\n\tMAXIMUM_NUMBER_OF_HS_THREADS = 0x100,\n\tMAXIMUM_NUMBER_OF_HS_GLOBALS = 0x400,\n'
     '\tHS_THREAD_STACK_SIZE = 0x200\n};\n'),
    ('game_state_data_new("hs thread", 0x100, 0x218);',
     'game_state_data_new("hs thread", MAXIMUM_NUMBER_OF_HS_THREADS, sizeof(struct hs_thread_datum));'),
    ('game_state_data_new("hs globals", 0x400, 8);',
     'game_state_data_new("hs globals", MAXIMUM_NUMBER_OF_HS_GLOBALS, sizeof(struct hs_global_datum));'),
    ('hs_external_global_count*2<0x400,', 'hs_external_global_count*2<MAXIMUM_NUMBER_OF_HS_GLOBALS,'),
]

_CAN_CAST_OLD = (
    '\tif (HS_TYPE_IS_OBJECT(desired_type))\n'
    '\t{\n'
    '\t\tobject_type = desired_type - _hs_type_object;\n'
    '\t\tif (HS_TYPE_IS_OBJECT(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn hs_object_type_can_cast(\n'
    '\t\t\t\tactual_type-_hs_type_object,\n'
    '\t\t\t\tobject_type);\n'
    '\t\t}\n'
    '\t\telse if (!HS_TYPE_IS_OBJECT_NAME(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn FALSE;\n'
    '\t\t}\n'
    '\n'
    '\t\tgoto cast_object_type;\n'
    '\t}\n'
    '\telse if (HS_TYPE_IS_OBJECT_NAME(desired_type))\n'
    '\t{\n'
    '\t\tif (!HS_TYPE_IS_OBJECT_NAME(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn FALSE;\n'
    '\t\t}\n'
    '\n'
    '\t\tobject_type = desired_type - _hs_type_object_name;\n'
    '\n'
    'cast_object_type:\n'
    '\t\treturn hs_object_type_can_cast(\n'
    '\t\t\tactual_type-_hs_type_object_name,\n'
    '\t\t\tobject_type);\n'
    '\t}\n')

# H-4a: no goto, two textual returns
EDITS['H4a'] = [(_CAN_CAST_OLD,
    '\tif (HS_TYPE_IS_OBJECT(desired_type))\n'
    '\t{\n'
    '\t\tobject_type = desired_type - _hs_type_object;\n'
    '\t\tif (HS_TYPE_IS_OBJECT(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn hs_object_type_can_cast(\n'
    '\t\t\t\tactual_type-_hs_type_object,\n'
    '\t\t\t\tobject_type);\n'
    '\t\t}\n'
    '\t\telse if (HS_TYPE_IS_OBJECT_NAME(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn hs_object_type_can_cast(\n'
    '\t\t\t\tactual_type-_hs_type_object_name,\n'
    '\t\t\t\tobject_type);\n'
    '\t\t}\n'
    '\t\telse\n'
    '\t\t{\n'
    '\t\t\treturn FALSE;\n'
    '\t\t}\n'
    '\t}\n'
    '\telse if (HS_TYPE_IS_OBJECT_NAME(desired_type))\n'
    '\t{\n'
    '\t\tif (!HS_TYPE_IS_OBJECT_NAME(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn FALSE;\n'
    '\t\t}\n'
    '\n'
    '\t\tobject_type = desired_type - _hs_type_object_name;\n'
    '\n'
    '\t\treturn hs_object_type_can_cast(\n'
    '\t\t\tactual_type-_hs_type_object_name,\n'
    '\t\t\tobject_type);\n'
    '\t}\n')]

# H-4b: /Od shape (one call in the object branch through a computed actual object type)
EDITS['H4b'] = [(_CAN_CAST_OLD,
    '\tif (HS_TYPE_IS_OBJECT(desired_type))\n'
    '\t{\n'
    '\t\tshort actual_object_type;\n'
    '\n'
    '\t\tobject_type = desired_type - _hs_type_object;\n'
    '\t\tif (HS_TYPE_IS_OBJECT(actual_type))\n'
    '\t\t{\n'
    '\t\t\tactual_object_type = actual_type-_hs_type_object;\n'
    '\t\t}\n'
    '\t\telse if (HS_TYPE_IS_OBJECT_NAME(actual_type))\n'
    '\t\t{\n'
    '\t\t\tactual_object_type = actual_type-_hs_type_object_name;\n'
    '\t\t}\n'
    '\t\telse\n'
    '\t\t{\n'
    '\t\t\treturn FALSE;\n'
    '\t\t}\n'
    '\n'
    '\t\treturn hs_object_type_can_cast(\n'
    '\t\t\tactual_object_type,\n'
    '\t\t\tobject_type);\n'
    '\t}\n'
    '\telse if (HS_TYPE_IS_OBJECT_NAME(desired_type))\n'
    '\t{\n'
    '\t\tif (!HS_TYPE_IS_OBJECT_NAME(actual_type))\n'
    '\t\t{\n'
    '\t\t\treturn FALSE;\n'
    '\t\t}\n'
    '\n'
    '\t\tobject_type = desired_type - _hs_type_object_name;\n'
    '\n'
    '\t\treturn hs_object_type_can_cast(\n'
    '\t\t\tactual_type-_hs_type_object_name,\n'
    '\t\t\tobject_type);\n'
    '\t}\n')]

# H-4c: fuller /Od shape on top of H-4b (object_name branch passes desired-object_name directly)
EDITS['H4c'] = [
    ('\tshort desired_type)\n{\n\tshort object_type;\n\n\tmatch_assert("c:\\\\halo\\\\SOURCE\\\\hs\\\\hs_runtime.c", 0x5a4,\n',
     '\tshort desired_type)\n{\n\tmatch_assert("c:\\\\halo\\\\SOURCE\\\\hs\\\\hs_runtime.c", 0x5a4,\n'),
    ('\t\tshort actual_object_type;\n'
     '\n'
     '\t\tobject_type = desired_type - _hs_type_object;\n',
     '\t\tshort object_type= desired_type - _hs_type_object;\n'
     '\t\tshort actual_object_type;\n'
     '\n'),
    ('\t\tobject_type = desired_type - _hs_type_object_name;\n'
     '\n'
     '\t\treturn hs_object_type_can_cast(\n'
     '\t\t\tactual_type-_hs_type_object_name,\n'
     '\t\t\tobject_type);\n',
     '\t\treturn hs_object_type_can_cast(\n'
     '\t\t\tactual_type-_hs_type_object_name,\n'
     '\t\t\tdesired_type-_hs_type_object_name);\n'),
]

# H-1c: const-correct enum definition pointer (hs.h:242 declares hs_enum_table const)
EDITS['H1c'] = [
    ('\tstruct hs_enum_definition *enum_definition;\n\tshort enum_value= value.short_integer;\n',
     '\tstruct hs_enum_definition const *enum_definition;\n\tshort enum_value= value.short_integer;\n'),
]

# H-5 (research probe): HCEX 'static long f(long)' converters
def _between(start, end, occurrence=1):
    i = -1
    for _ in range(occurrence):
        i = _SRC.index(start, i + 1)
    j = _SRC.index(end, i)
    return _SRC[i:j + len(end)]

_H5_PROTO_OLD = _between('static union hs_conversion_result hs_long_to_boolean(\n',
                         'static long hs_object_to_object_list(\n\tlong object_index);\n')
_H5_PROTO_NEW = (
    'static long hs_long_to_boolean(\n\tlong l);\n'
    'static long hs_short_to_boolean(\n\tlong s);\n'
    'static long hs_string_to_boolean(\n\tlong n);\n'
    'static long hs_data_to_void(\n\tlong n);\n'
    'static long hs_short_to_real(\n\tlong s);\n'
    'static long hs_long_to_real(\n\tlong l);\n'
    'static long hs_enum_to_real(\n\tlong e);\n'
    'static long hs_real_to_short(\n\tlong r);\n'
    'static long hs_real_to_long(\n\tlong r);\n'
    'static long hs_long_to_short(\n\tlong l);\n'
    'static long hs_object_name_to_object_list(\n\tlong object_name_index);\n'
    'static long hs_object_to_object_list(\n\tlong object_index);\n')
_H5_DEF_OLD = _between('static union hs_conversion_result hs_long_to_boolean(\n\tunion hs_conversion_result value)\n{\n',
                       'static long hs_object_name_to_object_list(\n\tshort object_name_index)\n{\n')
_H5_DEF_NEW = (
    'static long hs_long_to_boolean(\n\tlong l)\n{\n'
    '\t*(boolean *)&l= l==0;\n\n\treturn l;\n}\n\n'
    'static long hs_short_to_boolean(\n\tlong s)\n{\n'
    '\t*(boolean *)&s= *(short *)&s==0;\n\n\treturn s;\n}\n\n'
    'static long hs_string_to_boolean(\n\tlong n)\n{\n'
    '\tlong result;\n\n'
    '\t*(boolean *)&result= csstrlen((char const *)n)==0;\n\n\treturn result;\n}\n\n'
    'static long hs_data_to_void(\n\tlong n)\n{\n'
    '\treturn 0;\n}\n\n'
    'static long hs_short_to_real(\n\tlong s)\n{\n'
    '\tlong result= *(short *)&s;\n\n'
    '\t*(real *)&result= (real)result;\n\n\treturn result;\n}\n\n'
    'static long hs_long_to_real(\n\tlong l)\n{\n'
    '\t*(real *)&l= (real)l;\n\n\treturn l;\n}\n\n'
    'static long hs_enum_to_real(\n\tlong e)\n{\n'
    '\tlong result= *(short *)&e+1;\n\n'
    '\t*(real *)&result= (real)result;\n\n\treturn result;\n}\n\n'
    'static long hs_real_to_short(\n\tlong r)\n{\n'
    '\t*(short *)&r= (short)*(real *)&r;\n\n\treturn r;\n}\n\n'
    'static long hs_real_to_long(\n\tlong r)\n{\n'
    '\treturn (long)*(real *)&r;\n}\n\n'
    'static long hs_long_to_short(\n\tlong l)\n{\n'
    '\t*(short *)&l= (short)l;\n\n\treturn l;\n}\n\n'
    'static long hs_object_name_to_object_list(\n\tlong object_name_index)\n{\n')
EDITS['H5'] = [(_H5_PROTO_OLD, _H5_PROTO_NEW), (_H5_DEF_OLD, _H5_DEF_NEW),
               ('\tobject_index = object_index_from_name_index(object_name_index);\n',
                '\tobject_index = object_index_from_name_index((short)object_name_index);\n')]

EDITS['H5b_S2a'] = [('static long hs_long_to_short(\n\tlong l)\n{\n\t*(short *)&l= (short)l;\n\n\treturn l;\n}\n',
                     'static long hs_long_to_short(\n\tlong l)\n{\n\tlong result;\n\n\t*(short *)&result= (short)l;\n\n\treturn result;\n}\n')]
EDITS['H5b_S2b'] = [('static long hs_long_to_short(\n\tlong l)\n{\n\t*(short *)&l= (short)l;\n\n\treturn l;\n}\n',
                     'static long hs_long_to_short(\n\tlong l)\n{\n\tlong result= l;\n\n\t*(short *)&result= (short)l;\n\n\treturn result;\n}\n')]

# H-6 (research probe): HCEX inspector signatures
EDITS['H6'] = [
    ('typedef void (*hs_inspection_procedure)(\n\tshort type,\n\tunion hs_conversion_result value,\n\tchar *result);\n',
     'typedef void (*hs_inspection_procedure)(\n\tshort type,\n\tlong value,\n\tchar *buffer);\n'),
]
for _n in ('boolean', 'real', 'short_integer', 'long_integer', 'string', 'enum'):
    EDITS['H6'].append(('static void hs_inspect_%s(\n\tshort type,\n\tunion hs_conversion_result value,\n\tchar *result);\n' % _n,
                        'static void hs_inspect_%s(\n\tshort type,\n\tlong value,\n\tchar *buffer);\n' % _n))
    EDITS['H6'].append(('static void hs_inspect_%s(\n\tshort type,\n\tunion hs_conversion_result value,\n\tchar *result)\n{\n' % _n,
                        'static void hs_inspect_%s(\n\tshort type,\n\tlong value,\n\tchar *buffer)\n{\n' % _n))
EDITS['H6'] += [
    ('\tsprintf(result, "%s", value.boolean ? "true" : "false");\n', '\tsprintf(buffer, "%s", (boolean)value ? "true" : "false");\n'),
    ('\tsprintf(result, "%f", value.real);\n', '\tsprintf(buffer, "%f", *(real *)&value);\n'),
    ('\tsprintf(result, "%d", value.short_integer);\n', '\tsprintf(buffer, "%d", (short)value);\n'),
    ('\tsprintf(result, "%ld", value.long_integer);\n', '\tsprintf(buffer, "%ld", value);\n'),
    ('\tsprintf(result, "%s", value.string);\n', '\tsprintf(buffer, "%s", (char const *)value);\n'),
    ('\tshort enum_value= value.short_integer;\n', '\tshort enum_value= (short)value;\n'),
    ('\tsprintf(result, "%s", enum_definition->values[enum_value]);\n', '\tsprintf(buffer, "%s", enum_definition->values[enum_value]);\n'),
    ('\tunion hs_conversion_result *value = hs_stack_allocate(thread_index, sizeof(long));\n\n\tmatch_assert("c:\\\\halo\\\\source\\\\hs\\\\hs_library_internal_runtime.h", 0x2bc,\n',
     '\tlong *value = hs_stack_allocate(thread_index, sizeof(long));\n\n\tmatch_assert("c:\\\\halo\\\\source\\\\hs\\\\hs_library_internal_runtime.h", 0x2bc,\n'),
    ('\t\t\tthread->stack->expression_index)->data)->next_node_index,\n\t\t\t&value->long_integer);\n',
     '\t\t\tthread->stack->expression_index)->data)->next_node_index,\n\t\t\tvalue);\n'),
]
