# Lane A (AI core) — rejected hypotheses and bounded reopening criteria

Baseline `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`, worktree
`C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`.

These are measurements, not coverage. A reopening criterion confers no credit.

---

## Portfolio-wide negatives (orchestrator)

### R1. Compiler-flag provenance is dead for all eleven translation units

`gate.py source/ai/<unit> --cflag <F> --all` for every allowed TU against
`/QIfist /Ob0 /Ob1 /Ob2 /Os /Ot /Gy /GF /Op /G5 /G6 /Oi- /Oa /Ow` — **143
whole-TU compiles, not one changed any unit's exact count in either direction.**
Baselines reproduced exactly: props 16, ai_script 115, encounters 60,
actor_firing_position 26, ai 43, actors 72, actor_looking 14, actor_moving 31,
ai_communication 44, actor_perception 35, ai_debug 57.

`/QIfist` is fully inert on the ten units that do not already carry it (only
`source/ai/ai_debug.c` does, per `config/config.json`).

**No `config/config.json` change is justified anywhere in this portfolio.**
Reopen only with direct target/compiler build provenance — never because a flag
moves bytes.

### R2. There is no naming-gap or alias opportunity

`tools/campaign/namegap.py --json` returns **zero** proposal rows tree-wide. No
residual in this portfolio is secretly already-exact under another symbol name,
and no `config/symbols.json` rename is available. Do not re-run this as new work.

### R3. Thirteen of the 34 residuals are parked; three were re-baselined

`config/parked.json` holds 184 entries, **13 of which belong to the eleven allowed
units**:

    actors                 _actor_customize_unit, _actor_create_for_unit,
                           _actors_spawn_from_unit, _actor_input_update
    actor_firing_position  _pre_evaluator_attack
    actor_perception       _actor_perception_friend_prop_is_attacking,
                           _actor_perception_aiming_vector_test_blockage
    ai                     _ai_test_line_of_fire, _ai_test_line_of_sight,
                           _ai_test_ballistic_line_of_fire
    ai_communication       _ai_communication_get_player_rating,
                           _ai_communication_actor_talk_weight,
                           _ai_communication_finished

No park was **retired** (no parked function reached strict exact). Three were
**re-baselined** under the campaign's re-baseline rule after their retained
source measurably improved: `_pre_evaluator_attack`, `_ai_test_line_of_fire` and
`_ai_test_ballistic_line_of_fire`. Each entry's `measurements.base` and
`objdiff_percent` were refreshed and its `evidence` extended with the new lever
and a reopening condition. All fuzzy bytes stay at **zero exact credit**. No
unrelated park was touched, and the count remains 184 active / 0 stale / 0 invalid.

The park validator is what caught this: a source change to a parked function
fails `ninja progress` until its measurements are refreshed, which is the
intended safety net and the reason an earlier naive scan of this file (matching a
concatenated `unit:function` string rather than the separate `unit` and
`function` fields) was wrong.

### R4. The objdiff data-section percentages are reporting artifacts

`build/report.json` flags four units with non-`.text` sections below 100%
(ai_script `.rdata` 99.97, ai `.data` 97.81, ai_communication `.data` 50.00 and
`.rdata` 97.59, ai_debug `.bss` 95.95 and `.data` 60.71). A strict identity check
(`scratch/orch/data_identity.py`: raw bytes + full relocation target list in
address order + symbol offsets and classes) finds:

- **ai_communication `.data`** — 276 bytes, 69 relocations, `bytes_equal=True`,
  `relocs_equal=True`, `owners_equal=True`. All 69 name-table pointers resolve to
  the same string literals in the same order (verified by demangling both sides).
  The 50.00% figure is pure reporting.
- **ai_debug `.data`** — 56 bytes, 11 relocations, bytes and relocations equal.
- **ai `.data`** — the single "differing" relocation is the csplit one-past alias:
  January's split has no symbol for `_ai_sound_volume_names`, so a pointer to
  offset 1528 resolves as `_ai_update_section+1528`, the same address.
- `.bss` raw-byte inequality everywhere is BSS non-materialization, not content.

**No data-content work exists in this portfolio.** Never spend a wave on a data
fuzzy percentage without running the strict identity check first.

### R5. The 61 storage-class divergences are a `symbols.json` default

61 named symbols differ in COFF storage class (January 2 = external, ours 3 =
static): ai_script 5, actor_firing_position 18, ai 4, ai_communication 2,
ai_debug 32. **Every one has `"static"` unset in `config/symbols.json`**, so
csplit defaulted the split label to external — the same mechanism recorded for
`objects.obj` in `docs/matching_methodology.md`.

Decisive control: **56 of the 61 are functions that already gate EXACT**
(`_pre_evaluator_global`, `_post_evaluator_attack`, `_ai_debug_render_encounter`,
all seventeen exact `_code_000378xx..0003af00` owners). Storage class is not
blocking any residual here, and the symbols genuinely are file-local in our
source. Not actionable, not edited. Do not mistake this for 61 linkage defects.

### R6. Surplus COMDAT copies are systemic, not per-unit defects

`_point_from_line3d`: January's split has **one** definer
(`source/ai/action_charge.obj`, the copy the linker selected) and 17 referrers;
our build has **seven** definers including `ai_debug`. VC7 emits a copy in every
TU that uses the shared non-static `__inline`; the split attributes only the
selected one. `ai_debug.obj` carrying a copy is the inherited, expected state,
not a house-rule-6 violation. The requirement is only that this lane neither adds
nor removes one, verified with `--forbid-emitted-symbol _point_from_line3d`.

The same property explains every `.rdata` COMDAT count difference: for
`encounters`, all 10 ours-only string COMDATs exist elsewhere in the January
image — **zero** are genuinely absent.

### R7. `ai_script.obj` is function-complete but not whole-object admissible

After wave A1 it is `exact 116 residual 0 unwritten 0` with
`matched_code_percent 100.0`, yet `tools/audit_object_admission.py` still does not
list it, because `.rdata` measures 99.97% (118 COMDATs against January's 103 —
the R6 attribution property). **Function exactness and whole-object completion
are reported separately** (house rule 32). No `config/config.json` status change
is proposed for any unit in this lane.

### R8. Zero invented diagnostic strings anywhere in the portfolio

A per-function string-literal relocation diff over all 34 residuals
(`scratch/orch/fn_string_diff.py`, keyed on the content-derived mangled name):
**no Lane A unit emits a string literal absent from the whole January image**, and
**32 of 34 residuals have exactly matching string sets**. Their divergences are
therefore not missing diagnostics — do not look for one there. The two exceptions
are `_ai_communication_finished` (16 January strings, 0 ours) and
`_ai_communication_event` (7 missing); both are documented as positive findings,
not rejections.

### R9. The inherited `#pragma optimize("", off)` in `ai.c` is load-bearing

`source/ai/ai.c` lines 2888/3033 enclose exactly one function,
`_ai_handle_spatial_effect` (1,130 bytes, currently EXACT and accepted at
baseline). Probe with in-memory `--edits`: deleting both lines moves the unit from
`exact 43 residual 3` to `exact 42 residual 4`. The directive is load-bearing.

**Inherited, disclosed, not removed.** Removing it would trade away an inherited
exact function (house rule 29). It is the only pair of findings the baseline fake
scan reports for `source/ai` (2 of the 22 inherited tree-wide leads). Recorded as
an owner item: read charitably, `/Od` reproducing January where `/O2` cannot is
itself evidence that January built this function with optimization off; read
strictly, house rule 26 would not admit a code-generation directive proposed
fresh. No new pragma was added anywhere in this lane.

---

## Function-level rejections

### R10. `_ai_test_line_of_fire` — the parenthesising spelling stays rejected

A strict-exact form exists (wave D `lof2`: bare `(pills[pill_index].width)`
parentheses plus a corrected store order) and was rejected by a prior
orchestrator because no genuine macro spells those parentheses; the wave-3
call-site census refuted the wrapper-macro hypothesis. **That ruling is binding
and this lane does not override it** — its reopening criterion (an attested or
owner-approved parenthesising spelling for `fast_vector_intersects_sphere` radius
arguments, which would also unlock players `_player_examine_nearby_device`,
collisions `_object_test_vector` and bipeds `_biped_update_moving`) is not met by
anything found here.

The *statement-order* half of that candidate (`line_of_fire = FALSE;` before
`blocking_prop_index = pills[pill_index].prop_index;`, corroborated by January's
store order and by HCEA) is separable and was evaluated on its own merits.

### R11. `_ai_test_line_of_sight` — still policy-blocked

Exactness requires January's indeterminate `collision_fraction` read, which the
campaign forbids, and even the uninitialised draft measures 1024 against 1008.
There is no landable path under any policy outcome. Reopen only on owner reversal
of the uninitialized-read policy **and** a scope/type shape producing frame 0x80.

### R12. `_ai_scripting_migrate_and_speak` — the alternative shapes are refuted

Landed (wave A1) as a preserved original defect. Every alternative that could emit
`add esp, 0x14` against a four-`%s` format was refuted from January's own bytes:
substituting a different third value; reordering the four values; reading the
format as three conversions plus a literal `%` (refuted by the raw `.rdata`:
`pct_count 4`, `pct_s_count 4`); dropping a different value; a wrapper or macro
supplying the fourth conversion (no such owner exists); a historical revision
(23-blob `branch_sweep` census, best 113). Also inert: declaration-order
permutations, a frame hypothesis (both sides `sub esp, 0x404`), a register-tie
hypothesis (16/16 relocations, 0 differing rows).

### R13. `_actor_emotion_update` — two condition-order shapes are worse, not inert

January starts the threat scan at index 9 (`mov eax, 9`) where we start at 8;
writing January's `for (priority = NUMBER_OF_ACTOR_THREAT_TYPES; ...)` equalises
the instruction counts at 487/487 and collapses the divergence from 18
`bijection_walk` events to one region. That form reads `specific_threats[9]`,
one past a 9-element array — a real January off-by-one, with the layout
independently proven by two exact functions (`action_fight` 6/6 reads
`cumulative_threats[5]`; `_actor_combat_update`, 4,672 bytes exact, reads
`cumulative_threats[_actor_threat_visible]`).

**Not landed**, given the campaign's binding precedents against unsafe or
indeterminate reads (`_ai_handle_editing`, `_ai_test_line_of_sight`). Recorded as
an owner decision with complete byte evidence.

The remaining region is the `!defensive_crouch` / `crouch` test order at
`actor_perception.c:3737`. **Two shapes measured and both are worse**, producing
1696 bytes / 40 relocations against January's 1664 / 38:

1. nested order swapped — `if (!defensive_crouch) { if (crouch) ... }`
2. combined — `if (!actor->emotions.defensive_crouch && crouch) ...`

Testing `defensive_crouch` first stops VC7 sharing the `defensive_crouch_timer`
tail with the preceding clearing block. **Do not repeat these two.** January must
reach that order another way — most plausibly a single `if/else` on
`defensive_crouch`, or `crouch` itself materialised differently.

### R14. `_actor_perception_refresh` — inlining the squared distance is unavailable

`distance_squared` (`actor_perception.c:5816`) is used twice, at :5830 and :5901,
so it cannot become an inline call-site expression. The sibling residual
`_prop_status_refresh` already uses the inline form at :4451 and is also
non-exact, so inline-versus-named is not the distinguishing factor. There is no
call boundary between the integer field loads and the square, so the
store-across-a-call law cannot prove a statement order here.

---

## Reopening criteria that remain open

| function | criterion |
|---|---|
| `_ai_scripting_migrate_and_speak` | owner ratification (or reversal) of the insufficient-vararg disclosure; one-token revert |
| `_actor_emotion_update` | owner approval of the `NUMBER_OF_ACTOR_THREAT_TYPES` off-by-one, **plus** a crouch-test order that preserves the shared timer tail |
| `_actor_perception_update`, `_actor_perception_refresh` | a source mechanism that moves the x87 duplicate/square after the integer prop-field run; the FP-intermediate type / interleave lever is the one untested candidate |
| `_ai_test_line_of_fire` | an attested or owner-approved parenthesising spelling for `fast_vector_intersects_sphere` radius arguments |
| `_ai_test_line_of_sight` | owner reversal of the uninitialized-read policy **and** a shape producing frame 0x80 |
| `_ai_communication_event` (second allegiance block) | a `game_allegiance_get_incidents` declaration in its genuine owner header `source/game/game_allegiance.h` — a deferred header prerequisite, not workable in this lane |
