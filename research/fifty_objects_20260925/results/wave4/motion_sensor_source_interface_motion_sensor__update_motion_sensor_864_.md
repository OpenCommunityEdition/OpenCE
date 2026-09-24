# source/interface/motion_sensor :: _update_motion_sensor (864)

## verdict
NO_PROGRESS

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\u9.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\cand_full.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\owner_gated_full.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\od_update_motion_sensor.txt"
]

## production_changes
None landable. A held candidate body is in owner_gated_full.patch / cand_full.c: the /Od-faithful u9 body, 864/39, sha 0dcb6bf20658cf6fc67d2948266ab15da7ff49e2cf3d2d2f33d343cb2fe7cf1c. It newly emits the _distance_squared3d and _vector_from_points3d COMDATs. Both are byte-identical to January's selected action_alert copies and pass provider_link in both orders, but rule (i) admits a new header COMDAT only from a strictly exact caller. So this body can land only together with the exact _motion_sensor_update (the owner ruling below), or once it is itself exact.

## evidence
/Od 0x642ff0 (od_update_motion_sensor.txt) plus RTC and HCEX names (reference_point x2, unit_center, player_center, point, dummy, customs, custom_temp; HCEX puts the first reference_point and unit_center at the same offset). These fix the statement order and the local set: no player_index local; short loop-1 counter with a blips pointer; separate long loop-2 counter; customs[i] indexed. u9 reproduces the w3c u7 result on the current tree. The only difference is 8 bytes at 0x16b..0x178 in loop 1: January sums (jj+ii)+kk, ours (ii+kk)+jj. Loop 2 matches. Measured inert: u8 /Od declaration order; u9 local set; u10 dead named game_time_get local; dead-local oracles (long K=0..16, real K=0..8), a used copy-chain oracle (long K=0..10, real K=0..8) and a TU extern oracle (K=0..40), all one sha, so this is not M8, H3 or a name-count tie; lab L3/L4/L5/L7/L8/L9 statement and argument permutations. Only L1 (point computation removed, not admissible) moves the order, to k,j,i. Diagnostic e3 removes the stack sentinel and the order is unchanged, so there is NO check_stack_buffer dependency.

## blockers
(1) The loop-1 reassociation order of the inlined distance_squared3d, which depends on the preceding point.x/point.y computation. (2) The P1/rule-(i) COMDAT admission, coupled to _motion_sensor_update.

## reopen_criteria
A decoded rule for VC7 FP reassociation ranking (which addend pairs first), or first-party evidence of a different loop-1 data flow. After the owner rules on _motion_sensor_update, land u9 as a zero-credit park re-baseline (864/39, sha 0dcb6bf2...).

## task notes
Landable now: production.patch. It is zero exact credit: a structural fix to _render_motion_sensor (the slot swap is gone; the park re-baselines to 784/40, sha 69f4678045fb...), one less surplus COMDAT (`_power`), and a .bss symbol-order fix, so object_audit now shows 0 of 44 January symbols wrong. The patch touches only motion_sensor.c: no header, no consumer sweep, and no symbols.json change.

Dependency on the stack sentinel: none of the three residuals depends on check_stack_buffer (lab diagnostics e2 and e3 remove the whole sentinel and the residuals stay). The only sentinel-area byte difference in render is an alignment pad driven by code offsets. The object still needs the hud_stack_check lane's check_stack_buffer packet before it can be admitted.

Held by owner: owner_gated_full.patch gives exact 17 / residual 2. After an owner ruling, the only open rows would be the render scheduler tie and the update reassociation tie.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\motion_sensor\LEDGER.md. It records every probe with hypothesis, evidence, change and result: r1-r9 for render, u7-u10 for update, the m4/m5 re-verification, lab e1-e3, d1-d3, L1-L9 and bss1, and all name/local-count oracles. /Od dumps are in od_*.txt. The tree was clean before and after (`git status` empty); no tracked file was touched.
