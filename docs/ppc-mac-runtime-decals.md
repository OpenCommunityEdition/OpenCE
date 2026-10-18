# PPC Mac runtime decal conversion boundary

The native world now owns Blood Gulch's eight runtime decal records, four
scenario palette references and the two cluster ranges: cluster 3 owns [0,4)
and cluster 19 owns [4,8). Palette indices are 3,3,3,3,0,0,0,0. Position floats
are decoded from little-endian bits; signed angle bytes are preserved. Bounds,
finite positions, palette handles, copied names and complete non-overlapping
cluster coverage are validated before publication.

## Native definitions and assets

The clean source proves a 0x10c decal definition and a 0x68 nested shader,
with shader map handle at 0xe4. Native schema witnesses compile on PPC and
Intel. The converter owns fields and references, validates domains and rejects
cyclic decal chains. The asset loader recursively publishes decal definitions
and their bitmap dependencies before source mappings close. Blood Gulch loads
four decal definitions; total owned bitmap count is five, including earlier
contrail assets.

The standalone decoder probe passes 23 checks on the MacBook, PowerBook G4
and iBook G4, including invalid type, NaN radius, wrong bitmap group and a
self-cycle. Logs: `build/mac-audit/decal-decode-{build,powerbook,ibook}.txt`.

## Engine integration

The real structure/decal/rasterizer initialization callbacks are wired to the
native headless device identity. The real LRUV deletion clears its cache and
embedded array; `debug_free` now recognizes registered arena ownership, avoiding
libc frees of arena-backed storage. Arena teardown still checks disposal and
live allocations. Headless identity is not a working GPU renderer.

The full `objects_update` probe now resolves first-person weapon models,
animation graphs and native dialogue definitions. The installed player dialogue
has no live sound references. Documented zero-filled dialogue enum gaps are
normalized to NONE only after validating their bytes; other invalid references
remain rejected.

Intel debugger evidence shows eight actual decal datums created by the
visibility transition inside `objects_update`. The callback subsequently stops
in `unit_update -> lights_illumination_at_point -> structure_test_vector ->
structure_render_surface_from_point_and_leaf`, requesting surface reference 2168
from an unpopulated block. This is the next world-conversion boundary, not a
successful full object update or playable map. Evidence:
`build/mac-audit/object-update-surface-backtrace.txt`.

A separate `--decal-lifecycle` mode tests the genuine visibility-driven creation
and removal callbacks and two map lifecycles without claiming completion of
`objects_update`. Full updates keep their original callbacks and fail visibly
at unsupported dependencies. No drawing or complete game tick is established.

The explicit lifecycle probe now passes on Intel and both G4s: two map cycles,
eight decals created and removed per cycle, 137 runtime checks and zero
failures. Logs: `build/mac-audit/decal-lifecycle-{build,powerbook,ibook}.txt`.
The expanded eight-step biped regression also passes on all three machines,
including exact numerical checks of the first decal position and signed angles.
