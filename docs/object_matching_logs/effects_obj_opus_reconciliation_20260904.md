# `effects.obj` Opus reconciliation (2026-09-04)

## Scope and provenance

This packet selectively reconciles the frozen Opus `effects.obj` work onto
canonical campaign commit `99a8e4735d72041554019e879d2488bae53c5fcb`.
The donor was inspected read-only at
`C:/halo-worktrees/opus-small-families-30k-20260902`, tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`.  The principal donor commits are
`608c66380dca2d841b1e95150d1f88abe41a06d8` and
`8aed00b49b8b77188dc3aa2ce213ade5440f4e74`.

The donor was not cherry-picked.  Its complete `effects.c` reconstruction was
replayed into an isolated worktree, then audited and corrected against:

- the complete donor `scratch/reports/effects.md` experiment log;
- the donor commits, their parents, and the final donor source/header diffs;
- all five canonical effects ledgers: lifecycle, random-wrapper, public-pair,
  reconnect, and first-person-weapon stop/start work;
- January's split `effects.obj`, including every function boundary,
  relocation, storage class, data owner, assertion string, and disassembly;
- the later open HaloCEA effects source family as independent behavioral and
  naming corroboration;
- the open Marathon `effects.c` and `effects.h` family as design-style
  corroboration only, never as evidence of identical Halo implementation;
- canonical campaign methodology, throughput, fake-match, parked-function,
  stable-verdict, semantic-progress, and object-admission documentation; and
- the repository `CLAUDE.md`/`AGENTS.md` rules and current project headers.

## Result

The focused hardened gate advances from **8 exact / 0 residual / 33
unwritten** to **31 exact / 10 residual / 0 unwritten** out of 41 functions.
The frozen donor's uncorrected frontier was 32/9/0.  One of those donor exact
functions depended on an indeterminate read, so this packet deliberately
credits one fewer exact function.

The strict admitted delta is:

- **23 newly exact functions**;
- **5,728 padded exact code bytes**;
- **5,544 meaningful exact code bytes**; and
- **2,200 exact data bytes**.

`effects.obj` now contains complete, coherent implementations for all 41
target functions.  It is intentionally not labeled a complete object because
ten ordinary-C reconstructions remain fuzzy, five BSS bytes are not yet
fully owned with January's exact storage topology, and a target-absent
compiler helper COMDAT remains.

| newly exact function | meaningful | padded | relocs | normalized SHA-256 |
|---|---:|---:|---:|---|
| `_dangerous_effects_near_player` | 482 | 496 | 19 | `742b0b10bb7ba5d14436acc02cc9bedfd5506166e91ca7c9f0468478ec93fd7d` |
| `_effect_allocate` | 337 | 352 | 24 | `b697df4198ab5891df49a3f7c5dad56a00a9739f232ac3b3350822b602f48501` |
| `_effect_build_location` | 127 | 128 | 4 | `0202b5cf7b408366ac1319a3ae4aa283874e32208772bfc69f429fc32f05dfe3` |
| `_effect_build_locations` | 169 | 176 | 4 | `5d38de90aebef4f48d3db0e4b45d1a98e4c65243da1d9b7bb14b72da7e1c5a2a` |
| `_effect_delete` | 179 | 192 | 9 | `20d8efc5026f5d4735b495f3ad9bebe9f2449a8418bcd172875bb17fa587d019` |
| `_effect_get_random_seed` | 31 | 32 | 3 | `ba6cbd360809ba56a84e769e58bb834a38795e8ffa3e1124b5de11e380c725c6` |
| `_effect_location_get_next_instance` | 224 | 224 | 13 | `5edfadf2d0b21bdd78bc44c829572d9138ea65d500edf95e7515c85dab385b4a` |
| `_effect_marker_list_get_marker` | 399 | 400 | 25 | `7c6c87298a0ff3052ccf91a470040793d063644875653d34a6a1c570dcf1c527` |
| `_effect_marker_list_get_markers_by_name` | 171 | 176 | 9 | `38adabcb42132a9a748abd1ff2e32cf0542147871879bc84f5d32a58de1e44d4` |
| `_effect_new_attached_from_markers` | 657 | 672 | 55 | `5ec7eb5742f237fe6308d237ba569d67b2cc20004467b2bdc17d24d24b9f2f63` |
| `_effect_new_from_object` | 515 | 528 | 44 | `2bb87bfcdbb187fbdda834b5114025a5368989072453000ca35e93202d911736` |
| `_effect_new_looping` | 233 | 240 | 15 | `9388fb1e1db6d9ede404b2f2bec1aa3bae517ffb8f816616d6aae6f88206e2a8` |
| `_effect_new_unattached_from_markers` | 561 | 576 | 44 | `c34b6e49aa596caf4e35dcbd5ac3bbee0c5410f7de97825ef4304678ead3c706` |
| `_effect_random_angular_velocity` | 125 | 128 | 4 | `aefda9e29709d3a6791257029c19efd24d47c30d976fc6d880469ee0b91c1f63` |
| `_effect_random_translational_velocity` | 210 | 224 | 5 | `e5d03c383f2cce81c821879e53bde733bb0f7909398bd705af7212162aaf81a3` |
| `_effect_real_random_range` | 115 | 128 | 1 | `b0bc808462db0f6ae5df2357bf289b3e850d931f6e9f102e770cdef90a7e69f5` |
| `_effect_set_event` | 144 | 144 | 8 | `2f102d562c1cc0e364aa5f533381d9994ab86ff16624250c56d731c0502ab609` |
| `_effect_stop` | 144 | 144 | 5 | `272d8ac7eb12f5825d80e8739ea3fb9fe42b73c859bb0739d040178b7bb6eed8` |
| `_effects_start_on_first_person_weapon` | 163 | 176 | 13 | `46d6ed0f682dbce3fa0644308ab8d9a60ee041aa51f5fedb885d779f380f4a86` |
| `_effects_stop_on_first_person_weapon` | 241 | 256 | 11 | `75f540f60c7fbc988baa3cfbea4d8342d4d6e79233a5b63ce16d60ab91da9a60` |
| `_effects_update` | 129 | 144 | 13 | `008bd39d702d7df0078dcfadf79e7ec0328507697feb52114477ba1eb7f0c0fc` |
| `_impulse_effect_initialize` | 173 | 176 | 9 | `88675bfb46115c1acbbb43934258e2167e92b7b64ae7ad99179b15c6b0c7d78d` |
| `_real_local_random` | 15 | 16 | 2 | `a8ad3cb30f329f39f3193d4112f79d5983d27e7d41b1c99b5e0b4cd98e555bd0` |

The eight inherited exact functions remain exact.  The stable verdict sweep
therefore records exactly the 23 rows above rather than counting rewritten
but already-exact lifecycle leaves as new progress.

## Names, ownership, and interfaces

All 21 target `code_0008...` placeholders now have semantic subsystem names
and true file-static linkage in `config/symbols.json`:

`effects_object_is_corpse`, `effect_scale`, `effect_get_random_seed`,
`effect_real_random_range`, `effect_random_angular_velocity`,
`effect_allowed_by_environment`, `effect_part_allowed_by_disposition`,
`effect_set_event`, `effect_build_location`, `effect_get_node_matrix`,
`effect_evaluate_function_integral`, `effect_random_translational_velocity`,
`effect_allocate`, `impulse_effect_initialize`, `effect_build_locations`,
`effect_generate_particles`, `effect_generate_part`,
`effect_marker_list_get_marker`, `effect_generate_parts`,
`effect_marker_list_get_markers_by_name`, and `effect_update`.

These names follow the recovered call graph, Halo's subsystem vocabulary, and
the independently named later-source counterparts.  No address-derived name
is used as a production identifier.  `effect_location_get_next_instance`
remains external because both the current canonical interface and January's
symbol topology identify it as such.  The anonymous BSS owner at `0x43D584`
is now the descriptive TU-static `global_effect_marker_list`; the adjacent
debug byte remains the external `debug_effects_nonviolent` symbol required by
the debug console.

Public declarations live in `effects/effects.h`, and that header owns the
correct pointer/boolean signature for marker-name arrays and deterministic
selection.  Ad-hoc declarations were removed from `game.c`, `scenario.c`,
and `hs_library_external.c`.  `particle_system_new_unattached` moved to its
real owner, `effects/particle_systems.h`.  Every affected caller was updated
to pass `char const **`, `boolean`, and `NULL` values without integer pointer
casts.  A full transitive Ninja rebuild was required and completed after
these header changes.

The recovered large packet layouts remain TU-private to avoid the measured
VC7 definition-position sensitivity of broad headers.  Their sizes and key
offsets are asserted.  Source uses engine `real`, typed color/vector/matrix
types, named environment/disposition/camera/tag enums, `TEST_FLAG`/`SET_FLAG`,
typed effect/location/object/biped accessors, `TAG_BLOCK_GET_ELEMENT`, and an
`effect_definition_get` wrapper around `tag_get`.  Function signatures follow
the repository's one-parameter-per-line convention, no-argument lists use
`void` on its own line, and every void function has an explicit terminal
`return;`.

## Safety and source-authenticity adjudications

Two donor shapes were not admitted even though they tracked January machine
code more closely:

1. `effect_allowed_by_environment` was byte exact only when the invalid-enum
   assertion path returned an uninitialized local if `system_exit` ever
   returned.  The retained source initializes the result from the meaningful
   `anywhere` predicate and overrides it in the air/water arms.  Vacuum and an
   unexpectedly returning invalid arm safely produce `FALSE`.  This loses one
   128-byte exact function, but removes undefined behavior.  Other safe
   initializations caused the compiler to inline the assertion into both
   large callers and were rejected as broader regressions.
2. January's unknown-part assertion demonstrably calls `csprintf` with the
   format literal in the destination position.  The target's three-argument
   call and absence of a `temporary` relocation prove that original mistake,
   but executing it would write through a string literal before the fatal
   assertion.  The retained call formats into the real global `temporary`
   buffer and is intentionally fuzzy.

The link reads following `datum_delete` in `effect_delete` and
`effects_stop_on_first_person_weapon` were separately audited rather than
rejected by appearance.  This engine's data arrays are fixed backing stores;
`datum_delete` clears the datum header and does not free or overwrite the
slot.  January's load order is therefore a still-backed slot read, not a C
object-lifetime violation.  The retained comments state that dependency.

No forced inline/noinline, volatile or register forcing, compiler barrier,
pragma, dummy path, inert expression, raw structure offset, representation
pun, malformed variadic call, false `noreturn` contract, assembly, fake
dependency, or nonsensical exact-only branch remains.

## Honest residual frontier

All ten complete residuals are fail-closed parks in `config/parked.json` with
target/candidate hashes, relocation counts, similarity, exhausted natural
source families, and explicit reopen evidence:

| function | target / candidate padded | target / candidate relocs | objdiff | bounded reason |
|---|---:|---:|---:|---|
| `_effect_allowed_by_environment` | 128 / 112 | 10 / 10 | 55.909092% | unsafe donor exactness rejected; closest safe switch retained |
| `_effect_evaluate_function_integral` | 176 / 176 | 17 / 16 | 82.694916% | selective zero-return tail merging |
| `_effect_generate_part` | 992 / 976 | 27 / 28 | 85.99458% | safe formatter destination plus one unrecovered local lifetime |
| `_effect_generate_particles` | 1,888 / 1,616 | 46 / 46 | 68.30481% | January inlines three range helpers; natural VC7 does not |
| `_effect_generate_parts` | 592 / 592 | 15 / 15 | 91.225% | local/register allocation and one alignment jump |
| `_effect_get_node_matrix` | 80 / 80 | 3 / 3 | 92.88461% | equivalent return-arm order |
| `_effect_part_allowed_by_disposition` | 32 / 32 | 0 / 0 | 79.166664% | private register convention only |
| `_effect_scale` | 48 / 48 | 0 / 0 | 87.5% | out-of-line exact shape conflicts with exact 128-byte caller |
| `_effect_update` | 1,328 / 1,312 | 48 / 48 | 94.71298% | downstream helper-inline/register schedule |
| `_effects_object_is_corpse` | 80 / 64 | 2 / 2 | 67.870964% | natural VC7 tail-merges three false exits |

The codegen classifier returns `UNKNOWN` for nine residuals.  It recognizes a
return-shape signature for the corpse predicate, but that class is not an
allowed manifest recipe and no authentic spelling prevents the merge; the
park therefore remains conservatively `unclassified`.  No object-completion
or exact-function credit is claimed for these rows.

## Data and helper ownership

The rebuilt object matches all **664 `.rdata` bytes** and all **1,536 `.data`
bytes**.  The target BSS is five bytes: its four-byte
`global_effect_marker_list` has the correct static ownership and offset, while
`debug_effects_nonviolent` is truthfully emitted as an external COMMON byte
instead of being folded into the TU-static BSS section.  Consequently the
ordinary report credits 2,200/2,205 data bytes rather than manufacturing an
aggregate BSS facade.

`effects.obj` contains **no `point_from_line3d` symbol or COMDAT**, preserving
the January inline schedule required by the campaign.  It does emit a
48-byte select-any `scale_vector3d` helper COMDAT absent from the target
object, so whole-object admission remains correctly unavailable.  The
existing `vehicles.obj` point helper ownership was not introduced or altered
by this packet's two pointer/boolean call-site corrections; the stable sweep
shows no vehicle function regression.

## Verification

All verification ran in the isolated worktree with independent `build/` and
`split/`; the donor and canonical working trees were read-only.

- Focused gate: **31 exact / 10 residual / 0 unwritten**.
- Full Ninja build and report: code **866,169 / 2,198,102 bytes** and
  **5,998 / 11,060 functions**; data **2,009,870 / 4,176,062 bytes**;
  391/833 objects linked.
- Full semantic report: 473 units scanned, 6,052 semantic exact, 168 hidden
  exact, 6,070 accepted exact, and **zero unit errors**.
- Stable before/after sweep over all 8,245 target function sections:
  **23 gains / 5,728 padded bytes / zero regressions**.
- Progress baseline/after: code **860,625 / 5,975** to
  **866,169 / 5,998**; data **2,007,670** to **2,009,870**.
- Park audit: **215 active / 0 stale / 0 invalid**.
- Fake-match scan over all nine changed production source/header files:
  **0 review leads / 0 skipped**.
- Object-admission audit: **0 candidates / 0 contradicted / 0 revoked** and
  only the two inherited `player_rumble`/`models` rejections.
- `python -m pytest -q`: **261 passed**.
- `git diff --check`: clean apart from Git's informational line-ending
  notices.
- COFF symbol census: no `point_from_line3d` owner in `effects.obj` or any
  other effects/game/scenario/HS object rebuilt by these interface changes.
- The protected external canonical bitmap working copy remained byte-for-byte
  at SHA-256
  `AB6002BF2BAECDBC18C7B308BB7846C58FBCF1C7C72F586621D2B1F271D49140`
  (`bitmap_utilities.c`) and
  `B435F38E406C7DCDB60B4DF12E78BCF98D1E4B0E57FE2AB2240BC78F799EBA93`
  (`bitmaps.h`).
