# REVIEW - hud_nav_points P4 (shared stack frame), wave 4

Reviewer slug: scratch/w/review4_hud_nav_points/. Tree 16542e46, clean before and after.
Worker slug: scratch/w/hud_stack_check/ (patch P4_hud_nav_points_c_shared_stack_frame.patch, depends on P1 and P2).

## Verdict

APPROVE P4, but only inside the atomic packet: P1 (hud_draw.h) + P2 (hud_draw.c) + P3 or 01RS (hud_unit.c) + P4 + P5 + P6 + P7.

- P4 cannot land on its own. Without P1 it does not compile (the macro and STACK_BUFFER_LENGTH live in P1).
- Without P2, the pair link fails: LNK2005 against the production NODUP `_check_stack_buffer`, reproduced here in both orders.
- If the owner picks the P-A focused header instead of P1, P4 has to be regenerated.

P4 is zero-credit hygiene, not an admission closure. See "Scope" below.

## Independent checks (all reproduced, nothing taken from the worker)

| Check | Result |
|---|---|
| git apply --check on 16542e46 | P4 alone clean; P1, P2 clean; P1+P2+01RS+P4+P5+P6+P7 clean |
| Candidate reproduction | `git show HEAD` blobs + `patch -p1` P1/P2/P4 give the same text as the worker's cand/ files (x3) |
| Harness control | production TU with the include redirected to a verbatim header copy: 0 non-debug differences |
| gate --all | 31 EXACT / 1 residual (_custom_render_nav_point), identical rows to production |
| Strict object compare (own fullcmp: raw bytes, relocs, flags, COMDAT selection, all symbols) | only `+ _check_stack_buffer` (.text 48, SELECT_ANY, external) |
| Park drift | _custom_render_nav_point normalized sha ad77c226... = parked.json base (1632/95); January diff still only +0x58a..+0x591 |
| New COMDAT identity | section_infos_equal to build/split hud_draw `_check_stack_buffer` (48 B, 0 relocs, c7049c31...) |
| object_audit | prod vs cand audit text differs only by `+.text sym:_check_stack_buffer size 48`; 46/46 January symbols ok; FAIL(1) = the park |
| surplus_identity | 5 candidate-only code COMDATs, all IDENTICAL to January's selected copies |
| provider_link | vs production hud_draw: FAIL (expected control); vs HEAD+P1+P2 hud_draw.obj: PASS (21/21); 5-object packet link in 3 orders: 0 LNK2005/1169 |
| pdb_storage | 0 disagreements; `_check_stack_buffer` is PUBLIC in cachebeta (external is correct) |
| /W3 /Zs | warning multisets identical (pre-existing C4013 'error', C4146, 11x C4244) |
| fake_match_scan | 0 leads (candidate TU and P1 header) |
| Data | every January-owned data/rdata/bss section `ok` in the audit; all data sections raw-identical to production |

## Law and house-rule checks

- **Law (i), a new header COMDAT from a non-exact caller.** The TU has two callers:
  - `_hud_update_nav_point_local_player` is STRICT EXACT.
  - `_custom_render_nav_point` is parked. Its inlined sentinel tail (+0x5bb..end) is byte-identical to January.
  - Lab `labs/lab_exact_caller_only.c` removes the parked caller's use. The COMDAT is still emitted, still identical to January's, and the exact caller stays EXACT.
  - So the emission is owned by a strictly exact caller. This is not the damage-A / cross_product2d class, where the only caller was non-exact.
- **STRIP TEST.** Lab `labs/st1_top_decl.c` removes the macro block and uses a plain expansion with `short corrupt_index;` at function top.
  - The result is EXACT and the whole object is identical.
  - So the macro's block scope is byte-inert and buys nothing.
  - The macro is chosen on the January evidence that each sentinel's two asserts share one line (F2 census); the bytes don't require it.
- **/Od declaration order.** The /Od build contains no sentinel: `odbuild str` finds neither "corrupt stack at" nor "corrupt return address", and the sanity check `file hud_nav_points.c` finds 5 functions. HCEX has no check_stack_buffer. No /Od frame exists to contradict, so law (d) does not apply.
- **Hand-expanded helper.** P4 removes two hand copies of check_stack_buffer, which used 4 invented labels/gotos (the held class).
- **No new blockers.** It adds no float, view cast, consumer-local prototype or invented aggregate.
- **Naming.** `match_assert_stack_frame(file, line)` follows the existing `match_assert_valid_*(file, line, ...)` header-macro convention. Bungie's name is lost; disclosed as descriptive.
- **Consumer list for P1.** 23 TUs, all spelled `#include "interface/hud_draw.h"`, recursive grep including spaced directories; no header includes it. The list equals the worker's.
- **Own shadow sweep.** All 23 consumers, HEAD + packet, include redirected to the slug P1 header.
  - Gate rows are identical everywhere.
  - Raw bytes and relocations are identical for every section once `$Lnnnn` label numbers are normalised. Those numbers shift with the header's added names; this is invisible to section_infos_equal and objdiff, because January's split names them fn+offset.
  - The only differences are the expected `_check_stack_buffer` NODUP->ANY in hud_draw and +ANY in hud_nav_points, hud_unit, hud_weapon and motion_sensor. models with P7: 0 differences.
- **symbols.json.** No change needed.

## Scope and caveats for the integrator

1. **hud_nav_points is NOT admission-ready apart from the park.** The lane log line "only _custom_render_nav_point remains" misses these pre-existing law (g) blockers, which P4 does not touch:
   - **Consumer-local prototypes of other files' functions** that have owner headers: `object_get_bounding_sphere` (objects.h), `game_engine_render_nav_points` (game_engine.h), `unit_get_head_position` (units.h). Also `object_try_and_get_and_verify_type`, `game_engine_get_goal_position` and `extern hud_globals`.
   - **TU-local struct views with opaque spans:** `struct hud_globals_definition { ...; byte unknown1BC[0x294]; }`, `struct hud_nav_object_datum { byte unused[0xB6]; word damage_flags; }` and a local `struct scenario_cutscene_flag`.
   - **`float` for real:** `hud_nav_point_datum.z_offset` and the `float vertical_offset` parameters.
2. **P4 is contingent on P1/P2 being admitted**, which the owner/other reviewers decide: hud_draw.h as the home, the P7 dead-include removal, or the P-A fallback. Once P1 lands, P4 is required: otherwise hud_nav_points keeps a hand copy of a header helper, the law (g) class.
3. **Advisory for P1, not a P4 issue.** The Sept-2001 atlas flags `_get_return_eip i hud_draw.obj`, so in 2001 it was a header inline. P1 keeps it a plain naked hud_draw.c function. Either way is byte-inert and earns no credit. Also: check_stack_buffer is absent from the Sept-2001 publics but PUBLIC in January (a storage change between builds, not a contradiction).
4. **After integration:** full build + stable sweep of all 23 hud_draw.h consumers and the parks, re-measured on the live tree together with any other wave-4 header packets.
