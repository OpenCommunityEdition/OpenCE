# `ai.obj` responsible-unit exact leaf: proper-header boundary (2026-09-05)

## Result

`ai_get_responsible_unit` is a credible, first-natural, strict-exact public
leaf in isolation, but the complete source-authentic integration packet is
**withheld**.  Its required `ai.h` ownership repair changes two previously
strict Race functions in the full 63-consumer rebuild.  The function therefore
contributes **zero admitted code bytes and zero admitted functions** at this
checkpoint.  All four production-source edits were restored; a clean full
rebuild returns to the published 6,217 strict owners with zero gains and zero
regressions.

This is a header/compiler-context boundary, not a failure of the recovered
function semantics.  No caller-local declaration, alternate header, include
position permutation, declaration-order search, or compatibility shim was
retained to evade it.

## Exact leaf evidence

The first natural scratch replay advances `ai.obj` from **14 exact / 0
residual / 32 unwritten** to **15 / 0 / 31**.  The recovered target function is
119 meaningful / 128 padded bytes with four relocations and normalized
SHA-256
`7b070ce474264ae86baca1f59e9d3f82ce5576b0105898cd34cf9d5233d5c145`.
Target and candidate owners are both external storage class 2, type `0x20`,
selection 1.  All fourteen inherited strict AI owners remain equal.

The complete target/baseline/candidate census is code `46/14/15`, non-code
`50/2/2`, and COMMON `0/0/0`.  The sole added definition is the genuine target
owner `_ai_get_responsible_unit`; there is no removed or candidate-only code
owner, no runtime-data change, and no emitted `_point_from_line3d`.  The only
new undefined references are the real dependencies `_ai_debug`,
`_game_connection`, and `_object_try_and_get_and_verify_type`.

January PDB2.00 names the function at RVA `0x2E440`.  Its target body reads a
32-bit object handle, a byte `responsible_for_weapon_fire` argument, and the
real unit fields `gunner_object_index`, `driver_object_index`, and
`player_index`.  It uses the established `unit_try_and_get` and `unit_get`
access macros.  The Xbox tail tests `game_connection()` and the genuine
`ai_debug.ignore_player` byte at relocation addend `+6`; it suppresses a
player-controlled responsible unit only in local/offline play when that debug
flag is set.  HCEA independently corroborates the public
`(int object_index, uint8_t responsible_for_weapon_fire)` API and the core
gunner/driver/self selection semantics.

A complete January split-object relocation census finds six real callers.
Four are already implemented and strict exact:

- `actor_action_handle_panic_from_burning_to_death`;
- `actor_action_handle_panic_from_attached_projectiles`;
- `actor_perception_find_killer_prop_index`;
- `actor_perception_find_recent_damaging_prop_index`.

The two remaining callers, `ai_handle_death` and `ai_handle_damage`, are still
unwritten in `ai.obj`.  All six incoming relocations are zero-addend REL32.
The exact leaf does not depend on or validate the incomplete local
`ai_globals_prefix` or the iterator views elsewhere in `ai.c`.

## Required coherent source packet

The trial applied the complete ownership repair rather than accepting an
isolated body behind misplaced declarations:

1. `source/ai/ai.c` directly included its owning `ai/ai.h` and the genuine
   `ai/ai_debug.h`, then defined the natural typed body.
2. `source/ai/ai.h` received the public prototype with `long object_index` and
   `boolean responsible_for_weapon_fire`, one parameter per line.
3. The duplicate caller-local prototype was removed from
   `source/ai/actions.c`, which already includes `ai.h`.
4. `source/ai/actor_perception.c` directly included `ai.h`, and its duplicate
   caller-local prototype was removed.

The implementation uses semantic identifiers, typed object-access macros,
ordinary defined control flow, and an explicit return.  It adds no assembly,
forced inline, pragma, volatile scheduling, representation cast, raw offset,
address-style name, fake dependency, synthetic anchor, or helper COMDAT.

## Full 63-consumer trial

The root captured all 63 current `ai.h` consumer objects before applying the
coherent packet, rebuilt the complete graph once, and compared every runtime
owner.  Compilation reached report generation, but progress validation
correctly rejected the build because the existing
`ai_communication_update_speech_timers` park became stale.  The independent
stable sweep fell from 6,217 to 6,216 strict owners: the new 128-byte AI owner
became exact while two inherited Race owners ceased to be exact.

The complete consumer audit records 22 changed objects and 68 changed runtime
owners.  After normalizing compiler-local `$L...` identities, 65 changes are
label-only symbol/relocation metadata with unchanged runtime bytes and
resolved destinations.  Exactly three function bodies change materially:

| Object / function | Before | Trial | Effect |
| --- | --- | --- | --- |
| `ai_communication.obj::_ai_communication_update_speech_timers` | 672 padded, candidate hash `885fdb81966284acf5ba7138a4bbbd2221e7c61c89114fdd6f53b96ba89b81a0` | candidate hash `7db4861a5adf519e0457a0ffe785ff4d47e878fda47cc0a06206a529776ac91c` | already residual; its approved park hash becomes stale |
| `game_engine_race.obj::_race_touch_flag` | strict, 336 padded, hash `7379f4da19781c7ca051ed1ea425409e198538fdfc0eead023a327074414af24`, 30 relocs | hash `6243e9cd589765b788b40d971bb10526cc1d74fdf8bed1788c60c1da80f3b9bf`, 28 relocs | strict regression |
| `game_engine_race.obj::_race_engine_player_update` | strict, 160 padded, hash `9eb8b9b17a472d3156bef8970dea5165e37fbfd05ab9e5b7ba8ac5112b3439b7` | hash `2f03f0b8dc9007be70312f1f171490349657cc74bb72ab75dcc8cd2bc69fed3e`; final Race call relocates from `+144` to `+142` | strict regression |

There are no lost runtime owners and no COMMON-owner changes.  The only added
owner anywhere in the 63 consumers is the intended AI function.  The two Race
changes are real code-generation changes, not the benign local-label
renumbering seen in the other 65 records.

## Restoration and ruling

The full four-file packet was removed as one unit.  After restoration there is
no textual diff under `source/ai`, the full Ninja graph passes, and
`scratch/ai-responsible-unit-source-restored-20260905.json` is byte-identical
to the pre-trial stable snapshot: 6,217 strict owners, zero gains, and zero
regressions.  This confirms the trial left no canonical source or accounting
change.

The leaf must remain withheld until authentic source/type/compiler-context
evidence permits the public declaration to coexist with the two strict Race
owners and the existing AI Communication park.  Reopen on such evidence, not
on header-position shuffling, duplicate prototypes, alternate declaration
spelling, compiler flags, or source-shape tuning.  The separate older-Opus
major-upgrade and inactive-swarm leaves were not compiled or adjudicated in
this packet.

## Preserved evidence

- provisional source/ownership handoff:
  `scratch/ai_responsible_unit_production_manifest_20260905.md`;
- first-natural edit manifest:
  `scratch/ai-responsible-unit-one-shot-20260905.edits.json`, SHA-256
  `58c8c91967837972876235aeb82d6308e489e6ba5437111ad6798bca0755ecb6`;
- first-natural scratch object:
  `scratch/ai-responsible-unit-one-shot-20260905.obj`, SHA-256
  `a593bc40e141dbb6bc3e5a54cb9f756e6ae5facc60ed0ae108c51c3be5ab8f8a`;
- root-integrated trial AI object:
  `scratch/ai-responsible-unit-canonical-first-20260905.obj`, SHA-256
  `85f242a79f4dd0f287aa78d23d93575f6614c2bc958c1ab187b90b5cc8c531fc`;
- AI complete-owner audit:
  `scratch/ai_responsible_unit_canonical_owner_audit_20260905.json`, SHA-256
  `e79a6c254e0a712590fe01ee2de39d458093c94319f84f398a4029ab0f5dfa5e`;
- 63-consumer runtime audit:
  `scratch/ai_responsible_unit_consumer_runtime_audit_20260905.json`, SHA-256
  `0488a76ed0cab58a9d3e2126d83ee2a4b7bdc71e5436feb90a4d88c2608eab4a`;
- local-label/material-delta classifier:
  `scratch/ai_responsible_unit_consumer_delta_classify_20260905.py`, SHA-256
  `d470c07a841b3d4b5661b9b59c9834e2485a3aceac974e84f8d3f625c740d040`;
- failed trial stable snapshot:
  `scratch/ai-responsible-unit-canonical-first-20260905.json`, SHA-256
  `de99cee804332741d923224817220a82d974fd968fec04230017ee04fe8cbd29`;
- stale-park proof:
  `scratch/ai-responsible-unit-parks-trial-20260905.json`, SHA-256
  `65a6fcf0e9e19db40a86d935cc01e76bf587d645d5cb4397407cc2ce6265eacc`;
- pre-trial and post-restoration stable snapshots:
  `scratch/transport-receive-canonical-first-20260905.json` and
  `scratch/ai-responsible-unit-source-restored-20260905.json`, both SHA-256
  `db5d69b6a9ed3f679b14b9511276b771e3593385f97ab52d8ea557579840fe90`.
