# interface/virtual_keyboard::_virtual_keyboard_select

## verdict
NO_PROGRESS

## newly_exact_functions
null

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/s2.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/hr1.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/s3b.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/LEDGER.md"
]

## production_changes
None needed for credit. Optional house-rule cleanup that does not change any bytes: scratch/w/virtual_keyboard/production.patch (unified diff against source/interface/virtual_keyboard.c). It replaces the default arm's hand-expanded `virtual_keyboard_get_character(virtual_keyboard_key_layout[row][column])` with `virtual_keyboard_get_current_character()`. Verified: all 20 function normalized hashes equal build/base, select stays 025bd53f (the park base), and object_audit output is identical to base. Keep the park. Do NOT land w1's `word buffer_size` lever (s0/s2 sha ff65b27b) while the function is not exact, because it moves the parked bytes.

## evidence
Starting point: s0 = production + w1's `word buffer_size` (1088/123). Aligned diff shows the only difference is where the merged left/right tail sits (5 instructions: `mov [cursor],eax; push 1; mov [first_key],bl; call feedback; add esp,4`) plus the jump-table entry for the right arm (412 vs 388).

/Od readout of the later build (fn 0x68ef60) gives the same left-then-right source order and the same statements.

Mechanism, decoded with /FAsc labels:
- Both builds do the same IL tail merge: a new store block, a phi into eax, and the later arm's `mov eax,ecx`.
- Only the survivor differs. Ours keeps the RIGHT arm's if-skip label ($L23111); January keeps the left arm's.
- The survivor is the first predecessor in the switch-END predecessor list. A flat switch whose END holds fallthrough code keeps the source-LAST arm.
- An END that is an empty jump block reverses this and keeps the earlier arm (January's layout).

Tested:
- Lab e1-e30 on the w3 replica:
  - arm count or position, default position, extra calls, && conditions, loop wrapper, if without else, return after the switch, inline helper: all keep the last arm.
  - Nested switch (e16/e17) and switch inside if-ELSE (e22): the earlier arm hosts (January's shape).
- Two EXACT corpus functions confirm it (scratch/w/virtual_keyboard/xjscan.txt):
  - _virtual_keyboard_process_internal (nested switch; still hosts left when first_key is moved after the if, lab_pi1-4)
  - _render_debug_add_cache_entry (switch followed by a skip-else jump).
- Real TU:
  - s3b (switch wrapped in `if (keycode >= NUMBER_OF_CONFIGURABLE_VIRTUAL_KEYS) {...} else {insert}`) reproduces January's left-host layout but adds `cmp ax,0x24; jl`.
  - s3a, s3c (VALID_INDEX) and s3d (unsigned range cast) also add dispatch compares; VC7 never folds the redundant range test.
  - s1: swapping left/right source order changes the layout (so w1's 'emission follows case value' is wrong here) but the tail still sits after the source-last arm.
  - Declaration-count oracle sweep (1-32 dummy externs, oracle only): inert.

Full probe record: scratch/w/virtual_keyboard/LEDGER.md.

## blockers
January's single range check (`add eax,-0x24; cmp eax,7; ja default; jmp [table]`) and a default arm that falls straight into the shift-reset code fix a flat switch with fallthrough END code. Every structure found that makes the switch END a jump (nested switch, if/else wrapper) adds dispatch code. The remaining devices (one-trip loop, goto) are forbidden fake matching. No lawful source handle found; the stop rule fired.

## reopen_criteria
Reopen only if one of these arrives. (a) A January-plausible select structure in which the switch END is an empty jump block but which still compiles to exactly one range check; start from s3b.c, which already gives the right layout with one extra compare. (b) A C2-level decode (for example the dbg32c watchpoint on the tail-merge predecessor list) showing another IL fact that orders the switch-END predecessors. If either makes the tail placement match, re-apply `word buffer_size` from s0/s2 and retire the park.

## task notes
Neither function closed, so the object stays at 18/20 exact. object_audit on the base object shows every January data, rdata and bss section and all 37 symbols matching; only the two functions differ. Candidate-only surplus is the pooled string literals from this TU (ui\english, <unknown>, ui\shell\bitmaps\white, bitmap) and the XDK D3D tables, which already existed. surplus_identity reports 0 candidate-only code COMDATs.

The main new finding is the tail-merge survivor mechanism. It is proven on 2 EXACT functions from the corpus plus lab and real-TU probes, and should be added to the campaign laws: an IL tail merge keeps the first predecessor in the switch-END list; a flat switch keeps the source-last arm; an END that is an empty jump (nested switch or if-else wrapper) keeps the earlier arm.

It corrects two earlier ledger claims:
- w1's 'emission follows case value' is wrong for this function: s1 shows the layout follows source order.
- w3's conclusion of a 'late machine-level cross-jump' is wrong: VC7 has no machine-level suffix cross-jump, and the merge is an IL one with a new store block.

No tracked files were edited and no ninja or git-mutating commands were run. The only production artifact is the optional byte-neutral production.patch.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/virtual_keyboard/LEDGER.md
