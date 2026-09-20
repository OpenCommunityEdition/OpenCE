# Lane A — AI core exact-match closeout: handoff

## Identity

| | |
|---|---|
| Worktree | `C:\halo-worktrees\claude-lane-a-ai-core-20260920` |
| Branch | `claude/lane-a-ai-core-20260920` |
| Baseline | `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c` (parent of `d41b606c`, on `jonas/exact-pilots`) |
| Repository | `C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git` |
| Pushed | **No.** Nothing was pushed and no canonical checkout was modified. |
| Tree state | clean (`git status --short` empty) |

The baseline was reproduced exactly before any work began: Halo accepted
**7,291 / 7,574** functions and **1,443,979 / 1,770,166** meaningful bytes;
overall **7,558** functions / **1,465,211** bytes; parks **184 active / 0 stale /
0 invalid**; admission audit **19 / 0 / 8 / 0**; tools tests **1,152 passed, 5
skipped, 26 subtests**.

The portfolio re-measured to **exactly 79,657** remaining meaningful bytes, per
unit identical to the brief's figures.

## The single most important structural fact about this portfolio

**There are zero unwritten functions.** All eleven units are fully written; the
79,657 bytes are **34 residuals**. The campaign's highest-yield lever — "write
unwritten functions before fighting residuals" — does not exist here, and the
lane's realistic ceiling is correspondingly lower than a byte count alone
suggests.

Thirteen of the 34 are already parked in `config/parked.json`.

## Result

### Strict credit

| | |
|---|---:|
| New strict meaningful bytes | **214** |
| New strict padded bytes | **224** |
| New strict functions | **1** |
| Whole-object completions | **0** |
| **Regressions** | **0** |

`source/ai/ai_script` closes at **116/116 exact, 0 residual, 0 unwritten**.

Production totals moved 1,443,979 → **1,444,193** / 1,770,166 Halo meaningful
bytes and 7,291 → **7,292** / 7,574 functions; overall 1,465,211 → **1,465,425**
and 7,558 → **7,559**.

### Fuzzy extent, all at explicitly zero exact credit

Seven retained source corrections across six units. The largest:
`_ai_communication_finished` moved from **912 bytes / 29 relocations / 305
instructions to 1,568 / 86 / 482** against January's 1,584 / 87 / 484 — objdiff
**42.12% → 91.44%**, gap **672 bytes and 58 relocations → 16 bytes and one
relocation**. Full rows are in the admission manifest.

## Commits, in order

    6130661c  (baseline, not ours)
    9eca0524  Close ai_script.obj at 116/116 (+214 meaningful bytes)
    8110e554  Lane A wave A2: three zero-credit source corrections
    4fa32b2c  Lane A wave A3: restore the missing communication diagnostic graph
    e99467da  Lane A wave A4: actor_move_update structural corrections

Each commit is independently buildable, verified, and cherry-pickable.

## Changed files

    source/ai/ai_script.c              wave A1  (+214 strict bytes)
    source/ai/actor_firing_position.c  wave A2  (zero credit)
    source/ai/actors.c                 wave A2  (zero credit, data topology)
    source/ai/ai.c                     wave A2  (zero credit)
    source/ai/ai_communication.c       wave A3  (zero credit)
    source/ai/ai_debug.c               wave A3  (zero credit)
    source/ai/actor_moving.c           wave A4  (zero credit)
    config/parked.json                 four entries re-baselined
    docs/object_matching_logs/*        wave reports and this handoff

**No header was edited. No `config/config.json` or `config/symbols.json` change.
No comparator, target object, build tool or acceptance-policy change. No park was
retired and no unrelated park was touched.**

## Per-wave results

| wave | commit | strict bytes | what landed | report |
|---|---|---:|---|---|
| A1 | `9eca0524` | **+214** | `ai_script` closes at 116/116 | [waveA1](claude_lane_a_waveA1_ai_script_20260920.md) |
| A2 | `8110e554` | 0 | `_pre_evaluator_attack` recovers the missing relocation (38→39); `_ai_test_ballistic_line_of_fire` reaches 296/296 instructions; `_ai_test_line_of_fire` store order; `actors.c` drops a 12-byte `.bss` January does not have | [waveA2](claude_lane_a_waveA2_fuzzy_20260920.md) |
| A3 | `4fa32b2c` | 0 | the missing `_ai_communication_finished` diagnostic graph (912→1,568 bytes, 29→86 relocations); one `ai_debug` boolean spelling | [waveA3](claude_lane_a_waveA3_communication_20260920.md) |
| A4 | `e99467da` | 0 | `_actor_move_update` (3,088→3,120 bytes, 869→882 instructions) | [waveA4](claude_lane_a_waveA4_actor_moving_20260920.md) |

Two research waves ran behind these: wave 1 spread 22 agents across all eleven
disjoint translation units (diagnose then reconstruct), and wave 2 put five
agents on the precisely localized near-misses. Their negative results are
consolidated in
[the rejected-hypotheses ledger](claude_lane_a_ai_core_rejected_hypotheses_20260920.md).

## The one item that needs an owner ruling

`_ai_scripting_migrate_and_speak` — **the whole 214-byte strict gain rests on it.**

January cleans `0x14` bytes (five dwords) at the `error()` call site and loads
`speech_type` only *afterwards* at `+0x7d`, while the format literal read out of
January's `.rdata` is `"%s: ai_migrate_and_speak %s %s %s"` — **four `%s`
conversions for three values.** Passing insufficient varargs is undefined
behaviour, and the concrete consequence is disclosed in the source: the fourth
conversion's slot is the saved-EDI home, so `vsprintf` dereferences the caller's
entry EDI as a `char *`.

It is landed under the original-bug policy with a full `BUG (preserved for exact
matching)` comment. Supporting it: the branch is reachable only under
`ai_debug.print_migration` / `print_scripting`, so no shipping configuration runs
it; the same defect survives unrepaired in the 2020 build; and the tree already shipped **54** `BUG`
disclosures before this one (33 `BUG (preserved` and 22 `BUG (original` now, one
of which is this lane's), including a UB-class out-of-bounds read at
`source/bitmaps/bitmap_extract.c:2101` and a January diagnostic-argument defect in
this very file at `source/ai/ai_script.c:1597`.

Against it: `astra_90pct_rejected_hypotheses_20260920.md` (Wave 6) says "this is
not permission to introduce UB in another residual" — written about a different
concession, but an owner may read it more broadly.

**If declined, the revert is a single token** (restore `speech_type` as the
fourth argument). `ai_script` returns to 115/116 and the lane's strict total drops
to 0. Nothing else depends on it.

## Portfolio-wide negatives (do not re-spend on these)

| | finding |
|---|---|
| **Compiler flags** | 143 whole-TU compiles across 14 options changed **no** unit's exact count. `/QIfist` is inert on all ten units that lack it. Dead lever. |
| **Naming / aliases** | `namegap.py` returns **zero** rows tree-wide. |
| **Data sections** | Every objdiff sub-100% data percentage is a reporting artifact; raw bytes, relocation targets and symbol offsets are already identical. No data work exists here. |
| **Storage class** | 61 divergences, all `symbols.json` defaults; 56 are on already-EXACT functions. Not actionable. |
| **Surplus COMDATs** | `_point_from_line3d` (1 January definer vs our 7) and every `.rdata` count difference are csplit attribution, not defects. Zero genuinely-absent strings. |
| **Diagnostic strings** | **Zero invented strings** portfolio-wide; **32 of 34** residuals have exactly matching string sets. |
| **Owner census** | Complete: every `report.json` owner has a gate row except `_ai_debug_render_actor_jmptable`, a label inside `render_actor`'s section. |

## Verification discipline

Worker self-reports were **not** taken on trust, and that mattered: of six
improvements proposed across two waves, **three were rejected after independent
re-measurement**, including one that broke `_ai_debug_render_actor`'s exact
24,976-byte size match. In `actor_moving`, *dropping* a worker's verbose `||`
rewrite in favour of the natural ternary was measurably better — 3,120 bytes
against January's 3,136, where the worker's form gave 3,104.

Two of my own claims were also withdrawn after re-measurement and are corrected
in place: an `ai_communication` operand swap I first reported as helpful is
**completely byte-inert**, and an apparent "four functions share one 12-byte
defect" reading was **refuted by a slot census**.

## Every remaining owner, with a concrete reopening condition

The frame and slot censuses (`scratch/orch/characterize.py`,
`scratch/orch/slots.py`) give a specific structural statement for each. Six
residuals have **frame-size mismatches**, which are structural by campaign law:

| function | January frame | ours | slot census | what to look for |
|---|---|---|---|---|
| `_pre_evaluator_attack` | `0x34` | `0x40` | 13 vs 14 | January materialises all three components of `direction` before the dot product and takes it j,k,i; we leave `.i` on the x87 stack, take it k,j,i and need two extra spills |
| `_encounter_create` | `0x14` | `0x10` | 5 vs 5 | **a register-allocation tie, not a declaration.** `[ebp-0x14]` is a *spill slot*, proven by a 32-bit store of a `short` return against a 16-bit compare at the same address. January borrows `bl` for `create_leader` inside `case _unique_leader_type_normal`, forcing `actor_type` to spill; we spill `create_leader` to `[ebp-1]` and keep `actor_type` in a register. Moving `actor_type` to function scope is byte-identical with the frame still `0x10` |
| `_ai_test_line_of_sight` | `0x80` | `0x8c` | 14 vs 17 | three extra objects; also policy-blocked (needs an indeterminate `collision_fraction` read) |
| `_actor_move_vector_avoidance` | `0x60e0` | `0x60ec` | 47 vs 50 | three extra objects |
| `_ai_debug_render_actor` | `0x810` | `0x81c` | **114 vs 110** | we use *fewer* slots in a *larger* frame — January **overlaps storage across disjoint lifetimes** more than our source lets VC7 do. A lexical-scoping question, not an extra declaration |
| `_ai_communication_event` | `0x126c` | `0x1264` | 100 vs 99 | plus a deferred header prerequisite, below |

Near-misses with identical relocation geometry (only non-relocated bytes differ):

- `_ai_communication_get_player_rating` (826 B) — one three-instruction x87 block
  (`fld`/`fmul`/`faddp` on `[ebp-0x18]`) sits 8 bytes earlier in January.
- `_ai_communication_actor_talk_weight` (904 B) — a dependency-free `push eax`
  transposition plus a `[ebp-2]`/`[ebp-3]` slot pair. The operand swap is inert.
- `_code_00039990` (845 B) — **two** `bijection_walk` events: one
  `fmul dword ptr [eax]` one slot earlier in January.
- `_actor_perception_friend_prop_is_attacking` (430 B) — one register plus label
  renumbering.
- `_actor_emotion_update` (1,654 B) — see the owner item below.

Localized single-block leads:

- `_actor_perception_update` (4,720 B) and `_actor_perception_refresh` (2,412 B)
  share **one** defect: January finishes a whole run of integer prop-field loads
  before duplicating and squaring the real; we interleave. Same instruction
  multiset, same relocations, same size. **7,132 bytes on one scheduling
  pattern.** Inlining is unavailable (`distance_squared` is used twice) and there
  is no call boundary to anchor a statement order, so the remaining candidate is
  the FP-intermediate type / x87-integer interleave lever.
- `_code_0003a910` (704 B) — size, relocation count and every relocation target
  already match; relocations 18-22 sit exactly **four bytes later** in ours, so
  one instruction before `0x24a` is four bytes too long.
- `_actor_look_update` (4,714 B) — frame, instruction count and relocation count
  all already match. **Exactly two locals are placed differently**: a `boolean` at
  `[ebp-8]` vs `[ebp-7]` and a dword at `[ebp-0x20]` vs `[ebp-0x24]`; every other
  slot agrees. Reopen only with a mechanism that moves those two without changing
  the `0x58` frame, the 1,439 instructions or the 159 relocations.
- `_prop_get_active_by_unit_index` (144 B) — 5/5 relocations, identical targets,
  one address differs by one byte; an ECX/EDX coloring contest.

Owner decisions:

- `_actor_emotion_update` (1,654 B) — January starts the threat scan at **9**
  (`mov eax, 9`) where we start at 8. Writing January's
  `for (priority = NUMBER_OF_ACTOR_THREAT_TYPES; ...)` takes the real differing
  regions from **3 to 2** (instruction counts are already 487/487). That
  form reads `specific_threats[9]`, one past a 9-element array — a real January
  off-by-one, with the layout **independently proven** by two exact functions
  (`action_fight` 6/6 reads `cumulative_threats[5]`; `_actor_combat_update`,
  4,672 bytes exact, reads `cumulative_threats[_actor_threat_visible]`).
  **OWNER RULING, 2026-09-20: do not land it while it remains fuzzy.** The read is
  not admissible to buy a partial improvement - it must deliver strict exactness
  before it can even be considered, and today it removes one of three real
  differing regions and the function stays residual. Not landed;
  `actor_perception.c` is unchanged by this lane. The one remaining
  region is the `!defensive_crouch`/`crouch` test order, where **two shapes were
  measured and both are worse** (1,696 bytes / 40 relocations against 1,664 / 38)
  because testing `defensive_crouch` first stops VC7 sharing the timer tail.
- `_ai_test_line_of_fire` — a strict-exact form exists (wave D `lof2`) but was
  rejected by a prior orchestrator for its bare call-site parentheses. **That
  ruling stands and was not overridden.** Only its separable store-order half is
  landed.
- The inherited `#pragma optimize("", off)` at `source/ai/ai.c:2888/3033` is
  **load-bearing** — removing it costs `_ai_handle_spatial_effect` (1,130 bytes,
  currently exact). Disclosed, not removed.

## Complete disposition of all 35 owners

Every owner in the 79,657-byte portfolio, with its live gate row at the tip
and a concrete reopening condition. Regenerate with
`python scratch/orch/make_dispositions.py`.

| unit | function | meaningful B | gate at tip | disposition |

|---|---|---:|---|---|

| `ai_script` | `_ai_scripting_migrate_and_speak` | 214 | `EXACT 224 ` | STRICT EXACT (admitted) |

| `props` | `_prop_get_active_by_unit_index` | 144 | `residual 144 [sha]` | parked - class C coloring tie |

| `encounters` | `_encounter_create` | 745 | `residual 752 [sha]` | parked - class C register allocation |

| `actor_firing_position` | `_pre_evaluator_attack` | 1113 | `residual 1120 [sha]` | improved, retained at zero credit |

| `ai` | `_ai_test_line_of_sight` | 996 | `residual 1008 [size 1040!=1008, sha]` | parked - policy blocked |

| `ai` | `_ai_test_ballistic_line_of_fire` | 930 | `residual 944 [sha]` | improved, retained at zero credit |

| `ai` | `_ai_test_line_of_fire` | 327 | `residual 336 [sha]` | improved, retained at zero credit |

| `actors` | `_actor_input_update` | 2380 | `residual 2384 [sha]` | parked |

| `actors` | `_actors_spawn_from_unit` | 668 | `residual 672 [size 688!=672, sha]` | parked |

| `actors` | `_actor_customize_unit` | 638 | `residual 640 [size 672!=640, sha]` | parked |

| `actors` | `_actor_create_for_unit` | 622 | `residual 624 [sha]` | parked |

| `actor_looking` | `_actor_look_update` | 4714 | `residual 4720 [size 4704!=4720, sha]` | parked - two-slot placement tie |

| `actor_looking` | `_actor_look_idle_find_prop` | 602 | `residual 608 [sha]` | parked |

| `actor_moving` | `_actor_move_vector_avoidance` | 4130 | `residual 4144 [size 4192!=4144, relocs 140!=135, sha]` | parked - helper mechanism unresolved |

| `actor_moving` | `_actor_move_update` | 3128 | `residual 3136 [size 3120!=3136, sha]` | improved, retained at zero credit |

| `actor_moving` | `_actor_path_refresh` | 1433 | `residual 1440 [sha]` | parked |

| `actor_moving` | `_actor_destination_update` | 968 | `residual 976 [sha]` | parked - class D |

| `actor_moving` | `_actor_move_test_avoidance_vector` | 743 | `residual 752 [size 544!=752, relocs 17!=16, sha]` | parked - same as vector_avoidance |

| `ai_communication` | `_ai_communication_event` | 8064 | `residual 8064 [size 7776!=8064, relocs 319!=336, sha]` | parked - header prerequisite for half |

| `ai_communication` | `_ai_communication_finished` | 1583 | `residual 1584 [size 1568!=1584, relocs 86!=87, sha]` | improved, retained at zero credit |

| `ai_communication` | `_ai_communication_actor_talk_weight` | 904 | `residual 912 [sha]` | parked - class D |

| `ai_communication` | `_ai_communication_get_player_rating` | 826 | `residual 832 [sha]` | parked - class D |

| `actor_perception` | `_actor_perception_update` | 4720 | `residual 4720 [sha]` | parked - x87/int interleave |

| `actor_perception` | `_prop_status_refresh` | 3674 | `residual 3680 [sha]` | parked |

| `actor_perception` | `_actor_perception_refresh` | 2412 | `residual 2416 [sha]` | parked - x87/int interleave |

| `actor_perception` | `_actor_emotion_update` | 1654 | `residual 1664 [sha]` | OWNER RULED: not landed while fuzzy |

| `actor_perception` | `_actor_perception_refresh_danger_zone` | 1472 | `residual 1472 [size 1504!=1472, sha]` | parked |

| `actor_perception` | `_actor_emotion_unopposable_retreat` | 1263 | `residual 1264 [sha]` | parked |

| `actor_perception` | `_actor_situation_update` | 1262 | `residual 1264 [sha]` | parked |

| `actor_perception` | `_actor_perception_friend_prop_is_attacking` | 430 | `residual 432 [sha]` | parked - class C/D |

| `actor_perception` | `_actor_perception_aiming_vector_test_blockage` | 385 | `residual 400 [size 384!=400, sha]` | parked |

| `ai_debug` | `_ai_debug_render_actor` | 24576 | `residual 24976 [sha]` | parked - frame/scoping |

| `ai_debug` | `_code_00039990` | 845 | `residual 848 [sha]` | parked - two events |

| `ai_debug` | `_code_0003a910` | 704 | `residual 704 [sha]` | parked - one instruction |

| `ai_debug` | `_ai_debug_render_actor_jmptable` | 388 | `(no gate row)` | not independently addressable |



Total: 79657 meaningful bytes across 35 owners.



## Reopening condition, per owner



**`_ai_scripting_migrate_and_speak`** (ai_script, 214 B) — owner ratification of the insufficient-vararg disclosure; single-token revert if declined



**`_prop_get_active_by_unit_index`** (props, 144 B) — a January counter-example whose loop head reads cmp ecx,-1 / mov <callee-saved>,ecx / je with ECX a loop-carried datum cursor, or a proven VC7 lever for that web. Nine mechanism-distinct probes all land on the same sha.



**`_encounter_create`** (encounters, 745 B) — an independently recovered source/donor showing a construct absent from the current body, or a proven lever over which callee-saved register the first-defined pointer takes. The 4-byte frame is a SPILL SLOT, not a declared local.



**`_pre_evaluator_attack`** (actor_firing_position, 1113 B) — a source form in which direction is fully materialised before the dot product (its address escaping to something VC7 will not inline) and the dot-product term order is j,k,i rather than k,j,i



**`_ai_test_line_of_sight`** (ai, 996 B) — owner reversal of the uninitialized-read policy AND a scope/type shape producing frame 0x80; even the uninitialised draft measures 1024 against 1008



**`_ai_test_ballistic_line_of_fire`** (ai, 930 B) — a source form whose last use of origin precedes the point at which velocity home becomes free, plus the fxch-form x87 spelling at 0x21a. Hoisting the two working copies above the collision_flags block is measured WORSE (11 -> 43 regions).



**`_ai_test_line_of_fire`** (ai, 327 B) — an attested or owner-approved parenthesising spelling for fast_vector_intersects_sphere radius arguments; the prior rejection of the bare call-site parentheses stands



**`_actor_input_update`** (actors, 2380 B) — every difference is downstream of one destructive add ebx,0x1ec at 0x6a6; relocations 0..49 are byte-identical and 50..88 uniformly +2



**`_actors_spawn_from_unit`** (actors, 668 B) — a constant -1 pin in a caller-saved register at 0x3b that January does not have; regions 2 and 3 are arithmetically downstream of region 1



**`_actor_customize_unit`** (actors, 638 B) — why the loop counter outranks the change-colors element for a callee-saved register; January emits the equipment-error string and _error pair at 0x213/0x21a BEFORE the 1.0f/0.0f constants, we sink it past them



**`_actor_create_for_unit`** (actors, 622 B) — a source form in which encounter_index has no live range across the swarm loop; the single-exit family is PROVABLY WRONG here because January overwrites ESI unconditionally at 0x44



**`_actor_look_update`** (actor_looking, 4714 B) — a mechanism that moves one byte local from [ebp-7] to [ebp-8] and one dword from [ebp-0x24] to [ebp-0x20] without changing the 0x58 frame, the 1,439 instructions or the 159 relocations



**`_actor_look_idle_find_prop`** (actor_looking, 602 B) — January folds the ai_debug compare into a single cmp ebx,[_ai_debug+56] where we materialise the load first; 12 slots on both sides, no extra local



**`_actor_move_vector_avoidance`** (actor_moving, 4130 B) — an AUTHENTIC mechanism for January absence of the actor_move_transform_avoidance_vector call - a genuine macro or __inline definition, an include/owner difference, or a same-compiler donor call site. Hand-expanding it is forbidden (house rule 13) and still leaves 16 bytes.



**`_actor_move_update`** (actor_moving, 3128 B) — the remaining 16 bytes and 6 instructions



**`_actor_path_refresh`** (actor_moving, 1433 B) — January three skip-branches fall to the return block following the guarded actor_path_clear call; we cross-jump them back to the earlier identical epilogue



**`_actor_destination_update`** (actor_moving, 968 B) — January keeps t on the x87 stack across both offset components and discards it with a third fstp st(0); also an addition operand order at 0x35b/0x36d



**`_actor_move_test_avoidance_vector`** (actor_moving, 743 B) — as _actor_move_vector_avoidance; January makes ZERO calls to the transform helper here where we make two, and references _global_zero_vector3d which we do not



**`_ai_communication_event`** (ai_communication, 8064 B) — the first print_allegiance console_printf block needs NO header change and is separately actionable; the second half needs game_allegiance_get_incidents declared in source/game/game_allegiance.h - a DEFERRED HEADER PREREQUISITE



**`_ai_communication_finished`** (ai_communication, 1583 B) — the remaining 16 bytes and ONE relocation; identify the missing reference with relocdiff and compare the 87 January rows against our 86 by target identity



**`_ai_communication_actor_talk_weight`** (ai_communication, 904 B) — a dependency-free push transposition in the actor_visibility_at_point argument list plus a canonicalised byte-OR load pair; the operand swap is proven byte-inert



**`_ai_communication_get_player_rating`** (ai_communication, 826 B) — one three-instruction x87 block (fld/fmul/faddp on [ebp-0x18]) sits 8 bytes earlier in January; relocation geometry is otherwise identical



**`_actor_perception_update`** (actor_perception, 4720 B) — a mechanism that moves the x87 duplicate-and-square after the run of integer prop-field loads; shares this defect with _actor_perception_refresh (7,132 bytes together). Also the squared-2D-distance macro in the right operand of a || at :6712.



**`_prop_status_refresh`** (actor_perception, 3674 B) — first drift -2 B at 0x9d6; 91/91 relocations and equal size



**`_actor_perception_refresh`** (actor_perception, 2412 B) — as _actor_perception_update. The declaration split of distance_squared at :5816 is measured byte-inert; inlining is unavailable because it is used twice (:5830 and :5901).



**`_actor_emotion_update`** (actor_perception, 1654 B) — OWNER RULING 2026-09-20: do not land the out-of-bounds read while the function remains fuzzy. It must deliver STRICT EXACT before it can be reconsidered; today it removes one of three real differing regions and the function stays residual. Reopen requires BOTH (1) a crouch-test order preserving the shared timer tail - two shapes measured, both worse at 1696/40 against 1664/38 - and (2) a demonstration that with that solved, for (priority = NUMBER_OF_ACTOR_THREAT_TYPES; ...) reaches strict exact.



**`_actor_perception_refresh_danger_zone`** (actor_perception, 1472 B) — VC7 disagrees with January's inline schedule in BOTH directions here: January inlines point_from_line3d (3x __real@42340000, 0 calls) where we call it, and January CALLS actor_get_perception_knowledge twice out-of-line where we inline it (giving us +2 _actor_data and +2 _datum_get). Both call sites genuinely call the accessor in source; nothing is hand-expanded. Splitting the nested point_from_line3d call measures 64 -> 58 regions but moves neither size nor relocations, and does not trigger the inline



**`_actor_emotion_unopposable_retreat`** (actor_perception, 1263 B) — the goto-removal rewrite is byte-inert and moves away from this file own goto done; idiom



**`_actor_situation_update`** (actor_perception, 1262 B) — first drift +23 B at 0x167; 28/28 relocations and equal size



**`_actor_perception_friend_prop_is_attacking`** (actor_perception, 430 B) — identical relocation geometry; one register choice plus label renumbering



**`_actor_perception_aiming_vector_test_blockage`** (actor_perception, 385 B) — January preserves a normalization reciprocal until after the second component store; the !(x > 0.0f) spelling merges one region but moves neither size, instructions nor relocations



**`_ai_debug_render_actor`** (ai_debug, 24576 B) — January sub esp,0x810 against our 0x81c while January touches 114 distinct slots and we touch 110 - January OVERLAPS storage across disjoint lifetimes more than our source lets VC7 do. A lexical-scoping question. Any candidate that loses the exact 24,976-byte size match is rejected.



**`_code_00039990`** (ai_debug, 845 B) — one fmul dword ptr [eax] sits one slot earlier in January; everything else in 848 bytes is identical



**`_code_0003a910`** (ai_debug, 704 B) — exactly one instruction before 0x24a is four bytes too long; size, relocation count and every relocation target already match



**`_ai_debug_render_actor_jmptable`** (ai_debug, 388 B) — a label inside _ai_debug_render_actor .text section with no gate row; closes only when render_actor closes




## Deferred header prerequisites

One, and only one:

| | |
|---|---|
| Symbol | `game_allegiance_get_incidents` |
| Genuine owner | `source/game/game_allegiance.h` (already owns `global_game_team_names` and the rest of the allegiance API) |
| Current state | declared in **no** header in the tree |
| Consumers | `source/ai/ai_communication.c` (the second `print_allegiance` block of `_ai_communication_event`) |
| Function expected to benefit | `_ai_communication_event`, 8,064 meaningful bytes |
| Baseline control | the unit is `exact 44 residual 4`; the first `print_allegiance` block needs **no** header change and is separately actionable |

No surrogate prototype was placed in any consumer, and this half of the block was
not attempted.

## Verification commands and actual outputs

    ninja -j4 all_source progress semantic_progress
      Validated parked compiler ties: 184
      halobetacache: 1,444,193 / 1,770,166 bytes (7,292 / 7,574 functions)
      All:           1,465,425 / 2,198,102 bytes (7,559 / 11,060 functions)

    python -m tools.campaign.stable_verdicts snapshot scratch/lane-a-after.json
    python -m tools.campaign.stable_verdicts diff scratch/lane-a-before.json scratch/lane-a-after.json
      gained source/ai/ai_script::section:51 _ai_scripting_migrate_and_speak 224
      gained 1 224 bytes; regressions 0            (exit 0)

    python scratch/orch/ledger_diff.py scratch/baseline/semantic_report.baseline.json
      +1 function / +214 meaningful bytes; REGRESSIONS: NONE
      baseline accepted entries 7558 -> current 7559

    python -m tools.parked_functions          184 active / 0 stale / 0 invalid
    python tools/audit_object_admission.py    19 candidates / 0 contradicted / 8 rejected / 0 revoked
    python tools/fake_match_scan.py --format json source/ai
                                              2 findings, both the inherited ai.c #pragma pair
    gate.py --forbid-emitted-symbol _point_from_line3d --all   PASS on every changed unit
    git diff --check                          clean
    git status --short                        empty

## Evidence not committed to git

Compact, reproducible scratch evidence lives under
`C:\halo-worktrees\claude-lane-a-ai-core-20260920\scratch\orch\`. No generated
disassembly, copied binary, object file or private symbol asset is committed.

| file | purpose |
|---|---|
| `unitcheck.py` | per-function before/after census with regression detection |
| `ledger_diff.py` | authoritative accepted-ledger accounting, in-scope vs out-of-scope |
| `divcount.py` | real differing aligned regions, normalising the cosmetic annotation |
| `characterize.py` / `characterization.txt` | frame, size, relocations, instructions and first real divergence for all 34 |
| `slots.py` | distinct `[ebp-N]` frame objects each side touches |
| `fn_string_diff.py` | per-function string-literal relocation diff (found the communication gap) |
| `fn_call_diff.py`, `datarelocs.py`, `data_identity.py`, `linkage_sweep.py`, `string_fidelity.py`, `inline_boundary.py` | the portfolio-wide negative results |
| `dumpfn.py` | disassemble a byte window of one function from any object |
| `make_manifest.py` | regenerates the admission manifest from live tool output |
| `triage_packet.md`, `orchestrator_negatives.md` | the full working record, including corrections |

## Statement

Nothing was pushed. No canonical checkout was modified. No other lane was merged,
rebased or incorporated. All work is committed on
`claude/lane-a-ai-core-20260920` and the tree is clean.
