# `connected_geometry.obj` — opus5 next-150K lane, wave n4 (2026-09-15)

Baseline and final real-file gate (`--forbid-emitted-symbol _point_from_line3d`, guard passes):
**6 exact / 4 residual / 0 unwritten**, unchanged row by row. Park drift 0 (both parks). Fake scan 0 leads.
`git diff --stat -- source/tool/connected_geometry.c` is empty. **No credit is claimed.**

## 0. The wave's leverage premise is empty for this object

The n4 dispatch asserted "unit never dispatched in any wave of this or the 250K lane". That is wrong:
`connected_geometry_obj_opus5_next150_n2_20260915.md` is a wave-n2 ledger of this lane and censused all four
residuals. The dispatch's admission test — a relocation-by-target census "naming exactly which call or global one
side references and the other does not" — produces NO delta on any of the four rows:

| function | JAN relocs | ours | by-target multiset |
| --- | ---: | ---: | --- |
| `_connected_geometry_add_triangle` | 14 | 14 | identical |
| `_connected_geometry_find_or_add_edge` | 7 | 7 | identical |
| `_connected_geometry_find_or_add_vertex` | 8 | 8 | identical |
| `_triangle_coplanar` | 11 | 11 | identical |

The rows `relocdiff --count-by-target` prints as differing are (a) uniform address shifts caused by the single
size gap in `find_or_add_edge`, and (b) the float constants, which January's csplit object carries as undefined
`['symbol','__real@...',0]` while ours carries the same name as `['defined-noncode','.rdata','__real@...',0]` with an
identical `symbolic=` resolution. That is a **csplit naming/definedness artifact**, not a missing call: it also
appears on rows of this object that are strict EXACT. Reported plainly as the dispatch instructs.

## 1. `_connected_geometry_find_or_add_edge` (240 target / 256 ours) — owner-blocked, now MEASURED not inferred

Frame census identical (`push ecx` = the single `[ebp-1]` byte). Relocations 7/7 identical by target and order,
every address uniformly +4: the whole size gap is ONE 4-byte instruction, `mov byte ptr [ebp-1],1` at our +0x10,
i.e. the `boolean forward = TRUE;` initializer. January writes `[ebp-1]` at exactly three sites and never on entry
(+0x47 forward match = 1, +0x4D reverse match = 0, +0x67 new edge = 1) and reads it once at +0xBE; the only path
reaching that read with no preceding write needs `geometry->edges.count < 0`, so January declares `forward`
uninitialised. The `>=` respelling that would define the read is refuted by +0x51 `cmp esi,[ebx+0x10]; jne`
(equality, not `jl`).

Shapes (5):

| id | shape | result |
| --- | --- | --- |
| e1 | `boolean forward;` (no initializer) | **240 / 7, 105 vs 105 instructions, byte-identical through +0xD6** |
| e2 | early exit `if (edge_triangle_index == NONE) return NONE;` | 256 / 7; WORSE — the merged NONE epilogue de-merges to an inline copy at +0xB0 |
| e3 | e1 + e2 | 240 / 7, same de-merge; worse than e1 |
| e4 | e1 + `else { return NONE; }` | byte-identical to e1 |
| e5 | e1 + `else { return NONE; }` with one joined `return edge_index;` | byte-identical to e1 |

New facts for the lane: (1) n2 *inferred* that a YES ruling on the uninitialised read would not close the row;
e1 **measures** it — the size gap and every instruction difference vanish and only a block swap survives
(January `[+0xC8 set-bit arm+ret][+0xD7 NONE epilogue][+0xE1 clear-bit arm+ret]`, e1 `[set][clear][NONE]`).
(2) e2/e3 show the merged `return NONE` epilogue takes the position of the SOURCE-LAST `return NONE;` statement and
de-merges if that statement moves earlier, so January's source-last `return NONE;` is the trailing one, exactly as
ours. (3) e4/e5 show the placement is inert to else-form and single-exit-join respellings. Consistent with
laws_w3 A43.

**ORCHESTRATOR PROPOSAL (owner ruling; unchanged in substance from n2, now byte-measured).** May `boolean forward;`
be left uninitialised here, reproducing January's latent indeterminate read on the `edges.count < 0` path?
Candidate: `scratch/workers/n4_connected_geometry/e1.c`. A YES ruling is still **not** worth 240 B on its own —
the block swap is now measured to survive it.

Reopen criterion: a source construct that places the merged `return NONE;` epilogue between the two `SET_FLAG`
ternary arms (equivalently, moves the ternary's else arm to the end of the function), together with the ruling.

## 2. `_triangle_coplanar` (parked `instruction-scheduling`) — skipped, 0 shapes

Census: frame identical, relocations 11/11 identical by target and address, 151 vs 151 instructions. Nothing
structural. The parked production body measures 10 differing instructions in three hunks at this HEAD.
n2's genuine-helper candidate `scratch/workers/n2_connected_geometry_actor_stimulus_etc/cg/v1.c` was re-verified at
this HEAD: **384 / 11 with exactly 4 differing instructions** (one swapped `fld/fmul` pair at +0x148 and
+0x155/+0x156), all six sibling EXACT rows held, guard passed. The whole remaining residual is x87 term order inside
one inlined `dot_product3d`, which the 2026-09-15 owner directive forbids spending on in this wave and whose only
recorded closing construct (a hand-expanded two-term split) is laws_w3 R15.

**Note for the parks owner:** the `config/parked.json` evidence text for this entry is STALE. It says "the sole
remaining difference is one dependency-free x87 operand choice at +0x11D/+0x11F"; the body actually parked differs in
three hunks / 10 instructions. n2's v1 is the body that matches that description, and n2 already filed the
park-remeasure proposal — it is not duplicated here.

## 3. `_connected_geometry_find_or_add_vertex` (parked `unclassified`) — skipped, 0 shapes

Census: frame identical (`push ecx` = one `[ebp-4]` slot), relocations 8/8 identical, 80 vs 80 instructions. The
residual is January's `fld [ebx+4]; fstp [ebp-4]` / `fld [eax+4]; fsubr [ebp-4]` x87 spill against our integer copy,
plus an ECX/EDX swap in the final three scalar copies — both forbidden categories this wave, and both halves already
proved by n2 (a named `real` local is copied with integer moves; an inline getter makes VC7 evaluate the call first).
n2's reopen criterion stands: a construct that evaluates `point->y` before the `dynamic_array_get_element` call
without binding it to a named local.

## 4. `_connected_geometry_add_triangle` — skipped, 0 shapes

On the laws_w2 section D do-not-repeat table. Relocations 14/14 identical by target; 158 vs 158 instructions; the
single address shift (0x10 vs 0x11) is the shrink-wrap itself. January delays `push ebx` to +0x23, after the
`triangle_index == NONE` exit, where ours issues it in the prologue; the two epilogues then differ only in
`pop edi/pop esi/or eax,-1` order and one `pop ebx` position. No law in A1-A45 addresses prologue shrink-wrapping and
this wave found no new January-side fact.

## 5. Checks

Every scratch candidate (e1-e5, and the re-verified n2 v1) kept the six baseline-EXACT siblings strict EXACT and
passed the `_point_from_line3d` guard. `scratch/parkcheck.py`: parks 2, drift 0. `tools/fake_match_scan.py`: 0 leads.
`tools/campaign/branch_sweep.py`: 10 historical blobs, best `exact 6 residual 4` (= current) — no donor exists.
The production file is byte-identical to HEAD. No header, config, ninja, configure, pytest or git action was taken.
