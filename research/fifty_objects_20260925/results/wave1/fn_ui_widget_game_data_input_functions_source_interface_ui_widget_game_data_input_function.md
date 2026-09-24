# source/interface/ui_widget_game_data_input_functions::_solo_level_select_list_update_displayed_items

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/ui_widget_game_data_input_functions/oracle/run.sh",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/ui_widget_game_data_input_functions/LEDGER.md"
]

## production_changes
None to land; keep the park. Suggested evidence-text update for the integrator: the residual is a TU-global name-count tie, not instruction scheduling. The body is already correct and matches January for some declaration count, so it is January-reachable.

## evidence
Name-count oracle (lab only, never land): K dummy `extern long` declarations make solo EXACT only at the following K. Production file, before solo: K=16,17 (0..75 swept). January-order p2.c: K=55,56 / 119,120 / 183,184, so the period is 64 and the window is 2 wide. Storage-restructured r1: K=49,50, with the same window when the dummies go before the globals block or before every #include, so every declaration in every header counts. r3 (two fewer file-scope statics): K=51,52, exactly the predicted shift of 2. Reordering solo's own locals (item_index first or last) is inert. Moving solo alone to its January slot (p1) or reordering the whole TU (p2) does not close it. The residual is the known pair of transposed back-edge reloads at +0x9a/+0x9d (mov eax,[ebp-0xc] / mov edi,[ebp+8]); frame, size and all 45 relocations already match.

## blockers
January's declaration count before solo must equal production+16/17 (mod 64), which is final.c+51/52 (mod 64). Every header in the TU contributes to that count, and reconstructing it needs January's real header and prototype set. Filler declarations would be fake matching.

## reopen_criteria
Reopen when genuine January declarations are recovered: real headers replacing the TU-local shadow structs, enums and typedef size-asserts, or missing prototypes and externs. Any such recovery that brings the prefix count into the window closes solo with zero change to its body. Confirm with oracle/run.sh (dummy names only as the measuring instrument) and re-gate. The whole board also needs re-checking, because header counts move other ties.

## task notes
The object is not complete; both functions are still non-exact. Baseline at e9e62b78 is 44 EXACT / 2 residual. The previous run had no slug directory, so this run started fresh.

1. solo is not a scheduling residual. It is a TU name-count tie that covers the whole TU (period 64, 2-wide window), measured to single-name precision. Its body is correct.
2. January's storage layout is recovered from HCEX + cachebeta evidence: static callbacks, a static table, and static-local .bss arrays. It is verified to fix every symbol and storage row of the object audit on a simulated split. It also reproduces January's function emission order and is byte-neutral for all 46 rows and for ui_widget.obj.
3. mgd stays a block-placement residual after more than 30 total shapes.

All tools are in the slug: reorder.py, mk_r.py, mk_bss.py, simgate.py, object_audit_sim.py, oracle/run.sh, layout_scan.py and lab/. The tracked tree is untouched (git status clean for source/ and config/).
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/ui_widget_game_data_input_functions/LEDGER.md
