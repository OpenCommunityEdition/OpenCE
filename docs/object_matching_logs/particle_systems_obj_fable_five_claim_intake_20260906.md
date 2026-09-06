# Fable `particle_systems.obj` five-claim intake (read only, 2026-09-06)

## Decision

Do **not** replay the committed Fable translation unit or its full two-header
replacement.  The defensible next packet is the two-function lifecycle/BSP
closure below, after one independently audited owner-schema trial:

| function | January name | linkage | meaningful / padded | relocs | normalized SHA-256 | decision |
|---|---|---|---:|---:|---|---|
| `particle_system_delete` | `_code_0008dd80` | file-private proposal | `166 / 176` | 9 | `f2c3884188b26d73b9d5a9d81de37b82c608b7db3db3038c2ef295266580b8b4` | include |
| `particle_systems_reconnect_to_structure_bsp` | same | public | `312 / 320` | 15 | `c630b0dfe3bfedc35c02a46b52dee34411bf25ac8c7c5f6f7a5aedceef400175` | include |

This is a coherent 478-meaningful/496-padded/24-relocation packet.  The
public reconnect routine is called by January `scenario.obj`; it in turn
calls the private delete routine.  The existing exact
`particle_systems_dispose_from_old_map` and January's still-unwritten update
routine are the other two callers of delete.  A complete relocation census
over all 833 January split objects found no call to delete outside this TU.
The two functions use only real datum/tag/location/object APIs and signed
`short` type iteration.  They emit no math inline helper and require no new
runtime data.

The same-TU-only census and recovered semantic topology support making delete
`static`, but the generic January split symbol itself has external COFF
storage.  Treat the source/config linkage change as one coherent private-owner
proposal, not as linkage recovered directly from the split object's storage
class.

There is **no honest no-header subset** among the five claims.  Every claimed
body reads fields hidden by the current `opaque` owners.  Re-declaring a
private prefix in the C file, retaining the donor's scratch-header include,
or adding raw offsets would only move the facade and is not acceptable.

## Exact five-claim accounting

The historical 1,264 padded bytes are exactly these five target functions:

| semantic function | January name | meaningful / padded | relocs | normalized SHA-256 | current ruling |
|---|---|---:|---:|---|---|
| `particle_system_delete` | `_code_0008dd80` | `166 / 176` | 9 | `f2c3884188b26d73b9d5a9d81de37b82c608b7db3db3038c2ef295266580b8b4` | next pair |
| `particle_systems_reconnect_to_structure_bsp` | same | `312 / 320` | 15 | `c630b0dfe3bfedc35c02a46b52dee34411bf25ac8c7c5f6f7a5aedceef400175` | next pair |
| `particle_system_new_particle_explosion` | `_code_0008f3c0` | `262 / 272` | 9 | `98a73c35d6c50bf0e149e8578ad961a43819188de11233d4efb13ad7fe702f28` | hold: false standalone linkage/owner closure |
| `particle_system_initialize` | `_code_0008f640` | `278 / 288` | 9 | `ca7322edb56c3275a3480eed1dfbfd87edfb8152a11ff78a201a35fceee09409` | hold: exact form was declaration-order retry |
| `particle_system_new_unattached` | same | `205 / 208` | 8 | `39b66f0d516aefd4eee70e8ddb5c9ce80138d8a1f11f467f688738ef348b8e38` | hold with its private initializer |

The total is 1,223 meaningful / 1,264 padded bytes and 50 relocations.  The
current canonical source is the pre-Fable blob and emits none of these five.

### Why the remaining 768 bytes stay held

1. `particle_system_initialize` did not arise from the recorded first natural
   form.  The Fable report explicitly says the exact 288-byte version was the
   second compile and that moving the runtime `type` declaration before the
   tag `type_definition` declaration was done because the earlier order sank
   ESI into the loop.  The later HCEA body uses the opposite, definition-first
   order and also differs in state initialization, so it is not independent
   support for the tuned declaration schedule.  No frozen pre-adjustment C or
   object was found; `w_ps_batch1.obj` already contains the exact hash.  A
   future readable initializer may be retained honestly as a fresh fuzzy body,
   but the Fable strict claim must not be replayed as first-natural evidence.

2. `particle_system_new_unattached` is a credible public body and its exact
   failure path is real: January deletes the allocated datum and returns
   `NONE` from the failed-initialize arm.  This is genuine new control-flow
   evidence relative to the 2026-08-21 rejected candidate, whose two delete
   relocations were seven bytes late.  It nevertheless requires the private
   initializer in the same coherent source packet.  Importing the initializer
   as an external facade would be wrong.  The pair also naturally emits a
   32-byte select-any `real_local_random_range` copy absent from January's
   `particle_systems.obj`; that is a function-level/whole-object ownership
   boundary, not code credit.

3. The Fable source defines `particle_system_new_particle_explosion` with
   external linkage while its own `config/symbols.json` proposal marks the
   target function `static`.  That external definition is what keeps an
   otherwise uncalled leaf in the candidate object; it is a false standalone
   emission boundary.  January retains the private callback through the
   32-byte `_rdata_0025a6b8` dispatch aggregate.  Its six function-pointer
   relocations are, in order, update-default, update-explosion,
   create-default, create-explosion, create-jet, and particle-update-default.
   The Fable source reconstructs neither that typed table nor a complete
   private callback closure.  A genuine future packet therefore needs the
   dispatch owner plus the jet and per-particle update callbacks (or their
   authentic complete caller closure), not an exported explosion leaf.  Its
   natural body also emits a 32-byte select-any `local_random_direction3d`
   copy absent from the January object.

The broad committed C additionally defines the still-held
`particle_system_update_particle_default`, `particle_system_new_particle_jet`,
and residual `particle_system_new_attached`.  The saved `w_ps_edits_v2/v3/v4`
files are explicit arithmetic-expression-order experiments for the update
callback.  Those bodies and experiments are outside the five-claim packet and
must not be imported or reopened by this reconciliation.

## Required authentic owner packet for the recommended pair

The current public owners are incomplete:

- `particle_systems.h` ends `particle_system_datum` at an `opaque38[0x120]`
  tail and leaves most of `system_particle_datum` opaque.
- `particle_system_definitions.h` is only a prefix through the interior
  `point_physics_index` at `+0x44`; it does not own the definition's `types`
  block at `+0x5C`.

The pair needs full, naturally shared records rather than a packet-specific
view:

- `particle_system_definition`, size `0x68`, with the DB-named
  `system_update_point_physics` tag reference at `+0x38` (its index is
  `+0x44`), `system_update_physics` at `+0x48`,
  `system_physics_flags` at `+0x4C`, physics constants at `+0x50`, and
  `types` at `+0x5C`;
- runtime `particle_type`, size `0x40`, including `first_particle_index` at
  `+0x3C`;
- `particle_system_datum`, size `0x158`, with its four `particle_type`
  records beginning at `+0x58`;
- `ps_particle_datum`, size `0x80`, with `next_particle_index +0x04`,
  `location +0x14`, and `position +0x1C`; and
- DB-authenticated flag indices `_particle_system_active_bit = 0` and
  `_particle_system_initializing_bit = 1`.

The target atlas's C++ symbols independently use the names `particle_type`,
`ps_particle_datum`, and `old_particle_system_type`.  The committed Fable
headers instead invent `particle_system_type_datum` and retain the canonical
`system_particle_datum` spelling.  Before a first canonical compile, use the
PDB/atlas-backed type spellings and the DB-backed field names above; do not
copy those weaker donor names merely because type names do not change code.
Likewise, replace the incumbent interior access with
`definition->system_update_point_physics.index`, not the donor's shortened
`definition->point_physics.index`.

For this two-body packet, the very large particle-state/type tag records,
shader definition dependency, physics enums, and explosion/jet constants in
the donor header are not needed.  The runtime records and the complete
0x68-byte system definition above are sufficient and avoid introducing
future-function schema merely to land two lifecycle bodies.  Any retained
unknown/reserved words must stay unnamed rather than receive guessed semantic
fields.

Required source/API ownership cleanup in the same coherent trial:

- add `particle_systems_reconnect_to_structure_bsp(void)` to
  `effects/particle_systems.h`;
- include that owner in `scenario/scenario.c` and remove both of its local
  particle-system reconnect/disconnect declarations (disconnect is already
  owned by the header);
- change the current bit-zero use in `particle_system_orphan` from the
  incorrect `_particle_system_attached_bit` name to the DB-authenticated
  `_particle_system_active_bit`; and
- change `code_0008e0d0` from the interior `point_physics_index` facade to the
  complete tag-reference owner described above.

`effects/effects.c` currently calls `particle_system_new_unattached` while
including only `particle_system_definitions.h`; it does not include the true
API owner despite the 2026-09-04 Effects ledger saying the declaration was
migrated.  Replace that include with `particle_systems.h` (which should
include its definition owner) before any future unattached-body packet.

Current direct include reality is broader than the Fable header request:

- `particle_systems.h`: `particle_systems.c`, `game.c`, `objects.c`;
- `particle_system_definitions.h`: `particle_systems.c`, `effects.c`,
  `objects.c`; and
- the correct API migration adds `effects.c` and `scenario.c` as direct
  `particle_systems.h` consumers.

Thus the trial must freeze and compare at least those five unique C units and
then run the real transitive Ninja graph.  The old report's “three consumers”
claim is not current-complete.  No durable canonical ledger records a failed
full schema blast.  The earlier Jonas ledger tested a much smaller datum-tail
refinement and restored it only because the then-unattached body missed; it
did not report a consumer regression.  The Fable batch's aggregate zero-loss
claim is useful provenance but is not a substitute for a current independent
header trial.

## Source and artifact provenance

- Read-only donor tip/commit: `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`,
  parent `96086c9c4c187dd539af13bdaa9252d6e46ad8f6`.
- Committed Fable C blob: `74ca2e87940e159f7c921cd4c590d52f8ccbb7a0`.
  Its parent blob is `4c30ed4b6dfbd22779bd774515c31a243a38c213`,
  the same logical source revision still present in canonical.
- `scratch/w_particle_systems.c` is the committed Fable source payload,
  SHA-256 `C3A25FD1F4D069B5AF298DB98D69813613E7ABCD71BE96CD97493F0C21D5E9DE`.
- The donor worktree is dirty only in this family through seven semantic
  rename substitutions in `particle_systems.c`; no five-body logic changed.
  Current dirty blob: `bb8b0f4343675e559ddf784836c6ea54fcc46a18`.
- Earliest saved four-claim object: `scratch/w_ps_batch1.obj`, SHA-256
  `40A5F5A9116A8A5750BF76CA338671B671993E60BEB4987BC6BCE68EE642DC9C`.
  It has the four non-explosion strict sections plus a candidate-only
  `real_local_random_range` owner.
- Saved expanded object: `scratch/w_ps_batch2.obj`, SHA-256
  `621EBA9B4D065A4AA5E01349D05051FFCAD457D7A89E5BB295CCB18BB202CB64`.
- Final saved donor object: `scratch/w_particle_systems.obj`, SHA-256
  `1FBAECE7E9E86FBF9D8D727ED85BE28A5760072AAE4C0627B7BD2F4F421A89DC`.
- January split object: SHA-256
  `A6C6B08ADA7FE558C34455C623D8CF5249798F7E023B3CB3DDD4167DC3AC20BA`.
- Canonical baseline object inspected read-only: SHA-256
  `DFBCF940D0832AD0DE0CA82397A2CABFB1D88E1C281E6F864C95473B29FE54AD`.
- Primary donor notes:
  `scratch/reports/particle_systems.md` SHA-256
  `B7984DA41882322A4CB1BB520FAF50740FCA2325D1039BCB7F0C6DC0E9D89697`,
  `scratch/facts/particle_systems.md` SHA-256
  `A05A2CD6DE638F064E16ED60CC158C18FF45FBFDBE5E814CEE0D71C8A0C74ADE`,
  and `scratch/hdr/particle_systems.md` SHA-256
  `31B3FD1129DB18472D2ADCA6FAA05C1980B4CAF888B71BAFCCCEBDEAA0984115`.
- Earlier failed-body history:
  `docs/object_matching_logs/particle_systems_obj_jonas_public_update_20260821.md`,
  SHA-256 `C3B064C9E7C0904F8DC737DE674E575776A14A99820398A45FF865BD0F9C903E`.
- Naming/layout corroboration:
  `research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
  lines 1163-1181 and the DB-derived HCEA headers under
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/`.

This intake made no production, donor, configuration, build, or object-file
changes and performed no compile or source-shape trial.
