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
writing January's `for (priority = NUMBER_OF_ACTOR_THREAT_TYPES; ...)` removes
that divergence. Measured honestly, instruction counts are **already** 487/487 at
production and the real differing regions go from **3 to 2** - the change buys one
of three. (An earlier note claiming "18 bijection events to one region" was wrong;
`bijection_walk` was counting the embedded jump table as instructions.) That form reads `specific_threats[9]`,
one past a 9-element array — a real January off-by-one, with the layout
independently proven by two exact functions (`action_fight` 6/6 reads
`cumulative_threats[5]`; `_actor_combat_update`, 4,672 bytes exact, reads
`cumulative_threats[_actor_threat_visible]`).

**Not landed. OWNER RULING, 2026-09-20: "Do not land `_actor_emotion_update`'s
out-of-bounds read while it remains fuzzy."**

That qualifier is the operative part. The out-of-bounds read is not admissible to
buy a partial improvement: it would have to deliver a strict exact match before it
could even be considered, and today it does not - it removes one of three real
differing regions and the function stays residual. The change is therefore not
landed and `source/ai/actor_perception.c` is unchanged by this lane.

**Reopening now requires BOTH, in this order:** (1) a source form for the
`!defensive_crouch` / `crouch` test order that preserves the shared
`defensive_crouch_timer` tail - two shapes are already measured and both are worse
at 1,696 bytes / 40 relocations against January's 1,664 / 38 - and (2) a
demonstration that with that solved, January's
`for (priority = NUMBER_OF_ACTOR_THREAT_TYPES; ...)` takes the function to strict
exact. Only then does the owner ruling get re-tested. This also aligns with the
campaign's binding precedents against unsafe or indeterminate reads
(`_ai_handle_editing`, `_ai_test_line_of_sight`).

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

---

## Wave 2 and orchestrator additions

### R15. `_encounter_create`'s 4-byte frame is a SPILL SLOT, not a declared local

I inferred from the slot census that January holds a 4-byte object where we hold
a 1-byte one, and read that as a type error. **That reading is refuted from
January's own bytes.** `[ebp-0x14]` is a compiler spill slot for the `short`
`actor_type`:

- `0x183  mov dword ptr [ebp-0x14], ecx` — a **32-bit** store of the return of
  `squad_get_actor_type`, which is `static short` (`encounters.c:2606`). A
  declared `short` home would be `mov word ptr [ebp-0x14], ax`; a declared `long`
  would need a `movsx ecx, ax` that January does not have.
- `0x1a8  cmp word ptr [ebp-0x14], 7` — a **16-bit** compare, so the variable is
  16-bit.
- `0x223/0x226  mov ecx, [ebp-0x14]` then `cmp cx, 7` — the classic full-register
  reload of a spill whose meaningful part is the low word.

One declared variable cannot be both dword-written and word-compared. Moving
`actor_type` to function scope was also measured **byte-identical, frame still
`0x10`** — VC7 enregisters it regardless of declared scope.

The real mechanism is a single register-allocation decision: January borrows `bl`
for `create_leader` inside `case _unique_leader_type_normal` (`xor bl,bl` at
`0x1a6`, `setge bl` at `0x1de`, reloading `ebx` from `[ebp-0xc]` at `0x220`),
which forces `actor_type` to spill; we spill `create_leader` to `[ebp-1]` instead
and `actor_type` keeps a callee-saved register.

Measured inert and **not to be repeated**: `actor_type` declared last in the loop
body; declared first; moved to function scope; and the inner-block carrier-scoping
shape wrapping the whole `switch` with `short actor_type = squad_get_actor_type(...)`.
One further shape moved code away from January and was discarded.

**Reopen** only with an independently recovered source or donor showing a
construct absent from the current body, or a proven VC7 lever controlling which
callee-saved register the first-defined pointer takes.

### R16. `_prop_get_active_by_unit_index` is a closed class-C coloring tie

Nine mechanism-distinct probes across two waves all land on the **same normalized
function sha**. Declaration scope is proven inert in *both* directions (widening
and narrowing), and local-table membership is proven inert. The whole divergence
is which of ECX/EDX holds the loop-carried next-index web, which forces one
push/load transposition and a one-byte relocation drift.

Do not re-derive: deleting the single-use `actor` local; declaring `prop_index`
inside the loop body; a nested traversal block placing declarations at their
definitions; and the six earlier probes. Note also that the object's own sibling
`_prop_iterator_new` uses the named-local spelling for this idiom, so the
production spelling is the better-corroborated one.

### R17. `actor_perception` — all three wave-1 proposals rejected

The file is **unchanged by this lane**. See N11 in the orchestrator record:
the `NUMBER_OF_ACTOR_THREAT_TYPES` off-by-one is an owner decision that buys one
of three regions; removing the `goto` scaffolding is byte-inert and moves away
from this file's own established `goto done;` idiom; and
`!(horizontal_aiming_magnitude > 0.0f)` merges one aligned region while moving
neither size, instruction count nor relocations.

### R18. Two further orchestrator probes, both refuted

- **`_ai_test_ballistic_line_of_fire`**: hoisting `point = *origin;` and
  `arc_velocity = *velocity;` above the `collision_flags` assignment, to make
  `origin` die earlier and flip the parameter home VC7 reuses. Measured **11 → 43**
  real differing regions and the instruction count falls from 296/296 to 293.
  Decisively worse.
- **`_actor_perception_refresh`**: splitting
  `real distance_squared = prop->distance * prop->distance;` into an uninitialised
  declaration plus a separate assignment before the
  `actor_perception_desire_prop` call. **Byte-inert** (9 real regions both ways,
  769/769 instructions). Unlike `_pre_evaluator_attack` there is no intervening
  CALL, so the store-across-a-call law gives VC7 no reason to honour the split.

### R19. The `_ai_communication_finished` cross-jump: five hypotheses, nineteen shapes, all refuted

Run under the owner directive *"chase the cross-jump on
`_ai_communication_finished`"*. Full mechanism, arithmetic and per-shape
measurements in
`docs/object_matching_logs/claude_lane_a_crossjump_ai_communication_finished_20260920.md`.
Nothing landed; `source/ai/ai_communication.c` is unchanged by the chase. Floor
= 1568 / 86 / 482, function-section sha1 `a8fcfe6782e01ed0`.

Two corrections to this lane's own earlier record, both re-derived here from
January's bytes:

- **The gap is 18 real bytes and 4 real instructions, not 16 and 2.** Both
  `gate.py` and `divcount.py` count trailing padding (January: one pad NOP after
  0x62f; ours: three after 0x61d). **A bare `1584` from this function is not a
  size match** — closing only the register half gives 1571 real bytes, which pads
  to exactly 1584 with twelve code bytes still missing.
- **The gap has four parts, not two, and one runs the other way.**
  `0x000..0x1e9` identical; `0x1e9..call _datum_get` **+21 B / +6 insn / +1
  reloc**; `call _datum_get..call eax` **+4 B** (a `jne` that is near in January
  and short in ours); `call eax..end` **−7 B / −2 insn**, i.e. **our** code is
  longer, because January cross-jumps the `"filter "` append into the
  `"nobody "` block's `lea edx/push/call` trampoline while we merge only the
  `call`. `21 + 4 − 7 = 18` and `6 + 0 − 2 = 4`.

The whole relocation deficit is **one** missing
`_ai_communication_find_global_actor_to_talk` call site; `_actor_data` is 3 and
`_datum_get` is 2 on both sides. Both builds emit two resolution blocks and
merge a *different pair*: January merges friend+preselected and keeps the enemy
arm private; we merge enemy+friend back through the `call` itself and keep
preselected private.

**The EBX/EDI swap is not a second decision.** It flipped to January's
assignment for free the moment one probe gave the enemy arm its own resolution,
reproducing `mov ebx,[ebp+8]` at 0x9f, `xor edi,edi; mov di,[...]` at 0xdb and
all three `mov edi,ebx` repairs at January's own offsets. It is also the
*correct* codegen: `unit_index` coalesces into EDI because all three callees
(`find_global_actor_to_talk`, `find_specific_actor_to_talk`,
`look_secondary_at_unit`) take it there, and all three are byte-exact in our
build. January declined the coalescing and paid three repairs.

Refuted families — **do not replay any of these**:

- **Enemy-arm private resolution (4 shapes).** Restores both call sites at
  January's offsets but emits three resolution blocks instead of two
  (`_actor_data` 4 vs 3): 1600 / 88 / 493. Hoisting `reply_actor_index` to fix
  that merges friend+preselected correctly (`_actor_data` → 3) but inserts a
  `mov edi,[ebp-0xc]` reload after each `datum_get` that breaks the enemy arm's
  tail merge (`_datum_get` 3 vs 2): 1616 / 88 / 501. Seeding it with
  `= preselected_reply_actor_index` and deleting the `else` **spills two frame
  slots and destroys the exact frame**: 1648 / 88 / 503.
- **EBX/EDI as an independent lever (5 shapes + control).** Every probe
  byte-identical to the floor. Declaring `speech_priority` textually first made
  VC7 hoist `mov edi,[ebp+8]` *into the middle of* the speech-priority
  computation rather than give up EDI.
- **Friend-guard spelling (5 shapes).** `!= NULL`, a named encounter local, and
  the `?:`-hoisted form are all **byte-for-byte identical** to the floor: VC7
  normalizes `a && a->f != NONE` and `(a ? a->f : NONE) != NONE` to the same IL.
  Binding `owner_team_index` to a local moves the wrong way (1566 → 1560 real
  bytes) by promoting `unit` into EBX and tightening the merge.
- **Dispatch form and case order (5 shapes).** Case source order and case-body
  braces are **byte-inert** for a compare-chain switch — VC7 lays the arms out
  in descending case value regardless. The campaign's case-order law applies
  only where a jump *table* exists. The `if`/`else if` chain that reaches size
  1584 is a **coincidental-compensation trap**: instructions move 482 → 489/491
  against a target of 484, relocations do not move, and it contradicts the
  `switch` over the same enum at `source/ai/ai_communication.c:4557`.
- **`reply_unit_index` data flow (5 shapes).** Hoists, symmetric datum binding,
  declaration-order swaps and scope moves are all byte-identical to the floor
  (and prove `reply_unit_index` never reaches a frame slot — it is in EBX on
  every path). Inverting the outer test is the only byte-mover and is a
  regression: regions 72 → 73 and `relocdiff` rows 58 → 65, because the three
  `match_assert` string relocations the floor matches at January's exact
  addresses (0x1be / 0x1c3 / 0x1c8) shift.

No sibling was lost by any shape: the unit census stayed
`exact 44 residual 4 unwritten 0` throughout and `fake_match_scan` reported zero
findings on every nominated shape.

**Reopening criterion.** Authoritative January source; or a same-compiler donor
emitting two `call _ai_communication_find_global_actor_to_talk` sites *without* a
third resolution block; or a decoded rule for VC7's cross-jump **candidate
ordering** — what makes the backend prefer merging {enemy, friend} over
{friend, preselected} when all three blocks are eligible. That rule would settle
the `"filter "` divergence too, which is the same decision in the debug-string
epilogue.

### R20. Five residuals decoded by hand with the corrected region metric

These were found by re-ranking the portfolio with `tools/campaign/real_regions.py`,
which strips relocation-spelling, relocation-site, branch-immediate and
padding-only noise from `alndiff`'s region count. The raw count systematically
**overstates the closest functions**, so these five had all been ranked further
away than they are. Full table in
`claude_lane_a_residual_triage_20260920.md`; per-function notes in
`scratch/orch/{fpia,pathrefresh,ball}/NOTES.md`.

**No landing is proposed from any of them.** Every probe below was gated; none
touched a production file; the whole-unit censuses never moved.

#### `_actor_path_refresh` (1,440 B) - **0 real regions**, +12 code bytes

Every instruction's text matches. The entire residual is **three branches at
4 bytes each**: January encodes `jne 0x542` / `jp 0x542` / `jp 0x542` short to
the NEAR copy of the return epilogue; we encode near to the FAR copy 0x267 bytes
back. **Both copies of the epilogue exist in both builds** - this is not a
missing or extra block, only which one the three `&&` short-circuit failure
edges bind to. Source site is `source/ai/actor_moving.c:2064`, the
`if (success && endpoint.target_radius > 0.f && distance < ... && ... < 0.5f)`
guard; on every failure edge `success` is known TRUE, so the following
`if (!success)` const-folds away and control goes straight to `return success`.

Cross-jump target selection - but **not automatically unreachable**: the
owner-approved `_actor_perception_refresh` landing closed exactly this class by
giving an else-if arm its own resolution. That is the lever to try here.

#### `_actor_look_idle_find_prop` (608 B) - **1 real region**

    January  cmp ebx, dword ptr [_ai_debug+0x38]
    ours     mov eax, dword ptr [_ai_debug+0x38] ; cmp ebx, eax

The field is `ai_debug.selected_actor_index`, and the source is **already** the
natural form `if (actor_index == ai_debug.selected_actor_index)` at both of its
two sites (`actor_looking.c:728` and `:767`). The split appears exactly where the
scheduler is filling an `fcos` latency gap - January fills it with one
instruction, we fill it with two - so this is instruction selection under x87
scheduling, not a source shape. Note the function is also **3 bytes short
overall**, so a second, branch-width difference is hiding behind the
normalisation; do not treat "1 real region" as "one byte from exact".

#### `_actor_perception_friend_prop_is_attacking` (432 B) - **1 real region**

    January  mov cl, byte ptr [ebx+0x1ec] ; test cl, cl ; jle 0x179
    ours     mov al, byte ptr [ebx+0x1ec] ; test al, al ; jle 0x176

`attacking` is spilled across `unit_get_aiming_vector` and reloaded into AL.
January loads `known_enemies` into **CL**, which keeps AL live, so both early
exits return AL directly at 0x179; we load into AL, destroying it, so both exits
must jump to 0x176 and reload. The two differing branch displacements are a
**consequence** of the register choice, not independent. Tails are byte-identical
from 0x168.

Probes, all measured:

| shape | real regions | verdict |
|---|---:|---|
| nested `if (!attacking) { if (known_enemies > 0) {` | 3 raw / 1 real | **INERT**, byte-identical to the floor |
| swap the `&&` operands | 9 | **WORSE** - reorders the tests; January tests `attacking` first, so our order was already right |
| drop the `> 0` | 1 | INERT, and not proposed regardless: `jle` is a signed test, so it is a semantic change |

The guard spelling is a **dead lever** here. Confirms again that `&&` and a
nested `if` normalise to the same IL.

#### `_ai_test_ballistic_line_of_fire` (944 B) - 8 real regions, NEW observation

Five of the eight are a **parameter-slot role swap** that the previous wave did
not record. The signature puts `origin` at `[ebp+0xc]` and `velocity` at
`[ebp+0x14]`; both are copied out and their dead incoming slots are reused as
scratch - and the two builds assign the reuses the other way round:

    January  mov dword ptr [ebp+0xc], 0xc2b3     collision_flags -> origin's slot
    ours     mov dword ptr [ebp+0x14], 0xc2b3    collision_flags -> velocity's slot
    January  fst dword ptr [ebp+0x14]            x87 temp -> velocity's slot
    ours     fst dword ptr [ebp+0xc]             x87 temp -> origin's slot

The remaining three are one `mov ebx` scheduled a slot early and one x87 operand
order (`fld [ebp+0x14]; fxch st(1); fmul [ebp-0x24]` against our
`fld [ebp-0x24]; fmul st(1)`).

| shape | real regions | verdict |
|---|---:|---|
| floor | 8 | - |
| swap `point = *origin;` and `arc_velocity = *velocity;` | **15** | WORSE |
| swap the `ai_debug.ballistic_lineoffire_start` / `_vector` assignments | **10** | WORSE |

So the order in which the two parameters are consumed is **not** the handle on
which dead slot each scratch value inherits. This belongs in the function's
existing `instruction-scheduling` park as evidence, not as a new structural lead.

#### `_actor_situation_update` (1,264 B) - **2 real regions**

One five-instruction test block (`mov al,[edi+0x12f]; test; je; cmp word
[edi+0x38],0; jne`) that January places **out of line** at 0x21b and returns
from with a `jmp 0xeb`, and that we place **inline** at 0xe9. January therefore
has one instruction MORE (363 against our 362). Block placement, and the
"give the arm its own block" lever from `_actor_perception_refresh` is what to
try. Not probed.

#### Method note worth keeping

Rank by REAL regions, never the raw `alndiff` count, and always read the
**real code end** alongside it. `0 REAL` means every instruction's *text*
matches - it does **not** mean exact, because normalising a branch immediate
also hides a short-versus-near encoding difference, which is a real byte
difference. `_actor_path_refresh` is the worked example: 0 real regions and
still 12 bytes out.

### R21. Wave D's five clean negatives, and the three board-wide censuses they produced

Five of wave D's ten targets returned no landing after heavy probing. Each is
recorded here so the shapes are never re-spent. **All five bank ZERO bytes.**
Per-function detail in `scratch/res4/{path-refresh,situation-update,talk-weight,idle-find-prop,friend-prop}/NOTES.md`.

| function | bytes | shapes gated | best result | class |
|---|---:|---:|---|---|
| `_actor_path_refresh` | 1,440 | 23 | equals floor (0 REAL, +12 B) | cross-jump tie |
| `_actor_situation_update` | 1,264 | 52 | equals floor (2 REAL) | block-placement tie |
| `_ai_communication_actor_talk_weight` | 912 | 41 | equals floor (3 REAL) | scheduling + commutative canonicalisation |
| `_actor_look_idle_find_prop` | 608 | 22 | equals floor (1 REAL, 3 B) | x87-shadow selection + relaxer anomaly |
| `_actor_perception_friend_prop_is_attacking` | 432 | 23 + 10 flags | equals floor (1 REAL, 4 B) | return-block splitting / liveness |

The uniform signature - *dozens of shapes, every one either byte-identical to the
floor or strictly worse* - is what a backend tie looks like from the source side.
Nothing here is a new lever; three of them produced one.

#### Census 1: cross-jump duplicate binding (`scratch/res4/path-refresh/xjscan.py`)

For every function in every split object, find each epilogue copy, then count the
conditional branches that bind to an epilogue copy when a byte-identical copy
exists EARLIER.

    JANUARY 273 functions        OURS 275 functions
    agree 269      January-only 4      ours-only 6

**Binding to the later duplicate is the norm and our compiler reproduces it in
269 of 275 opportunities.** All ten divergences are residual functions, and in
nine of them the binding is a *consequence* of a large structural residual.
`_actor_path_refresh` is the only function on the whole board where this binding
is the SOLE difference - so there is no exact sibling to learn a source shape
from, and the usual "find a closed example of the idiom" route is empty. Its
entire residual is three branches at 4 bytes each (short-to-near), and twelve of
the sixteen landable spellings left the body byte-identical while moving nothing.

#### Census 2: hoisted-block placement (`oolblock.py` / `inlineblock.py` / `hoistdiff.py`)

    functions compared             7729
    identical hoisted-block count  7716   (99.83%)
    January hoists more               9
    we hoist more                     4

Block placement is essentially deterministic and source-driven; the entire board
contains **thirteen** placement divergences, every one a residual. The useful
discrimination: **eight of the nine January-only hoists carry a SURVIVING VALUE
out of the arm** (`xor bl,bl`, `mov eax,0`, `mov edi,0`, `mov eax,2`,
`inc eax; mov [..],ax`, three pushes, flags to a shared join). That is exactly
the class the owner-approved `_actor_perception_refresh` landing closed with
`if (c) x = TRUE; else x = FALSE;`, and it is the reason the lever does NOT reach
`_actor_situation_update` - alone among the nine, its hoisted block carries
nothing at all, so there is no value for a source-level assignment to express.

In-scope divergences for this lane:

    JANUARY HOISTS, WE DO NOT
      _actor_situation_update                        carries NOTHING - lever cannot reach it
      _actor_move_vector_avoidance                   `inc eax; mov [ebx+0x5f0],ax`  <- VALUE
      _actor_perception_aiming_vector_test_blockage  `mov ecx,1`                    <- VALUE
    WE HOIST, JANUARY DOES NOT
      _actor_look_update
      _actor_emotion_unopposable_retreat

This census is the wave's most actionable product: it converts "block placement"
from a vague suspicion into a five-function work list with a lever attached.

#### Census 3: branch relaxation (`scratch/res4/idle-find-prop/relaxval2.py`)

Modelling VC7's relaxer as "shorten a branch if it fits, recomputing the layout
to fixpoint" predicts the encoded width of essentially every branch:

    JANUARY 11328 branches, 33 mismatches (0.29%)
    OURS    11382 branches, 41 mismatches (0.36%)

The anomalies are deterministic rather than random - ten AI-tree conditional
anomalies reproduce at the IDENTICAL address in both builds. `_actor_look_idle_find_prop+0xdf`
is the only AI-tree conditional anomaly January has that we do not, and the 125
bytes of its span are **byte-identical between the builds**, so no source change
inside the span can move the decision without also moving those bytes. To
reproduce January the relaxer would have had to see a span at least 3 bytes
longer than the final code - a late code-shrinking transformation our build does
not perform. That is a closed-form impossibility argument for this site.

#### Two campaign laws corrected by this wave

- **Function-scope promotion changes the frame only when it creates a lifetime
  overlap that block scope did not have.** Promoting `attacking` to function
  scope in `_actor_perception_friend_prop_is_attacking` (p19) changed nothing,
  because it overlaps nothing. The earlier unconditional phrasing overstates it.
- **The `_ai_communication_actor_talk_weight` park reason was wrong.** Its third
  region is not a slot pair and not scheduling; it is commutative-operand
  canonicalisation, already fed the correct frame ranking. Regions 1 and 2 are
  genuinely scheduling, confirmed by three independent sweeps.

#### Do not re-spend

Casts, parentheses and qualifiers anywhere in these five functions (repeatedly
byte-identical - the STRIP TEST keeps returning "the plain spelling already IS
the floor"); `&&` versus nested `if`; operand swaps on a commutative `|`;
declaration order and initialiser form; extra declaration counts (0 through 64
tested on two of them - the declaration-count oracle does not reach these ties);
and compiler flags, swept on three of them with no flag reaching any tie.

### R22. Loop spelling is NOT a lever on this board - and "backward branch" is measurement trap #5

Motivated by the standing law that VC7 unrolls a small constant-trip `for`/`while`
but not a `do..while`, so a loop visible in January's bytes proves the spelling.
That law makes loop SHAPE look like a promising source-reachable axis. It is not,
and this is the measurement that closes it.

`scratch/orch/loopshape.py` classifies every loop in every function of both
builds as `DOWHILE` (conditional back-edge), `ROTATED` (conditional back-edge
whose test is also the target of a forward branch from above) or `TOPTEST`
(unconditional back-edge), and diffs the per-function multiset.

#### The trap, found in my own tool

The first run reported **43 divergences**. That number was wrong, because the
classifier counted every backward branch as a loop. Two idioms already censused
in this campaign emit backward branches that close no cycle at all:

- a **hoisted block** ends in a backward `jmp` to its join;
- a **cross-jump** binds backward to an earlier duplicate of a block.

So the first run was largely re-detecting the hoist and cross-jump censuses
under a new name. Adding a real cycle test - build the intra-function CFG and
keep a back-edge `a -> t` only when `t` can reach `a` by forward control flow -
removes **23 of the 43**, including every function whose residual was already
known to be one of those two classes: `_actor_path_refresh`, `_actor_move_update`,
`_actor_move_vector_avoidance`, `_actor_perception_aiming_vector_test_blockage`,
`_actor_customize_unit`, `_actor_type_flood_desire_shamble`, `_structure_test_line2d`.

**Record this as measurement trap #5: a backward branch is not a loop.** Any
census that counts them will silently double-count the hoist and cross-jump
populations.

#### The corrected result

    functions compared             7729
    identical loop-shape multiset  7709   (99.74%)
    DIVERGENT                        20   (6 in source/ai/, 4 in this lane's files)

And the four in-lane divergences do not survive inspection as loop differences
either. Each reduces to **one extra out-of-line block**, whose backward `jmp`
happens to close a graph cycle because the block sits above its join:

    _actor_emotion_unopposable_retreat  ours has one extra `0x129 jmp -> 0xc9`
    _ai_communication_finished          January has one extra `0x3dd jmp -> 0x39e`
    _actor_situation_update             January's known out-of-line block at 0x21b
    _ai_communication_event             placement inside an 8,064 B structural residual

`_actor_situation_update` is the cleanest proof that the cycle test is necessary
but **not sufficient**: January and our build have the same 1,264 bytes and only
2 REAL differing regions, yet January shows four cyclic back-edges to our one.
Identical instructions placed differently create and destroy graph cycles. A
static cycle test cannot separate a source loop from a placement-induced one.

#### Verdict

On the whole board, 7,709 of 7,729 functions agree on loop shape, and every
in-scope exception is explained by block placement rather than by `while` versus
`do..while`. **Do not spend a wave re-spelling a loop.** The unroller law
remains true and useful as a *read* - a loop present in January's bytes proves
the spelling - but it is not a *lever*, because our build already agrees
essentially everywhere.

#### One concrete observation banked from the sweep

In `_ai_communication_finished` (1,584 B target, ours 1,568 - sixteen bytes
short) January and our build lay out two guarded debug-string appends in
opposite order:

    JANUARY  39e  if (ai_debug.X) { push "rand-failed "; jmp <shared csstrcat tail> }
             3bc  if (ai_debug.X) { csstrcat(buf, "0-chance "); }  jmp 0x39e
    OURS     394  csstrcat(buf, "0-chance ")
             3a8  if (ai_debug.X) { push "rand-failed "; jmp <shared tail> }

January places the "0-chance " block AFTER the "rand-failed " guard and returns
to it with a backward `jmp`, so the guard at 0x39e has two predecessors - one
falling in from the preceding eight-argument `csstrcat`, one jumping back. Ours
lays them out sequentially and falls through. This is one of the four parts of
the previously logged 18-byte / 4-instruction gap, now localised to a specific
pair of source statements rather than to the function as a whole.

### R23. Wave E's four clean negatives, and the four reusable laws they produced

Four targets returned no landing after **115 gated shapes** between them. All four
bank ZERO bytes. Detail in `scratch/res5/{render-actor3,customize-unit,look-update2,spawn-from-unit}/NOTES.md`.

| function | bytes | shapes | residual after decode |
|---|---:|---:|---|
| `_ai_debug_render_actor` | 24,976 | 4 | flying-axes family decoded; byte budget nets exactly zero |
| `_actor_look_update` | 4,720 | 17 | 120 REAL to 96 register-blind; **no missing computation** |
| `_actors_spawn_from_unit` | 672 | 56 | 16 B = 11 in one block + 5 of consequent alignment |
| `_actor_customize_unit` | 640 | 38 | 43 REAL to **ONE** binary allocator decision |

The pattern worth noting: in every one of the four, the *reported* region count
badly overstated the residual, and a **register-blind structural diff** cut it
down - 120 to 96, and 43 to 1. Run that normalisation before ranking a target.

#### LAW: an if/else whose condition is a disjunction lays the ELSE arm out FIRST

For `if (A || B) { X } else { Y }`, VC7 emits Y before X. Established in a lab
across four variants and **confirmed in January's own bytes on
`_actor_action_handle_evasion`**. It holds in both builds, independent of arm
size and of which arm contains the call. Only a no-else `if`, a single-test
`if/else`, a condition bound to a temp, or a duplicated arm gives the canonical
order.

This **retires the whole "the arms are the wrong way round" hypothesis family**.
Corollary, separately measured: `if (A != NONE || B != NONE) {X} else {Y}` and
`if (A == NONE && B == NONE) {Y} else {X}` emit the identical section, so the
disjunction-versus-inverted-conjunction spelling is byte-inert. Do not spend a
shape on it.

#### LAW: the declaration-count oracle does NOT reach inside a function body

Seventeen shapes on `_actor_look_update` establish that within a single function
VC7's frame packing and register allocation are insensitive to declaration
ORDER, to declaration GROUPING or count-per-statement, and to SCOPE DEPTH -
three of them dumpfn-verified byte-identical to the floor. Separately, a TU-global
numbering shift is not a codegen lever: changing COMDAT order and the internal
label counter left the function byte-identical, which also refutes the standing
"extra leading header-inline COMDATs perturb this function" theory.

The declaration-count oracle remains real **across a shared header**; it simply
does not apply within one function body. That boundary is now measured.

#### `_actor_customize_unit`: a 21-byte residual accounted for byte-exactly

The seed's "43 real regions" was an artefact of `real_regions.py` not normalising
register names. Corrected for padding nops and register-blind, the true residual
is **2 surplus instructions and 21 surplus bytes**, and every byte is attributed:
three branches emitted NEAR not SHORT (+12), an entry spill (+3), a loop-init
form (+2), three `mov esi,[ebp-4]` restores (+9), a loop-tail counter reload
(+3), against -3 for holding a value in ebx and -5 for a byte-register choice.
Sum = +21 = 659-638 exactly. Root cause is one binary allocator decision about
the third callee-saved register in the change-colour loop.

Twenty-six of its thirty-seven gated shapes are **byte-identical to the floor**,
which is the headline negative: this function's IL is fully normalised over
scope, loop form, guard form, macro expansion and declaration order. Also
confirmed here: if/else ARM ORDER is byte-inert, so a sunk error block's
placement is a register-allocation consequence; and a named temp versus nesting
a call in an argument list is byte-inert.

#### `_ai_debug_render_actor`: the wave-C header prerequisite is REFUTED

The flying-axes family is decoded. January materialises each
`actor->output.throttle` component ONCE and spills two of them into the SAME
frame cell - a cell reused for two different values is a **compiler temp, not a
named local**, so January's source has no extra locals and needs no header. Its
phase order is TERM-major in source order, with the `.j`/`.k` partials
accumulated in `throttle_vector`'s own slots while `.i` rides the x87 stack. Ours
folds the throttle components as memory operands, reading each three times
(displacement census 0x6e0/0x6e4/0x6e8: January 4/4/2, ours 12/12/6) and
reassociates `a+b+c` into `(a+c)+b`.

The construct that reproduces it is the codebase's own inline helpers, because an
inline function's `real` parameter is ONE IL value - `scale_vector3d` followed by
`point_from_line3d`. **That is a live lead, not a negative**, and it is the next
thing to try on this function.

Two further negatives banked here: operand order of multiplication is byte-inert
(a swap of every product in both flying arms emitted a byte-identical section,
confirming the standing law), and the whole-function immediate census has
**zero** differing rows apart from x87 stack-register operands, so the wave-C
constant fix holds and there is no second wrong literal to find.

#### `_actors_spawn_from_unit`: 16 bytes, fully attributed

The seed's branch-width hypothesis is refuted - branch widths match one for one.
The 16 bytes are 11 in the encounter/squad selection block plus 5 of consequent
loop-head alignment filler. The 11 decompose as +8 (we duplicate two stores in
BOTH arms where January has one copy in the merge block - a tail merge), +5 (we
materialise -1 twice where January materialises once), +2 (our merge compares
read memory where January compares registers), -3 and -1. Every byte is
accounted.

### R24. Wave F: six negatives, every one reduced to a single named decision

Six targets, **103 gated shapes**, no closure, zero bytes. What makes this wave
different from earlier ones is that **every negative is now a byte-exact ledger**
rather than a shrug. Detail in `scratch/res6/*/NOTES.md`.

| function | bytes | shapes | the single decision it reduces to |
|---|---:|---:|---|
| `_ai_test_ballistic_line_of_fire` | 944 | 26 | which recycled dead-param slot the x87 temp gets |
| `_prop_get_active_by_unit_index` | 144 | 50 | whether one `mov esi,ecx` is speculated above a branch |
| `_actor_move_test_avoidance_vector` | 752 | 25 | the transform is inlined twice in January, not called |
| `_ai_communication_finished` | 1,584 | 8 | a cross-jump pair choice, plus a hidden branch WIDTH |
| `_actor_create_for_unit` | 624 | 7 | whether `-1` is PINNED in a register or rematerialised |
| `_encounter_create` | 752 | 7 | whether `actor_type` is homed or enregistered |

#### The method is now settled: register-blind FIRST, then byte attribution

`real_regions.py` does not normalise register names and systematically
overstates. Across the two waves the correction is large and consistent:

    _encounter_create        43 REAL -> 16 -> 9 truly structural
    _ai_communication_finished  34 REAL -> 15
    _actor_look_update      120 REAL -> 96
    _actor_customize_unit    43 REAL -> 1 allocator decision
    _actor_create_for_unit   12 REAL -> 1 allocator decision

Then attribute the surplus BYTES cause by cause until they sum exactly. Five
functions across the two waves now have ledgers that close to the byte (+21, +8,
+16, +57/-57, 0). **A residual that does not close to the byte is not yet
understood, and a shape spent before it does is a shape wasted.**

`_prop_get_active_by_unit_index` is the instructive exception: register-blind
changed *nothing*, because both builds already spell the cursor `ecx` and the
saved copy `esi` at every site. Its byte attribution sums to ZERO surplus - 144
= 144, real code ends at 0x8f on both sides - and the only differing bytes are
the 2-byte `mov esi,ecx` and the `je rel8` that follows. Fifty shapes could not
move it.

#### TRAP #6: a branch-WIDTH delta is invisible to every instruction-level tool

`_ai_communication_finished` carries +4 bytes at one site that is **the same
instruction on both sides** (`jne` into the 0-chance block), differing only in
encoded width. It never appears as a region in `real_regions.py` and never moves
the instruction count, so every instruction-level tool reports that region as
equal. Only a branch-width census finds it.

Its full ledger, which closes exactly to the 16-byte gap: cross-jump pair choice
plus EBX/EDI over 0x1e9..0x2d3 = +21 across 7 regions; the hidden width delta =
+4; our `"filter "` block carrying its own `lea`+`push` = -7; the
rand-failed/0-chance placement = +25/-25 **net zero**; two pure-scheduling pairs
= net zero; padding = -2. So the placement asymmetry recorded in R22 is real but
**costs nothing**, and part (d) is refuted as source-reachable.

#### LAW: recycled dead-parameter homes rank by DESCENDING REFERENCE COUNT

A function's dead incoming parameter slots are reused as scratch. Which recycled
object gets which slot is not arbitrary:

> Recycled dead-parameter slots are handed out in **descending address order** to
> the recycled objects ranked by **descending reference count**, ties broken
> earlier-created-first.

Measured on `_ai_test_ballistic_line_of_fire` from both sides:

    floor    seg_end_time 6 -> +0x20 | collision_flags 3 -> +0x14 | x87 temp 2 -> +0x0c
    JANUARY  seg_end_time 6 -> +0x20 | x87 temp       3 -> +0x14 | collision_flags 3 -> +0x0c

The whole 944-byte residual is whether the x87 temp holding `segment_time` gets a
**third** reference to its home slot, which lifts it above `collision_flags` in
that ranking.

Note the scope carefully. LAW F3 refuted "VC7 ranks frame objects by descending
refs/size" for frame objects **in general**. This is a narrower and different
population - *recycled dead-parameter homes* - and for that population a
reference-count ranking does hold and is measured from both sides. The two are
not in conflict; do not let this be read as reinstating F3.

#### `_actor_create_for_unit`: the zero-register pin, again

Register-blind makes the two bodies instruction-for-instruction identical except
for how `NONE` is held. January **PINS** it (`or esi,-1`, then `cmp eax,esi` and
`cmp [ebp+0x10],esi`, returning it at two epilogues); we **rematerialise**
(`cmp eax,-1`, `cmp [ebp+0x10],-1`, `or eax,-1`). Two things follow from the pin
rather than being separate causes: January's `push ebx` sinks to 0x24 after both
guard branches, and because ebx is unsaved on the guard path January needs a
**fifth** epilogue that does not `pop ebx` - five against our four. Ledger sums
exactly to +8.

This is the recurring zero-register pinning mechanism on a new function, and the
function is **already parked with class `register-allocation`** - a park recorded
long before this wave measured the same thing independently. Any shape that moved
it would also have staled that park entry.

#### `_encounter_create`: 57 bytes each way, netting zero

Five of its sixteen register-blind regions are not differences at all: one
`sub esp` consequence, three `[ebp-1]` versus `[ebp-2]` slot renumberings at
identical encoded width, and two jump-table rows that are **trap #2/#4** -
January's table relocates onto `_encounter_create+0x2c0`, ours onto `$L17058`,
same seven bytes and same relocation type. True structural residual: nine
regions, target-only 57 bytes against ours-only 57, net zero.
