#!/usr/bin/env python3
"""Extract unchanged engine HS storage validators for native ownership tests."""
from pathlib import Path
source=Path('source/hs/hs.c').read_text()
names=['hs_scenario_string_constants_valid','hs_scenario_syntax_data_valid']
output=['/* Actual hs.c storage validators; generated, do not edit. */',
        '#define MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO 19001']
for name in names:
    start=source.index('static boolean '+name+'(\n\tstruct scenario const *scenario)\n{')
    body=source.index('{',start); depth=1;end=body+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    output.append(source[start:end])
Path('build/mac-audit/mac_scenario_script_validators.h').write_text('\n'.join(output)+'\n')
