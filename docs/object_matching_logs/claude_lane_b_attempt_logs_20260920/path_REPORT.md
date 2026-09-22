# path.c Lane B worker REPORT

Resumed 2026-09-21: candidate.c existed and was byte-identical to source/ai/path.c (prior run interrupted after the baseline dump).

## Baseline (python tools/campaign/gate.py source/ai/path --all)

```
EXACT       384  _build_path_edges_for_surface
EXACT       112  _closest_available_point_on_surface
residual    208  _closest_point_to_attractor  [sha]
EXACT       240  _path_3d_available
EXACT       144  _path_3d_build_path
EXACT       192  _path_attractor_weight
EXACT       128  _path_get_node
EXACT       560  _path_heap_bubble_down
EXACT       480  _path_heap_bubble_up
EXACT       128  _path_heap_insert
EXACT       288  _path_heap_pop_cheapest_node
EXACT        16  _path_heap_verify
EXACT        48  _path_input_new
EXACT        64  _path_input_set_attractor
EXACT        32  _path_input_set_search_bounds
EXACT        48  _path_input_set_start
EXACT        16  _path_input_set_target_object
EXACT        80  _path_node_from_hash_table
EXACT       304  _path_state_approach_point
EXACT       592  _path_state_begin
EXACT      1184  _path_state_build_path
EXACT        48  _path_state_destination
residual    496  _path_state_estimated_distance  [size 528!=496, sha]
EXACT       256  _path_state_find
EXACT        64  _path_state_new
EXACT        64  _path_state_reset
residual   2096  _path_state_traverse  [sha]
EXACT        16  _paths_dispose
EXACT        16  _paths_dispose_from_old_map
EXACT        16  _paths_initialize
EXACT        16  _paths_initialize_for_new_map
EXACT        96  _surface_is_broken
== exact 29  residual 3  unwritten 0  (of 32 listed)
```

## Attempt log

### A1 `_path_state_estimated_distance` - re-gate prior-lane scratch body e4 (opus5-150k w3c)
Evidence: docs/object_matching_logs/path_obj_opus5_250k_w3c_20260915.md s2 (four byte-proven source facts: distance3d argument roles,
named MIN operand, rotated while, `result = TRUE` before the direction `if`). Spliced as try_1.c.
- before: `residual    496  _path_state_estimated_distance  [size 528!=496, sha]`
- after : `residual    496  _path_state_estimated_distance  [sha]` (191 vs 191 insns; all offsets aligned except a 3-cycle register rotation,
  the 0xf9/0xfc reload order, and `result` homing: January `mov al,1; mov [ebp+0x17],al ... mov al,[ebp+0x17]`, ours rematerialises `mov al,1` twice
  and puts path_distance in the dead parameter home [ebp+0x14] where January uses [ebp-4]).
Verdict: strictly closer, not exact. Kept as the working body for further probes.

### A2..A14 `_path_state_estimated_distance` - DIAGNOSTIC probes of what homes `result` in memory (none is a candidate)
- try_2 `result = TRUE` at top of the found block: result becomes a memory byte ([ebp-1], frame 0x18) AND the register rotation becomes January's
  (state=EDI, node=ESI, node_index=EBX). So the rotation is a CONSEQUENCE of result being memory-homed, not a separate tie.
- try_3/try_4 `result = TRUE` before `*distance_reference = distance` / before `if (closest_approach_to_attractor_reference)`: result stays a register web (AL) + remat.
- try_5 non-constant def: unrelated shape (bl), discarded.
- try_6..try_14 bisect: a constant-def web gets a caller-saved register + rematerialisation after calls whenever eax/ecx/edx is free from the def to the first call;
  it is memory-homed only when no register is free there (P1 position). In January's final bytes eax/edx ARE free at 0x106..0x122, so January's web
  is not a rematerialisable constant web in the allocator's eyes (or carries another def/use we do not have).
- try_15 direction block hoisted after the if/else under `if (result && estimated_direction_reference)`: still remat (`mov al,1` at 0x1e5). Rejected.

### A15 `_closest_point_to_attractor` - M8 dead-local sweep D=1..8 (DIAGNOSTIC)
All eight D give the identical 7-hunk diff: MOD8-INSENSITIVE (both operands are frame aggregates, outside M8).

### A16 `_closest_point_to_attractor` - `point_from_line3d(p0, &segment, t, result);` for the three hand-written component stores (try_cpa1.c)
Evidence: January's else-arm is exactly `result = p0 + segment*t` = the real_math.h helper; wave-7 commit cf097d7f records the same finding.
- gate: EXACT (`== exact 30  residual 2`), 87 vs 87 insns, but
  `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d ... section-size=48` - the guard FAILS.
Verdict: NOT ADMISSIBLE under the lane ownership rule (`_point_from_line3d` owned by action_charge.obj only). Not kept in candidate.c. NEEDS OWNER RULING.

### T1 `_path_state_traverse` - re-gate prior-lane scratch body S3 (opus5 next150 tierB)
Evidence: docs/object_matching_logs/path_obj_opus5_next150_tierB_20260915.md (frame 0x85c, three byte booleans, single address-taken attractor float,
loop-top exhausted test, in-place probe loop). Spliced as try_tr_s3.c.
- baseline: `residual   2096  _path_state_traverse  [sha]` (frame 0x864 vs January 0x85c, 2 rets vs 1)
- S3     : `residual   2096  _path_state_traverse  [size 2112!=2096, sha]`, 657 vs 652 insns, 61 alndiff hunks; frame/rets equal January.

### T2 `_path_state_traverse` - DIAGNOSTIC: `point_from_line3d` for the two hand-expanded `base + t*edge_vector` sites (try_tr_1/2/3)
Evidence: A16 (the same helper closes `_closest_point_to_attractor` outright); January's midpoint block is `fld v.i; fmul 0.5; fadd p.x` = the helper's `v->i*t + p->x` operand order.
- pinned site only (tr_1): 47 hunks. both sites (tr_2): 45 hunks. midpoint only (tr_3): 59 hunks. Helper is INLINED at both sites (no call).
- With both sites the edge-cursor bias becomes January's (+0x18, [eax-0x18]/[ecx-4]/[ecx]/[ecx+4]), the magnitude_squared3d block and the destination distance3d block
  become byte-equal; what is left is (a) the t-numerator dot order (January k,i,j; ours k,j,i) (b) the squares of `distance3d(&cheapest_node->entry_point,&entry_point)`
  (January k,j,i; ours i,k,j) and the loop-head pad those 4 bytes cause.
- guard: `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d ... section-size=48` -> FAILS. Not admissible under the lane rule; NEEDS OWNER RULING (same as A16).
- M8 dead-local sweep on tr_2 (instrument only): D=2 and D=3 make block (b) byte-equal (42 hunks; only block (a)+pad remain), D=8==D=0 (instrument clean);
  block (a) is D-insensitive.

### T3..T9 `_path_state_traverse` (all on the helper-based diagnostic body tr_2; guard FAILS for every one of them)
- tr_4 swapped dot arguments: byte-identical to tr_2 (operand order inert, LAW M8).
- tr_5 `/ edge_length_squared` instead of the second `magnitude_squared3d`: wrong - January keeps TWO cells ([ebp-8] first store, [ebp-0x38] named copy, `fdiv [ebp-8]`). Rejected.
- tr_6 PIN(...) passed straight as the helper's `t` argument: 660 insns, 85 hunks. Rejected.
- tr_7 nested `if` + `real diameter`: byte-identical to tr_2 (&&-vs-nested-if dead; +1 local does not move block (b)).
- tr_8 tr_7 + named `margin`: 655 insns, 75 hunks (`fdivr` hoisted). Rejected (matches the tierB do-not-repeat).
- **tr_9 named `real dot = dot_product3d(&vector, &edge->edge_vector); t = dot / magnitude_squared3d(&edge->edge_vector);`**:
  `residual   2096  _path_state_traverse  [sha]`, 652 vs 652 insns, size equal, 15 hunks. Block (a) is now January's k,i,j (`fxch st(2) ... faddp st(2)`);
  the ONLY remaining non-relocation rows are block (b), 4 rows: squares of `distance3d(&cheapest_node->entry_point, &entry_point)` (January k,j,i; ours i,k,j).

### T10 `_path_state_traverse` - tr_9 + nested `if` with `real diameter = pathfinding_radius * 2.0f;` (try_tr_10.c) -> EXACT, guard FAILS
Evidence: (1) M8 dead-local sweep on tr_9 (instrument only): D=1 and D=2 are strict EXACT, D=3..8 are not, D=8==D=0 -> January has 1 or 2 more IL-visible locals (mod 8);
(2) January computes `fld r; fadd st,st; fld st(0); fmul st(1)` AFTER the 16.0f compare has passed, i.e. the doubled radius is one value used twice, evaluated inside the first test.
- `EXACT      2096  _path_state_traverse` (`== exact 30` with only this function changed).
- Strip tests: tr_13 (= tr_9, `&&` with the doubled radius spelled twice, no local) NOT exact (block (b) 4 rows); tr_12 (`diameter` declared beside `edge_length_squared`, `&&` form)
  NOT exact (659 insns, 2r hoisted before the 16.0f compare); tr_7 proves nested-if vs `&&` alone is byte-inert, so the lever is the +1 IL local. WHICH local January had is under-determined
  (no January PDB locals); `diameter` is the one with byte-side support (single 2r value, computed after the first compare).
- named `real dot`: strip (tr_7/tr_2) NOT exact (block (a) k,j,i + 1 extra fxch + the loop-head pad). Precedent in this same file: `real distance_squared` in `_path_state_begin` (commit cf097d7f), law "a named real local routes x87".
- guard: `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d` -> NOT in candidate.c. Saved as owner_ruling_candidate.c.

### T11 `_path_state_traverse` - tr_10 with the two helper calls hand-expanded again (try_tr_11.c) = the body in candidate.c
- `residual   2096  _path_state_traverse  [size 2112!=2096, sha]`, 657 vs 652 insns, 61 hunks, 62 non-relocation rows (baseline: 132 hunks, 445 rows, frame 0x864 vs 0x85c, 2 rets vs 1).
  Guard passes. Without the helper the magnitude/dot/distance3d x87 orders and the strength-reduced edge-cursor bias (+0x1c vs January +0x18) cannot be reached; tierB's mod8 verdict on this
  shape was INSTRUMENT-PERTURBS / no uniform count.

### A17..A22 `_path_state_estimated_distance` further shapes on e4 (all `residual 496 [sha]`, none moves `result`)
- M8 dead-local sweep D=1..8: all byte-identical to e4 (MOD8-INSENSITIVE). H3 filler-prototype sweep K=0..32: all byte-identical (H3-INSENSITIVE). So neither the reload order at 0xf9 nor the result homing is numbering-keyed.
- try_16 uninitialised `result` + `result = FALSE;` as the LAST else statement: identical to e4 (return is still split per edge).
- try_17 first walk loop as `while`: identical. try_18 `?:` passed straight to vector_from_points3d: identical. try_19 separate block-local walk node: identical.
- try_20 `result = TRUE` after the direction block: 192 insns, remat. try_21/22 explicit second `return` in the else arm: 192 insns, remat.
- Corpus census (scanpat.py): `mov al,1; mov [ebp+disp>=8],al` occurs in exactly TWO January functions: this one and units `_code_001a0cf0` (there it is an assignment to a byte PARAMETER, a different thing).
  `mov al,1; mov [ebp-N],al` occurs in 6 functions, 5 of them EXACT in our tree; the smallest (`_object_select_random_region_permutations_by_variant`) shows the mechanism:
  `result` is MEMORY-homed because two different defs reach one use, and the same-block return copy is forwarded as `mov al,1` sharing the constant register with the store.

## FINAL STATE

candidate.c (guard PASS) = real source + e4 body for `_path_state_estimated_distance` + tr_11 body for `_path_state_traverse`; `_closest_point_to_attractor` untouched.

```
== emitted-symbol guard passed (1 forbidden names checked)
residual    208  _closest_point_to_attractor  [sha]
residual    496  _path_state_estimated_distance  [sha]
residual   2096  _path_state_traverse  [size 2112!=2096, sha]
== exact 29  residual 3  unwritten 0  (of 32 listed)
```
unitcheck: `SUMMARY gains=0 regressions=0 other=2`. newsyms: `candidate newly DEFINES 0 symbol(s)` / `OWNERSHIP PASS`. fake_match_scan: 0 review leads. CL /Zs /W3: 13 warning lines, identical to the real source (no C4013, no C4700/C4701).

owner_ruling_candidate.c (guard FAIL, NOT the deliverable) = candidate.c with `point_from_line3d` at three sites:
```
EXACT       208  _closest_point_to_attractor
residual    496  _path_state_estimated_distance  [sha]
EXACT      2096  _path_state_traverse
== exact 31  residual 1  unwritten 0  (of 32 listed)
```
unitcheck `SUMMARY gains=2 regressions=0 other=1`; newsyms `+ _point_from_line3d .text VIOLATION: January does not define it` / `OWNERSHIP FAIL`; guard `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d ... section-size=48`.
NEEDS OWNER RULING: 2,304 padded bytes (208 + 2,096) are byte-exact the moment `point_from_line3d` may be called from path.c. Note our path.obj ALREADY emits nine helper COMDATs January's path.obj lacks
(`_distance3d _distance_squared3d _dot_product3d _magnitude3d _magnitude_squared3d _normalize3d _scale_vector3d _square_root _vector_from_points3d`); `_point_from_line3d` would be the tenth of the same systemic class.

## Remaining blockers (byte ledgers)

`_path_state_estimated_distance` (e4 body, 191 vs 191 insns, 496 = 496): every differing byte is one of
 1. `result` homing, 3 instructions: January `106 mov al,1 / 108 mov [ebp+0x17],al ... 1a7 mov al,[ebp+0x17]`; ours `106 mov al,1 ... 1ae mov al,1` (remat), no store. (+3 -3 +1 bytes = the 1-byte shift of the else block.)
 2. consequence of 1: spill-slot assignment - January path_distance -> [ebp-4] (node_index's dead spill cell), ours -> [ebp+0x14] (the dead parameter home January gives to `result`): 4 rows.
 3. consequence of 1 (proved by try_2, where a memory-homed result reproduces January's colouring): the 3-cycle EDI/ESI/EBX rotation of state / node+distance_reference / node_index: ~80 rows.
 4. reload order at the join 0xf9: January `mov ecx,[ebp+0x14]` then `mov eax,[ebp-8]`; ours value first. Not M8, not H3, present in try_2 as well.
 The decision to reach: what makes VC7 treat the `result = TRUE` web as NON-rematerialisable (memory-homed) while its def sits in the same block as the `if (estimated_direction_reference)` branch.
 Measured: a constant-def web is memory-homed only when no caller-saved register is free between the def and the first call (try_2/6/7/10 vs try_9/11..14), and January's own bytes leave eax and edx free there.

# WAVE 3 (2026-09-21, fresh candidate.c from the landed source; owner ruling 5a.1 now admits header-inline COMDAT emission)

### W3-1 `_closest_point_to_attractor` - real `point_from_line3d(p0, &segment, t, result);` for the three hand-written stores
Evidence: January else-arm = `v->i*t + p->x` operand order of the helper (predecessor A16).
- before `residual    208  _closest_point_to_attractor  [sha]` -> after `EXACT`; `== exact 30  residual 2`.
- admit_check: `_point_from_line3d 48 B header-inline ... byte-identical: True => OK`, `ADMISSION CONDITIONS (1)+(2): PASS`, unitcheck gains=1 regressions=0. KEPT in candidate.c.

### W3-2 `_path_state_traverse` t1 = landed body + real helper at both entry-point sites (/Od 0x4c6d96 and 0x4c6f73 both CALL point_from_line3d)
- `residual 2096 [size 2112!=2096, sha]`, 659 vs 652 insns, 19 non-reloc rows: block (a) dot term order (Jan k,i,j / ours i,j,k) + block (b) distance3d squares + pads. Edge-cursor bias now January's.
### W3-3 t2 = t1 + block-scoped second `bsp` (/Od: separate slot [ebp-0x8f4] for the 0x466 assert): identical to t1 (19 rows).
### W3-4 t4 = t1 + `struct path_node previous_node_values = *new_node` (/Od RTC local, size 68, written once never read; VC7 deletes the copy, no rep movs, frame unchanged): identical to t1.
### W3-5 t3 = t1 + BOTH /Od locals (+2 IL locals): block (b) becomes January's (11 non-reloc rows left, all = block (a) + its 8-byte consequences: long jne at 0x22f, loop-head pad, tail pad).
  Matches predecessor's M8 finding (D=2/D=3 closes (b)) with GENUINE first-party locals instead of `real dot`/`diameter`. Assert-line drift /Od-vs-January is a constant +1 from 0x35D to 0x472 -> same line count in between.
- DIAGNOSTIC M8 sweep on t3 D=1..8: never closes block (a); D=8 != D=0 (instrument perturbs). (a) is not an in-function local-count defect.
### W3-6..10 block (a) probes on t3 (all `residual 2096 [size 2112!=2096, sha]`, block (b) stays January's)
- t5 swapped dot args: identical to t3. t6 `t = dot(); t /= mag();`: dot terms j,k,i. t7 DIAGNOSTIC `real dot` (+3 total): j,k,i (the predecessor's k,i,j came from dot on ITS count, not from dot itself).
- t8 every declaration in the /Od slot order (edges inside the while, new_node_index after passable, distance_to_destination before total_cost_estimate): byte-identical to t3 (declaration order inert).
- t9 = t3 + /Od-attested `assert(path_heap_verify(state, NONE, NONE))` (/Od line 0x47a, +1 drift -> January 0x479; emits no code, verify inlines to TRUE): dot terms flip i,j,k -> j,k,i. So an invisible inline expansion re-keys block (a).
- t10 DIAGNOSTIC 3-parameter verify twin: identical to t9. M8 sweep on t9 D=1..8: block (a) stays j,k,i for every D (not local-count keyed).
- term orders seen: i,j,k (t1,t3) / j,k,i (t6,t7,t9) / j,i,k (t3+D>=6); January k,i,j = (kk+ii)+jj, a REASSOCIATED sum, i.e. a canonical-sort key, all three are rotations of i,j,k.
### W3-11 t11 = full /Od-shape rewrite of the edge-loop body (frag_t11.c) -> `_path_state_traverse` EXACT (`== exact 31  residual 1`), 652 vs 652 insns, 0 non-reloc rows
/Od facts used (od_traverse.txt): `if (passable) {` block owning new_node_index(=NONE)/entry_point/... (slot order -0x82d,-0x834,-0x844..; NONE store at 0x4c6d63 AFTER the passable test);
if / else-if / else chain instead of two `continue`s (0x4c716a and 0x4c718b both `jmp 0x4c73ed` = the new_node_index test, not the loop increment); hash probe as a `for` (increment block 0x4c71a9);
`if (new_node_index != NONE) {` block owning new_node + previous_node_values (RTC name, size 68, `rep movsd` 0x4c7428) + second bsp slot -0x8f4; `assert(path_heap_verify(...))` /Od line 0x47a (January 0x479);
else-arm `closest_approach = 0; cost = linear_distance;` (0x4c704a); `closest_distance < 4.0f` (0x4c76ee); real point_from_line3d calls (0x4c6d96, 0x4c6f73).
candidate.c := t11. Strip tests follow.
### W3-12 strip tests of t11 (strip.py; every row is a full gate)
| strip | result |
|---|---|
| no `previous_node_values` | NOT exact `[size 2112!=2096, sha]` -> REQUIRED |
| second `bsp` folded back into the outer `bsp` | NOT exact -> REQUIRED |
| `distance_to_destination < 4.0f` (landed spelling) | NOT exact -> `closest_distance < 4.0f` REQUIRED (/Od 0x4c76ee compares [ebp-0x8f8]=closest_distance) |
| `!bounded || dist <= max` | NOT exact `[sha]` (NaN sense; January needs the negated `>`); `!(a && b)` and `!a || !(b > c)` both exact |
| verify assert dropped / else-arm order / decl order / edges at fn scope / new_node assign-vs-init / while-vs-for hash probe / `if (!passable) continue;` / `if (new_node_index == NONE) continue;` | all EXACT either way (byte-inert) |
### W3-13 t12 = MINIMAL: landed body + real helper x2 + block-scoped second `bsp` + `previous_node_values` + `closest_distance < 4.0f` -> EXACT (`== exact 31  residual 1`)
candidate.c := t12 (byte-inert /Od restructurings and the code-less verify assert NOT applied: not confirmable in January bytes). unitcheck gains=2 regressions=0; admit_check PASS; fake scan 0 leads; castcheck real=1 landing=1 (no new view casts).
DISCLOSURE: `previous_node_values` is written once and never read (a debugger aid); VC7 deletes the copy (no rep movs, frame 0x85c unchanged). First-party: /Od RTC descriptor names it (size 68 = sizeof(struct path_node)), `rep movsd` at 0x4c7428 right after path_get_node. January side: constant +1 assert-line drift 0x35D..0x472 (same line count as /Od between asserts 0x425 and 0x466) and the x87 square order of block (b) 0x2cc-0x2d8 is reached only with BOTH /Od locals present (predecessor's blind M8 sweep had already said "+2 or +3 locals").
### W3-14 `_path_state_estimated_distance` e1 = /Od loop-2 shape -> EXACT on the first compile (`== exact 32  residual 0`)
/Od fn 0x4c63b0: after the first walk `current_node_index = child_node_index;` (0x4c657e `mov ax,[ebp-0x38]; mov [ebp-0x34],ax`), the second walk tests/advances [ebp-0x34]=current_node_index (0x4c6586, 0x4c65f3)
and the `?:` tests current_node_index (0x4c65f9). Landed source iterated child_node_index. January side: with the copy, `result` becomes the spilled byte web `mov al,1; mov [ebp+0x17],al ... mov al,[ebp+0x17]`,
path_distance moves to [ebp-4] and the EDI/ESI/EBX 3-cycle becomes January's - i.e. every row of the predecessor's ledger. No decoration: one assignment + the iterator name.
candidate.c := e1.

## WAVE 3 FINAL STATE (candidate.c = landed source + 6 edits, CRLF only, 1813 lines)
```
python tools/campaign/gate.py source/ai/path --source scratch/lane_b/w/path/candidate.c --all
== exact 32  residual 0  unwritten 0  (of 32 listed)
```
- unitcheck `SUMMARY gains=3 regressions=0 other=0` (+_closest_point_to_attractor 208, +_path_state_estimated_distance 496, +_path_state_traverse 2096).
- admit_check: `_point_from_line3d 48 B header-inline ... byte-identical: True => OK`, `ADMISSION CONDITIONS (1)+(2): PASS`.
- `--forbid-emitted-symbol _point_from_line3d`: `FORBIDDEN-EMITTED-SYMBOL _point_from_line3d ... section-size=48` (guard FAILS by construction; this is the emission owner ruling 5a.1 admits). newsyms: `+ _point_from_line3d .text VIOLATION` / `OWNERSHIP FAIL` - same single symbol, nothing else new.
- fake_match_scan 0 leads; castcheck real=1 landing=1; CL /Zs /W3 census identical to the real source ({C4146:1, C4244:12}, no C4013/C4700/C4701).
- Strip tests ON THE FINAL FILE: without previous_node_values -> traverse `[sha]`; second bsp folded into the outer one -> `[sha]`; `distance_to_destination < 4.0f` -> `[size 2112!=2096, sha]`. Each element is necessary.
