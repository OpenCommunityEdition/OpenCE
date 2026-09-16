# next-150K lane — FINAL HANDOFF (2026-09-15 to 2026-09-16)

Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/next-150k-house-clean-20260915`.
Frozen baseline **`ae10935da609486c2f5484757c897fa73e6fdf03`**; final commit **`26601453e`** plus this documentation
commit. **Nothing was pushed** (no upstream is configured). **The canonical checkout was never touched**: it is still
`6e579b395` on `jonas/exact-pilots`, with only its own 8 pre-existing dirty files.

Supersedes the interim version of this file written at `927d135ec` (+6,804).

## 1. Result

| measure | value |
| --- | ---: |
| **New strict meaningful exact bytes (the target metric)** | **9,406** |
| New strict padded bytes | 9,504 |
| Newly exact functions | 15 |
| Newly exact whole objects | 1 (board 308 -> 309 of 619) |
| Newly accepted but NOT strict | 0 |
| Fuzzy bodies landed (always zero credit) | 0 |
| Park reopens / parks removed | 11 (279 -> 268 active) |
| Zero-credit park re-baselines | 3 |
| **Regressions** | **0** |
| Target | 150,000 |
| Shortfall | 140,594 |

Accepted ledger 7,300 / 1,338,063 -> **7,315 / 1,347,469**. Strict snapshot 7,266 -> **7,281** exact of 8,245. Board
1,387,661 -> **1,397,165** of 1,922,413 padded bytes; functions 7,264 -> 7,279.

**The target was not met and was not reachable.** It was reported unreachable at startup, before any spend, and every
subsequent measurement confirmed it (section 7).

### 1.1 Credited functions

| wave | object | function | padded | meaningful | how it closed |
| --- | --- | --- | ---: | ---: | --- |
| n2 | ai/ai_debug | `_ai_debug_render_encounter` | 2336 | 2330 | January's counting loop is a `while` with the initialiser before it; plus an atlas-authenticated names-only rename of `_code_00041220` |
| n2 | ai/actor_perception | `_actor_perception_refresh_test_object` | 1744 | 1740 | wave n2 |
| n1 | structures/structure_detail_objects | `_structure_render_detail_objects` | 1136 | 1133 | natural array initializer, the form the w2 verifier required (R6) |
| n1 | text/draw_string | `_bitmap_draw_character` | 944 | 944 | park reopen |
| Tier A | effects/player_effects | `_player_effect_start` | 736 | 735 | LAW M8 + A4 helper routing: one `set_real_vector3d` call |
| n5 | bink/bink_playback | `_bink_alloc@4` | 496 | 494 | LAW Z diagnosis, plus the owner-admitted one-site `__asm { int 3 }` |
| n1 | bink/bink_playback | `_bink_free_texture_cache` | 96 | 95 | the park premise was the repo's own `match_dassert` wrapper |
| Tier A | ai/actor_moving | `_actor_get_stopping_distances` | 464 | 449 | LAW M8 (one IL local too many) + A9 switch width over adjacent enum values |
| n1 | interface/hud_sounds | `_hud_play_sound` | 352 | 349 | one-use alias removed (A5); not the `volatile`/`goto` body |
| ruling | render/render_debug | `_render_debug_player` | 272 | 266 | LAW D: declared extent, written `MAXIMUM_RENDER_DEBUG_PLAYER_TEXT_LENGTH + 1` |
| n1 | scenario/wind | `_wind_variance_get` | 224 | 213 | January's integer sign-clear, under an owner ruling |
| Tier B | interface/hud | `_temporary_hud_draw_reticle` | 208 | 196 | signed `% 16`: a signed named count replaces the `size_t` NUMBEROF |
| ruling | interface/hud_draw | `_real_rgb_color_to_pixel32` | 192 | 181 | owner-admitted helper asm, evidenced by dword fistp and non-top-slot fmul |
| Tier B | rasterizer/rasterizer_transparent_geometry | `_rasterizer_transparent_geometry_initialize` | 160 | 149 | single exit `success = success && aux()` |
| Tier B | items/weapons | `_weapon_can_be_fired` | 144 | 132 | single exit; two byte-proven distinct FALSE exits (A26) |

## 2. Waves, gains and cost

| wave | scope | strict meaningful | subagent tokens |
| --- | --- | ---: | ---: |
| n1 | 13 fresh structural groups | +2,734 (5 fns) | 4,505,694 |
| n2 | 6 fresh unmined groups | +4,070 (2 fns) | 2,290,464 |
| n3a | 12 parked-structural groups in mined units | **0** | 3,851,252 |
| n4 | 12 evidence-led groups (named missing/extra calls) | **0** | 3,246,976 |
| hs probe | 3-angle study of `_hs_compile_and_evaluate` | 0 (produced LAW Z) | 1,026,709 |
| n5 | LAW Z targets | +494 (1 fn) | 1,081,729 |
| hs research | round-2 probe | 0 (Z4-Z6, S, X; hs proven unreachable) | 938,860 |
| research campaign | 6 corpus studies + adjudication (one interruption, resumed) | 0 directly | 2,920,673 on resume |
| Tier A | the campaign's measured closures | +1,184 (2 fns) | orchestrator-verified |
| owner rulings | render_debug extent + RGB helper | +447 (2 fns) | orchestrator-verified |
| Tier B | final wave, 10 named functions | +477 (3 fns) | 3,257,063 on resume |

The Tier B wave was paused once, 7 minutes in, at the owner's request. Measured from the first run's transcripts, the
pause discarded 263,038 output tokens, 3,269,814 uncached and cache-write tokens, 91,754,617 cache-read tokens and 353
tool calls. The loss scales with elapsed time; pausing at a wave boundary costs nothing.

## 3. Owner rulings obtained in this lane

- **`_wind_variance_get`:** the 2026-08-31 inactive-union hold was lifted for this function only. January spills the
  scaled real, clears its sign bit with a 32-bit `and dword ptr [ebp+0xc],0x7fffffff` and reloads it as a float, which no
  floating spelling produces. Minimum hunk; `_wind_variance_initialize` untouched.
- **`_bink_alloc@4`:** one unit-local `__asm { int 3 }`, as a helper under house rule 10. A capstone census of all 833
  January objects finds four in-body int3, exactly one in Halo source; `__debugbreak()` provably relocates the trap
  from 0xd1 to 0xd4, so only inline assembly reproduces January's placement. That one instruction only.
- **`_render_debug_player`:** the pre-LAW-D buffer-size rejection is superseded. The extent is written in the house
  `[X_LENGTH+1]` idiom as a named enumerator in the file's existing constants enum, never a bare `1025`.
- **`_real_rgb_color_to_pixel32`:** the single evidenced asm block, justified by affirmative hand-written evidence
  (dword fistp, fmul on non-top x87 slots). This does not authorize `_real_argb_color_to_pixel32`, `_fast_ftol_C` or
  assembly elsewhere.
- **Declined** (do not re-propose): the bitmap_utilities cast trade (it trades an exact row), the actor_combat shared
  working vector (standing hold plus read-before-write UB), and every 150K-lane held item (SSE `__asm` distance macro,
  both `.bss` splits, the camera anchor split, the 7 whole-object admissions, P7). No `_point_from_line3d`-dependent
  reconstruction was used anywhere.

## 4. Header and config changes, with consumers

- **No header was modified.** `git diff --name-only ae10935da HEAD -- '*.h'` is empty.
- `config/symbols.json`: one in-place line edit (line 654), `_code_00041220` -> `_ai_debug_render_encounter`, landed with
  the matching C identifiers. Consumer: `source/ai/ai_debug.c` only. Authenticated by the January map atlas
  (`scratch/atlas/ai_debug.obj.txt` line 44), whose offsets for this object cross-validate against symbols.json; no PDB
  public exists at 266784, consistent with the static. csplit regenerated every split object; the sweep was clean.
- `config/parked.json`: in-place edits only. 11 parks removed by the unpark tool as they became exact, and 3 already
  fuzzy parks re-baselined with dated evidence (`_structure_get_planar_fog`, `_sound_refresh_looping`,
  `_update_channel_for_looping_sound`).

## 5. Compiler laws produced (the lane's most durable output)

Ledgers: `opus5_next150_compiler_laws_20260916.md` (round 1, bannered as corrected),
`opus5_next150_compiler_laws_round2_20260916.md` (round 2, bannered where the campaign overturned it), and
`opus5_next150_research_campaign_20260916.md` (the adjudicated campaign). All detectors were validated for zero false
positives against 7,283 strict-exact controls.

- **LAW Z:** the whole-function constant-zero register pin is source-determined. EBX is the only byte-addressable
  callee-saved register, so it is a two-way contest; what competes is a web's span, not its ordinal position.
  "January pins, we do not" is a source defect, not a register tie. Its round-1 store-count boundary was refuted and
  retracted. **Z4:** VC7 coalesces pointer copies before ranking webs, so respelling a phi is structurally inert.
- **LAW I:** the inliner prices a helper by how it is spelled, against the call block's frequency.
  `REAL_MATH_EXTERNAL_<X>` makes a helper extern for the whole TU. **The 82 KB "inline boundary" family was
  misattributed**: 93.6% of it makes the same inline decision at every site.
- **LAW M8:** the offset-0 term of an inlined helper loads first the operand with the smaller IL-local ordinal mod 8.
  **Source operand order is inert** for a bare product, so the R3 swap never worked. This produced both Tier A closures.
- **LAW F3 / D / U:** frame layout by tier (byte cells, scalars, aggregates). This **refutes the long-standing
  "refs/size" frame rule**. A dead whole object costs no frame; a partly homed aggregate costs its declared extent.
- **Three separate count keys** replace the old "declaration-count oracle": M8 (in-function IL locals mod 8), H3
  (pre-function declarations mod 32) and F1 (inline-body IL structure consuming per-TU numbering).
- **CJ4** (cross-jump survivor binding), **LAW C** (small struct copies), **LAW E** (emission order), the loop-head
  alignment law, and round 2's LAW S (slot assignment, polarity since corrected) and LAW X (cross-jump tail merge).

## 6. Verification (final, at `26601453e`)

- `ninja`: pass; the parked-function check validates 268 ties.
- `stable_verdicts` snapshot `scratch/opus5-next150-FINAL.json` diffed against `scratch/opus5-next150-before.json`:
  **gained 15 / 9,504 padded / regressions 0**.
- `tools/campaign/board.py`: 309/619 objects, 1,397,165/1,922,413 bytes, 7,279/8,245 functions.
- `tools.parked_functions`: **268 active / 0 stale / 0 invalid**.
- `tools.audit_object_admission`: **0 contradicted**, 7 rejected, 8 candidates. The candidate count rose from 7 because
  `structure_detail_objects` became whole-object eligible; it is **not admitted** and carries zero credit.
- `pytest --basetemp scratch/pytest-next150-FINAL`: **1,151 passed, 5 skipped, 26 subtests**, identical to the baseline.
- `git diff --check ae10935da HEAD`: pass. `git status --short`: no tracked dirt.
- Per-file audit of all 16 changed `.c` files (`scratch/final_audit/audit.py`):
  - `_point_from_line3d` guard: passes on 15. ai_debug is a **pre-existing** baseline failure; its `ae10935da` copy
    fails identically.
  - fake_match_scan: 14 files are 0/0 against the baseline. The two increases are **exactly the two owner-admitted asm
    blocks** (bink_playback 1/0; hud_draw 3/2, where 2 are the pre-existing naked `get_return_eip`).
  - Owner census against the baseline: `$L` entries are compiler-internal labels being renumbered, not owners. The
    real changes are all intended: the authenticated rename; the `end bink_alloc` literal correctly disappearing
    (January does not own it); the RGB helper's own assert literals; and `_set_real_vector3d` / `_set_real_point3d`,
    which are in the admitted select-any class (laws_w2 A4, laws_w3 A30).

## 7. Measured ceiling

At startup, before any wave, the whole-board portfolio (`scratch/opus5-next150/portfolio.json`) held 457 non-exact
source functions / 450,776 meaningful bytes, of which **135,795 were eligible**. Closing every eligible function at
100% would still miss 150,000, and 62,283 of that is sha-only.

The research campaign then adjudicated what remains:

| pool | bytes | disposition |
| --- | ---: | --- |
| Proven unreachable by admissible source | 4,581 | retired: CJ4, H3, LAW U reads, the carrier-only parameter home, P7-bound, the ESP anchor |
| csplit alias rows | 5,856 | retired from strict credit (bytes already identical) |
| Retire-pending (measured exhausted) | 1,884 | excluded |
| Strict-exact candidates blocked on UB or steering | ~6,063 | owner-declined class (indeterminate reads, a stack overrun, a steering subscript, `_point_from_line3d`) |
| Tier B not closed | 7,121 | all 7 stopped correctly at allocation or x87 ties |
| Tier C hypotheses | ~52,000 | not schedulable without new evidence |

**No sufficiently large admissible portfolio remains.** Per the owner's directive, no further research or residual wave
is to be launched from this lane.

## 8. Unresolved dependencies

1. **Tier B, not closed:** `_path_state_traverse` (structural gap closed in scratch; an x87 term-order tie and an
   edge-cursor IV bias remain), `_flag_update` and `_bitmap_2d_sharpen` (register allocation; lens refuted at HEAD),
   `_light_volume_render` (x87 residency tie), `_actor_stimulus_prop_acknowledged` and `_stack_memory_pool_allocate`
   (register allocation after the structural facts matched), `_particle_system_new_particle_jet` (x87 operand order).
   The per-object `*_next150_tierB_*` ledgers record each census.
2. **UB-blocked strict-exact candidates**, which need owner rulings the owner has so far declined:
   `_get_particle_world_position` (two indeterminate array reads), `_motion_sensor_update` (an uninitialised read), and
   the s3tc trio (an indeterminate member plus a 60-byte stack overrun). The s3tc assert text `u>=0 && u<=4` can be
   restored independently on `docs/assertions.md` grounds, but that alone gains nothing.
3. **`_hs_compile_and_evaluate`:** proven unreachable by legitimate source. Its parameter-home phi with cross-jumping
   sprintf arms needs four web references, which need six in-region zero stores, and only five exist.
4. **`_king_calculate_hill_state`:** retire-pending. VC7 sinks both arm stores into the zero pin, and every measured
   non-sinking route is prohibited.

## 9. Integration order

The history is linear. Apply `git log --reverse ae10935da..HEAD`:

1. `3083f800b` — four park reopens and the planar-fog remeasure
2. `034141b12` — wind, under the owner ruling
3. `6ebd05a96` — actor_perception and ai_debug, **including the symbols.json rename**; the config line and the three C
   identifiers must land together
4. `927d135ec` — sound_manager zero-credit re-baselines
5. `3faefe7f6` — interim handoff, manifest and LAW Z ledger (documentation)
6. `9470345c0` — bink allocator, under the int 3 ruling
7. `6e26c8ed9` — round-2 laws and round-1 corrections (documentation)
8. `417fc10e4` — Tier A closures
9. `1068df6d6` — research campaign ledger (documentation)
10. `6d6529d98` — the two owner-ruled closures
11. `26601453e` — Tier B closures
12. this documentation commit

`config/parked.json` is edited in place in most code commits, so cherry-picking out of order conflicts on it and fails
the ninja park check. Integrate as one ordered range.

Changed source files: `ai/actor_moving.c`, `ai/actor_perception.c`, `ai/ai_debug.c`, `bink/bink_playback.c`,
`effects/player_effects.c`, `interface/hud.c`, `interface/hud_draw.c`, `interface/hud_sounds.c`, `items/weapons.c`,
`rasterizer/rasterizer_transparent_geometry.c`, `render/render_debug.c`, `scenario/wind.c`, `sound/sound_manager.c`,
`structures/structure_detail_objects.c`, `structures/structures.c`, `text/draw_string.c`; plus `config/parked.json`,
`config/symbols.json` and documentation under `docs/object_matching_logs/`.

## 10. Overlap

Codex's verified packet `faae809ab` owns `transport_endpoint_set_winsock.c`, `transport_endpoint_winsock.c` and
`network_connection.c`. Those three, plus `matrix_math.c`, `units.c` and `vehicles.c`, are untouched, and none of the
six corresponding park entries was edited or re-baselined. The Codex action-family reservation (`action_alert`,
`action_converse`, `action_flee`, `action_guard`, `action_obey`, `action_search`, `action_uncover`, `actions.c/h`,
`ai_runtime.h`, `units.h`) was honoured: none was dispatched or modified. `git diff --name-only ae10935da HEAD` over
all 17 files is empty. The two protected transport functions appear in the law ledgers only as read-only evidence.
