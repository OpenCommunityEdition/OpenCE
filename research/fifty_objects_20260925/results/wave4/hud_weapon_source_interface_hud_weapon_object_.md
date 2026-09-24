# source/interface/hud_weapon (object)

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\LEDGER.md"
]

## production_changes
None landable. Owner-gated: owner_gated_uwlp.patch or owner_gated_uwlp_after_P5.patch. Research-held: rwh_fidelity_candidate.patch.

## evidence
Baseline: 13 exact / 3 residual. With the owner uwlp body: 14/16. object_audit (owner_uwlp.obj): 23/23 January symbols, .bss _weapon_hud_globals ok, FAIL(2) = _crosshairs_draw and _render_weapon_hud. pdb_storage: 0 disagreements. git status stayed clean.

## blockers
_crosshairs_draw (owner uninitialised-read class plus fuzzy), _render_weapon_hud (allocation and x87 tie), _hud_update_weapon_local_player (owner uninitialised-read class).

## reopen_criteria
Owner rulings on the authentic uninitialised-read class (uwlp and crosshairs_draw), plus an rwh lever per the rwh entry.

## task notes
Nothing was landed or made landable for credit. uwlp is exact, but only in the held form: `result` is left unassigned on the unreachable default arm. January's T+0x144 load of the never-stored home proves that form. Patches are provided for both tree states: before and after the hud_stack_check P5.

rwh has a new partial decode. The flash-loop and statics-loop allocation residuals share one cause: a +1 ESI neighbour preference in the C2 chooser. It comes from the copy of local_player_index into get_hud_state's private ESI argument; lab G1 and G2 flip both loops to January's registers. The x87 add-chain order is a separate residual. No evidence-backed source lever was found. Every TU name-count, prototype and dead-local oracle is inert for rwh.

The rwh fidelity candidate cannot land alone. It newly emits header COMDATs from a non-exact caller (P1 class).

Interaction with the concurrent hud_stack_check packet: the rwh bytes are identical with its inline check_stack_buffer. The uwlp owner body stays exact on top of it. My production-tree uwlp patch conflicts with P5 on one context line, so use the _after_P5 variant once P5 has landed.

Tools copied into the slug and run only there: Lane A dbg32c.exe (sha1 21c75281, our own debugger over our CL). Its outputs are confined to scratch/w/hud_weapon/dbg.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_weapon\LEDGER.md
