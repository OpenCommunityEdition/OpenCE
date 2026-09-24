# source/interface/hud_weapon::_render_weapon_hud

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\rwh_fidelity_candidate.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\rwh_fidelity_candidate.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\c_od_union_names_tblock.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\mkcand.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\rv.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\dbg\\run_prio.py",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\dbg\\show.py"
]

## production_changes
None landable now. The fidelity candidate is scratch/w/hud_weapon/rwh_fidelity_candidate.c, with patch rwh_fidelity_candidate.patch (`git apply --check` clean on 16542e46). It replaces only the rwh body. It is held by wave-3 law (i)/P1: compared with build/base it newly emits the header-inline COMDATs _distance3d, _distance_squared3d, _magnitude_squared3d, _vector_from_points3d and _fast_ftol from a non-exact caller, and drops _power. Land it only together with an rwh closure.

## evidence
Base: the w1 R3 / structural r3u body. Changes applied: /Od-attested, byte-inert spellings (pow(10.0f,4), since /Od 0x632150 is C++ pow(float,int), not real_math power; the ?: unit index and the fast_ftol decimal inline as call arguments; `scenario = global_scenario_get(); map_type_flags = 0; SET_FLAG(...,0,...)` from /Od 0x63b40a, with crosshairs /Od 0x636452 the same idiom); RTC 0x63ba40 names numbers/numbers_real/position/target_position; /Od target-block declaration order; the admitted local-union sentinel transfer.

Candidate result: 2672/84, and the relocation-target multiset equals January's (production has an extra __ftol2). Relocations 0..53 sit at January's addresses, with 118 alndiff hunks against production's 208 (production is 2624/85, uses a hand-expanded distance (rule 6), (short)fmod, and flags/cutoffs locals). The TU-level union-element variant (CC4 E7d; invented type name, and HCEX says float[8]) gives 112 hunks and relocs 0..57 aligned.

Decoded residual (new):
1. The flash-loop IV swap (T hud=ESI/flags=EDI) and the statics-loop allocation (T definition in EDI, &statics in ESI, element_index homed), plus the frame-slot permutation downstream of them, share ONE cause. The C2 chooser (Lane A dbg32c run on a byte-identical lab TU, scratch/w/hud_weapon/dbg) gives p_hud (pri 46) and the statics &statics web (pri 34) an ESI cost of 1 against EDI 0. That +1 is the neighbour preference of the local_player_index web's copy into get_hud_state's private ESI argument (T+0x35 `mov esi,[ebp+0xc]`, the same bytes in both). Lab G1 (get_hud_state made non-static) and G2 (constant argument) both flip the flash loop and the statics loop to January's registers. Bisection shows the hw_v4 overlay `flags` webs, which take ESI earlier, have the same effect. So the tie is source-reachable in principle but upstream.
2. The x87 add-chain order (T j2,i2,+,k2,+ against ours k2,j2,+,i2,+) is independent (it stays kji in G1/hyb).

Measured inert (lab oracles, never landing): extern names 0..40 before the prototype and before the definition, prototypes 0..34, dead locals 0..16, initialised locals, definition order (January section order), pow/power, cutoffs pointer, age_percentage local, flash-counter scope ×2, the state_index local, the scenario idiom ×2, inverted flash if/else, the stack-check lane's inline check_stack_buffer (rwh byte-identical with it), and 1-6 later magnitude3d inlines for the x87 order.

## blockers
(1) Allocation tie keyed on an unidentified upstream IL fact: the ESI neighbour preference from the get_hud_state private-ABI argument copy. (2) An independent x87 add-chain order. (3) Admission of the candidate is held by P1 (new header COMDATs from a non-exact caller). Its explicit block after csmemset follows January's prologue order and does not depend on the stack-check packet (the bytes are identical with it). Methodology stop rule reached.

## reopen_criteria
A first-party fact that changes local_player_index's web or ESI copy preference, or colours a higher-priority web into ESI before the flash loop, while keeping the else-block bytes (they already match January). Test with scratch/w/hud_weapon/rv.py (it prints the x87/flash/statics signatures) and dbg/run_prio.py plus show.py (chooser costs). Also a source construct that yields January's jik x87 order: only ctf_init's first distance_squared3d site shows it. Land rwh_fidelity_candidate.patch only with the closure.

## task notes
Nothing was landed or made landable for credit. uwlp is exact, but only in the held form: `result` is left unassigned on the unreachable default arm. January's T+0x144 load of the never-stored home proves that form. Patches are provided for both tree states: before and after the hud_stack_check P5.

rwh has a new partial decode. The flash-loop and statics-loop allocation residuals share one cause: a +1 ESI neighbour preference in the C2 chooser. It comes from the copy of local_player_index into get_hud_state's private ESI argument; lab G1 and G2 flip both loops to January's registers. The x87 add-chain order is a separate residual. No evidence-backed source lever was found. Every TU name-count, prototype and dead-local oracle is inert for rwh.

The rwh fidelity candidate cannot land alone. It newly emits header COMDATs from a non-exact caller (P1 class).

Interaction with the concurrent hud_stack_check packet: the rwh bytes are identical with its inline check_stack_buffer. The uwlp owner body stays exact on top of it. My production-tree uwlp patch conflicts with P5 on one context line, so use the _after_P5 variant once P5 has landed.

Tools copied into the slug and run only there: Lane A dbg32c.exe (sha1 21c75281, our own debugger over our CL). Its outputs are confined to scratch/w/hud_weapon/dbg.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_weapon\LEDGER.md
