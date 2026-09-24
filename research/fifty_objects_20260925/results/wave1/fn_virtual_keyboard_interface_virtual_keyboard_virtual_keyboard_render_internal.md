# interface/virtual_keyboard::_virtual_keyboard_render_internal

## verdict
NO_PROGRESS

## newly_exact_functions
null

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/w1_v1_p.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/w1_E001_p.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/lab_pin1.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/LEDGER.md"
]

## production_changes
None. Keep the production 1584/95 fuzzy body. Once a body is byte-exact, a prerequisite config fix is ready: January's loop-2 bound relocation at +0x60c is spelled `_item_update_section+0` (csplit alias of `_keyboard_rect+352`). It can be retargeted in config/relocs.json to the `_keyboard_rect` file offset plus addend 352, following the Lane D precedent. It is pointless until the code is exact.

## evidence
Production is 1584/95. w1 v1 (per-arm bitmap fetch) rebuilt on the current base gives 1616/95 but still about 93 aligned diff blocks; E001 gives 1600/95.

New analysis of the NONE (-1) prologue pin. January pins -1 with `or edi,-1` at 0x3e, uses it 7 times and rematerializes it at 0x11c; ours uses immediates.
- Constant-use count: ours has 18 uses of -1 across the function and January has 18, so the count is not the difference.
- Corpus: 106 January functions pin -1 in the prologue, for example EXACT _first_person_weapon_draw, which pins purely from `!= NONE` compares plus system_exit(-1).
- ORACLE lab_pin1: replacing every -1 / 0xffffffff after the second draw_string_set_draw_mode makes our prologue pin appear (in ebx, displacing the 640 pin). So our -1 constant is one whole-function web that never wins a register.
- Replacing only one region (lab_pin2 later colors, lab_pin3 later NONEs, lab_pin4 caret/text-box/third call, lab_pin5 loops): no pin.
- Source hypotheses, all inert: lab_style (`long style` via a lab redeclaration, confirms fresh_graphs) and r1 (a caret `pixel32 caret_color` local, suggested by a later /Od build text-box function 0x68fdb0 [ebp-0x68]).

/Od (0x68fa00, 0x68fdb0) is a later redesign with no key grid, so it gives no evidence for loops 1 and 2.

## blockers
Several independent residual classes remain:
1. January splits the -1 web so the prologue part gets edi; our whole-function web never wins a register, and no source-level IL fact for this was found.
2. w1's key-IV base (+0x3c) versus the arm-layout conflict.
3. Register rotation in loop 2, the label home slot and the switch-index copy.
4. The caret width reload and swapped slots.
5. The loop-1 font-header reload and interleaving of the rect copy with argument pushes.
6. The csplit relocation alias, which relocs.json can fix once the body is exact.

## reopen_criteria
Reopen if one of these arrives. (a) A source or IL fact that separates the prologue -1 constant web from the later -1 uses; lab_pin1 is the oracle to check against, and an allocator trace (dbg32c) of the -1 web split is the suggested instrument. (b) w1's criterion: a natural switch spelling that gives the +0x3c key-IV base while keeping the per-arm fetch layout (start from w1_v1_p.c or w1_E001_p.c). Strict credit then also needs the relocs.json alias retarget.

## task notes
Neither function closed, so the object stays at 18/20 exact. object_audit on the base object shows every January data, rdata and bss section and all 37 symbols matching; only the two functions differ. Candidate-only surplus is the pooled string literals from this TU (ui\english, <unknown>, ui\shell\bitmaps\white, bitmap) and the XDK D3D tables, which already existed. surplus_identity reports 0 candidate-only code COMDATs.

The main new finding is the tail-merge survivor mechanism. It is proven on 2 EXACT functions from the corpus plus lab and real-TU probes, and should be added to the campaign laws: an IL tail merge keeps the first predecessor in the switch-END list; a flat switch keeps the source-last arm; an END that is an empty jump (nested switch or if-else wrapper) keeps the earlier arm.

It corrects two earlier ledger claims:
- w1's 'emission follows case value' is wrong for this function: s1 shows the layout follows source order.
- w3's conclusion of a 'late machine-level cross-jump' is wrong: VC7 has no machine-level suffix cross-jump, and the merge is an IL one with a new store block.

No tracked files were edited and no ninja or git-mutating commands were run. The only production artifact is the optional byte-neutral production.patch.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/LEDGER.md
