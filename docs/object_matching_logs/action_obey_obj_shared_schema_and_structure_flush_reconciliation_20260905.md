# ActionObey: shared owners and structure-cache flush reconciliation

Date: 2026-09-05. Parent publication: `31a1927c951479106d0475ee99e1fb236a15e89d`.

## Accepted checkpoint and accounting

This checkpoint adds the genuine `action_obey_flush_structure_indices` leaf
and repairs the shared types, enum owners, and public API declarations needed
by the larger Fable ActionObey packet. It does not import that packet wholesale.

- ActionObey advances from 9/27 to **10/27 exact functions**, 439 to **485
  meaningful code bytes**, and 480 to 528 padded bytes.
- Incremental credit is **46 meaningful / 48 padded bytes**, one function,
  and no data. The complete-object count does not change.
- The final canonical build reports **910,347 / 2,198,102 meaningful exact
  code bytes**, **6,187 / 11,060 credited functions**, **2,061,020 / 4,176,062
  matched data bytes**, and **391/833 Matching objects**.
- The complete stable sweep advances 6,232 to **6,233** exact owners out of
  8,245 measured owners, with zero inherited exact losses. This owner census
  is not the same denominator or admission policy as the credited-function total.

Canonical root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.
Donor root, read-only throughout:
`C:/halo-worktrees/fable-50k-small-families-20260904`, tip
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`.

## Source ownership and credibility

`actions.h` owns the public ActionObey declarations. The packet adds the nine
genuine target-public declarations in its existing ACTION_OBEY section and
removes their six consumer-local copies from `actions.c`. The redundant
`actor_action_change` declaration in `action_obey.c` is removed; its existing
owner declaration is unchanged.

The leaf uses `actor_get`, the complete `obey_state_data`, the named
`destination_valid` and `destination_surface_index` fields, `NONE`, and an
explicit return. January independently materializes the obey state at actor
offset 0x9C, tests the byte at 0xCC, and conditionally stores -1 at 0xDC.

Provenance adjudication: this is an authenticated exact reconstruction, **not
a natural first-shot result**. Fable's report says the local `state_data`
pointer corrected an earlier missed candidate. That history remains visible
in the independent review. The accepted pointer is ordinary meaningful source,
matches adjacent action-family source organization, and is independently
supported by the target state-base computation. It is not an inert lifetime
anchor, aliasing trick, or invented control flow. This ruling does not authorize
replaying exhausted spelling/order searches on other functions.

The shared owner correction is one coherent packet:

- `actors.h` owns the full 0x40 `swarm_component_datum`, the 0x14
  `swarm_wander_control`, the five component flags, and `swarm_component_get`.
  Direct HCEX and HCEX_Release PDB records prove signed `short flags`, the
  explicit byte `pad` at 0x1B, and the complete anonymous wander/obey union at
  0x1C. The wander record has ordinary implicit alignment, not an invented
  named padding field. Local partial/duplicate views are removed from
  `actors.c`, `actor_type_infection.c`, and `actor_perception.c`.
- `actions.h` owns the five simple-control bits. Existing 0x24/0x58/0x84
  obey record shapes are unchanged. Infection's local copies are removed.
- `bipeds.h` owns the six directly authenticated runtime bits. Bit zero is
  airborne, not limping. The five misleading uses in `units.c` and one in
  `players.c` are renamed; duplicate local enums in Infection, Bipeds, and
  PlayerControl are removed. All values and underlying datum storage remain
  unchanged. Units remains **189/189 exact**.
- `ai_scenario_definitions.h` owns the five authenticated command-list bits,
  loop-until modifier, and four movement-facing values. The latter names are
  explicitly descriptive reconstructions from January behavior, not claimed
  recovered spellings. Unneeded later manual-BSP and count names are not imported.
- `actors.h` owns `actor_move_keep_moving_past_destination(long)` and
  `action_charge.c` loses its foreign local declaration.
- `recorded_animations.h` owns the genuine play/play-and-delete/play-and-hover
  signatures `(long, short)`. `hs.c` includes that owner and loses the three
  mismatched local `(long, word)` declarations. Definition and caller code
  are unchanged.

No private begin/perform facade, flat obey-only swarm view, new raw offset,
new inline directive, or new math wrapper is introduced by this checkpoint.
It does not bless the inherited ActionObey math-wrapper/callback debt: that
requires the real private caller graph and remains the next reconciliation step.

## Verification and the two residual changes

The public-header phase freezes all 47 actual Ninja consumers: 3,079 runtime
sections unchanged, 54 compiler-local-label spelling changes, and no runtime,
owner, COMMON, or forbidden point-helper addition. All 6,232 exact owners survive.

The subsequent shared-schema phase freezes all **58 actual consumers**, not
merely lexical include matches. Its full runtime comparison has 4,544 unchanged
sections, 62 compiler-local-label-only changes, and exactly two residual code
changes. Neither was accepted from strict-function counts alone:

1. `infection_swarm_control` remains 3,360 padded bytes with 103 relocations.
   Three clear-bit masks use shorter signed-immediate encodings after the
   authenticated signed-word field correction. Exhaustive evaluation covers
   every 16-bit flag value at all three sites: **196,608 comparisons pass**.
   The first two sites explicitly zero upper EAX. The third does not require
   that assumption: only AL is tested and AX stored; TEST replaces the flags,
   and both successor paths fully define EAX before reading it full-width.
   All 130 direct branches, both table jumps, all 103 relocation records,
   shifted local labels, exact allowed alignment bytes, and the masked
   80-byte table/tail are checked. No owner, data, COMMON, or definition changes.
2. `ai_communication_update_speech_timers` remains 672 bytes with the same 43
   ordered relocations. Its only change swaps the independent loads of DI
   from `[EBP+0x10]` and EBX from `[EBP-8]` in 0x167..0x16E. All 27 branches
   are unchanged; none enters the block interior; there is no indirect jump
   or relocated operand in the block. The rest of the bytes are identical.
   Its honest fuzzy score returns from 97.76382% to the previously observed
   **97.67839%**. This is explicitly disclosed, not hidden as an exact gain.
   No speech-timer source or scheduling hint was changed.

The first Ninja invocation compiled the consumers but correctly stopped its
progress step on the now-stale speech-timer evidence. Only after full semantic
review and independent proof was that park remeasured. Final Ninja succeeds.
Root independently replays the hardened residual proof with the same JSON hash.
The final 58-object freeze matches the reviewed after-freeze across all **4,608
runtime sections** with no changes at all.

Final checks:

- Complete stable sweep: +1 exact function / 48 padded bytes, zero losses.
- Leaf: **1,024 cases each** against the January target, final actual object,
  and isolated build. All 256 flag-byte representations and four surface
  values; full modeled actor-state writes, call arguments, stack, preserved
  registers, and direction flag checked. Limit: valid modeled actor handle
  and `datum_get` contract, not a whole-game execution test.
- Separate VC7 scratch TU: **24 compile-time layout/value checks** pass;
  it emits no code and earns no progress credit.
- **288 tests plus 26 subtests pass** after the final park update.
- Parks: **284 active, zero stale, zero invalid**.
- Admission: zero candidates, zero contradictions, five existing displayed
  rejections, zero revocations. No new `_point_from_line3d` emission/reference.
- Protected bitmap source/header hashes are unchanged. No source implementation
  in the new Opus rasterizer/device/widget reservation is modified.

## Frozen evidence and continuation

Key manifests (all under canonical `scratch/`):

- `action-obey-public-api-before-20260905.json` / `...after...`:
  `22082d37d81366a681520949d9502107f7ea43b8e185f29936643cd07c89d0d5`
  / `49d6f456321f119a67d25ac7324d82b4fc282d43cf1e86044f4a342e43284f65`.
- `action-obey-shared-schema-before-20260905.json` / `...after...` / `...final...`:
  `90becfaff10da7f9e549e43d28385da41c37675d51d3c9319347bb7b71600253`
  / `b7fef54d4e8030efd58299c5dd290f7e3e9a0ba0c80e8bd8aaa70a929d95c831`
  / `d88762b5f013727b37c746f8e290a5719779e40ec00a9ae10b5accc6e9fd24bb`.
- `action_obey_shared_schema_residual_independent_proof_v2_20260905.json`
  and root replay both hash
  `2a673deafe4714f0ad74ff074feb24d8cb34cb4f87fe6485fbca89d5911c2e4f`.
- `action-obey-structure-leaf-final-runtime-20260905.json`:
  `b4ce9623dafa0c31d541f6a6aa16528c3c843522f542c47e367c45566b08e059`.

See the sibling API/leaf independent review, shared-schema evidence, and
shared-schema residual proof ledgers for source hashes and detailed ownership.
The private-closure plan is preserved separately. Both real private command
bodies, further actor/modifier enum ownership, and ordinary math-header
materialization remain scratch research until their coherent packet passes.
Do not count the donor's remaining exact claims or these proposals as integrated.

The separate Bitmaps intake preserves 18 distinct donor exact candidates /
4,736 padded bytes, with prototype/ownership/provenance issues still awaiting
reconciliation. It adds no canonical credit in this checkpoint. The Fable lane
as a whole remains unfinished; donor committed and dirty work is preserved.
