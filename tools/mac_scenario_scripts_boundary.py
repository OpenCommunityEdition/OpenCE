#!/usr/bin/env python
"""Extract unchanged engine HS storage validators for native ownership tests.
Runs on Python 2.6 and 3: usage mac_scenario_scripts_boundary.py [OUTPUT]
(default build/mac-audit/mac_scenario_script_validators.h)
"""
import sys
from mac_gen_io import read_text, write_text

source = read_text('source/hs/hs.c')
names = ['hs_scenario_string_constants_valid', 'hs_scenario_syntax_data_valid']
output = ['/* Actual hs.c storage validators; generated, do not edit. */',
          '#define MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO 19001']
for name in names:
    start = source.index('static boolean ' + name + '(\n\tstruct scenario const *scenario)\n{')
    body = source.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    output.append(source[start:end])
destination = sys.argv[1] if len(sys.argv) > 1 else 'build/mac-audit/mac_scenario_script_validators.h'
write_text(destination, '\n'.join(output) + '\n')
