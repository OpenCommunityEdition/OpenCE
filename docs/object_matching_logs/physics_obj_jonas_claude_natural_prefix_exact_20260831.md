# `physics.obj` Claude natural-prefix recovery (2026-08-31)

## Result

Claude/Fable campaign commit `e02971dfd` supplied ordinary typed C for the
small unwritten prefix of `physics.obj`.  Four bodies survive this audit as
new strict-exact functions.  Together with the previously accepted
`_physics_test_point`, the translation unit advances from `1/17` to `5/17`
strict functions and remains correctly `NonMatching`.

| Function | Meaningful / padded bytes | Relocations T/B | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_pin_fraction` | `113/128` | `2/2` | `7db92c519ccb57b12f19b04d4b99fc5caaa4190ed48b4025b301357f1d6c023b` | new strict exact |
| `_physics_get_features_in_sphere` | `171/176` | `3/3` | `2bb49780912ee4b1a8ca51d86d50370013c762559a833a912e17516e6b783699` | new strict exact |
| `_physics_instance_new` | `248/256` | `6/6` | `1858dbc01b28d1ed83a44a0c5f27b802380081567417ac8f0dcbbcafa1a8b418` | new strict exact |
| `_physics_test_point` | `154/160` | `2/2` | `2ff01415c7e3798fcb87bd26fc2392bdd80db2ff2a038dba17100ed1b8dc01e3` | inherited strict exact |
| `_physics_test_vector` | `284/288` | `5/5` | `4b4c96c4cfa798065b040f669deb12744c5417a06596f24a63bebec87f665169` | new strict exact |

The wave adds `816` meaningful bytes (`848` padded bytes) without changing
any previously emitted function.  The five accepted sections total `970`
meaningful bytes (`1,008` padded bytes).

## Semantic audit

The retained bodies are coherent engine operations rather than source-shape
spells:

- `pin_fraction` returns a normalized fraction for either increasing or
  decreasing endpoints and clamps at both ends;
- `physics_get_features_in_sphere` transforms every mass point to world space,
  scales its radius by the instance matrix, and emits typed collision features;
- `physics_instance_new` resolves the object's physics definition and builds
  the instance matrix around its center of mass; and
- `physics_test_vector` inverse-transforms the ray, chooses the earliest mass
  point sphere hit, constructs its plane, and transforms that plane back to
  world space.

The apparently unused `center` and `radius` parameters of
`physics_get_features_in_sphere` are not a coincidental-match invention.  The
HCEA full-function oracle at
`research-cache/halocea-full-blobs-20260830/src/blam/physics/physics_get_features_in_sphere.c`
(SHA-256
`5b9ae5fad6d99065b6dc6ccbc35a18e428b8f7c6e5727d595e30af258ffc96db`)
explicitly records those parameters as unused, and January's target
disassembly unconditionally enumerates the transformed mass points.  The
retained typed loop agrees with both independent evidence sets.

All public result types and prototypes live in `physics.h`.  The translation
unit uses the owning collision, object-definition, object-runtime, and physics
definition headers; it does not duplicate external declarations or reconstruct
raw layouts.  No address-named private function or BSS-named global was added.

`physics_test_vector` open-codes the three assignments of the existing
`point_from_line3d` inline.  This is the same ordinary arithmetic and preserves
the exact January instruction schedule, while complying with the campaign rule
that this translation unit must not emit a `point_from_line3d` COMDAT.  The
candidate emits only the expected existing math helper COMDATs, all with COFF
selection `2` (`pick any`); `_point_from_line3d` is absent.  No forced inline,
noinline annotation, optimizer pragma, assembly, `volatile`, barrier, pointer
pun, raw offset, fake dependency, or dead code is present.

## Header blast-radius proof

`physics.h` has six C consumers besides its owner: `vehicles.c`,
`collisions.c`, `point_physics.c`, `objects.c`, `projectiles.c`, and `items.c`.
Each was independently recompiled with the PID-isolated campaign gate after the
header additions.  Against the pre-edit `build/base` objects, every target
function retained the same normalized bytes and relocation identities; the
exact-function sets were identical in all six units.

| Consumer | Exact | Residual | Unwritten | Candidate function changes vs pre-edit base |
|---|---:|---:|---:|---|
| `source/units/vehicles` | 34 | 5 | 0 | none |
| `source/physics/collisions` | 8 | 0 | 12 | none |
| `source/physics/point_physics` | 6 | 0 | 0 | none |
| `source/objects/objects` | 121 | 0 | 0 | none |
| `source/items/projectiles` | 13 | 0 | 17 | none |
| `source/items/items` | 17 | 1 | 0 | none |

This directly checks the C2 definition-position hazard identified elsewhere in
the campaign.  The orchestrator still owns the consolidated full build and
whole-board sweep.

## Deliberately excluded Claude bodies

`_render_debug_physics` is independently strict exact at `272` padded bytes,
12 relocations, normalized SHA-256
`d22fc648c16f41c42b5753553f7589f2dbf74f7322cb83137f38d65ee67e4926`.
It is not integrated in this lane because its authentic dependency
`render_debug_vectors` lacks a declaration in its owner
`source/render/render_debug.h`.  Keeping Claude's translation-unit-local
prototype would violate the owner-header rule.  Reopen it after adding that
owner declaration and rechecking the header's consumers.

Claude's vehicle-collision reconstruction is also excluded.  The atlas names
it `_physics_compute_vehicle_collision`, but the candidate is a natural
register-allocation residual: target and candidate are both `1,168` bytes with
14 matching relocation destinations, while their normalized hashes are
`5aae4029c87d46546c56a16dd66295a34483988ce63e9c2e5ee4db7fe12b6333`
and `7ad4895560b9ee124803527c5545d91b3be3ad61f075a4f1817b35aa213af90a`.
More importantly, Claude's body depends on guessed translation-unit-local
vehicle runtime layouts.  An equal envelope does not justify importing those
guesses.  No residual or guessed structure is retained and no fuzzy credit is
claimed.

## Pending semantic symbol-map renames

This lane was prohibited from editing the shared symbol map.  The January
atlas plus HCEA signatures support the following private owner renames for a
later coordinated metadata change:

- `_code_00140090` -> `_get_material_type` (atlas-backed);
- `_code_001404d0` -> `_compute_ground_plane` (atlas-backed);
- `_code_00140620` -> `_friction_evaluate` (atlas-backed);
- `_code_001412a0` -> `_physics_compute_biped_collision` (atlas-backed);
- `_code_00141710` -> `_physics_compute_vehicle_collision` (atlas-backed);
- `_code_00141ba0` -> `_physics_compute_unit_collisions` (atlas-backed);
- `_code_00141d20` -> `_rotate_vectors3d_by_angular_velocity` (atlas-backed);
- `_code_00142690` -> `_physics_update_old` (inferred, not atlas-backed).

The last name is explicitly weaker evidence: it is the sole unmapped HCEA
private body and occupies HCEA's corresponding position between
`physics_update_new` and `physics_update`.  It must remain labeled inferred
until stronger symbol evidence appears.

## Validation boundary

Validation used `tools/campaign/gate.py source/physics/physics --all`, direct
`tools/coff_compare.py` comparisons for all five emitted January owners, COFF
header/symbol inspection of candidate-only COMDATs, and isolated recompiles of
all six header consumers.  The final physics gate reports `5` exact, `0`
residual, and `12` unwritten target functions.  The existing 16-byte `.data`
section and four-byte `.bss` section retain their pre-wave hashes
`61324a19b6c2392d9ef4cf216a1dfa1d66d065c99f936630fcb31beffc509028`
and `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
No non-code credit is claimed.
