# Ten verified Halo object completions (2026-09-23; corrected 2026-09-24)

The initial **426/833 -> 437/833** display was overcredited. A later
cross-lane ordinary-link check found that this batch's `leaf_map` and the
inherited `biped_limp_noodle` both emit a `_distance3d` copy incompatible with
January's selected ActionVehicle provider. Both whole-object labels were
withdrawn. On that corrected basis this batch adds **ten** complete objects:
**425/833 -> 435/833** (Halo-owned **315/468 -> 325/468**). The two
`leaf_map` functions remain strict exact, adding **1,420 meaningful / 1,440
padded code bytes** but *not* a complete object. No Claude limited-lane file
was edited by this batch. See
`claude_object_closure_canonical_reconciliation_20260924.md` for the provider
proof and merged result.

## Seven newly admitted `Matching` units

| Unit | Target functions | Boundary resolved |
| --- | ---: | --- |
| `source/ai/action_obey` | 27/27 | Removed the manual `random_range` wrapper and macro rename, using the real shared-header inline instead. All extra helpers are compatible SELECT_ANY copies, and ordinary ActionObey/ActionCharge and random-range-provider links pass both orders. The older rejection asserting a 20/27 graph and failed ActionCharge link was remeasured and retired. |
| `source/ai/ai_script` | 116/116 | Target-owned data and compatible selected inline helpers were checked; pooled COMMON entries agree with January. |
| `source/ai/encounters` | 61/61 | Target-owned data and compatible selected inline helpers were checked; the pooled COMMON entry agrees with January. |
| `source/game/game_engine_ctf` | 39/39 | The sole 136-byte vtable attribution gap is a false negative; all 28 relocations resolve to the same image destinations. |
| `source/game/game_engine_race` | 36/36 | The sole 136-byte vtable attribution gap is a false negative; all 25 relocations resolve to the same image destinations. |
| `source/game/player_control` | 49/49 | The target-owned data and selected helper copies are exact; 30 ordinary provider-order link probes pass. A candidate-only TU-static name points **inside** the identical 1,536-byte target-owned data section, not to extra storage. |
| `source/rasterizer/xbox/rasterizer_xbox_transparent_geometry` | 22/22 | All 40 target-owned noncode sections and 22 selected helper copies agree; ordinary links pass in both orders. There are no address-named source definitions. |

The admitted shared-header inline COMDATs use the later owner-approved
folded-inline exception: the real helper call remains in source, the emitted
copy is byte-identical to January's selected copy, the caller is strict exact,
and ordinary selected-provider links and whole-board regression checks pass.
Candidate-only helper copies receive **no** function credit in their emitting
TU. No forced inline, manual expansion, or provider metadata edit was used.

## Three existing `Matching` labels made genuinely complete

`game_allegiance`, `game_engine_slayer`, and `hud_definitions` already had
exact code and `Matching` labels, but their last 40-, 136-, and 20-byte data
sections were undercredited. The fail-closed semantic-data verifier now checks
each section's payload, flags, owner, relocation locations/types, and
independently resolved destinations before adding **196 data bytes** and
allowing whole-object completion. These are accounting corrections, not newly
written data. The same proof corrects the `leaf_map`, CTF, and race data
sections (2,784 additional credited data bytes), though `leaf_map` remains
whole-object incomplete for the separate code-provider conflict. All six snapshots live in
`config/semantic_data_matches.json`; each is rechecked on every report build.

## Verification and exclusions

- Full `ninja` passes; the stable per-function sweep against
  `scratch/sixth_five_exact_20260923.json` gains only the two `leaf_map` bodies,
  with **zero regressions**.
- **7,576/8,245** strict functions; Halo code **1,548,728/1,770,166**
  meaningful bytes. The previous code count was 1,547,308.
- Parks: **103 active, zero stale/invalid**. Pytest: **1,154 passed,
  5 skipped, 26 subtests**. Both edited source files have zero fake-match
  review leads; `git diff --check` passes.
- Admission contradictions fall from 26 to 23; the remaining ones are not
  silently waived.
- `game_engine`, `path`, `actor_firing_position`, and `model_animations` stay
  `NonMatching`: their candidate `_distance3d` conflicts with the selected
  ActionVehicle provider. `path_obstacle_avoidance` has other NODUP link
  conflicts. `rasterizer_xbox_hardware_geometry` retains six address-named
  private functions, contrary to the naming rule. These were not admitted.
