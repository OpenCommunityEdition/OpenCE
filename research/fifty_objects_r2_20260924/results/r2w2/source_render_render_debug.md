# source/render/render_debug (admit source/render/render_debug)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_render_debug\\patches\\05_render_debug_genuine_helpers_and_vehicle.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_render_debug\\patches\\06_render_debug_tentative_debug_toggles.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_render_debug\\files\\c06\\source\\render\\render_debug.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_render_debug\\files\\c06\\source\\units\\vehicle_datum.h",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_render_debug\\sweep\\c06\\source\\render\\render_debug.obj"
]

## production_changes
Zero credit. Two patches to apply one at a time from the repo root.

(1) scratch/w/r2w2_render_debug/patches/05_render_debug_genuine_helpers_and_vehicle.patch (sha256 1303b91b...) changes render_debug.c and units/vehicle_datum.h:
- three rule-6 hand expansions become real_math.h point_from_line3d calls (render_debug_vector, render_debug_tick without its negative_tick_size temporary, render_debug_line_offset);
- the caller-local vehicle_runtime_datum/render_debug_vehicle_data view (opaque unknown* spans) and its 4 offset checks are deleted;
- render_debug_player uses the owner type struct vehicle_datum through a new typed macro, vehicle_datum_try_and_get, placed in vehicle_datum.h next to vehicle_datum_get;
- the dead REAL_MATH_EXTERNAL_{ARCTANGENT,DOT_PRODUCT3D,PLANE3D_DISTANCE_TO_POINT} defines are removed.

(2) scratch/w/r2w2_render_debug/patches/06_render_debug_tentative_debug_toggles.patch (sha256 c8a386b4...): the six `extern boolean debug_*;` lines become bare tentative definitions `boolean debug_*;`.

No change to symbols.json, config, parks, semantic entries or real_math.h. The change lines are identical to the round-1 05/06; only hunk line numbers moved (+1, from 7979cf8f).

## evidence
All outputs are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_render_debug\ (MANIFEST.md, LEDGER.md).

Gate and audit:
- Gate: 36/36 EXACT for production, c05 and c06. Per-function size, reloc and sha rows are identical to production.
- gate.py itself cannot shadow a modified shared header, so the candidate was compiled in a shadow tree and scored with gate.py's row logic (tools/gate_obj.py). gate.py on a lab copy that defines the macro locally also gives 36/36.
- Full build sweep: all 612 TUs compiled from shadow trees. The control is 612/612 identical to build/base. Candidates are 611/612 identical; only render_debug.obj differs. vehicle_datum.h includers actor_moving, physics and vehicles are byte-identical.
- object_audit against build/split (symbols.json is unchanged): PASS, 63 January symbols, 0 differ. All 27 data sections (26 .rdata + .bss 29708) are ok. The only new surplus row is +.text _point_from_line3d 48.
- pdb_storage: 0 disagreements.
- surplus_identity: 12 candidate-only code COMDATs, all IDENTICAL. _point_from_line3d (48 B, 0 relocs, sha 9b763841) equals January's only definer, action_charge.
- provider_link: PASS on all 39 surplus rows in both orders, including _point_from_line3d vs action_charge. nodup_census: 26 SELECT_ANY definers, 0 NODUP.
- objdiff 3.3.1 mini project: identical to production (code 8235/9448, 34/36 on the 2 known $L under-credit rows; data 30236/30236).

Precedent for 05 (house_rules.md:33-40, 3c74fa36; handoff ruling #2 at lines 69-75), every condition holds:
- genuine shared-header __inline;
- no hand expansion (the patch removes three);
- COMDAT byte-identical to January's selected copy;
- callers strictly exact (vector 192/13, tick 112/1, line_offset 128/2);
- full sweep with zero regressions;
- selected-provider link passes in both orders.
January render_debug has no out-of-line reference to the helper (all-inlined class). hs_runtime (05255584) landed the same class in this lane.
/Od attestation: 0x84a920, 0x84a510 and 0x848640 make REAL calls to point_from_line3d (0x42e0d0) with exactly the patch's arguments. The asserts sit at lines 388-390, zero drift from ours.

Precedent for 06 (objects.c tentative definitions, Matching, fb0d8145), every condition holds:
- The six are January linker_common records 50-55, 1 byte each, referenced only by render_debug and hs_globals_external.
- New board census: 74 January COMMON records have exactly the {hs_globals_external, X} referencer shape. Canonical defines 15 of them, all in X, all bare `boolean name;` (units x8, structure_visibility x2, objects x5, all Matching). 0 are defined outside X.
- Per-symbol evidence, not adjacency:
  - In the 2001-09-25 retail cache.map, render_debug.obj is not linked and all six are absent, while hs_globals_external.obj is linked and still carries the debug_bsp name literal.
  - debug_permanent_decals first appears together with render_debug_structure_decals (08-15 beta: 5 toggles; 09-25 beta: 6).
  - In the /Od build, the toggles at 0xf0afb0-b7 are interleaved with render_debug's own cache .bss (0xf0afb4/b8/bc/c0). Each toggle is read by exactly one render_debug function.
- The only COFF change is six section-0 symbols going from value 0 to 1 (COMMON 1 = the January record size); sections are byte-identical.
- All six are cachebeta publics.
- Links: with January linker_common.obj and with base hs_globals_external, both orders, 0 LNK2005/LNK1169 and all six resolved (production leaves 6 unresolved).

Apply checks: `git apply --check` gives rc 0 for 05 and 06 at HEAD. Sequential `git apply --directory` into LF and CRLF copies reproduces the candidate modulo EOL. git status is unchanged (only the pre-existing untracked research/fifty_objects_r2_20260924/).

## blockers
None for 05/06 as zero-credit admission hygiene.

The integrator should confirm the 06 evidence standard. It is per-symbol (map presence/absence, /Od interleave, sole non-hs referencer, 15 uncontradicted canonical rows of the same shape), not the pool-adjacency standard that was excluded on 2026-09-22.

Whole-object completion is NOT reached by this packet. It still needs the owner ruling Q2d on the invented render_debug_globals_definition aggregate with its opaque pad members (D0: admit it as is; D1: seven `= 0` descriptive statics, lab in research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/lab/), then patch 08 (config status flip). This is a held owner-queue class. No first-party names exist for the cache variables: the 2001 maps' Static symbols sections list only render_debug functions, and there are none in cachebeta publics, HCEX or the atlas.

## reopen_criteria
Object completion reopens when:
- the owner answers Q2d with D0 or D1; then apply the lab D1 pair if D1, then patch 08 (render_debug NonMatching -> Matching, +1 object); or
- a first-party name source for the render_debug cache variables appears.

If the Codex packet reserved for math/real_math changes point_from_line3d's body, re-run surplus_identity and provider_link for every emitter, this one included, before landing 05.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_render_debug\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_render_debug\LEDGER.md
