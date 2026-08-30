# `actors.obj` Jonas prop-switch first shot

## Result

Starting from clean cumulative commit
`df41735da9d7aaf9753c150e9eee7ab954bd68a6`, this isolated lane restores
`_actor_switch_props` in the first and only production candidate compile.  Its
complete 464-byte padded COFF section and all seven ordered relocations are
strictly identical to January.  `source/ai/actors.obj` advances from 24/76 to
25/76 strict functions and from 1,459 to 1,908 meaningful strict code bytes.
The object remains honestly `NonMatching`.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_switch_props` | 449 | 464 | 7 | `077e0801f697c2007d587114989d197ac3e047ca260fd51cc6f0a9987dc02024` |

The hardened comparator reports zero normalized differing bytes.  The seven
ordered i386 relocations are:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x008` | `DIR32` | `_actor_data` |
| `+0x012` | `REL32` | `_datum_get` |
| `+0x165` | `DIR32` | `_swarm_data` |
| `+0x16C` | `REL32` | `_datum_get` |
| `+0x182` | `DIR32` | `_swarm_component_data` |
| `+0x190` | `REL32` | `_datum_get` |
| `+0x1B5` | `REL32` | `_actor_action_replace_prop` |

Together, the new function and all 24 inherited strict functions account for
2,112 padded bytes and 99 ordered relocations.  The focused frozen-TU replay
reports 25 exact, zero residual, and zero unwritten among the accepted set.

## January and compiler authority

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January `build/split/source/ai/actors.obj` SHA-256:
  `632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.
- Authenticated `xbox/bin/vc7/CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Build contract: the unchanged ordinary `/O2 /Oy- /DDEBUG /Dxbox` object
  edge; no compiler, option, rule, or comparator change.

January fixes the complete ABI and topology.  The function is public cdecl,
takes three full-width actor/prop datum indices, begins with `actor_get`, and
ends with `actor_action_replace_prop(actor_index, old_prop_index,
new_prop_index)`.  Its field sequence is target, fire target, grenade,
surprise, panic, combat transition, retreat, pursuit, postcombat,
destination, three look directions, then the swarm-component loop.  The loop
uses a signed short component cursor, indexes `swarm->component_indices`, and
updates the named combat-target prop at component offset `0x14`.  January's
seven relocations close every external dependency.

## Authenticated donor and cross-build boundary

The direct semantic donor is the user-supplied HCEA checkout at clean commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  Its source is
`src/blam/ai/actor_switch_props.c`, Git blob
`a49d766a333b071a33821eac86c07df515b21751`, and checked-out file SHA-256
`253c3a0a51574956c7e7d9f8c22d0079b8a57880eb8071beb1f5e19d5c285725`.
The donor independently supplies the same three-argument semantics, field
order, guarded enum resets, signed-short swarm loop, and final action-layer
call.  January, not the donor, supplies byte, ABI, layout, and relocation
authority.

The current Stian and Pastudan trees and their `CLAUDE.md`/`AGENTS.md` files
were inspected as corroborating context.  Their current function carrying the
same spelling is an unrelated later-build two-argument routine and was
explicitly rejected as a donor.  A read-only all-ref source/history and ledger
census found no earlier production definition or candidate compile for this
three-argument January function.  The existing Actors ledgers reviewed before
the freeze were:

- `docs/object_matching_logs/actors_obj_codex_checkpoint.md`;
- `docs/object_matching_logs/actors_obj_jonas_maintenance_pair_20260821.md`;
- `docs/object_matching_logs/actors_obj_jonas_reset_leaf_first_shot_20260826.md`.

The current campaign methodology, exact-match acceleration, shape-transfer,
admission, parked-function, and object-comparison tooling documents were also
reviewed before selection.  No Claude-owned production source or dirty donor
file was consumed.

## Retained readable C89 source

The pre-shot `source/ai/actors.c` blob was
`598cbbad64526c85329bd759deaa9c7b20e2f151`.  The retained source blob is
`48c924ac130985ad73b1000a4da931049195334f`; its worktree SHA-256 is
`67bd23bb827fbb5d0ac39792247dcf9cb735737d12318de95057bbe0f9d0df57`.
`source/ai/actors.h` remains unchanged at blob
`3f1d86b7f2e0a42d454aa5d0d05d4ff86d9b3be2`.

The retained implementation is an ordinary typed translation of the donor
topology against canonical named fields.  It adds:

- three TU-local named enum constants for the missing fire-target and
  prop-destination values;
- a private, size- and offset-asserted swarm-component datum view exposing
  `combat_target_prop_index` by name;
- a typed `datum_get` accessor macro and the ordinary action-layer prototype;
- the public C89 body in January source order between target derivation and
  position-index flushing.

The code uses one parameter per line, declarations at block starts, and an
explicit terminal return.  It contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, barrier, raw address or raw-offset access,
pointer/integer reconstruction, type or union pun, undefined behavior,
inactive-union access, byte forcing, compiler control, or tool exception.
The layout assertions validate declarations only and emit no runtime bytes.

## One-shot discipline

After the source and support declarations were frozen, the ordinary
production object edge was invoked once.  An earlier sandbox-denied attempt
never started `ninja.exe` and emitted no object or candidate.  The single
production candidate object has SHA-256
`87b6815f126121721980ef3dcf19b064ae9be8266ab0a63e0ab5fa12142715b9`.
Its target function immediately compared strict exact, so no
source spelling, declaration, topology, scheduling, or compiler experiment
followed.  Later compiles were acceptance replay gates of the unchanged
retained source, not new candidates.

## Frozen whole-TU boundary

The focused acceptance replay proves `_actor_switch_props` plus every inherited
accepted Actors packet:

`_actors_initialize`, `_actors_dispose`, `_actors_initialize_for_new_map`,
`_actors_dispose_from_old_map`, `_actor_is_noncombat`, `_actor_in_combat`,
`_actor_is_fighting`, `_actor_is_leaping`, `_actor_get_weapon`,
`_actor_has_ranged_weapon`, `_actor_target_unit_index`,
`_actor_derive_target_information`, `_actor_flush_position_indices`,
`_actor_has_unlimited_grenades`, `_actor_change_encounter`,
`_actor_unit_control_crouch`, `_actor_unit_control_jump`,
`_actor_unit_control_primary_trigger`, `_actor_unit_control_secondary_trigger`,
`_actor_unit_control_throw_grenade`, `_actor_unit_control_exact_facing`,
`_actor_unit_control_stop_animation_impulse`, `_actor_delete_props`, and
`_code_0002a150`.

No accepted sibling is lost.  No header, configuration, exception, parked
record, completion label, target artifact, or unrelated production source is
changed.

## Full gates and progress

The complete Halo and libcmt object graphs pass.  Required report, semantic,
progress, admission, parked, protected-Units, policy, and test gates report:

- semantic audit: 470 units, 4,919 functions evaluated, 4,802 semantic exact,
  4,812 accepted exact, 131 hidden exact, and zero unit errors;
- strict progress: 384/833 objects, 4,773/11,060 functions,
  606,875/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes;
- Halo progress: 282/468 objects, 4,606/7,574 functions, and
  593,961/1,770,166 meaningful code bytes;
- admission audit: zero candidates, zero contradictions, zero revocations;
- parked validation: 12 active, zero stale, zero invalid;
- tooling tests: 212/212 pass (the sandbox-only pytest cache warning is
  non-functional);
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json`: 1,599,057 bytes, SHA-256
  `b9ecfa128c6e67e4d36b1dc1041cd53558afee06338ac0f623a3f7cbbf2e8b22`;
- `build/semantic_report.json`: 3,267,795 bytes, SHA-256
  `e5bb3f3990965e7527b94e6a8a414dc23541d3627e32d4fbe057cd4a17e2ec53`;
- `git diff --check`: pass.

Only `source/ai/actors.c` and this additive ledger are retained.  Protected
Units, Vehicles, Matrix Math, AI Debug, Motion Sensor, Path Smoothing, and all
other excluded/currently owned lanes are untouched.  Nothing is pushed.

## Canonical integration replay

The isolated commit was later integrated onto canonical parent `4f97feb9`,
whose Actors source blob `2ac26a90b758532c5e57c0f6b4d8b09a7de0a7e7`
already contained four additional independently accepted Actors leaves and a
typed local actor-iterator view. The conflict resolution was strictly
additive: it retained that iterator, combined the two non-overlapping enum
sets, and added the prop-switch component view and exact body unchanged.

The integrated source blob is
`30bdf27f05c8c6f95b4f610bf842660be0062174`; its rebuilt object SHA-256 is
`d9ae111f40946a0d7aaaf0b7a4a8dc294fe1544a9e8a37fc06c0c93c1efdc15e`.
The hardened direct comparison again proves `_actor_switch_props` exact at
464 padded bytes, seven ordered relocations, and normalized SHA-256
`077e0801f697c2007d587114989d197ac3e047ca260fd51cc6f0a9987dc02024`.
The complete canonical object gate now reports 29 exact, zero residual, and
47 unwritten of all 76 January functions; the four exact leaves newer than the
isolated starting point remain exact.
