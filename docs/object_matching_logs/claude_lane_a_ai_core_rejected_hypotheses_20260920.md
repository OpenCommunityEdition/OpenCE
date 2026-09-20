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
