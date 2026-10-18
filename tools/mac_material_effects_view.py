#!/usr/bin/env python
"""Generate the native endian-safe view of original material effects."""
import sys

source = open("source/effects/material_effects.c", "r").read()
old = '''\t\t\tmaterial_type = scenario_location_underwater(&collision.location, &collision.point, NULL)
\t\t\t\t? _material_effect_underwater_material_type
\t\t\t\t: *(long *)&collision.material_type;'''
new = '''\t\t\t/* port: the Xbox code reads this short-plus-padding pair as a
\t\t\t   little-endian 32-bit word, then keeps its low 16 bits. Read the
\t\t\t   declared material_type field directly on native big-endian PPC. */
\t\t\tmaterial_type = scenario_location_underwater(&collision.location, &collision.point, NULL)
\t\t\t\t? _material_effect_underwater_material_type
\t\t\t\t: mac_native_material_effect_collision_type(&collision);'''
if len(sys.argv) not in (2, 3):
    raise SystemExit("usage: mac_material_effects_view.py output.c [helper.c]")
if source.count(old) != 1:
    raise SystemExit("material-effect collision material source boundary changed")
marker = "/* ---------- public code */"
helper = '''/* port: testable native equivalent of the Xbox little-endian short
   selected from collision.material_type and its following padding word. */
long mac_native_material_effect_collision_type(
\tstruct collision_result const *collision)
{
\treturn collision ? (long)collision->material_type : NONE;
}

'''
if source.count(marker) != 1:
    raise SystemExit("material-effect public-code insertion boundary changed")
source = source.replace(marker, helper + marker, 1)
with open(sys.argv[1], "w") as output:
    output.write(source.replace(old, new))
if len(sys.argv) == 3:
    with open(sys.argv[2], "w") as output:
        output.write('#include "physics/collisions.h"\n')
        output.write(helper)
