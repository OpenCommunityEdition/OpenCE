# `recorded_animation_playback.obj` vector helpers

## Result

Three compressed-vector helpers are strict semantic-COFF exact:

- `code_000821d0`, the signed-byte controller-delta helper, contributes 64
  meaningful / 64 padded bytes and has no relocations;
- `code_00082210`, the signed-short controller-delta helper, contributes 61
  meaningful / 64 padded bytes and has no relocations; and
- `code_00082250`, the controller-to-vector expansion helper, contributes 63
  meaningful / 64 padded bytes and has three relocations.

Together they add 188 matched meaningful code bytes. The object advances from
8/14 to 11/14 exact functions and from 833 to 1,021 matched meaningful code
bytes. It remains `NonMatching`; no completion label, semantic exception,
data credit, or parked entry is added.

The target/rebuilt normalized SHA-256 values are:

- `code_000821d0`:
  `df600181376779416c50b285c00951043e40d1367fc0cba02e2155e640a28e3b`;
- `code_00082210`:
  `1037feab265603a1ed0735c8dda0a855be91bf9bec1e49356b1616eaf023cde2`;
- `code_00082250`:
  `06e9aabd76b7d6ca7f206fb81dca24da7274f6238f2eab8836455a997ab02a66`.

The expansion helper's relocations are identical at offsets `0x20`, `0x2C`,
and `0x34`: two DIR32 references to `__real@3b4de32e` and one REL32 call to
`vector3d_from_euler_angles2d`, with identical target ownership and addends.

## Evidence and provenance

January's split-object disassembly proves both controller helpers update the
yaw field in memory, wrap values outside `[-1000, 1000]`, then apply the pitch
delta. The char helper sign-extends two bytes; the short helper reads two
signed words. The expansion helper sign-extends yaw and pitch, multiplies both
by the `0x3B4DE32E` real constant, and calls
`vector3d_from_euler_angles2d` with a typed two-angle local.

The later-build HCEA tree at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently authenticates the
payload layouts and full vector-handler semantics. Relevant donor SHA-256
values are:

- `src/blam/game/update_controller_char.c`:
  `41f00f4c6409679ecf4c89af232d6835d4e3b48a1d149ef0778ee02834041cae`;
- `src/blam/game/update_controller_short.c`:
  `cda5c1fbe98d5786fb2f77caecdb8300b4f43a1273215154de0f7fda036e491b`;
- `src/blam/models/uncompress_vector_from_controller.c`:
  `3e229fc41169814b918c9be51c2075ac2bcca0f1cd22310ae0e734d507c1f859`;
- `src/blam/models/apply_vector_char_difference.c`:
  `df6a774999b6500736cb46116eea8bd214d3438fba3847db920d91b5e7999e74`;
- `src/blam/models/apply_vector_short_difference.c`:
  `846cf775bb41b16a80a58efc4fe1896b0a4cf910e82fe1cfc3c3b647212d20e0`.

Every local Claude branch was searched for the unit, helper names, and donor
names. Branch sources contain only the original skeleton. The four branches
with residual documentation (`finish-hs-20260816`, `libcmt-stream`,
`near-complete-objects-20260816`, and `untried-objects-20260816`) contribute
only residual inventories and the target-derived string synopsis. The synopsis
authenticates the two large handlers' assertion strings but contains no helper
source candidate, compile measurement, exclusion matrix, fixed point, or
object-specific ledger.

## Candidate history

The evidence waves were narrow and monotonic:

1. A whole-record standard-C `memcpy` candidate made `code_00082250` exact,
   but VC7 combined or hoisted the controller payload loads.
2. Field-sized copies eliminated the aggregate load but kept payload values
   live too early.
3. Typed event access produced the target's individual byte/word loads, while
   a local yaw value still selected the wrong accumulator register.
4. Updating `control->yaw` in memory first reproduced the target's EAX/EDX/ECX
   register topology; only the two branch blocks remained reversed.
5. Testing the `> 1000` case first reproduced both helpers exactly. The
   already-exact expansion helper remained unchanged.

The complete HCEA-derived char and short vector handlers are retained as
house-legal semantic foundation. Each has the target's 512 padded bytes and
all 19 relocation destinations, but relocation addresses and normalized bytes
remain different; they receive no exact credit. Their current ordinary fuzzy
measurements are 94.35% and 93.87778%. A 32-bit flag-local experiment shrank
both handlers to 496 bytes and was rejected; the 16-bit mask topology is
retained. No fixed-point claim is made for either handler.

The retained source uses typed payload records through a standard
`void const *` object-pointer conversion, not raw offsets or an explicit
pointer cast. It contains no assembly, explicit intrinsic, raw address,
pointer/integer reconstruction, pointer or union pun, `register`, `volatile`,
pragma, annotation, barrier, synthetic reference, byte forcing, or
compiler-flag change. Every parameter is vertical and every void function
ends with an explicit `return;`.

## Reproducibility and gates

- Frozen pre-wave object:
  `build/audit/recorded_animation_playback_vector_family_baseline_20260827.obj`,
  raw SHA-256
  `8f01fe91f9b5cba988fb72aed7fbf9ff52c8dbf9c15d65838c9b22ab155faba2`.
- Whole-record-copy miss:
  `build/audit/recorded_animation_playback_vector_family_aggregate_miss_20260827.obj`,
  raw SHA-256
  `ee864f770fa45bc308231a3069608945c8993b595479116c05267b29e84dc22a`.
- Field-copy miss:
  `build/audit/recorded_animation_playback_vector_family_field_copy_miss_20260827.obj`,
  raw SHA-256
  `1793ac1064b6a816ed142fb532b252e23ce08ba3715aa97b0fc32a7b8a87857a`.
- Local-yaw miss:
  `build/audit/recorded_animation_playback_vector_family_local_yaw_miss_20260827.obj`,
  raw SHA-256
  `825d94bb817ee65435fb9aabd4e27bc771723e8db4762f6ea76d95d33acb6785`.
- Branch-order miss:
  `build/audit/recorded_animation_playback_vector_family_branch_order_miss_20260827.obj`,
  raw SHA-256
  `0da17b6761c205017479c02bcbda9c323db7df8481f6be26877107294216e2ee`.
- Retained object:
  `build/audit/recorded_animation_playback_vector_helpers_retained_20260827.obj`,
  raw SHA-256
  `9628e29020ce8b55235385817c7e2807cb3796d697ec0d3ccf4bf6f9954ad877`.
- Retained source SHA-256:
  `20b84ef05c421f5ac73d8407dbe77ec630b40f26dfe86a6b45076f455db289cb`.
- Production-flag XDK 3911 `/Zs`: pass before every code-producing wave.
- Hardened exact-set comparison: three newly exact functions and zero lost
  exact functions.
- Halo and libcmt build graphs: pass.
- Semantic audit: 470 units, 4,413 functions evaluated, 4,265 semantic exact,
  4,325 accepted exact, and zero unit errors.
- Campaign progress: 377/833 objects, 4,298/11,060 functions, and
  516,378/2,198,102 code bytes. Halo progress: 275/468 objects,
  4,131/7,574 functions, and 503,464/1,770,166 code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tooling tests: 179/179 pass, with only the inherited unwritable pytest-cache
  warning.
- Protected source, configuration, semantic ledgers, comparator/admission
  tooling, compiler flags, and object status are unchanged.

This checkpoint claims three exact functions and 188 meaningful code bytes,
not a completed object or any data ownership.
