# `vehicles.obj` typed four-leaf wave (Jonas, 2026-08-27)

## Result

Four ordinary-ABI owners are byte-exact and relocation-exact:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_vehicle_place` | 38 | 48 | 2 | `8f169239f9b6853abf4a29020a714c5b57951f3c0c4c9787ba623235475d71b9` |
| `_vehicle_causes_collision_damage` | 44 | 48 | 2 | `fc7bcbf23697fd8da1098d90d0032e6ea0d589824735ca3b95d151678242b8fb` |
| `_vehicle_is_flipped` | 44 | 48 | 2 | `36cf257c3d2f357d48340b1c1476d3c0c48fc59568f3aa3f9fe88b9e276d296d` |
| `_vehicle_hover` | 71 | 80 | 2 | `fa55dee91e21e92f2ec1ef82a136e34f8aa1fe3649b5079219002f5cedfcb110` |

`vehicles.obj` advances from 8/39 to 12/39 strict-exact functions and from
144 to 368 matched padded code bytes. The wave adds 197 meaningful and 224
padded bytes. All eight inherited exact owners remain exact, so `lost_exact`
is empty. The object remains `NonMatching`; no data or whole-object completion
is claimed.

## Frozen evidence

- January split object raw SHA-256:
  `F3DE171EF028B6254B0A2A53505D3CCBD47C5B3C80B5B7FE5251A917B9B91A81`.
- Production candidate object raw SHA-256:
  `3D7A72F62A060D16706C88760FC850A0BDC0EE7B25C5F09CB0A57AF9092891DF`.
- Pre-wave source Git blobs:
  - `source/units/vehicles.c`:
    `2f4d88e5afc049805d06ee944f0d3e1d066e3882`.
  - `source/units/vehicles.h`:
    `8d148cc8daf3c28fb2070d45fdbd4be015468706`.
  - `source/units/vehicle_definitions.h`:
    `16a03ed21569cd009f828ec77e21bbcb5ed867ec`.
- Retained source Git blobs:
  - `source/units/vehicles.c`:
    `bbe70172a0ffc7fbb6529fed1493ffd86364f057`.
  - `source/units/vehicles.h`:
    `22c91477c2b958e30225bd1ebef38c0926b8c490`.
  - `source/units/vehicle_definitions.h`:
    `f5dae19bb9a65401bc6b7b342deacbceb16802be`.

## Typed source authority

January's PDB names, external COMDAT boundaries, cdecl stack inputs, and exact
relocation identities are the PC authority. HCEA independently authenticates
the behavior and field layouts in:

- `build/audit/refs/halocea/src/blam/units/vehicle_place.c`;
- `build/audit/refs/halocea/src/blam/units/vehicle_causes_collision_damage.c`;
- `build/audit/refs/halocea/src/blam/units/vehicle_is_flipped.c`;
- `build/audit/refs/halocea/src/blam/units/vehicle_hover.c`;
- `build/audit/refs/halocea/src/headers/scenario_vehicle_datum.h`;
- `build/audit/refs/halocea/src/headers/vehicle_definition.h`;
- `build/audit/refs/halocea/src/headers/vehicle_datum.h`; and
- `build/audit/refs/halocea/src/headers/vehicle_flags.h`.

The retained PC views use named fields plus compile-time size and offset
assertions. Scenario placement is typed at permutation `+0x28` and unit data
`+0x48`. Collision damage reads vehicle-definition flags at `+0x2F0` through
`vehicle_specific_definition_get`. Flipped state reads the typed object up
vector. Hover state uses `vehicle_runtime_get`, a 16-bit flags field at
`+0x424`, and the typed hover anchor at `+0x454`. The 16-bit flags type is
material: it makes VC7 emit January's direct byte OR/AND instructions while
preserving the database-authenticated field width.

No owner uses raw object or tag access. Typed object access is centralized in
the vehicle subsystem macro, and typed tag access is centralized in the
vehicle-definition macro. Every parameter is on its own line, `void` argument
lists follow house style, and every retained function ends in an explicit
return.

## Rejected adjacent candidates

Two HCEA-backed neighbors were measured and removed:

- `_vehicle_reset` reached the target's 176-byte size, two relocation
  identities, and instruction-for-instruction field stores. Its remaining
  mismatch was only the placement of caller stack cleanup relative to six
  independent post-`csmemset` stores. Target normalized SHA-256:
  `603882e23140e0be20e31e5becbb77dc7b093f50dafb9d91847fd2876551c9ca`.
- `_vehicle_new` reached the target's 112-byte size, four relocation
  identities, and exact control/data topology. Its remaining mismatch was an
  EAX/ECX register allocation permutation in the at-rest flag selection.
  Target normalized SHA-256:
  `03130ee5339659924cdc47c9f141b23a10dc9a8921098aec20aa49e51804efff`.

Neither residual owner is retained, parked, excepted, or credited. No register
steering, barrier, qualifier experiment, ABI annotation, or compiler-control
device was used.

## Strict comparison and containment

Direct `section_infos_equal` comparison passes all four owners, including every
relocation address, type, identity, and addend. The whole-unit scratch census
reports 12 exact, 0 residual, and 27 unwritten owners. The production board
repeats 12/39 and 368/17,232 padded code bytes.

## Repository gates

- Complete Ninja build: pass.
  - 470 semantic units scanned and zero unit errors.
  - 4,481 functions evaluated, 4,358 semantic-exact, and 4,385
    accepted-exact.
  - Campaign: 377/833 complete objects, 4,358/11,060 exact functions, and
    521,397/2,198,102 meaningful code bytes.
  - Halo: 275/468 complete objects, 4,191/7,574 exact functions, and
    508,483/1,770,166 meaningful code bytes.
- `python -m tools.audit_object_admission`: pass, zero candidates, zero
  contradictions, and zero revocations.
- `python -m tools.parked_functions`: pass, three active, zero stale, and zero
  invalid entries.
- `python -m pytest -q`: 194 passed.
- `git diff --check`: pass.
