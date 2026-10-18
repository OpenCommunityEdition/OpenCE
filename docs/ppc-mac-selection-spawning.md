# Animation selection and object-construction prerequisites

Work observed 2026-10-15. Original installed map files remain untouched.
This increment does not establish successful `object_new`, a spawned biped,
weapon creation, movement simulation or a playable Blood Gulch session.

## Native selection graph

`retail_animation_graph.c` owns native seat/class/type records, aiming bounds,
IK names, animation-index lists, sound handles and all decoded uncompressed
clips. Source spans, counts, indices, strings, finite values, complete salted
sound handles and next-animation-chain cycles are checked. Total clip payloads
are bounded. Unsupported nonempty root animation families are rejected rather
than exposed as fake empty blocks. This is selection data; graph node payloads
and remaining general animation families still need full native support.

MacBook-built readers run on Intel MacBook, PowerBook G4 and iBook G4 with
identical results: 24 seats,58 classes,127 types,211 lists,3899 indices,44 IK
points,23 sounds,254 clips; digest `0a472803`. All12 mutation checks pass on each
machine. Host ASan/UBSan runs pass as well.

## Constructor input bundle

`retail_object_spawn.c` decodes a deliberately named input bundle, never an
engine-tag overlay. It follows multiplayer biped/model references and converts
relevant object placement fields/references, biped dimensions and cached node
indices, initial weapon references, weapon labels, magazine counts/ammunition
and trigger/projectile metadata. It checks referenced tag groups and full
salted handles, bounds counts and rejects nonfinite/malformed values.

Observed on all three Macs, digest `7ea7d0c1`:

| Field | Blood Gulch value |
| --- | --- |
| Biped | `e2070093`, pelvis/head nodes 0/12 |
| Standing/crouching height and radius | 0.7 / 0.5 / 0.2 |
| Assault rifle | `e25e00ea`, label `ar`, 1 magazine/1 trigger |
| Rifle initial ammo | 240 total: expected 60 loaded,180 spare |
| Pistol | `e34d01d9`, label `hp`, 1 magazine/1 trigger |
| Pistol initial ammo | 60 total: expected 12 loaded,48 spare |

Ammo expectations follow the inspected `weapon_new` arithmetic, not execution
of that constructor. Thirty malformed/synthetic checks pass on all three Macs;
local ASan/UBSan also passes. The bundle omits substantial tag payloads and must
not be cast to `biped_definition` or `weapon_definition`.

## Actual object allocation and relocation

`mac_object_pool_probe.c` links the real `data.c` and `memory_pool.c` units,
using native engine object headers (12 bytes), biped allocations (1152 bytes)
and weapon allocations (636 bytes). It exercises allocation, freeing a middle
block, compaction with updated owner pointers, growing a biped-sized allocation
for19 node matrices, byte preservation, allocation failure and full cleanup.
Twenty checks pass on Intel and both G4s. This validates allocator prerequisites;
it intentionally does not claim that allocating storage runs any constructor.

## Native compiler boundary

`engine_xdk_prefix.h` provides native Mac C calling conventions for clean XDK
declarations, namespaced Winsock types and scoped aliases for duplicate typedefs
that GCC4 rejects. It uses only `port/include/xdk`, not proprietary SDK headers.
It preserves target architecture and TLS declarations; it does not implement
Xbox services or structured exception handling. Whole `units.c` compiles on
both MacBook targets with this boundary. Whole `bipeds.c` and `weapons.c`
also compile for PPC/i386. `tools/mac_constructor_compile.sh` reproduces them;
`mac_spawn_layouts.c` checks72 actual header sizes/offsets used by the input
bundle. All72 pass on Intel and both G4s. These are compilation/layout results,
not execution of the constructors. Real native services must be supplied
before calling functions that use them.

## Reproduction and outstanding creation work

Run on the MacBook source stage:

- `sh tools/mac_animation_graph_checks.sh <bloodgulch.map>`
- `sh tools/mac_object_spawn_checks.sh <bloodgulch.map>`
- `sh tools/mac_object_pool_checks.sh`

Run the corresponding PPC binaries on each G4. Evidence in ignored
`build/mac-audit`: `animation-graph-build.txt`, `object-spawn-build.txt`,
`object-pool-build.txt`, `powerbook-spawn-data.txt`, `ibook-spawn-data.txt`,
`powerbook-object-pool.txt`, `ibook-object-pool.txt`, `graph-sanitized.txt`,
`object-spawn-sanitized.txt`, `units-xdk-*-build.txt`, `constructor-build.txt`,
`powerbook-spawn-layouts.txt` and `ibook-spawn-layouts.txt`.

Actual construction still needs native complete object functions/change-color
permutations/widgets and attachment descriptors, unit camera/dialogue/HUD blocks,
owned model geometry/animation data, collision resistance/regions/node BSPs,
remaining weapon/item/effect payloads, object-type registration, node matrices,
map cluster reconnection and attachment/effect services. The actual
`unit_add_initial_weapons` recursively calls `object_new`; it cannot be replaced
with successful-looking dummy weapon records. See `ppc-mac-biped-plan.md`.

## Real engine lookup, fitting and seat mutation

`mac_animation_selection_checks.sh` builds whole `units.c`, `objects.c`,
`data.c` and `model_animations.c` with dead stripping. The scoped native tag
registry refuses unknown handles/groups. Case-insensitive label comparison is
an ASCII CRT adapter. No gameplay function is replaced with a success stub.

The adapter binds native names, metadata and nested selection descriptors for
this function subset. It is not a complete engine animation graph: transforms,
IK/aiming services and remaining tag payloads must not be inferred from this
binding. The diagnostic unit definition contains only fields required by the
selected function. It is never passed to object_new or presented as a converted
biped tag. Its temporary unit record uses a real datum-managed object header,
and the engine's real object_get_and_verify_type resolves it.

Actual results on MacBook Intel, PowerBook G4 and iBook G4:

- 533 checks, zero failures on each machine.
- All 254 animation names and uppercase variants resolve through the actual
  `animation_graph_get_animation_by_name` implementation.
- Nested standing/pistol idle and forward slots resolve to 146/150;
  standing/rifle slots resolve to 168/173. Slot meanings come from engine enums.
- Actual `unit_animation_get_fitting` accepts valid node capacity and rejects
  invalid indices/insufficient capacity. One logged engine error is EXPECTED:
  a 19-node clip cannot fit an 18-node model. Do not treat it as hardware failure.
- Actual `unit_set_seat("stand")` updates the diagnostic unit to seat 4, base 2,
  class 5, type 0, flags 0. This last matching class follows the original function's
  iteration behavior with no requested weapon label. Uppercase behaves equally;
  an unknown seat fails and preserves sentinel fields.

This verifies actual seat-state mutation, not automatic locomotion selection,
weapon pickup/equipping, constructor execution or simulation. The full biped
creation milestone remains open. Both G4s ran Leopard; no Tiger runtime claim.

Evidence: `animation-selection-build.txt`, `powerbook-animation-selection.txt`,
`ibook-animation-selection.txt`, `animation_selection_imports_*.txt` in
`build/mac-audit`. Final PPC binary SHA256:
`f693e55583e0daea1c22b45bb8306f470d1486f36760e3d0a15d6ae0cf0b44f4`.
Reproduce with `sh tools/mac_animation_selection_checks.sh <bloodgulch.map>`
on the MacBook, then run its `engine_animation_selection_ppc` on each G4.
The final executable has no unresolved weak imports. Existing 22 reader/geometry
unit tests also pass. No source files changed in this increment; native-only
files do not require a repeated Android/web build.
