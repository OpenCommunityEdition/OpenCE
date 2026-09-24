# REVIEW source/interface/motion_sensor :: .bss symbol layout (object admission)

approve: True

## per_object
[
 {
  "unit": "source/interface/motion_sensor (.bss hunk: motion_sensor_globals = NULL, sweep_theta = 0.f)",
  "approve": true,
  "issues": "None blocking. Gate unchanged at exact 16 / residual 3, and all 75 sections are coff-identical to base. object_audit symbols go from 2 differing to 0, and resolved .bss relocation targets now equal January's. Lab tests show both initialisers are required and the declaration order is January's. HCEX confirms the names, types and file-static storage. No new surplus; provider link PASS; data already 100%, so zero credit. Apply bss_only.patch from the reviewer slug (hunk 1 only); the render hunks of production.patch are out of scope."
 }
]

## checks
All checks ran at 16542e46; git status was empty before and after. Scope is only the first hunk of scratch/w/motion_sensor/production.patch. Reviewer slug: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_motion_sensor_____bss_symbol_layout__object_admission_\ (LEDGER.md, REVIEW.md).

(1) Candidate. Built cand_bss.c as the working-tree motion_sensor.c plus only hunk 1. The patch file is bss_only.patch, the worker's first hunk verbatim with a CRLF body. `git apply --check` passes, and `patch -o` onto the working-tree copy reproduces cand_bss.c byte for byte. The whole production.patch also applies and equals the worker's cand_prod.c.

(2) gate.py --all. Base and candidate give identical output: exact 16, residual 3 (motion_sensor_update, render_motion_sensor, update_motion_sensor, all pre-existing). No row lost.

(3) Object-level comparison. My objcmp.py (coff_compare.section_infos_equal over all 75 sections) finds 0 section differences between base and candidate, apart from the .debug$S path. The only symbol changes are _next_vertex_index 8->0 and _sweep_theta 0->8. My base compile also equals build/base/…/motion_sensor.obj on every section.

(4) object_audit. Base: 44 January symbols, 2 differ, FAIL(5). Candidate: 44 January symbols, 0 differ, FAIL(3). The 3 remaining failures are the residual .text functions; the .bss section is 13/13 ok.

(5) Lab proof of the VC7 .bss law in this TU:
- only motion_sensor_globals initialised: 3 symbols differ
- only sweep_theta initialised: 2 differ
- both spelled `= {0}`: 0 differ (the zero spelling is inert)
- both initialised, declared in swapped order: 2 differ
Both initialisers are necessary and sufficient, and the current declaration order is January's. The gate stays at 16/3 in all four variants.

(6) January side. config/contribs.json has the object's own 13-byte .bss contribution at 0x453AC8. It contains the public next_vertex_index at offset 0 and debug_motion_sensor_draw_all_units at 0xc, so both are initialised definitions, not COMMON, and no uninitialised static can come before offset 0. tinfo relocations show initialize storing the malloc result to .bss+4 and tick using .bss+8 as a float. That is consistent with the symbols.json attribution, so this is not an attribution defect.

(7) Resolved relocation targets (section owner + offset) of the 16 exact functions:
- Base differs from January in 6 functions; the candidate differs in 4.
- Every .bss reference now resolves identically to January.
- The 4 remaining differences are the pre-existing folded-literal class (__real@ and ??_C@ literals).

(8) pdb_storage: 0 disagreements. motion_sensor_globals and sweep_theta are absent from the cachebeta publics, so they are file statics, and the hunk keeps storage class 3.

(9) HCEX.pdb (DIA2Dump):
- motion_sensor_globals: File Static, `struct motion_sensor_globals_definition *`
- sweep_theta: File Static, float (= real)
Names, types and storage match. The atlas has no data entries for these names. I inspected the later /Od 2020 build's data layout; it is inconclusive because it comes from a modern compiler, and I did not rely on it.

(10) Surplus and link:
- provider_link with the candidate and --baseline=base.obj: no new surplus, PASS.
- Full provider_link: 14/14 PASS.
- surplus_identity: 9 COMDATs, 0 not identical, unchanged.

(11) Data: build/report.json shows motion_sensor data at 501/501 (100%), so zero credit, as the worker claimed. There are no semantic_data_matches, object_admission_rejections or parked.json entries that this hunk touches.

(12) House-rule review:
- STRIP TEST: removing the initialisers brings back the 2-symbol difference, so they are load-bearing. January's own .bss layout attests them under a measured compiler law, which is the established data-closure lever. They add no code store, filler, pragma or name, and change no semantics.
- Spelling follows precedent in the sibling HUD files: `= NULL` as in hud_unit.c and hud_weapon.c (also game.c, objects.c, game_allegiance.c), and `0.f` as in players.c. `real` is kept.
- The hunk changes no header, symbols.json, block scope, prototype, view cast or COMDAT, and falls in no owner-gated class. It does not overlap the held hud_unit stack-sentinel item or any other held item.

## issues
No blocking issues. The admission fix is verified and approved as zero-credit hygiene. It removes the 2-symbol .bss offset blocker: object_audit goes from 44/2 to 44/0 with no code, relocation or data change.

The integrator should apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_motion_sensor_____bss_symbol_layout__object_admission_\bss_only.patch. It is exactly hunk 1 of the worker's production.patch. The file has CRLF content lines, and `git apply --check` is clean at 16542e46.

The render_motion_sensor hunks in that same production.patch (the one-`weight`-local change, `pow`, and the corner/blip_pos renames) are NOT covered by this review. Apply them only under their own review.

Non-blocking observations, outside this hunk:
(a) HCEX types `next_vertex_index` as `short` while ours is `long`. It is unreferenced in the TU and byte-inert under the 4-byte static padding law. I left it untouched; it could be revisited later as type hygiene with January evidence.
(b) The object is still not complete. Three residual functions remain: _motion_sensor_update is owner-gated (authentic uninitialised-read bug class), and _render_motion_sensor and _update_motion_sensor are scheduler/sum-order residuals. This fix only removes the .bss symbol-offset blocker.
