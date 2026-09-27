# RF-A: `_actor_path_refresh` (source/ai/actor_moving.c) - working notes (banked as I go)

Lane `claude/remaining-frontier-20260926`, base b62f74c1 (canonical 8cda1f91). Worker RF-A, solo.
Started 2026-09-26T20:55Z (clock-read). Scratch: scratch/rf/workers/RF-A/.

## 1. Records gathered (do not repeat)
- Lane A (claude-lane-a-ai-core-20260920): scratch/orch/pathrefresh/NOTES.md, scratch/res4/path-refresh/NOTES.md;
  docs/object_matching_logs/claude_lane_a_ai_core_rejected_hypotheses_20260920.md:430-446, 520-560.
  Spellings measured: v1 (nest float conjuncts in if(success)), v2 (+local endpoint radius, WORSE 1456),
  v3 (hoist `if(!success) clear` out of else, WORSE 1424), c1 (braces), c3 (no braces), d5 (swap refreshed/new_destination),
  s1/s2 (parentheses), s7 (nest 2nd if(success) inside 1st), s9 (4 nested ifs), s12 (named condition value
  `at_destination`), s14 (`success == FALSE`), s15 (empty then-arm, FAKE), C_blank, F_scope, H_oneline,
  A_assert (success=FALSE after assert, WORSE), X_else (fold !success into else, FORBIDDEN/WORSE 1472).
  Diagnostics d1-d4 (semantics-changing): deleting `if(!success) clear` or adding a trailing statement flips binding.
- opus5 fresh-graphs (opus5-30k-fresh-graphs-20260914/scratch/workers/actor_moving.md): v1 HCEA order, v2 current,
  v3 else-chained failure clear (1456), v4 nested if (= v2), v5 early returns (1456/54), v6 distance at top + early
  return TRUE (1472/54). 150k_w1: pr1 nested-success shape worse.
- opus5 next150 (F5): run1 p1-p5 and declaration count 1/3/8/16 inert; laws_w3 A43 "no source lever moves survivor".
- W3 compiler-application: section 0 negative; od_path_refresh_46c720.asm (/Od readout, reused here).

## 2. Bytes (floor = lane source, gate residual 1440 [sha])
Instruction text identical; January 1428 real bytes + 12 NOP, ours 1440. The three `&&` failure edges of the late
`if (success && endpoint.target_radius > 0.f && distance < destination.target_radius && distance - endpoint.target_radius < 0.5f)`
guard (Jan 0x50e jne / 0x51e jp / 0x534 jp) bind:
  January -> 0x542 (epilogue copy that follows the LATE clear inside the guard; short jcc, 2 B)
  ours    -> 0x2dd (epilogue copy that follows the `if (!success) actor_path_clear()` block; near jcc, 6 B)
/FAsc of the floor (scratch/rf/workers/RF-A/floorL.cod): the canonical exit is `$L14919` at 0x2dd, labelled, directly
after `$L14922` (= line 2082 `if (!success) actor_path_clear`), and the 0x54e copy after the late clear is UNLABELLED.
Other exits: 0x557 `$L13384` first-arm inlined actor_path_clear with scheduled epilogue; `$L14916` 0x586 `mov al,1`
(value-constant return: success known TRUE when no refresh is needed).
So in January the canonical exit stays after the late clear (the end of the function in source order) and the
`!success` clear block (moved up to follow the default case at 0x2d1 in BOTH builds) carries a clone; in ours the exit
travelled with the `!success` clear block.

## 3. /Od readout (od_path_refresh_46c720.asm, first-party /Od build, reused from W3) - shape differences vs ours
- A: 0x46cbb0 `if (!path_available) { success = FALSE; } else {...}` (jmp over the refresh after success=FALSE; no
  second `if (success)` re-test).
- B: 0x46cbc1 `flag = TRUE;` stored BEFORE `actor_test_destination`; on TRUE, flag = have_previous_destination &&
  distance_squared3d(...) > 0x3c23d70b (folded 0.1f*0.1f); then `if (flag)`.
- C: [ebp-0x3c] real = 0.f at the top of the success block, passed as path_3d_build_path's radius (constant; inert
  in optimised code; name unknown) - not used.
- /Od also initialises avoidance_distance = 0.f: REFUTED for January (no store before the escaping call).
- Assert lines: /Od 0xc28/0xc65 vs January 0xb7f/0xbbc: both +169, span 61 lines in both builds.

## 4. RESULT (2026-09-26T21:12:51Z)
- P01 (A alone): INERT (sha16 89b79760 = floor).
- P02 (B alone): EXACT, sha16 b6b01d01147885d0 = January, unit 33/3 (no loss).
- P03 (A+B): EXACT, same bytes as P02.
Both predictions for B were wrong (I predicted inert). Cards in cards/P01-P03.txt.
- N1 (control, dead local at P02's position, not landable): INERT (floor sha) -> the flip is structural, not a
  name/declaration-count artefact. Other residual rows byte-unchanged in N1 and P02.

## 5. Mechanism readout (/FAsc, evidence/*_FAsc_actor_path_refresh.cod) - facts vs inference
FACT floor: `$L14922` (!success clear, 0x2d1) is followed by the LABELLED exit `$L14919` (0x2dd); the copy after the
  late clear (0x54e) is unlabelled; the value-TRUE exit `$L14916` (mov al,1) carries a LOWER label number than `$L14919`.
FACT P02: the !success clear `$L14926` (0x2d1) is followed by an UNLABELLED clone; the exit after the late clear is
  the labelled `$L14922` (0x542, float jcc `jne SHORT $L14922`); the value-TRUE exit `$L14923` now numbers AFTER it.
  The canonical (labelled) exit sits in its source-last position, the CJ4 norm (F5: last copy in 514/563 groups).
INFERENCE (not traced in C2): the flag form changes the order in which the jump-threading/value-return split creates
  the two value classes of `return success` (constant TRUE vs variable); with the flag the variable-value exit is
  formed first and stays where the source puts it, so the !success clear block moves behind `default:` alone and
  receives a clone (LAW B: returns merge on value; the canonical block owns the $L; conditional branches bind to it).
  A C2-level trace (which pass, which list) was NOT done; the causal input is established only at source level
  (P02 exact vs N1 inert vs floor).

## 6. Landing checks (P02 primary; P03 alternative, same bytes)
- gate --all: 33 exact / 3 residual (was 32/4); residual rows move_update, vector_avoidance, test_avoidance_vector
  byte-unchanged (normalized sha equal to floor).
- keyed_diff vs build/base/source/ai/actor_moving.obj: 1 changed (_actor_path_refresh -> EXACT vs January),
  0 added, 0 removed, 0 losses (keyed_diff.txt).
- raw sections (secraw.txt): .debug$S differs only by the compiled file path (also in the floor control);
  relocations of _actor_move_try_evasion_direction / _actor_move_calculate_movement differ only in local $L jump-table
  label NUMBERS (same section, same offsets/values; TU-global label counter shifted by the new labels); symbol
  count 415 = 415, no new symbol names other than renumbered $L labels, no COMDAT/data change.
- /W3 /Zs: 12 warnings before and after, all in cseries.h / real_math.h, identical modulo line numbers; none in
  actor_moving.c.
- fake_match_scan: 0 leads (floor, P02, P03).
- Strip test: P02 has ONE change (B). Strip B -> floor residual (+12 B, three near jcc): B buys the whole 12-byte
  residual. P03 = A+B: strip A -> P02 EXACT (A buys 0 B, byte-inert); strip B -> P01 residual (B buys 12 B).
- Name `build_path` is descriptive/inferred (no scalar names in /Od; precedent `boolean build_attack_vectors`
  actor_firing_position.c:1971). Name-count control N1: inert.
- Card-driven compiles: 4 (P01, P02, P03, N1) of 30.
Patch sha256: P02 1524e285..., P03 e3f9d94f... (against lane HEAD b62f74c1, actor_moving.c sha256 14039b8c...).
