# source/interface/hud_weapon::_hud_update_weapon_local_player

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\owner_uwlp.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\owner_gated_uwlp.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\owner_gated_uwlp_after_P5.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_weapon\\after_p5\\owner_hud_weapon.c"
]

## production_changes
None landable. There is an owner-gated exact form, ready and held. (a) scratch/w/hud_weapon/owner_gated_uwlp.patch: `git apply --check` is clean against 16542e46. (b) scratch/w/hud_weapon/owner_gated_uwlp_after_P5.patch: the same body for use after the hud_stack_check lane's P5 (hud_weapon.c shared stack frame) lands. The two conflict on the stack_buffer context line, so apply the one that matches the tree. Both replace only the uwlp body with the structural-lane body (claude-structural-20260923 REPORT) plus BUG-style comments. No header/config/symbols change and no new symbols (the owner_uwlp.obj symbol census equals build/base).

## evidence
Re-measured on 16542e46. s1.c/owner_uwlp.c: gate --all gives exact 14 / residual 2; the uwlp row is EXACT 1472/68, sha 0524ed47, and no other row moved; the _point_from_line3d guard passes. It is also exact on top of the stack-check lane's header inline (after_p5/gtu_owner.c, 14/16).

The shape is /Od-attested (fn 0x638150): jump-table case order, `long unit_index` local, `struct crosshair_state *state`, the redundant `(rem||loaded)&&` terms, and the primary trigger tested in fired_secondary_with_no_ammo (/Od 0x638a07 `and edx,0x800`; January T+0x3b7 `test ch,8`).

January byte proof of the uninitialised web: T+0x144 `mov esi,[ebp-0x28]` loads return_eip's home into result's register at loop entry. result's home is never stored, so the frame packer merged it with return_eip; the default arm (T+0x404 assert, falling to +0x424 `test si,si`) reads it. /Od 0x638ac2..0x638af3 has the same missing store and NO _RTC_UninitUse (its RTC descriptor lists only the two 64-byte arrays). Load-bearing controls (structural lane): restoring `result = FALSE` gives 1488 (wrong size); the secondary trigger bit gives 1488.

object_audit with owner_uwlp.obj: 23/23 January symbols ok, .bss ok, FAIL(2) = _crosshairs_draw and _render_weapon_hud only. pdb_storage: 0 disagreements.

## blockers
Brief section 7 / structural owner packet section 7: the exact form needs `result` unassigned on the default arm after match_assert(!"unreachable"). It is read only if system_exit returns, which is the authentic uninitialised-read class (same as _effect_allowed_by_environment). The primary-trigger test in the secondary state is authentic January and /Od behaviour, and the owner should also confirm it (production currently tests the secondary bit).

## reopen_criteria
The owner admits the unreachable-default unassigned-result form (and the primary-trigger test). Then apply owner_gated_uwlp.patch, or the _after_P5 variant if the stack-check packet has landed, and run the full sweep. No hud_weapon.h/header consumers are involved.

## task notes
Nothing was landed or made landable for credit. uwlp is exact, but only in the held form: `result` is left unassigned on the unreachable default arm. January's T+0x144 load of the never-stored home proves that form. Patches are provided for both tree states: before and after the hud_stack_check P5.

rwh has a new partial decode. The flash-loop and statics-loop allocation residuals share one cause: a +1 ESI neighbour preference in the C2 chooser. It comes from the copy of local_player_index into get_hud_state's private ESI argument; lab G1 and G2 flip both loops to January's registers. The x87 add-chain order is a separate residual. No evidence-backed source lever was found. Every TU name-count, prototype and dead-local oracle is inert for rwh.

The rwh fidelity candidate cannot land alone. It newly emits header COMDATs from a non-exact caller (P1 class).

Interaction with the concurrent hud_stack_check packet: the rwh bytes are identical with its inline check_stack_buffer. The uwlp owner body stays exact on top of it. My production-tree uwlp patch conflicts with P5 on one context line, so use the _after_P5 variant once P5 has landed.

Tools copied into the slug and run only there: Lane A dbg32c.exe (sha1 21c75281, our own debugger over our CL). Its outputs are confined to scratch/w/hud_weapon/dbg.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_weapon\LEDGER.md
