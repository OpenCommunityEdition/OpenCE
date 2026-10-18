#!/usr/bin/env python3
"""Export the exact private effect layouts needed by the native decoder."""
from pathlib import Path
import re
source=Path('source/effects/effects.c').read_text()
output=['/* Generated from private effects.c declarations; do not edit. */',
        '#ifndef HALO_MAC_NATIVE_EFFECT_LAYOUT_H','#define HALO_MAC_NATIVE_EFFECT_LAYOUT_H',
        '#include "effects/effect_definitions.h"',
        '#include "shaders/shader_definitions.h"']
for name in ('effect_particles_definition','effect_location_definition'):
    match=re.search(r'struct '+name+r'\n\{[^{}]*\n\};',source)
    if not match:raise ValueError('private effect declaration changed: '+name)
    output.append(match.group())
particle_source=Path('source/effects/particles.h').read_text()
particle=re.search(r'struct particle_definition\n\{[^{}]*\n\};',particle_source)
if not particle:raise ValueError('particle definition changed')
output.append(particle.group())
output.append('typedef char native_particle_definition_size[sizeof(struct particle_definition)==0x164?1:-1];')
output.extend(['typedef char native_effect_particles_size[sizeof(struct effect_particles_definition)==0xe8?1:-1];',
    'typedef char native_effect_locations_size[sizeof(struct effect_location_definition)==32?1:-1];','#endif'])
Path('port/mac/native_effect_layout.h').write_text('\n'.join(output)+'\n')
