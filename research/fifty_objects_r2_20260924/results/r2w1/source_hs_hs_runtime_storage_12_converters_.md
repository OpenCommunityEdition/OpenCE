# source/hs/hs_runtime (storage: 12 converters)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime_storage\\hs_runtime_converter_storage.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime_storage\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime_storage\\cand.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime_storage\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime_storage\\LEDGER.md"
]

## production_changes
One patch: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime_storage\hs_runtime_converter_storage.patch. It is LF, has 12 hunks and changes 24 lines. It adds the `static ` prefix to the 12 prototypes (hs_runtime.c:469-491) and the 12 definitions (:1541-1647) of hs_long_to_boolean, hs_short_to_boolean, hs_string_to_boolean, hs_data_to_void, hs_short_to_real, hs_long_to_real, hs_enum_to_real, hs_real_to_short, hs_real_to_long, hs_long_to_short, hs_object_name_to_object_list and hs_object_to_object_list. `git apply --check` is clean in the worktree. GNU patch on the f6d00a8c blob reproduces the LF-normalised candidate exactly. No symbols.json edit is needed: lines 3203-3214 are already "static": true and the split has storage 3. No shared-header change, no config status flip (the object stays NonMatching at 64/66), and no parks to retire.

## evidence
All gate rows are unchanged: 64 exact, 2 residual, 0 unwritten, identical to baseline. Comparing the base object with the candidate, all 172 sections are identical in bytes, flags and relocations, and all 505 symbols match. The only difference is the 12 converters' storage class, which goes from 2 to 3. The base object is identical to build/base/source/hs/hs_runtime.obj. object_audit: symbol mismatches go from 12 to 0, and the total goes from FAIL(14) to FAIL(2), where the 2 are the untouched .text residuals. The PDB-storage check run on the candidate (pdb_storage_cand.py, same logic): disagreements go from 12 to 0. provider_link: FAIL(12) 'NO JANUARY PROVIDER' becomes PASS; the remaining 25 surplus rows pass in both link orders. surplus_identity: the 4 COMDATs are byte-identical to January's copies. The .data section (_hs_type_inspectors, 9804 B, containing typecasting_procedures) audits ok. January evidence: none of the 12 names is in the cachebeta publics. The only code publics in RVA 0xB9C00-0xBA600 are _hs_runtime_dispose, _hs_can_cast and _hs_cast; the converters sit at 0xBA220-0xBA38F. HCEX.pdb reports all 12 as `static function: true` with undecorated names `static long hs_<name>(long)`, while the control hs_cast has a PublicSymbol and no static flag. A grep over source/ and libs/ finds the 12 names only in hs_runtime.c: prototypes, the matrix and definitions. They are address-taken, so no private ABI is possible. The 20260904 reconciliation ledger's 'Deliberate linkage exception' said static helpers would be dead-stripped until the genuine matrix existed. The matrix is now present and audits ok, so that exception no longer applies. Composition check: the patch applies with offset -8 on the sibling's render_debug_trigger_volumes candidate c2.c. The combined file gates 65/1/0, and its object differs from the sibling's c2.obj only in the 12 storage classes. Files: LEDGER.md, MANIFEST.md, cand.c, cand.obj, gate_base.txt, gate_cand.txt, audit_base.txt, audit_cand.txt, pdb_storage_cand.txt, provider_link_base.txt, provider_link_cand.txt, hcex_all.txt, converter_rows.txt and objcmp_base_vs_cand.txt, all under C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime_storage\.

## blockers
Claim dependency: the whole unit source/hs/hs_runtime is held by sibling 'r2w1-hs_runtime::_render_debug_trigger_volumes (1088 B)'. The literal sub-key claim succeeded, so I worked only in scratch. The integrator must sequence this patch with the sibling's patch. They touch disjoint hunks: the sibling edits lines 1018-1131, and I verified the two compose byte-inertly. Object completion is still blocked by two things. First, _hs_evaluate_begin_random: a tie-shaped residual of 4 ModRM bytes, parked as private-register-convention and retired by triage. Second, _render_debug_trigger_volumes (the sibling's route). The 4 surplus header COMDATs (_scale_vector3d, _set_real_point3d, _set_real_vector3d, _vector_from_points3d) depend on that route under rule 6.

## reopen_criteria
Nothing to reopen for storage: the patch is complete and byte-inert. Whole-object admission becomes possible once the sibling's _render_debug_trigger_volumes route lands strict-exact with admissible COMDATs, and once _hs_evaluate_begin_random gets new first-party evidence (/Od or HCEX locals order) that breaks its 4-byte register tie. At that point, rerun the full section-4 audit on the combined candidate.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime_storage\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime_storage\LEDGER.md
