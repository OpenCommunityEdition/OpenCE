# source/interface/hud_weapon::_crosshairs_draw

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\cd_r0.c"
]

## production_changes
None.

## evidence
Held owner item; not re-probed beyond a re-measure. The ruling form (production with an uninitialised firing_active, cd_r0.c) stays residual: relocs 77 against 76, 693 against 704 instructions, 157 alndiff hunks, frame 0x314 against 0x318.

January facts re-read from T_cd.txt:
- The if-chain handles states 18/8/9, and the state-14 path reads firing at T+0x495 with no reaching write (the /Od build at 0x6363e0 behaves the same).
- The animated/zoom colour tails cross-jump (T+0x51c `jmp 0x393`).
- The aim case jumps straight to draw when frame_index=0 is known.

Prior art: the structural c2r /Od switch shape reached 2224/74; binding bitmap_group_get (Lane C P5) gives January's 0x318 frame.

## blockers
Authentic uninitialised read (brief section 7 owner class). On top of that, still structurally fuzzy even in the ruling form.

## reopen_criteria
The owner admits the uninitialised firing read. Then a full /Od-guided rebuild combining the c2r switch shape, the P5 bitmap_group_get binding and the cross-jumped colour tail.

## task notes
Nothing was landed or made landable for credit. uwlp is exact, but only in the held form: `result` is left unassigned on the unreachable default arm. January's T+0x144 load of the never-stored home proves that form. Patches are provided for both tree states: before and after the hud_stack_check P5.

rwh has a new partial decode. The flash-loop and statics-loop allocation residuals share one cause: a +1 ESI neighbour preference in the C2 chooser. It comes from the copy of local_player_index into get_hud_state's private ESI argument; lab G1 and G2 flip both loops to January's registers. The x87 add-chain order is a separate residual. No evidence-backed source lever was found. Every TU name-count, prototype and dead-local oracle is inert for rwh.

The rwh fidelity candidate cannot land alone. It newly emits header COMDATs from a non-exact caller (P1 class).

Interaction with the concurrent hud_stack_check packet: the rwh bytes are identical with its inline check_stack_buffer. The uwlp owner body stays exact on top of it. My production-tree uwlp patch conflicts with P5 on one context line, so use the _after_P5 variant once P5 has landed.

Tools copied into the slug and run only there: Lane A dbg32c.exe (sha1 21c75281, our own debugger over our CL). Its outputs are confined to scratch/w/hud_weapon/dbg.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_weapon\LEDGER.md
