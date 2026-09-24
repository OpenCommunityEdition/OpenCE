# review source/camera/editor_flying_camera
approve=True

## per_object
[
 {
  "unit": "source/camera/editor_flying_camera (packet A: production.patch)",
  "approve": true,
  "issues": "None blocking. 21/21 strict EXACT, object_audit PASS, surplus identical to production, provider link PASS, data unchanged and still credited by the existing grouped entry. The integrator must record in the ledger the rulings on the attested dead `left` pair and the byte-inert euler view cast. Config ops: retire the park, config.json:741 to Matching after the full chain, reword the stale grouped-entry reason sentence."
 },
 {
  "unit": "source/camera/editor_flying_camera (packet B: storage_optional.patch + symbols.json lines 14089/22443)",
  "approve": true,
  "issues": "Verified with a real scratch csplit run: only this object changes, 40/40 sections exact, audit PASS, objdiff 3.3.1 credit unchanged. Required in the same commit: the camera-static-sections _editor_camera_speed owner.storage re-pin from 2 to 3 for target and base. Fixes the 2 storage-class disagreements with cachebeta publics that already exist in production."
 }
]

## checks
All results are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_editor_flying_camera\REVIEW.md. The tracked tree was never modified.

1. Patches. HEAD is 931ed8dc. `git apply --check --whitespace=error-all` is clean for production.patch, for storage_optional.patch, and for both together. HEAD blob plus production.patch gives cand.c, which is byte-identical to the worker's F2.c after LF normalisation. cand.c plus the storage patch gives candB.c, which is identical to F2_static.c.

2. gate.py --all on cand.c against the real split: 21/21 EXACT. _editor_camera_set_position_and_roll is 432 bytes with 30 relocations, raw sha 71253260755d0770 (same as the worker's F2.obj).

3. object_audit on cand.obj: PASS (47 January symbols, 0 differ). Surplus list is identical to production. Comparing all 81 sections of cand.obj and production with section_infos_equal, only the function and .debug$S differ; .debug$S is not January-owned. surplus_identity: 17 code COMDATs, 0 not identical. provider_link: PASS for cand.obj and for candB.obj.

4. /Od build re-read.
   - od_spar.txt plus rtc_at.py confirm the RTC names at 0x527c9c: matrix, forward, left, up, diff, hack_angles.
   - The statement order matches the worker's account: cross_product3d(&up, &forward, &left) at 0x527bb5, then normalize3d(&left), then the diff cross product.
   - The slot at -0x68 (`left`) is never read afterwards, so `left` is dead in /Od too.
   - The angles pointer is pushed unchanged to 0x527700, which is editor_camera_set_focus (asserts "position"/"angles" at lines 133/134).

5. NEW independent evidence: assert line anchors.
   - January to /Od line offsets are +4 for get_focus, set_focus, move_to_point, set_position, set_position_and_roll (169/170 -> 173/174) and set_mode (302/308 -> 306/312).
   - They are +5 from update (340) onward.
   - There is zero drift across the whole set_position_and_roll body. That span in the later build contains the `left` declaration and the cross/normalize pair, so January's same span has the same line count. This supports the pair being in January's source, not a later addition.

6. My strip and control tests (gate --all):
   - R1, `left` plus the pair removed: residual, sha c334539c (the 3 x87 blocks). The pair is load-bearing.
   - R2, normalize3d(&left) dropped: identical bytes.
   - R3, cast dropped: identical bytes plus a C4133 warning. The cast is byte-inert.
   - R4, declaration only: residual. It is not a count effect.
   - R5, cross_product3d(&forward, &matrix.up, &left): residual. The effect depends on which vectors are read.
   - R6, pair moved before normalize3d(&forward): 21/21. The /Od position is inside the working window.

7. Warnings and fake scan. /W3: cand and candB produce the same warnings as HEAD. fake_match_scan: 0 leads.

8. Data.
   - objdiff 3.3.1 (sha1 3130e428) was run on a scratch project. The production row reproduces build/report.json exactly (.bss 50, .data 22.24, .rdata 100).
   - cand: code 5065/5065 and 21/21 functions, data unchanged.
   - Using tools.semantic_progress, the camera-static-sections member snapshots are unchanged for packet A.

9. Packet B, verified with a real csplit run (not the emulation).
   - I ran build/tools/csplit.exe into scratch with a config copy. The control run with the unmodified config was byte-identical to build/split.
   - With the worker's symbols.json, only editor_flying_camera.obj changes among the 833 objects. It is byte-identical to the worker's emulated split.
   - candB against that split: 40/40 January sections exact. object_audit pointed at it: PASS (47/0).
   - objdiff 3.3.1 data percentages are unchanged; .rdata stays 100%.
   - The grouped-entry snapshot differs only in the _editor_camera_speed owner storage, 2 -> 3, for both target and base.
   - cachebeta_publics.txt does not list _editor_camera_speed or _editor_camera_speed_steps, so they are file-static; _editor_custom_render is public.
   - No other split or base object names either symbol. The symbols.json copy differs only at lines 14089 and 22443, edited in place.

10. Config targets checked: the parked.json entry is at lines 1392-1402 and config.json line 741 is the editor_flying_camera NonMatching row. The unit has no admission-rejection entry, and allow_incomplete_unit only relaxes a check (semantic_progress.py:678).

## issues
No technical defect found; every measured claim in the proposal reproduced. Policy rulings and conditions for the integrator:

(1) Dead `left` pair. It is dead in January (VC7 removes it) and it fails a literal strip test. I rule it ADMISSIBLE, not a section-7 filler:
   - the first-party /Od build has the same RTC name and the same statement pair, with the same arguments, position and declaration order;
   - January's own assert line anchors show zero drift across the function body;
   - the effect depends on which vectors are read (R5 and the worker's C1 are not exact);
   - it is not a declaration or count effect (R4 is not exact).
   This fits the "independent evidence authenticates that exact construct" standard. The in-source comment discloses that `left` is never read. The integrator must record this ruling and the anchor-drift evidence in the object ledger. Escalate to the owner only if section 7 is read as covering attested dead statements.

(2) euler3d->euler2d cast. It meets the brief's section-2 criteria: per-site /Od pass-through, compatible yaw/pitch prefix, byte-inert strip test, and a strictly exact caller. It is not owner-gated. It is the first cast of this kind in the tree, so disclose it in the docs ledger. The parked.json evidence sentence that favours "copying instead of punning" is superseded: the copy form is not January's code (464 bytes).

(3) Landing conditions.
   - Packet A: retire the parked.json entry _editor_camera_set_position_and_roll (source/camera/editor_flying_camera). Change config.json line 741 from NonMatching to Matching only after the full chain passes. Reword the stale "remaining camera code function stays residual" sentence in the camera-static-sections reason.
   - Packet B: apply it in the same commit as the symbols.json lines 14089 and 22443. Re-pin the grouped entry's _editor_camera_speed owner.storage from 2 to 3 for BOTH target and base, or progress will fail with "snapshot changed". Its objdiff 3.3.1 credit is already measured unchanged.
   - Recommend landing A and B together. With A alone, the object still has 2 storage-class disagreements with cachebeta publics; they already exist in production and A does not introduce them.

(4) Optional follow-up, not blocking: HCEX names the .data+0 file static `speed`, while the project keeps the name editor_camera_speed.
