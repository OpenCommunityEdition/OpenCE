# next-150K lane — HANDOFF (started 2026-09-15, completed 2026-09-16)

Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/next-150k-house-clean-20260915`.
Frozen baseline **`ae10935da609486c2f5484757c897fa73e6fdf03`**; final commit **`927d135ec`** plus this documentation
commit. **Nothing was pushed. The canonical checkout was never touched** (it remains `6e579b395` on
`jonas/exact-pilots` with only its own pre-existing dirty files).

## 1. Result

| measure | value |
| --- | ---: |
| **New strict meaningful exact bytes (the target metric)** | **6,804** |
| New strict padded bytes | 6,832 |
| Newly exact functions | 7 |
| Newly exact whole objects | 1 (board 308 -> 309 of 619) |
| Accepted semantic/COFF-shape functions newly accepted but NOT strict | 0 |
| Fuzzy bodies landed (always zero credit) | 0 in this lane |
| Park reopens | 5 |
| Park re-baselines (zero credit) | 3 |
| **Regressions** | **0** |
| Target | 150,000 |
| Shortfall | 143,196 |

Accepted ledger 7,300 / 1,338,063 -> **7,307 / 1,344,867**. Strict snapshot 7,266 -> **7,273 exact** of 8,245.
Board 1,387,661 -> **1,394,493** of 1,922,413 padded. Parks 279 -> **274 active / 0 stale / 0 invalid**.

### Credited functions

| object | function | padded | meaningful | how it closed |
| --- | --- | ---: | ---: | --- |
| ai/ai_debug | `_ai_debug_render_encounter` | 2336 | 2330 | January's counting loop is a `while` with the initialiser before it, not a `for` with an empty body. Carries an authenticated names-only rename (see section 4). |
| ai/actor_perception | `_actor_perception_refresh_test_object` | 1744 | 1740 | wave n2 |
| structures/structure_detail_objects | `_structure_render_detail_objects` | 1136 | 1133 | natural array initializer — the form the w2 verifier asked for when it rejected the hand-written `a[0]=0; memset` expansion (R6) |
| text/draw_string | `_bitmap_draw_character` | 944 | 941 | park reopen |
| interface/hud_sounds | `_hud_play_sound` | 352 | 349 | removing the one-use `index` alias and reading `sound_indices` directly (A5). **Not** the `volatile`/`goto` body the park record shows was deliberately replaced |
| scenario/wind | `_wind_variance_get` | 224 | 224 | recovered integer sign-clear, under an explicit owner ruling (section 4) |
| bink/bink_playback | `_bink_free_texture_cache` | 96 | 87 | the park premise was the repo's own `match_dassert` wrapper; Bungie's own `random_math.c` spells those two sites `vassert`, and the tree is 874 `match_vassert` to 14 `match_dassert` |

## 2. Waves

| wave | scope | dispatched | strict result | subagent tokens |
| --- | --- | ---: | --- | ---: |
| n1 | fresh structural priority (section 8 list), 13 groups | 16,583 meaningful | 5 park reopens, **+2,734** | 4.5M |
| n2 | remaining unmined units, 6 groups | 17,032 | 2 exact + 1 authenticated rename, **+4,070** | 2.3M |
| n3a | parked-structural rows in already-mined units, 12 groups | 32,635 | **0**, 2 zero-credit remeasures | 3.9M |
| n4 | evidence-led: rows with a named missing/extra call, 12 groups | 22,605 | **0** | 3.2M |
| hs-pin | 3-angle probe on `_hs_compile_and_evaluate` | 1 function | **0**, but produced LAW Z | 1.0M |

**The n1 pattern that mattered:** all five n1 landings were parks whose recorded premise was wrong. The park text named
a compiler tie; the real cause was a source defect — a missing statement whose relocation January owns, a hand-expanded
initializer, a one-use alias, a repo-side assert-API spelling, a recovered integer idiom.

**n3a tested whether that generalises to already-mined units. It does not** (12 groups / 32,635 meaningful -> 0 strict),
and "park-premise refutation in previously mined units" is now in the do-not-repeat inventory at the owner's direction.

**n4 tested the strongest remaining signal** — a relocation-by-target census naming a callee one side references and the
other does not — and returned zero. The censuses were real but did not convert: several resolved to naming artifacts,
one was an A17 discarded-result call, and the two that produced strict-exact scratch candidates
(`_motion_sensor_update`, the s3tc trio) are blocked on owner rulings involving an uninitialised read, a documented
assert-text reversal and a 60-byte stack overrun.

## 3. The compiler laws this lane produced

Full statement and evidence: **`opus5_next150_compiler_laws_20260916.md`**. In brief:

- **LAW Z — the constant-zero register pin is source-determined.** Over 572 paired objects: January pins and we pin in
  236 functions (227 strict exact); January pins and we do not in 5 (0 exact); **we pin and January does not in zero**.
  Z1: EBX is the only byte-addressable callee-saved register, so the pin is always a two-way contest for EBX, and no
  January function carries three genuine callee-saved webs and a pin. Z2: only zero **stores** seed the web, boundary
  `stores >= 2 x uses(strongest interfering competitor) + 1`. Z3: what competes is the competitor's span, not ordinal
  position; the zero variable's own seed position is inert, which refutes the "seeded flag" hypothesis in earlier
  ledgers. **Triage consequence: "January pins, we do not" is a SOURCE defect (a surplus long-lived value), not a
  class-D register tie.** The non-protected violators still open are `_king_calculate_hill_state` (448) and
  `_bink_alloc@4` (494).
- **Loop-head alignment law.** A loop head is padded to the next 16-byte boundary iff the distance is 1..8 (5 and 8 emit
  `jmp` plus a nop); 9..15 emits nothing. Detector `(-natural_head) mod 16`.

## 4. Header and config changes, with consumers

**No header was modified in this lane.** One config change:

- `config/symbols.json` line 654, in-place line edit, names only: `_code_00041220` -> `_ai_debug_render_encounter`,
  with the three matching C identifiers in `source/ai/ai_debug.c` (house rule 7 — no `code_<address>` names).
  **Consumers: that one translation unit.** Authentication: the January map atlas (`scratch/atlas/ai_debug.obj.txt`
  line 44), whose offsets for this object cross-validate against symbols.json (`_ai_debug_dispose_from_old_map`
  226736, `_ai_debug_drawstack` 235808, `_ai_debug_render` 269120); no PDB public exists at 266784, consistent with the
  static; corroborated by caller topology, the `encounter_*` callee set and the firing-position assert literal. csplit
  regenerated every split object and the whole-board sweep after the change was gained 7 / regressions 0. The rename
  carries **no additional byte credit**.
- `config/parked.json`: in-place edits only — 5 parks removed by the unpark tool as they became exact, 3 re-measured
  with dated evidence.

## 5. Owner rulings obtained during the lane

- **`_wind_variance_get`: the 2026-08-31 "inactive union member" hold was lifted for this function only.** January
  spills the scaled real, clears its sign bit with a 32-bit `and dword ptr [ebp+0xc],0x7fffffff`, reloads it as a float
  and takes the low byte — not producible by any floating spelling, and the previous `fabs()` body canonicalises to x87
  `fabs` at 208 bytes. Landed as the minimum function hunk (the two-member union form matching the three in-tree
  precedents), with `_wind_variance_initialize` untouched at its parked base. The ruling explicitly supersedes the
  2026-08-31 line, and that supersession is recorded in the object ledger.
- Standing rulings re-confirmed and honoured: the collision_bsp SSE `__asm` macro, both `.bss` splits, the
  editor_flying_camera anchor split, the 7 whole-object admissions, P7, the bitmap cast trade and the actor_combat
  shared working vector all remain declined; no `_point_from_line3d`-dependent reconstruction was used anywhere.

## 6. Verification (final, at `927d135ec`)

- `ninja`: pass; the parked-function check validates 274 ties.
- `stable_verdicts` snapshot `scratch/opus5-next150-final.json` diffed against `scratch/opus5-next150-before.json`:
  **gained 7 / 6,832 padded / regressions 0**.
- `tools/campaign/board.py`: 309/619 objects, 1,394,493/1,922,413 bytes, 7,271/8,245 functions.
- `tools.parked_functions`: 274 active / 0 stale / 0 invalid.
- `tools.audit_object_admission`: 0 contradicted, 7 rejected, 8 candidates (was 7 — `structure_detail_objects` became
  a whole-object candidate; **not admitted**, zero credit).
- `pytest --basetemp scratch/pytest-next150-final`: **1,151 passed, 5 skipped, 26 subtests**.
- `git diff --check ae10935da HEAD`: pass. `git status --short`: no tracked dirt.
- `fake_match_scan.py` over all 9 changed `.c` files: **0 review leads**.
- `_point_from_line3d` guard: passes on every changed unit except `source/ai/ai_debug`, where it is a **pre-existing
  baseline failure** — the `ae10935da` copy of ai_debug.c fails identically (same symbol index 99, same 48-byte
  section) — and the owner census for that unit shows exactly `_code_00041220` removed / `_ai_debug_render_encounter`
  added and nothing else.
- Owner census on every other changed unit: no new candidate-only code/data/BSS/COMMON owner.

## 7. Measured ceiling (why 150,000 was not reachable)

Reported at startup from the whole-board portfolio (`scratch/opus5-next150/portfolio.json`, 457 rows built from the
COFF objects) and unchanged by the lane's work:

| pool | fns | meaningful |
| --- | ---: | ---: |
| All non-exact functions in source units at `ae10935da` | 457 | 450,776 |
| Blocked (protected 35, Codex-reserved 9, do-not-repeat 153, ruling/policy 84, owner-declined 14, do-not-spend 11) | 280 | 312,460 |
| **Eligible** | 173 | **135,795** |
| of which `[sha]`-only ties with no compiler-side lever | 86 | 62,283 |
| of which csplit reloc-identity (bytes already identical) | 16 | 6,283 |
| Non-exact in `libs/` (all libcmt, all have source) | 8 | 3,251 |

Closing **every eligible function at 100%** — which has never happened in any wave — yields 135,795, below the target.
Section 2's bar (~450K high-confidence, or ~750K generic residual) is missed by roughly a factor of six. After the
owner's five-category filter was applied (2026-09-15) the *active* portfolio was 28 functions / 23,544 meaningful, of
which wave n4 consumed 12 groups / 22,605 and landed nothing; **939 meaningful in 4 small groups is all that remains
untried**, and the 142 no-leverage rows (107,968 meaningful) must not be dispatched.

Prebuilt vendor objects (xapilib, binkxbox, d3d8) are excluded deliberately: section 9 forbids counting opaque prebuilt
objects as progress.

## 8. Levers measured EXHAUSTED (do not re-run without a new law)

From this lane and the 250K checkpoint ledger, all measured rather than assumed:

- **Static-linkage census (A1):** 53 January-static globals probed with a scratch csplit plus `static` in C —
  **0 function rows moved** in 17 units; `king_globals` static loses 2 exact rows.
- **C4013/C4133 census** over all 468 units: only 7 sites sit inside non-exact functions, all already known to w3.
- **objdiff under-credit:** 114 strict-exact rows score below 100%, and **all 114 are already in the accepted ledger**.
- **namegap:** 0 proposals.
- **Historical/donor mining** over 142 units and every blob: 13 candidates, **all 13 fail admission** — they are the
  bodies earlier lanes rejected on policy, plus one outright fake (thread_win32's blob declares another TU's *string*
  symbol as a `struct mutex_reference` to use its link address as a loop bound).
- **Park-premise refutation in previously mined units** (n3a): 32,635 meaningful -> 0 strict.
- **New-law detectors run this lane, both clean negatives:** no other function on the board carries January's integer
  sign-clear fingerprint; the remaining `match_dassert` sites are all in functions that are asm-rejected,
  owner-declined or held.

## 9. Unresolved dependencies (each needs an owner ruling, none is a worker task)

1. **motion_sensor `_motion_sensor_update`** — strict-exact candidate exists (`scratch/workers/n4_motion_sensor/m4.c`,
   1168/43, relocation multiset identical) and the park premise is refuted (January calls `_game_engine_running`
   twice). Needs a ruling on preserving an uninitialised `camera_positions[]` read **and** the `_vector_from_points3d`
   census ruling.
2. **s3tc `_RGBToColor` + `_DecodeBlockRGB` + `_DecodeBlockRGB__single_pixel`** (1,040 padded) — strict exact in
   scratch. Needs (a) restoring January's byte-proven assert text `u>=0 && u<=4` over the parked.json 0..3 directive
   (the verifier recommends granting this on `docs/assertions.md:93` alone, and it is independent of the rest);
   (b) an indeterminate-member read ruling; (c) admission of a 60-byte stack overrun. `_Quantize` is a coupled
   remeasure only if (b) is granted. Separately and unconditionally: the parked.json evidence for `_DecodeBlockRGB` is
   factually wrong — the census proves a call-count difference, not "instruction selection and scheduling".
3. **LAW Z violators not yet attacked under the law:** `_king_calculate_hill_state` (448; the surplus is a global
   read-modify-write `mov ebx,[0x194]; inc ebx` that January keeps in a scratch register) and `_bink_alloc@4` (494).
4. **`_hs_compile_and_evaluate`** — needs a ninth zero-valued statement January provably contains (the eight known ones
   are already written), or a source form removing one use from the strongest interfering competitor.

## 10. Integration order

The history is linear; apply `git log --reverse ae10935da..HEAD`:

1. `3083f800b` — four park reopens (structure_detail_objects, draw_string, hud_sounds, bink_playback) + the
   `_structure_get_planar_fog` remeasure. Touches `config/parked.json`.
2. `034141b12` — wind `_wind_variance_get` under the owner ruling.
3. `6ebd05a96` — actor_perception + ai_debug, **including the `config/symbols.json` rename**; the config line and the
   three C identifiers must land together.
4. `927d135ec` — sound_manager zero-credit re-baselines.
5. this documentation commit.

`config/parked.json` is edited in place in most commits, so cherry-picking out of order will conflict on it and fail
the ninja park check. Integrate as one ordered range.

## 11. Overlap

Codex's verified packet `faae809ab` owns `transport_endpoint_set_winsock.c`, `transport_endpoint_winsock.c` and
`network_connection.c`; those, plus `matrix_math.c`, `units.c` and `vehicles.c`, are untouched
(`git diff --name-only ae10935da HEAD` contains none of the six, verified). The Codex action-family reservation
(`action_alert/converse/flee/guard/obey/search/uncover.c`, `actions.c/h`, `ai_runtime.h`, `units.h`) was honoured: none
was dispatched or modified. The two protected transport functions appear in this handoff only as **read-only evidence**
for LAW Z.
