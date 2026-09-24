# source/interface/motion_sensor :: .bss symbol layout (object admission)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\cand_prod.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\motion_sensor\\lab\\bss1.c"
]

## production_changes
Part of C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\motion_sensor\production.patch (the first hunk; `git apply --check` is clean at 16542e46). In source/interface/motion_sensor.c, `static struct motion_sensor_globals_definition *motion_sensor_globals;` becomes `... = NULL;` and `static real sweep_theta;` becomes `static real sweep_theta = 0.f;`. There are no header, symbols.json or config changes.

## evidence
object_audit on the current build reported 2 of 44 January symbols wrong. January's .bss is next_vertex_index@0, motion_sensor_globals@4, sweep_theta@8, debug_motion_sensor_draw_all_units@0xc; ours put sweep_theta@0 and next_vertex_index@8. By the VC7 .bss law, uninitialised statics come first in name-hash order and `= 0` statics follow in declaration order. The public, `= {0}`-initialised next_vertex_index sits at offset 0 in January, so January's two statics must also have been initialised. With the initialisers, object_audit gives 44 January symbols and 0 differences. The gate stays at exact 16 / residual 3 with no row lost, and pdb_storage gives 0 disagreements. House precedent for the spelling: game.c `static ... *game_globals = NULL;`, players.c `static real ... = 0.f;`. Data was already 100% in report.json, so this earns zero byte credit. It only removes a symbol-offset blocker for completing the object.

## blockers
None for this change. The object still needs the three functions below, plus the hud_stack_check lane's check_stack_buffer packet (held class: the hand-copied sentinel with invented gotos).

## reopen_criteria
n/a

## task notes
Landable now: production.patch. It is zero exact credit: a structural fix to _render_motion_sensor (the slot swap is gone; the park re-baselines to 784/40, sha 69f4678045fb...), one less surplus COMDAT (`_power`), and a .bss symbol-order fix, so object_audit now shows 0 of 44 January symbols wrong. The patch touches only motion_sensor.c: no header, no consumer sweep, and no symbols.json change.

Dependency on the stack sentinel: none of the three residuals depends on check_stack_buffer (lab diagnostics e2 and e3 remove the whole sentinel and the residuals stay). The only sentinel-area byte difference in render is an alignment pad driven by code offsets. The object still needs the hud_stack_check lane's check_stack_buffer packet before it can be admitted.

Held by owner: owner_gated_full.patch gives exact 17 / residual 2. After an owner ruling, the only open rows would be the render scheduler tie and the update reassociation tie.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\motion_sensor\LEDGER.md. It records every probe with hypothesis, evidence, change and result: r1-r9 for render, u7-u10 for update, the m4/m5 re-verification, lab e1-e3, d1-d3, L1-L9 and bss1, and all name/local-count oracles. /Od dumps are in od_*.txt. The tree was clean before and after (`git status` empty); no tracked file was touched.
