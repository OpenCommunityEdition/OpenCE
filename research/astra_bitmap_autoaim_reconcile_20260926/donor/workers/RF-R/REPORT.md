# RF-R wave 4 report: B1 `_rasterizer_bitmap_new` and B3 `_poll_ep_array_compare_proc` (saved by the lead)

**Both NEGATIVE.** No candidate, no patch. Both mechanisms decoded to the C2 decision that picks the result, traced
at each step. Patch-only; no source/config/tools/build edits; no ninja; no commits; no other worktree touched.
Records: NOTES.md, cards\, evidence\. Scratch/tools `scratch\rf\workers\RF-R\` (rt.py dbg32c driver, names.py,
ilsnap.py, gtrace.py, stepwalk.py, walkat.py, cjscan.py + copies of RF-E's oracle and retail kit and od_dis). dbg32c
copy hash 32ee5e4d; every traced HEAD compile stock-equal.

## B1 `_rasterizer_bitmap_new`
Readout (HEAD, MARK 14/19): at MARK two separate return blocks with different values: N189 is the `return success`
label (L_ret) holding `s63 = success; JMP EXIT`; the no-device arm (ND) is `hw = NULL; s64 = #1` falling into EXIT;
nothing merges them before MARK. After MARK ND's `#1` becomes the EBX constant temp (per-function constant-temp hash
0x10722897, writer 0x1070316c, replaces it with s4), so at step #2 both tails are `al = bl`; the compare
(0x10747514 -> 0x107481a8/0x107480f9) checks only the register (+0x1c) for register operands.
Decoded rules:
- R1: the step-#2 cross-jump (0x107483d6), run for each JMP from 0x107494f3 when flag = 1, makes the FALL-THROUGH
  block host: the block that falls into the JMP's target label hosts the shared tail; the JMP side's matching tail is
  deleted and the JMP retargeted (0x107487cf -> 0x107074a3); any label met on the JMP side during the backward match
  is moved to the host (0x1074834b..0x10748387). Watchpoint trace (evidence\b1_wp_trace.txt): the JCC N180 is never
  written; L_ret is unlinked at 0x10784d03 and reinserted after ND's store at 0x10784d2c, then N190 deleted: the jne
  follows L_ret into ND.
- R2: the tail-duplication pass (0x1074cd50 -> walk 0x1074dafc) moves a return block reached only by jumps to the
  first jumping site in layout order (0x1074e237 -> 0x1074ca16, then 0x10712951 deletes the jump); other jumps get
  unlabelled copies; conditional branches stay bound to the moved original.
- R3: the backend splits a single exit return: jump and fall-through predecessors get inline copies; conditional-branch
  edges get copies at the function end, value-specialised where known; the last of these falls into EXIT.
- R4: C1 places an else arm ending in `return` after the trailing return (visible in the IL at optimizer entry).
Validation: `_get_next_event` (exact, FIRST binding): R1 finds no match (cjscan: 3 regions, 0 matches) because EXIT's
fall-through is the `al = #1` found block; R2 moves the jump-only `return result` block to the first jump site
(watchpoints: `N51.next := N85` at 0x1074ca22, then N51 deleted): the `je` binds to 0x6b = January's FIRST binding.
`_actor_path_refresh` (P02 exact) vs RF-A's floor: in the step-1 IL the last R3 copy is `al = #1` in P02 but `al = bl`
in floor, so in floor R1 folds the matching copies into the last one and R2 moves it to the first jump site (0x2dd):
consistent with RF-A's flag lever (partly traced: step-1 IL plus final bytes, no per-event watchpoints).
Condition January needs: L_ret must stay on the copy after the error block; at step #2 either (i) L_ret falls through
from the error block and EXIT's fall-through predecessor is not an `al = bl` tail, or (ii) L_ret is jump-only, the error
block's jump is first in layout order, and ND does not end up falling into it. The EBX pin makes ND's TRUE an `al = bl`
tail, and every spelling read or recorded leaves ND last.
Evidence: Aug-15-2001 debug (1749betaP 0x14c400) and Oct-12-2001 debug (2276betaP 0x168370, masked search) contain
January's body verbatim (ours 0 hits): source stable Aug-Jan. Sept-25-2001 retail (0x504890) and Oct 2276P (0x115de0)
have no HRESULT tests, no `rasterizer_error` call, no `success` variable: the per-arm check is debug-only (macro or
`#ifdef`); the retail binding is the merged TRUE return (says nothing about the debug jne). /Od dx9 0x7ea7d0 is a later
revision (single exit, four-argument `rasterizer_error`, no `success = TRUE` in the arms). No donor.
Cards (2 of 30): E1 (18:38:03) `else { hw = NULL; return TRUE; } return success;` (dx9 topology + January's constant-1
use): predicted identical to HEAD (front end sinks ND past the trailing return): correct (normalized sha 719469...,
jne -> 0x17e; MARK IL confirms ND last). D1 (18:40:31) diagnostic readout of Lane C's recorded A3: bytes identical as
predicted; structural prediction wrong (A3 uses R3: the conditional-edge copy is placed after ND and hosts, reaching
the same bytes by another route).
Reopen: first-party source or donor evidence of a return structure where, at step #2, ND is not the matching
fall-through predecessor of the return-success JMP's target (e.g. ND reaching the error path's return block by a jump,
that block laid out after the error block); or proof that a pass after step #2 turns a split `al = #1` copy into
`mov al,bl`. Check with stepwalk.py, gtrace.py (bps 0x10748426 / 0x107485b7 / 0x107487cf) and walkat.py 0x1074dafc
before compiling.

## B3 `_poll_ep_array_compare_proc`
Readout (HEAD, MARK 16/26): the deciding term is priority: B 14, A 12. B = 2x3 + 2x2 + 2x2 (definition in the entry
block plus one test in each of the two successor blocks); A = 4x3 (definition and single test, both in the entry
block; A's second test threaded away before MARK). B pops first, takes eax; A gets ecx.
Decoded formula: priority = sum(refs x m(block)), m = (number of webs touched in the block, including zero-reference
touches) x block frequency; count at 0x1072aac5..0x1072aae6, multiplier set at 0x1072aaf1; checked on
`_get_next_event` and `_compare_ulongs_descending`. The multipliers are 3, 2, 2: the two later blocks weigh 2 because
each touches B plus K0 (the constant-0 temp: defined by op268 at +5, used with zero benefit by all three NULL
compares, never coloured).
Forced oracles (diagnostic, zero credit), each giving January's exact bytes: D0 raising A to tie B at 14 (A wins the
[W+0x44] tie-break, 0x6 > 0x4); D0 forcing B to ecx; D1 forcing the later blocks' multiplier to 1 (B = 10 < 12).
January's IL needs K0 not touched in the two B-test blocks, or one more web touched in the entry block, with emitted
code unchanged.
Evidence: our source in retail config gives A = ecx with the same 14/12; Sept-2001 retail (0x453200) and 2276P both
have A = eax: the difference is in the IL and present in both Bungie configurations. /Od sibling comparator 0x68a790
(qsort caller 0x68a680, later PC revision) uses the single-exit `result` idiom; FO's census already covers it (cmp1
c_ppl, single-exit forms) and it gives A = ecx. No attested construct changes K0's touches.
Reopen: a first-party construct whose later B tests have no compare-with-constant node, or whose entry block touches
one more web, with emitted code unchanged; verify first with the multiplier trace (bp 0x1072aaf3 must read edx per
priced block as 3,1,1 or 4,2,2).

## Disclosures
RF-A's recorded floor.c was traced for the actor_path_refresh flip control without a card first (logged in
cards\DISCLOSURE_uncarded_readouts.txt as a readout only). Two card headers had times written before the clock read
(B1_E1's outcome line, B3_D0's header); both corrected by appended notes.
