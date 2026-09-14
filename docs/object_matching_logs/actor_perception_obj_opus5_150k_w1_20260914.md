# actor_perception.obj — opus5 150K house-clean lane, wave w1 (2026-09-14)

Worker scope: `source/ai/actor_perception.c` only. No header, config, build or other source file was edited.
Scratch evidence: `scratch/workers/actor_perception/` and notes `scratch/workers/actor_perception.md`.

## Baseline and final real-file gate

| state | exact | residual | unwritten | guard |
| --- | ---: | ---: | ---: | --- |
| baseline | 32 | 8 | 4 | pass |
| final real file | 32 | 11 | 1 (`_code_00020990`, pending rename) | pass |

The landing is a pure insertion (2,183 lines added, 0 removed). All 32 EXACT rows are identical to baseline
(row-by-row diff), the four parks do not drift, and `tools/fake_match_scan.py` reports 0 leads (baseline 0).
No new strict-exact credit is claimed for the real file: the four new bodies are zero-credit fuzzy landings for
previously UNWRITTEN functions. Three further exact gains are staged behind orchestrator/owner decisions (below).

## Per-function outcomes

| function | January | final real file | status |
| --- | --- | --- | --- |
| `_code_00020990` = `actor_perception_refresh_danger_zone` | 1472 / 50 | 1504 / 50 (alias) | fuzzy landed; EXACT with proposals A + C |
| `_actor_perception_refresh` | 2416 / 86 | 2416 / 86, 12 diffs | fuzzy landed |
| `_prop_status_refresh` | 3680 / 91 | 3680 / 92, 218 diffs | fuzzy landed |
| `_actor_perception_update` | 4720 / 170 | 4720 / 170, 72 diffs | fuzzy landed |
| `_actor_perception_desire_prop` (parked) | 560 / 16 | unchanged (parked) | park reopen proposed: strict EXACT |
| `_actor_get_perception_knowledge` (exact) | 240 / 10 | unchanged | proposal A keeps it exact with a short return |

"diffs" = aligned instruction lines differing after ignoring call/data relocation spellings (scratch `score.sh`).

## Key evidence

- **Private name.** January map atlas exact record `00020990 _actor_perception_refresh_danger_zone`; not a PDB
  public, so the source defines it `static`.
- **Knowledge helper is not inlinable in January.** January calls `actor_get_perception_knowledge` out of line at
  every in-TU site (danger zone, `prop_status_refresh` x2, `update`). The baseline-exact multiple-return `long` body
  is auto-inlined by VC7 into all callers. A single-result body with a `short` result and `short` return (January
  compares the orphan-derived value with `cmp ax,0xffff` and never sign-extends a return) is also strict exact and is
  not inlined. HCEX DB records `__int16`.
- **Danger zone.** Asserts at 3227 and 3257 with the exact January strings; vehicle/projectile/suicide case layout;
  `midpoint3d(point_from_line3d(...), &position, ...)` reproduces the x-component operand order; the vehicle-case
  `object_get(object_get_ultimate_parent(...))` needs the parent index in its own statement (argument push order);
  `fast_ftol` supplies the 32-bit inline `fistp`; secondary look 12 = `_secondary_look_dangerous_object`.
- **Refresh.** Overflow warning "actor_perception_refresh overflowed max %s (%d), discarding" (`_error_silent`, 128);
  assert 2669 `!prop->dead`; `desire_prop` is inlined; `NUMBEROF` would make the 128 comparison unsigned (`jae`), so the
  literal matches January; `bit_vector_or` takes the PVS pointer computed in its own statement.
- **prop_status_refresh.** Debug overrides use `game_connection() == _game_connection_local` with
  `ai_debug.ignore_player`, `invisible_player`, `blind` and `deaf`. The private ABIs of
  `actor_perception_assess_vehicle_danger` (position in EAX) and `actor_perception_assess_suicide_danger` (actor in EAX,
  unit in EDI) come out naturally. Closing speed is `-dot_product3d(subtract_vectors3d(...), &actor_to_prop)`.
- **update.** Assert lines 316–751 show that the function sits near the top of January's source. The halts at 721 and
  730 are `display_assert(NULL)` followed by `system_exit(-1)`, matching the house `match_vassert(file, line, FALSE, NULL)`
  precedent. The debug name tables confirm the knowledge enum names and `NUMBER_OF_ACTOR_KNOWLEDGE_TYPES`. Jump tables
  prove the case structure.

## Original behaviour preserved (each has target and layout proof, a BUG comment, and no UB)

1. The uncached-swarm branch of `actor_perception_refresh` walks the perceiving actor's `meta.swarm_unit_index`
   (`[ebp-0x34]` is `actor_get(actor_index)`), not the swarm actor's list.
2. The enemy and friend promotion loops in `actor_perception_refresh` break before advancing. The discard loop
   therefore deletes the entry that reached the limit when that entry was an existing prop. HCEA 2011 behaves the same.
3. `prop_status_refresh` compares desired-target ai-reference scope 1 with actor+0x3a and scope 2 with +0x3c. January's
   `actor_action_handle_vehicle_entry` does the reverse (scope 1 → +0x3c). HCEA 2011 `prop_status_refresh` has the same
   reversal.

## Park reopen: `_actor_perception_desire_prop`

The park premise was that January's `[ebp+0x17]` parameter-home reuse cannot be reached without a carrier union. The
new lever is **store order**. January clears `too_far` only after the last use of `prop_actor_index`. Declaring
`too_far` without an initializer and assigning it after the related-actor fetch reproduces the reuse with ordinary C.
Removing the dead `desire = dead` initializer changes nothing. Result: strict EXACT
560/16/`f9729e40ecdc2cbd` (January). The same store placement is visible in January's inlined copies inside
`actor_perception_refresh`. Candidate (final real file plus only this body):
`scratch/workers/actor_perception/P2_park_reopen_desire_prop.c`. It gates 33 exact; the other three parks are OK, the
guard passes and fakescan finds 0 leads.

## Orchestrator / owner proposals

- **A (header).** In `source/ai/actor_perception.h`, change `long actor_get_perception_knowledge(` to
  `short actor_get_perception_knowledge(`. The paired body is `scratch/workers/actor_perception/knowledge_final.c`,
  plus `_actor_combat_status_investigate = 2` in the TU-local enum. Candidate: `P1_knowledge_short.c` (32 exact, parks OK).
  Consumers: action_charge.c, action_fight.c, action_flee.c, actor_firing_position.c, ai_communication.c (the only caller;
  its park ledger already records that `short` made `_ai_communication_actor_talk_weight` closer).
- **B (symbols.json).** Rename `_code_00020990` (file_offset 133520) to `_actor_perception_refresh_danger_zone` with
  `"static": true`. Optionally make `_actor_perception_refresh` (144016) and `_actor_perception_refresh_test_object`
  (141488) static, paired with the source.
- **C (owner ruling).** Allow the `_point_from_line3d` COMDAT in this TU. January inlines it in the danger zone and calls
  it out of line from `actor_emotion_update`; the emotion_update bytes are identical with or without the TU's external
  define. With A + C the danger zone is strict EXACT (`P13_knowledge_pfl3d.c`: 33 exact; `P123_all.c` with the reopen:
  34 exact). The emitted-symbol guard fails by design.

## Rejected shapes

- `prop_acknowledged` as a macro. It inlines in `prop_status_refresh` (91 relocations) but costs 16 bytes, needs
  acknowledge's assert rewritten, and doubles `prop_get` in `update`.
- An early return in `prop_status_refresh`, `use_maximum_distance` else-if and `!prop_acknowledged` spellings (6 total).
  None inlines the helper.
- A new `prop_orphaned` static inline helper for the `update` tail. It adds a candidate-only COMDAT and gives more diffs.
- Five spellings of the squared 2D distance in `update`: TU macro, pre-computed locals, `vector_from_points3d`, comma
  form and `real_vector2d`. The x87 order does not change.
- Three spellings of the danger acknowledgement timer in `update`. The block order does not change.
- Four `distance_squared` declaration and scope variants in `refresh`, all inert.
- Header `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` removal. It is exact but emits a COMDAT, so it is left for the owner.

## Reopen criteria

- `_code_00020990`: proposals A + C (exact is already measured).
- `refresh` / `update`: after the desire_prop reopen, a natural lever for the inline argument-binding schedule (x87 load
  and store vs integer copies, 3 sites), then the `update` danger-timer block order and tail register tie.
- `prop_status_refresh`: evidence for January's `use_maximum_distance` spelling or nesting under the inline frequency
  law, then register ties.
