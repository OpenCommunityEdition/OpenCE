# ActorMoving natural aim math-COMDAT audit (2026-09-06)

## Verdict

The three added math COMDATs are genuine, byte-identical `real_math.h`
materializations, not filler and not body conflicts.  They nevertheless create
a **real whole-object link veto**, not merely a failure to classify
`actor_moving.obj` as Matching: the candidate owns SELECT_ANY copies while the
current canonical ActionCharge object owns NODUPLICATES copies of the same
three external names.  The already-proved ordinary both-input-order
ActionObey/Geometry links establish that this exact mixed-selection owner family
produces `LNK2005` rather than silently choosing the identical body.

In this ActorMoving candidate, `_actor_aim_jump` directly relocates to
`_scale_vector3d`, so the conflict is not dismissible as three entirely dead
fallbacks.  `_magnitude3d` and `_square_root` have no incoming candidate
relocation and are honest include-time fallbacks; they still collide by external
name with ActionCharge's NODUP owners.  Do not suppress them with manual math,
forced inline, metadata changes, or a raw/private alias.  They earn no
ActorMoving target-function credit.  Reopening whole-object linkage requires an
authentic canonical math-owner reconciliation, especially ActionCharge, not a
change to this natural fuzzy body.

No production source, header, config, or build artifact was edited, and no
compile or link was run for this audit.

## Frozen candidate and target comparison

| Object | SHA-256 |
| --- | --- |
| natural first candidate, `scratch/actor-aim-jump-fable-natural-first-20260906.obj` | `7f5804d1041cdf8ec25b61eac25cb4ac65f6339459653ace3776c9a844f4b38c` |
| frozen pre-packet ActorMoving, `scratch/actor-aim-jump-fable-natural-before-20260906/source/ai/actor_moving.obj` | `99ce0689f56803817e6ebcddf02b521006d20f76e194b7a7f283f8af6af36c44` |
| frozen schema-only ActorMoving, `scratch/actor-aim-jump-fable-natural-schema-20260906/source/ai/actor_moving.obj` | `7db6aea01fe47f3642aa0cc971daf24e64e2584c9e724a53835357483120dfbf` |
| frozen January ActorMoving, `scratch/actor-aim-jump-fable-natural-before-20260906/files/build/split/source/ai/actor_moving.obj` | `d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa` |

The candidate adds exactly four runtime sections to either frozen baseline:
`_actor_aim_jump` and the following three helper sections.  Each helper is an
external function (`storage=2`, `type=0x20`, `value=0`) in `.text` with flags
`0x60501020`, alignment shift 5, SELECT_ANY (`selection=2`), and zero
relocations.

| Helper | Padded bytes | Relocations | normalized/raw SHA-256 |
| --- | ---: | ---: | --- |
| `_square_root` | 16 | 0 | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` |
| `_scale_vector3d` | 48 | 0 | `6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b` |
| `_magnitude3d` | 48 | 0 | `114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c` |

January ActorMoving defines none of those helpers.  Its `_actor_aim_jump` is
selection 1 and remains the known target body (`256/4`, normalized SHA-256
`141b04fcbe71e0761c688c7a766b0791721fecc38d3d58eb14e6b319fa4f0e84`).
The natural candidate is the deliberately fuzzy body (`256/4`, normalized
SHA-256
`4087deef3f73fc2eae5eff84aa196f9f11f4c64d74b797c8e981124ab6eb5817`):
its same four relocation destinations end with `_scale_vector3d` at `+0xDF`
instead of January's `+0xDE`.  This is the disclosed semantic-fuzzy boundary,
not a reason to tune the source.

Candidate incoming-relocation census:

- `_actor_aim_jump +0xDF` -> `_scale_vector3d` (`REL32`);
- no relocation targets `_magnitude3d`;
- no relocation targets `_square_root`.

The candidate has neither a definition nor any relocation reference for
`_point_from_line3d`.  It adds no data, BSS, COMMON, constant, or other runtime
section.  `_actor_type_swarm_aim_jump` is the only newly undefined program API
and is the genuine typed dependency already identified by the source intake.

## Full owner census

A fail-closed fresh pass parsed exactly 572 current base objects and all 833
January split objects, with zero parse errors and with every file hash stable
across its read.  All definitions have the candidate's exact size, zero-reloc
payload hash, `.text` flags, alignment, external storage, function type, and
zero symbol value.  Selection is the only owner-property difference.

| Helper | Base definitions | January definitions | Combined selection 1 | Combined selection 2 | Payload mismatches |
| --- | ---: | ---: | ---: | ---: | ---: |
| `_square_root` | 57 | 1 | 2 | 56 | 0 |
| `_scale_vector3d` | 44 | 1 | 2 | 43 | 0 |
| `_magnitude3d` | 50 | 1 | 2 | 49 | 0 |

For every helper, the sole base selection-1 definition and sole January
selection-1 definition are both `source/ai/action_charge.obj`:

- current scanned base ActionCharge SHA-256
  `51d151d18841bd0d3b21348ab262996bea2615f6b3096625cbb0365f8aa2aed5`;
- frozen pre-packet ActionCharge SHA-256
  `64cbdbaf5507f67f634055749e5b8f02a30610f34cb32d7543a0231e350798f8`;
- January ActionCharge SHA-256
  `76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea`.

The two base ActionCharge whole-file hashes differ because of intervening
owner-header compilation context, but all three bounded helper sections retain
the exact hashes and selection-1 metadata above.  Current integrated Geometry
contributes the one new selection-2 base copy relative to the earlier frozen
geosphere census; its current object SHA-256 is
`c9711f8dd1a6dcd73b974f2c0dbb6d3f3655dbe1759737766684858981299afb`.

The earlier pinned geosphere owner census,
`scratch/fable_geometry_geosphere_typed_owner_audit_20260906.json` (SHA-256
`383ce4019a386b375a96232f253d959f4a95a4c16efe7dcdd918a2b129fba892`),
independently found the same bodies and unique ActionCharge selection-1 owners
across 572/833 objects before that Geometry copy joined the base tree.

## Inherited-object preservation

A complete section/symbol comparison of the frozen before object against the
candidate, repeated from the frozen schema-only object against the candidate,
finds:

- 40 shared runtime sections unchanged in bytes, normalized relocations,
  section flags/alignment/selection, owner symbol storage/type/value, and
  program-symbol identity;
- four added runtime code sections exactly as listed above;
- zero runtime sections lost and zero inherited runtime sections changed;
- zero COMMON or runtime-header changes;
- the sole shared non-runtime difference is anonymous `.debug$S` shrinking
  from 199 to 185 bytes with its expected debug hash change.

Thus the natural packet itself preserves inherited ActorMoving runtime
metadata.  Its blocker is the external COMDAT ownership collision, not an
inherited-body regression.

## Prior ordinary-link proof

`docs/object_matching_logs/action_obey_obj_partial_graph_reconciliation_20260905.md:103-125`
(SHA-256
`f4d2d542cb4800038d29f5a9b394071889a13ab6038f970f929d40e0b26fe33a`)
records ordinary unmodified base/base links with ActionCharge in both input
orders.  Both fail `LNK2005` for `_square_root`, `_scale_vector3d`,
`_magnitude3d`, and `_normalize3d`; no `/FORCE`, ignore, selection patch, or
object patch was used.  The Geometry intake revalidates that this is the same
byte-identical owner family at
`docs/object_matching_logs/geometry_obj_fable_geosphere_independent_intake_20260906.md:228-243`
(SHA-256
`3a6c18a2e332ef6d5f1e44aefd5b091317d42f7850aa646d5b993666308a42ef`).
ActorMoving contains the first three of those four colliding owners, including
the directly called scale helper.  Therefore the precedent applies without a
new link trial.
