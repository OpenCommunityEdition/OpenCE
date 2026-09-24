# source/camera/editor_flying_camera

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_editor_camera_set_position_and_roll"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\editor_flying_camera\\F2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\editor_flying_camera\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\editor_flying_camera\\F2_static.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\editor_flying_camera\\storage_optional.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\editor_flying_camera\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\editor_flying_camera\\LEDGER.md"
]

## production_changes
PACKET A (required). Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\editor_flying_camera\production.patch (git apply --check is clean at 931ed8dc). Applied to HEAD it reproduces scratch/w/editor_flying_camera/F2.c exactly (LF-normalised compare). The patch touches only editor_camera_set_position_and_roll in source/camera/editor_flying_camera.c:
(a) The `if (!editor_camera)` arm comes first and calls `editor_camera_set_focus(point, (real_euler_angles2d const *)angles); initialized = TRUE;`.
(b) The else arm uses the /Od RTC local names in /Od declaration order: `real_matrix4x3 matrix; real_vector3d forward; real_vector3d left; real_vector3d up; real_vector3d diff; real_euler_angles2d hack_angles;`.
(c) After `normalize3d(&up);` it adds `cross_product3d(&up, &forward, &left); normalize3d(&left);` with a one-line comment. Then `cross_product3d(&up, &matrix.up, &diff); normalize3d(&diff);` and `roll = angle_between_vectors3d(&up, &matrix.up) * dot_product3d(&forward, &diff);`.
(d) The unit_offset component copy and `reset_all = TRUE;` are unchanged.
Config changes for the integrator:
- Retire the park in config/parked.json for unit source/camera/editor_flying_camera, function _editor_camera_set_position_and_roll (the entry at about lines 1392-1402).
- In config/config.json line 741, change status NonMatching to Matching once the full chain passes.
- In the config/semantic_data_matches.json camera-static-sections entry, the reason sentence 'The remaining camera code function stays residual.' becomes stale; reword or drop it. allow_incomplete_unit becomes unnecessary but is harmless.
No header change. No symbols.json change is needed for packet A.
PACKET B (optional and separable; fixes 2 storage disagreements that already exist in production). Apply scratch/w/editor_flying_camera/storage_optional.patch. It adds `static` to `editor_camera_speed` and `editor_camera_speed_steps`. It is apply-check clean alone and together with packet A. Use the edited full copy scratch/w/editor_flying_camera/config/symbols.json: lines 14089 (_editor_camera_speed_steps) and 22443 (_editor_camera_speed) each gain `, "static": true` in place. Then run a csplit regen and re-pin the grouped entry's _editor_camera_speed owner storage from 2 to 3 for both target and base. The integrator must re-measure objdiff 3.3.1 data credit, because a static anchor may be undercredited.

## evidence
January target: 432 bytes, 30 relocations. Baseline production: 464/30, zero credit. Three x87 load-order blocks remained in earlier waves: the second product of the cross i-term, and the k and j dot products.

KEY SOURCE: the later first-party /Od+RTC build (halo_cache_symbols.exe) contains this function at 0x527a20. It has an aligned prologue, which odbuild's `file` command misses; I found it with od_linear 0x5279f0..0x527de0. Its RTC descriptor 0x527c9c names the aggregates in declaration order: matrix, forward, left, up, diff, hack_angles. Its statement order includes `cross_product3d(&up, &forward, &left); normalize3d(&left);` before `cross_product3d(&up, &matrix.up, &diff)`, and `left` is never read. It passes the euler3d `angles` pointer unchanged to set_focus. Dump files: od_spar.txt and rtc_at.py.

MEASURED (gate --all, raw function sha):
- c2 (the w3 form) reproduces the 3 blocks.
- Local names alone (P1) and the dot argument order (P2) change no bytes.
- A hand copy of the /Od cross_product3d body (L1) changes no bytes. January's out-of-line _cross_product3d in action_obey is byte-identical to ours.
- L2 adds the dead left cross product: **21/21 EXACT**.
- F2 is L2 plus normalize3d(&left), i.e. the /Od text verbatim. It is byte-identical (sha 71253260755d0770): VC7 removes the dead inline call.
- Strip and sensitivity tests:
  - Removing the cast (S1) gives identical bytes plus a C4133 warning, so the cast is byte-inert.
  - Old names (S2), dot argument swap (S3), left declared last (S4), swapped left-cross arguments (T1) and the /Od aggregate unit_offset copy (T3) all give identical bytes.
  - Moving the left cross after the diff cross (S5) is NOT exact.
  - Control C1, a dead cross of unrelated vectors with the same local count, is NOT exact. The effect therefore depends on the content (CSE aging of the up.* and forward.* leaves), not on a count or ordinal. The corpus verdict for this function was MOD8-INSENSITIVE.

SECTION-4 AUDIT on F2.obj:
- object_audit PASS: 40 January-owned sections ok, 47 symbols with 0 differing.
- The surplus list matches production, and all 34 surplus sections are section_infos_equal to production. Production surplus_identity: 17 code COMDATs, 0 not identical.
- provider_link: SELECTED-PROVIDER LINK PASS (35 rows including strings).
- Every data section is identical to production. .bss and .data are credited by the existing camera-static-sections grouped entry; .rdata is 100% in report.json.
- fake_match_scan: 0 leads.
- /W3 adds no new warning.
- pdb_storage: 2 disagreements that already exist in production, unchanged by F2 (see packet B).

Packet B evidence:
- F2_static gate is 21/21.
- object_audit against the real split fails only on the 2 storage classes, as expected before a regen.
- Against an emulated split (the real split with only those 2 storage bytes patched from 2 to 3): gate 21/21 and object_audit PASS.

Files are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\editor_flying_camera\: LEDGER.md, F2.c, F2_gate_all.txt, F2_object_audit.txt, F2_provider_link.txt, production.patch, storage_optional.patch, config/symbols.json.

## blockers
No comparator blocker: the function is strict EXACT and the object audit and provider link pass. The reviewer must rule on two policy items:

(1) The euler3d-to-euler2d prefix cast at the set_focus call. It meets the brief section-2 criteria for a new view cast:
- /Od-attested pass-through at 0x527ab6..0x527abe;
- prefix-compatible unions (yaw at 0, pitch at 4);
- byte-inert strip test (S1);
- strictly exact caller;
- disclosed in the ledger.

(2) The `left` statement pair. It is dead in January's object (VC7 discards it), but it is the later first-party build's text verbatim (RTC name, declaration position, statement position, operands, normalize call). It is load-bearing (stripped form P2 is not exact), position-specific (S5) and content-specific (C1). I class it as attested source, not a filler or oracle local. If the owner reads the section-7 'dead named local' class as covering even /Od-verbatim dead statements, downgrade this unit to FUNCTIONS_EXACT_OBJECT_BLOCKED with that ruling as the blocker.

Two storage disagreements with cachebeta publics already exist in production (_editor_camera_speed and _editor_camera_speed_steps are external but not PDB publics; HCEX names .data+0 as the file-static `speed`). They are the compromise the static-split lane accepted; packet B fixes them but needs a csplit regen and a data re-pin.

## reopen_criteria
None for code. For packet B: reopen if the regenerated split plus objdiff 3.3.1 undercredits the static-anchored .data or .rdata sections, or if the grouped data verifier rejects the storage-3 owner. For policy: reopen if the owner rejects the /Od-verbatim dead `left` pair. The only other explanation measured was aging of the same leaves by other dead code, and that would be a filler.

## task notes
Mechanism found. The x87 operand order for frame-local aggregate leaves follows CSE leaf aging: an operand whose address tree was already evaluated loads second. A dead inlined cross_product3d over (up, forward) ages exactly the leaves whose load order differed.

The missing statement came from the later /Od build's function at 0x527a20. `odbuild.py file` misses it because of the aligned-frame prologue (push ebx; mov ebx,esp; and esp,-8); od_linear finds it.

VC7 also deletes a dead call to an inline header helper (normalize3d(&left)). So the earlier assumption "January has 3 normalize calls, so no normalize(&left)" was wrong. A call count taken from January's bytes cannot exclude dead calls to inline helpers.

My first probe bodies had single-backslash __FILE__ literals because of heredoc escaping. I fixed and rebuilt them; the x87 conclusions are unaffected. No tracked file was edited. I briefly created and then deleted a temp copy of object_audit under scratch/tools to run the emulated audit.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\editor_flying_camera\LEDGER.md
