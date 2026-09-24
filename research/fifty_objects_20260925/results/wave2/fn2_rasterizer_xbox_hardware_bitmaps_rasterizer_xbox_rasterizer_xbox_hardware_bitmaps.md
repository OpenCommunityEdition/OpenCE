# rasterizer/xbox/rasterizer_xbox_hardware_bitmaps

## verdict
NO_PROGRESS

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\base.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\p\\t7.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\p\\t7.asm",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\od_7ea7d0.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\probe.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_hardware_bitmaps\\first_scan.py"
]

## production_changes
None. Keep the production source unchanged (identical to scratch/w/rasterizer_xbox_hardware_bitmaps/base.c) and keep the config/parked.json entry for _rasterizer_bitmap_new. No patch was written. There is no symbols.json or config change.

## evidence
Baseline gate --all: 18 exact, 1 residual (_rasterizer_bitmap_new [sha]), 0 unwritten. alndiff shows 150/150 instructions with no text differences. The only differing byte is +0x15A, the displacement of the jne at +0x159. January targets the epilogue at 0x171, just after the error() block; we target a byte-identical copy at 0x17e, just after the no-device `hardware_format = NULL` block (ND). Both copies are `pop esi; mov al,bl; pop ebx; pop ebp; ret`. object_audit on base.obj: every January section and symbol matches except this function (32 symbols, 0 differ).

New probes this wave (candidates and objects in scratch/w/rasterizer_xbox_hardware_bitmaps/p/):
- a1: case arms as in the /Od dx9 build, `if (result<0){success=FALSE; rasterizer_error}` with no `success=TRUE`. Result: 432!=400, 28!=26 relocs, 161 insns.
- a2: a1 plus the /Od single exit. Same regression as a1. Conclusion: the explicit `success=TRUE` arm is load-bearing. The same idiom is exact in rasterizer_xbox.c preinitialize.
- a4: reproduces Lane C's A4. 147 insns, loses the ebx=1 constant pin, binding still LAST.
- t7 and t7b: `else { hw=NULL; return TRUE; }` followed by `if (!success){...} return success;`. This form was not in any recorded probe list. Both are identical to base. The /FAsc listing (p/t7.asm) shows VC7 moves the textually earlier ND else-arm to the end of the function, which is January's ND position, but the merged canonical return (label $L20967) still follows ND.
- s1: a separate block-scoped `long result` in each case (the /Od build has three HRESULT slots). Inert.

Other evidence read:
- /Od fn 0x7ea7d0 (the dx9 rasterizer_bitmap_new) has a single exit and no success=TRUE in the case arms. It is a later revision.
- HCEX.pdb has no locals for this function (0xE4-byte PPC rewrite). HCEA research/halocea/src/rasterizer_bitmap_new.c decompiles that rewrite and is not source.
- Corpus study of the 33 January-exact FIRST-binding instances (the case where the jcc goes to the first copy, as January does here; scratch/w/.../first_scan.py over opus5 topo.json). Every one I read gets its layout from one of three things: a loop exit sunk after the return, a shrink-wrap pop block, or a return value specialised to different text (`mov al,1`). None of these applies to bitmap_new's bytes.

Full record: scratch/w/rasterizer_xbox_hardware_bitmaps/LEDGER.md.

## blockers
The residual is one jcc binding. VC7 folds `return success` and ND's `return TRUE` into one return block. Because ebx holds a constant 1 (the pin), `return TRUE` compiles to the same `mov al,bl` text. Across base, t7, A3, A4 and er, that block is placed after the last predecessor that falls into it: ND (ours) when ND is at the end, err when ND is inline (er). January needs the block after err with ND laid out after it, and no natural spelling measured does that.

Prior negatives already cover shared/split returns, polarity, goto and do-while, 24 case orders, declaration count 1..32, names, flags, and about 30 Lane C probes. This wave added 5 more. The methodology stop rule fired.

The object has no other blocker. Candidate-only surplus (XDK tables D3DPRIMITIVETOVERTEXCOUNT, D3DSIMPLERENDERSTATEENCODE, D3DTEXTUREDIRECTENCODE and two literals) would still need the surplus_identity and provider_link audits if this function ever closed.

## reopen_criteria
(a) A natural spelling where ND's return is not merged with `return success` but still emits `mov al,bl`: the return text differs at merge time and is identical in bytes. (b) Original January source, PDB locals, or an authenticated donor for this function. (c) A decoded C2 layout rule for where a merged return block goes relative to a sunk else-arm; t7 shows the sink happens before the return block is placed.

## task notes
No tracked files were edited, and no ninja, configure or git-mutating command was run. The integrator could append to the park evidence and to docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj.md that these forms are now also measured negative: /Od dx9 case arms without `success=TRUE` (a regression to 432 bytes), the else-arm early return with the error block after the if/else (t7/t7b, identical to base), and per-case block-scoped `long result` (inert). The t7 /FAsc listing is useful mechanism evidence: VC7 does sink the ND else-arm to January's end-of-function position, so what stays unreachable is only where the merged return block is placed.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_hardware_bitmaps\LEDGER.md
