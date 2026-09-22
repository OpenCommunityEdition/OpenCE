# Adversarial review: source/ai/actor_type_infection (Lane B)

Candidate: `scratch/lane_b/w/actor_type_infection/candidate.c` over `source/ai/actor_type_infection.c`.
Reviewer re-measured everything from scratch; no worker number was trusted. All reviewer artefacts are in this directory
(`full.diff`, `aln_cand.txt`, `strip.py`, `strip_out.txt`, `s_*.c/.obj`, `cand.obj`, `real.obj`, `data_*.txt`).

## 1. Re-gate (reviewer's own runs)

unitcheck:

    BASELINE   == exact 4  residual 1  unwritten 0  (of 5 listed)
    CANDIDATE  == exact 4  residual 1  unwritten 0  (of 5 listed)
    ~ _infection_swarm_control  residual 3616 [size 3360!=3616, relocs 103!=104, sha] -> residual 3616 [sha]
    SUMMARY gains=0 regressions=0 other=1

gate --all on the candidate: 4 siblings EXACT (304/688/96/96), `_infection_swarm_control` residual `[sha]`.
**Credit: 0 functions, 0 bytes.** The function is NOT exact; nothing may be counted.

- tinfo (January): size=3616 relocs=104 sha=64eacc5c1e2a60ca. Candidate object: size=3616 relocs=104 sha=1d4d8baf0b87f9c6.
- relocdiff --count-by-target: 104 target / 104 candidate / **0 differing rows** (multiset identical).
- alndiff: 1127 vs 1127 instructions. After removing reloc-spelling-only rows exactly **8 regions** remain, all x87:
  0x874/0x87c (inlined normalize3d: January `fld m; fmul st(1)` + shared pop; ours `fmul m` + `jmp +2`), and
  0x8e6, 0x8ee, 0x9c0, 0x9ce, 0xb59, 0xb64 (`fld X; fmul Y` with X/Y exchanged - commutative products of `up.i`).
  Every differing row is a commutative-operand or pop-placement choice: the candidate computes the SAME values as
  January on ALL paths (no branch, store, call, constant or reference differs). Worker's ledger confirmed.

## 2. Guard / 3. Ownership / 4. fake scan

- Guard `--forbid-emitted-symbol _point_from_line3d`: REAL source PASS, candidate PASS (no new emission).
- newsyms: `+__real@bd888889` ok, `+__real@bfc90fdb` ok (January defines both), `-__real@3f6eeeef` (January lacks it too). OWNERSHIP PASS. No new code symbol.
- Non-code sections real-object vs candidate-object: only those three constant COMDATs differ (plus the .debug$S temp object name). No string/data churn.
- fake_match_scan: 0 leads on the candidate, 0 on the real source. No new finding.
- CL /Zs /W3: 12 warning lines for both real source and candidate (identical header set), none in the .c, no C4013.
- Not parked (`config/parked.json` has no entry; only `config/symbols.json` names the function). No park refresh needed.
- Line endings: real 980 CRLF / 0 LF-only; candidate 1032 CRLF / 0 LF-only. Preserved.

## 5. Diff audit (276 insertions / 224 deletions)

Hunks outside the target function: exactly one - `#include "ai_scenario_definitions.h"` (file scope, line 51).
It is REQUIRED for the `_ai_atom_move_facing_*` enum names (the enum is defined only there) and is measured
byte-neutral (variant `s_include_numeric`: include removed + numeric case labels -> identical function sha, 4 siblings
still EXACT). It is a house-rule improvement (enum constants in the switch), not steering. All other hunks lie inside
`infection_swarm_control` (real-source lines 287-851). No baseline-exact function is touched.

I checked every structural choice in the rewrite against the first-party HCEA unoptimised debug build
(`ref_hcea_ghidra.c`, `hcea_ghidra.asm`) AND against January's own bytes (`jan_disasm.txt`). Findings:

| candidate construct | looks like steering? | evidence | verdict |
|---|---|---|---|
| `if (timer > 0) timer--; else switch(action){case search: case charge:}` | no | HCEA asm 0x481edc `JLE else; dec; JMP; else: switch-temp cmp 7 / cmp 10`; January 0x59 `jle` + `cmp eax,7 / cmp eax,0xa` | genuine |
| `(short)MAX(cooldown, 6.f)` | cast | HCEA `tmp = cooldown; if (cooldown <= 6) tmp = 6; (short)(int)tmp` = MAX ternary. Cast byte-neutral, suppresses C4244 | genuine |
| removed `if (unit_count < 1) return;` | no | HCEA has a single exit from the for-loop test | genuine (single-exit) |
| `unit = unit_get()` + block-local `biped = (struct biped_datum *)unit` x2 | temporaries | HCEA `local_28 = object_get_and_verify_type(idx,3)`; copies `local_78 = local_28`, `local_174 = local_28`. Byte-neutral | genuine |
| `short animation_state / aiming_speed` + `(char)` at the store | type + cast | HCEX PDB: `Local, Type: short, aiming_speed`; HCEA `local_4c`/`local_50` are 16-bit. Casts byte-neutral, suppress C4244 | genuine |
| second pointer `target_prop` (= best_prop / NULL) | extra variable | HCEA `local_58 = local_84` at block end and `local_58 = 0` in the else arm | genuine |
| `real score; ... score = 0.f;` AFTER the distance | reorder - LOAD-BEARING (decl-init -> size 3600) | HCEA stores `score = 0.0` after the distance3d() call returns (`local_ac = ...; local_b0 = 0.0`). With distance3d() forbidden by ownership the C89 spelling of that order is exactly this statement | genuine |
| best_* store order | reorder | byte-NEUTRAL vs HCEA order (score, prop, index, distance) and vs baseline order | harmless (see note N1) |
| redundant `animation_state = in_combat` in the search case (end) and charge/obey case (start) | redundant assignment - LOAD-BEARING (strip -> size 3584) | HCEA case 6 ends with `local_4c = 3`, case 9/10 begins with `local_4c = 3` | genuine Bungie redundancy |
| duplicated `rotate_vector_about_axis` in both wander arms | duplication | HCEA has the call in each arm; January has 3 reloc sites for 4 source calls (one tail-merged), multiset matches | genuine |
| `real angle_damping = angle * -0.06666667f` before the random call | temporary - LOAD-BEARING (inline -> size 3600) | January 0x566-0x58a: `fld angle; fmul -1/15; fstp [ebp-0x30]` BEFORE `call _real_random_range`, then `fadd angle; fadd [ebp-0x30]`. Evaluated before the call but added last = a named local. HCEA has the same pre-call local (`fStack_c0`) | genuine |
| `> 0`, `< 255` on `byte` ticks | compare spelling | January 0x5ae `test al,al; jbe` (unsigned >0); HCEA 0x482817 `cmp eax,0xff; jge` | genuine |
| airborne arm first, dead-parent arm first, `has_direction` block hoisted to loop level | reorder | HCEA 0x4827a4 `jz else` with the airborne arm physically first; HCEA `if (local_59)` sits at loop level after the parent block | genuine |
| obey facing `switch` with `has_direction = TRUE` first | restructure | HCEA switch temp `iStack_260`, cases 2,3 vs default, `local_59 = 1` before it | genuine |
| separation clamp as if / else-if / else | restructure | January references `__real@3fc90fdb` AND `__real@bfc90fdb` (PACKET multiset); HCEA stores `local_15c` directly in three arms (no ?: temp) | genuine |
| four `should_jump = TRUE;` statements | restructure | HCEA four byte stores, flags re-read each test; January four `mov al,1` | genuine |
| melee state if/else with two stores | restructure | HCEA 0x4834a7.. two direct byte stores | genuine |
| `unsigned long control_flags = 0; SET_FLAG(...)` BEFORE csmemset; `(word)` at the store | temporary + cast | January 0xcba-0xcc5 `neg al; sbb eax,eax; and eax,2; mov esi,eax` before `call _csmemset`, then `mov word [ebp-0xda], si`. HCEA dword `local_1c0`. Cast byte-neutral, suppresses C4244 | genuine |
| `control.primary_trigger = 0.f;` after csmemset | inert store - LOAD-BEARING (strip -> size 3600) | January 0xd00 `mov dword ptr [ebp-0xc4], 0` is IN January's bytes; HCEA `local_1a0 = 0` | genuine Bungie redundancy |
| `set_real_vector3d(&control.throttle, 1.f, 0.f, 0.f)` | helper call | January 0xd21-0xd35 three immediate stores 1,0,0 (baseline stored only `.i`); HCEA calls the helper; `_set_real_vector3d` already emitted by the baseline object | genuine, and removes a manual expansion |
| `control.facing_vector = ...; aiming = facing_vector; looking = facing_vector` | removes `facing` temp | HCEA copies facing_vector into the other two | genuine |

No volatile/register/#pragma/barrier/flag, no raw struct offsets, no punning casts, no UB (the only conditionally
initialised object, `direction`, is read only under `has_direction`; HCEA's own `__RTC_UninitUse("movement_vector")`
probe shows the original had the same shape; `target_prop` is assigned on both arms), no dummy locals, no impossible
conditions, no header/config edit.

## 6. Strip tests (reviewer-run, `strip_out.txt`; candidate fn sha 1d4d8baf0b87f9c6)

| variant | result | meaning |
|---|---|---|
| drop `(word)` | identical sha | not load-bearing; keep (silences C4244 at /W3) |
| drop `(char)` x2 | identical sha | not load-bearing; keep (C4244 x2) |
| drop `(short)` on MAX | identical sha | not load-bearing; keep (C4244) |
| include removed + numeric cases | identical sha | include is byte-neutral, needed for enum names |
| best_* in HCEA order / baseline order | identical sha | order inert |
| `dot_product3d(&direction,&up)` (HCEA order) | identical sha | inert |
| `real score = 0.f;` at declaration | size 3600 | load-bearing; first-party order evidence (above) |
| remove `primary_trigger = 0.f` | size 3600 | load-bearing; the store is in January's bytes |
| remove the two redundant animation_state stores | size 3584 | load-bearing; HCEA-attested |
| inline `angle_damping` | size 3600 | load-bearing; January evaluates it before the call and adds it last |

Every load-bearing item is attested by January's own instructions and/or the HCEA debug build. None relies on the
narrow owner admission; none is a decoration that only moves bytes. No decorated-only match exists (the function is not a match at all).

## 7/8. Credibility of the residual-improved body

Baseline body is structurally wrong (256 B short, frame 0xe4 vs 0xe8, relocs 103 vs 104, calls `_normalize3d`
out of line, lacks the `primary_trigger` store and two of the three throttle stores, lacks the +-pi/2 constants).
Candidate: same size, same frame, identical relocation multiset, identical instruction count, value-identical to
January on all paths, and statement-for-statement consistent with the first-party debug build. This is a more correct and
more credible reconstruction, not byte-chasing. Verdict: ACCEPT_FUZZY_ZERO_CREDIT.

## Notes for the orchestrator

- N1 (optional, byte-neutral): HCEA's statement order is `best_score; best_prop; best_prop_index; best_distance` and
  `dot_product3d(&direction, &up)`. Both are measured inert; landing either spelling is fine.
- N2: R13 (do not land a non-exact rewrite of a written residual) is an orchestrator policy call. On the merits the
  rewrite is strictly better and regression-free; it earns ZERO credit.
- N3 (OWNER RULING, not part of this landing): `exact_needs_ruling.c` = candidate.c with the three hand-expanded
  negations replaced by `negate_vector3d(&direction,&direction)` and the hand-expanded probe replaced by
  `point_from_line3d(&swarm_component->position,&direction,-0.2f,&probe)` (4 insertions / 12 deletions, nothing else).
  Reviewer re-gated it: `EXACT 3616 _infection_swarm_control`, `== exact 5 residual 0`. It FAILS the guard
  (`FORBIDDEN-EMITTED-SYMBOL _point_from_line3d`) and ownership (`+_negate_vector3d`, `+_point_from_line3d` VIOLATION).
  The hand expansions that candidate.c keeps (inherited from the baseline) exist ONLY because of that guard; HCEA
  calls the helpers. This is the strongest showing so far that January's raw object carried those COMDATs. Must not
  land without an explicit owner ruling.
- After landing: full ninja + per-function row diff (aggregate counts hide swaps). Function is not parked; no park refresh.

## Verdict

- `_infection_swarm_control`: ACCEPT_FUZZY_ZERO_CREDIT (exact_confirmed = false).
- 4 baseline-exact siblings: NO_CHANGE (still EXACT, untouched).
- Overall: LAND (zero credit), subject to the orchestrator's R13 policy decision.
