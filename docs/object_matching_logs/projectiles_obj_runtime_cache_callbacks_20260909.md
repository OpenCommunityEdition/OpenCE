# `projectiles.obj` runtime-cache callback packet — 2026-09-09

## Outcome

This packet starts from the requested canonical commit
`289b79d40db44a3bb9c202b10017ecbc6fadcaef`. It reconstructs six complete
projectile owners from January evidence and the independently recovered HCEA
source. Four owners are strict-exact and two honest residuals are retained
with zero exact credit:

- exact: `_projectile_export_function_values`;
- exact: `_projectile_calculate_deceleration_from_distances`, January
  `_code_000e7790`;
- exact: `_projectile_adjust_for_angular_velocity_change`, January
  `_code_000e7d80`;
- exact: `_projectile_calculate_deceleration`, January `_code_000e7e30`;
- complete residual: `_projectile_new`;
- complete residual: `_projectile_accelerate`.

The PID-local focused gate advances `source/items/projectiles.obj` from
**17 exact / 0 residual / 13 unwritten** to
**21 exact / 2 residual / 7 unwritten**. Strict credit increases by **689
meaningful / 720 padded code bytes**. The two residual bodies receive no
exact credit.

The three private helpers receive semantic symbol-map names authenticated by
HCEA. Public object-type callback declarations are owned by the narrow
`items/projectiles_callbacks.h` rather than repeated in
`objects/object_types.c`. The widely consumed `items/projectiles.h` and its
previously blocked public `struct projectile_datum` remain unchanged.

## Provenance and semantic map

January's split `projectiles.obj` remains authoritative for code bytes,
relocation addresses, relocation identities, and owner topology. Its raw
SHA-256 is
`0f558b623efe404e1c803b8fab03338c4f2b5b12af8ce14c4634c3ad59d6c8da`.

The independent semantic oracle is the clean HCEA cache at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. HCEA is not used as a PC byte
donor. Its PowerPC reconstructions authenticate the routine purposes,
signatures, projectile-definition fields, private projectile state, flag
meanings, and private names:

| Contract | HCEA path | Git blob |
| --- | --- | --- |
| exported input functions | `src/blam/items/projectile_export_function_values.c` | `033e53ef1ddf80c6c761f7c45ed2e9b52ea6353e` |
| distance-derived deceleration | `src/blam/items/projectile_calculate_deceleration_from_distances.c` | `73b2cb1000c33efceb874140216e6233edd70076` |
| angular-velocity cache | `src/blam/items/projectile_adjust_for_angular_velocity_change.c` | `34037d881f34b8f647a69c18414165cc01957405` |
| medium-specific deceleration cache | `src/blam/items/projectile_calculate_deceleration.c` | `e8aeda947c68f0fcacb26da1e7b2fcbd81be29eb` |
| new callback | `src/blam/items/projectile_new.c` | `56cf89b2a6bbf0001498675907a5e9494ac408d9` |
| acceleration callback | `src/blam/items/projectile_accelerate.c` | `fe8639d59707fc7f4a84661facae7a5659a1a8fd` |

The supporting authenticated HCEA headers are
`src/headers/projectile_datum.h` (`ffed1cce816f73a4a7d0ecc66855744d60f197ff`),
`src/headers/_projectile_datum.h`
(`ee907cffaaf4b22cfa548eb5fbe218a4bafe9f53`),
`src/headers/projectile_definition.h`
(`6171292053cd5daf5945095208735c8dd7882b0f`),
`src/headers/_projectile_definition.h`
(`4aad15cd35c45f19e2fda8521008a74270cb0817`),
`src/headers/projectile_export_function_mode.h`
(`3e71bd582e239e7c247afe09c9b154756a67bca0`), and the datum and definition
flag headers (`281bc6040fc35e858a71e29f683ee44b879857c5` and
`e2795bbbf833d6ec9944ef4f1cf6f08b7891ef6e`).

## Runtime layout boundary

The exact callbacks require state beyond the last field currently exposed by
the public `struct projectile_datum`. A complete typed runtime view is
therefore kept private to `projectiles.c`. Compile-time assertions prove the
January-observable offsets of `arming_time_delta` (`0x1FC`), `odometer`
(`0x200`), `deceleration` (`0x20C`), `rotation_axis` (`0x214`), and
`rotation_cosine` (`0x224`). All accesses use named typed fields.

This deliberately avoids reopening the prior public-header experiment that
regressed protected `units.obj`. There is no raw offset arithmetic, byte view,
representation cast, or expansion of the shared projectile datum. A direct
A/B also showed that exposing both newly reconstructed callback prototypes in
the widely included `projectiles.h` perturbs January's exact
`unit_preprocess_node_orientations` schedule. That spelling was rejected.
`projectiles.h` remains byte-identical, while the implementation and callback
table share the narrow owner header; the protected Units gate remains
strict-exact.

## Strict function evidence

The hardened comparator requires padded bytes, normalized bytes, and every
relocation address, type, destination, and addend to agree.

| Semantic owner | January owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_projectile_export_function_values` | same | 221 / 224 | 6 | `3cf0a7ce01922955191a26edccb2a4ae508c18abdeb92135b2d2d1ccd50a94ce` |
| `_projectile_calculate_deceleration_from_distances` | `_code_000e7790` | 88 / 96 | 2 | `4ad2a4a272fa5d1a29247f3221ffa11d22b8cd36c635d994bebcda1b1ffd16fe` |
| `_projectile_adjust_for_angular_velocity_change` | `_code_000e7d80` | 164 / 176 | 3 | `eb93f93d29c4d3142b0bd4b98896ca351d8db11d1ed2795b4d399f3a8d93dde8` |
| `_projectile_calculate_deceleration` | `_code_000e7e30` | 216 / 224 | 6 | `fd5544ddb4443613f83e9e1b969c507e3fdff1de4876657933e03b7da62c8f7d` |
| **Exact gain** | | **689 / 720** | **17** | |

The distance helper has only the zero-real relocations at `+0x0E` and
`+0x2A`. The angular-cache helper calls `object_get_and_verify_type` at
`+0x04` and owns the zero and one constants at `+0x30` and `+0x43`. The
deceleration-cache helper calls `object_get_and_verify_type` at `+0x05`,
`tag_get` at `+0x14`, and the renamed distance helper at `+0x33` and `+0x82`;
its zero-real references remain at `+0x51` and `+0xA0`. All addresses and
identities compare exact after the documented semantic aliases.

## Complete retained residuals

| Owner | Target / candidate padded | Target / candidate relocations | Target normalized SHA-256 | Candidate normalized SHA-256 | Objdiff |
| --- | ---: | ---: | --- | --- | ---: |
| `_projectile_new` | 432 / 432 | 16 / 16 | `85fac0f022801486c49b0eda0aca4c67263a4425a83e261c6aeaee8f8965820f` | `9d8477d25eb528d75b66dededc61869098cfbd1255b7aac1dd349f612d0901d0` | 96.93284% |
| `_projectile_accelerate` | 496 / 496 | 32 / 32 | `e2f31c3bd7a0aae6dc41d6cf08108355de63d6b3829568a59b363c3db0a5cfc1` | `a4c30f36eb44dae31674b6beef2e74edbbfb0d7970709ecb91c52db69c38682d` | 97.04402% |

`projectile_new` has all sixteen relocation identities at the exact target
addresses. Only ten normalized byte positions differ, in a short region where
the compiler schedules independent initial-velocity x87 work around the two
scenario-call argument addresses. `projectile_accelerate` has the same 32
relocation identities and padded size; a six-byte displacement of the final
assertion/helper relocation group follows a different natural x87 temporary
spill schedule.

The fail-closed codegen classifier reports `unclassified` for both functions,
so the parked manifest makes no narrower compiler-mechanism claim. Both bodies
are complete ordinary C and receive zero exact credit. No source reshaping,
fake dependency, forced inline, `register`, `volatile`, pragma, or undefined
behavior is retained to chase the remaining schedule.

## Helper ownership and remaining frontier

The seven remaining unwritten owners are:

| January owner | HCEA semantic map | Padded bytes | Disposition |
| --- | --- | ---: | --- |
| `_code_000e7630` | `projectile_set_action` | 32 | private collision helper; natural standalone trial was omitted by the compiler because its owner is not yet present |
| `_code_000e7650` | `projectile_effect_new` | 96 | private collision/effect helper; park with its natural large owner rather than synthesize retention |
| `_projectile_aim_ballistic` | same | 880 | prior blocked reconstruction remains unchanged |
| `_code_000e7f10` | `projectile_collision_test_line` | 512 | bounded future collision packet |
| `_code_000e8110` | `projectile_detonate` | 1,040 | bounded future detonation packet |
| `_code_000e88c0` | `projectile_collision` | 2,928 | large state machine and natural owner of the two small helpers |
| `_projectile_update` | same | 4,064 | large state machine |

The 32-byte setter was compiled once as a natural TU-private static. Because
its collision caller remains unwritten, MSVC correctly emitted no COMDAT.
The trial was removed. Making it public, adding a dead reference, or inventing
an anchor would violate January ownership. The effect helper is left with the
same natural owner for the same reason. No `point_from_line3d`,
`normalize3d_inline`, or `random_vector_in_cone3d_inline` owner is emitted.

## Policy and validation record

The retained source uses project `real`, `boolean`, vector, tag-block, object,
scenario, flag, and assertion types/macros. It contains no assembly, raw
address or byte-offset access, address-named source identifier, unsafe pun,
compiler barrier, fake condition, synthetic anchor, or code-generation-only
write. Static helpers remain private; public callbacks are declared by their
owner header.

Validation is intentionally focused and does not configure or invoke Ninja:

- PID-local `tools/campaign/gate.py source/items/projectiles --all` with the
  three semantic target aliases and forbidden-emitted-symbol guards:
  `21 exact / 2 residual / 7 unwritten`;
- protected `source/units/units.obj` sentinel gate:
  `189 exact / 0 residual / 0 unwritten`;
- `source/objects/object_types.obj` owner-header consumer gate:
  `31 exact / 0 residual / 0 unwritten`;
- fail-closed blocker classification for both retained residuals:
  `unclassified`;
- targeted COFF, fake-match, emitted-symbol, classifier, and parked-manifest
  unit tests: `107 passed`;
- fake-match scan of the changed C/header sources: zero findings;
- JSON parsing and `git diff --check`: pass.

The final focused candidate object raw SHA-256 is
`81f09674da54b49c8ed0db15299c0380ff14eaf65b29787e73ee87e0923b5423`;
raw COFF hashes are timestamp-sensitive audit artifacts. The compiler is
Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
using the repository's unchanged XDK 3911 flags.
