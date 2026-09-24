# source/interface/motion_sensor :: _motion_sensor_update (1168)

## verdict
BLOCKED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\cand_full.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\owner_gated_full.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\m4.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\od_motion_sensor_update.txt"
]

## production_changes
None landable (owner-gated). The strict-EXACT body (n4 m4, re-verified at 16542e46) is in cand_full.c / owner_gated_full.patch. It carries HCEX/Od names positions, local_player_true_index, num_recorded_blips, iter, center, dummy, and (m5) drops the player_index local as /Od 0x641c98 shows. Gate for cand_full: exact 17, residual 2 (_motion_sensor_update 1168/43 EXACT). object_audit: 44 January symbols, 0 differ. Its new COMDATs are identical to January's selected copies and pass provider_link both ways. If the owner admits it, unpark _motion_sensor_update and apply owner_gated_full.patch instead of production.patch; it contains production.patch. _update_motion_sensor then re-baselines to 864/39.

## evidence
The exact body keeps January's uninitialised positions[] read for a local player without a unit, with a BUG comment. NEW first-party evidence that this is an authentic January bug: the later /Od build (0x641b60) zeroes positions[i] (xorps stores at 0x641cf4) and re-tests the scan player's unit in the object loop (0x641e35..0x641e8c). So the code was fixed after January, the same class as king, glow and dead_camera. The fix-free spelling is impossible: n4 showed January's frame (0x284) and relocation multiset force this shape.

## blockers
Owner ruling on the authentic uninitialised-read BUG class. Rule (i) is then satisfied, because this exact function is the caller of the new COMDATs.

## reopen_criteria
The owner admits the BUG-commented uninitialised read. Land owner_gated_full.patch and unpark _motion_sensor_update in the same commit.

## task notes
Landable now: production.patch. It is zero exact credit: a structural fix to _render_motion_sensor (the slot swap is gone; the park re-baselines to 784/40, sha 69f4678045fb...), one less surplus COMDAT (`_power`), and a .bss symbol-order fix, so object_audit now shows 0 of 44 January symbols wrong. The patch touches only motion_sensor.c: no header, no consumer sweep, and no symbols.json change.

Dependency on the stack sentinel: none of the three residuals depends on check_stack_buffer (lab diagnostics e2 and e3 remove the whole sentinel and the residuals stay). The only sentinel-area byte difference in render is an alignment pad driven by code offsets. The object still needs the hud_stack_check lane's check_stack_buffer packet before it can be admitted.

Held by owner: owner_gated_full.patch gives exact 17 / residual 2. After an owner ruling, the only open rows would be the render scheduler tie and the update reassociation tie.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\motion_sensor\LEDGER.md. It records every probe with hypothesis, evidence, change and result: r1-r9 for render, u7-u10 for update, the m4/m5 re-verification, lab e1-e3, d1-d3, L1-L9 and bss1, and all name/local-count oracles. /Od dumps are in od_*.txt. The tree was clean before and after (`git status` empty); no tracked file was touched.
