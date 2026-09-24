# source/objects/widgets/glow

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_get_particle_world_position (in the candidate only; owner-gated, not landable)"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\cand_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\new\\config\\parked.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\new\\config\\config.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\cand_fixed.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\od_gpwp.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\hcex_gpwp.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\object_audit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\provider_link.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\glow\\surplus_identity_cand.txt"
]

## production_changes
OWNER-GATED: do not land without an owner ruling. The patch is C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\glow\production.patch. `git apply --check` is clean at 931ed8dc, and a trial apply on fresh copies reproduces the candidate files byte-for-byte. It makes three changes:
(1) source/objects/widgets/glow.c: the body of get_particle_world_position is replaced with the body in scratch/w/glow/cand_final.c. The full candidate file is that same path; it is the real file with only this function changed. The new body:
  - has January's frame of 196 bytes. Locals use the HCEX and /Od RTC names: traversal_points, traversal_up_vectors, traversal_left_vectors, traversal_time_indices, `forward` (the vector_from_points3d delta), u and v (the up and left spline results), plus named sine and cosine.
  - spells the 2- and 3-marker interpolation as vector_from_points3d plus point_from_parametric_line.
  - makes spline 1 write directly into particle->position. The point/vector casts are byte-inert.
  - copies the knot as the last statement of the default-case loop.
  - uses the /Od form `marker_index = PIN(...); particle->parent_marker_index = marker_index;`, which is byte-inert.
  - carries two `/* BUG (preserved for exact matching) */` comments, one for each defect.
(2) config/parked.json: the glow `_get_particle_world_position` park entry is removed (lines 1181-1191).
(3) config/config.json: source/objects/widgets/glow.c goes from NonMatching to Matching.
The edited JSON copies are in scratch/w/glow/new/config/. symbols.json does not change.
Fallback, only if the owner declines: re-baseline the park to scratch/w/glow/cand_fixed.c at zero credit. This is a defined-C corrected body in the same /Od idiom: 2544/55, same frame as January, all 24 sibling functions still exact. It replaces the current 2528/55 body, whose 0xf4 frame comes from an invented position_vectors staging array. Adopting it is the integrator's call, and the park measurements must be re-pinned.

## evidence
All gate runs used `gate.py --all --forbid-emitted-symbol _point_from_line3d`.

Baseline at 931ed8dc: exact 24 / residual 1 / unwritten 0; the residual is `_get_particle_world_position [size 2528!=1744, relocs 55!=43, sha]`.

cand_final.c: exact 25 / residual 0 / unwritten 0, and the point guard passes (rows in scratch/w/glow/cand_final.rows).

Section-4 audit on cand_final.obj:
- object_audit: PASS. 36 January symbols, 0 differ; every January .text and .rdata section is ok.
- Candidate-only surplus is the same list build/base already has: 10 real_math.h SELECT_ANY code COMDATs, the real_math.h assert string, __real constants and .drectve.
- surplus_identity, run against the candidate object: 10 of 10 COMDATs are IDENTICAL to January's selected providers.
- provider_link: SELECTED-PROVIDER LINK PASS for all 20 surplus symbols.
- pdb_storage: 0 disagreements across 36 symbols.
- Data: build/report.json shows glow .rdata at 392/392 (100%). There is no .data or .bss.
- There is no object_admission_rejections entry for glow.
- fake_match_scan: 0 leads.

New evidence this wave:
- The later first-party /Od build (function 0x798de0, dump in scratch/w/glow/od_gpwp.txt) contains both defects verbatim:
  - The 2-marker arm and both 3-marker intervals never write traversal_left_vectors: no cross_product3d, no interpolation.
  - On 3-marker interval 1 it stores `knots[2] = (knots[2]-knots[0])*0.5+knots[0]`, so knots[1] is never set.
  - The third spline call that reads the left vectors is unconditional.
  - It also shows the vector_from_points3d / point_from_parametric_line routing, applied to the up vectors as well. The helper at 0x79ce50 carries the y-base z quirk.
  - Its RTC descriptor names the locals, including `forward`.
  - This double-attests the w1/w3d/n3a January proof: these are authentic source defects, not a January compile artifact.
- HCEX.pdb (scratch/w/glow/hcex_gpwp.txt) supplies the local names and types. v shares slot 0xC0 with traversal_time_indices, so v is the last (left-vector) spline result.
- Strip test: removing every point/vector cast is still 25/25 exact, so the casts are type-only.

Probes (full list in LEDGER.md):
- Authentic names: byte-inert, 25/25.
- The /Od PIN form: byte-inert, 25/25.
- The /Od short marker-order local, at block or function scope: 24/1. There are 2 real regions, an x87 flip at +0x670 (M8), so January did not have that local.
- Corrected defined body: 2544/55, 745 vs 566 instructions, 71 real regions. This matches the n3a result that any defined fill of the side vectors adds 592 or more bytes and 12 constant relocations.

## blockers
Owner policy gate (brief section 7): the only exact body reproduces two authentic original uninitialised reads from address-taken automatic arrays:
- traversal_left_vectors[0..3] are never filled on the 2- and 3-marker paths, but the unconditional third nonuniform_cubic_spline_vector3d call reads them.
- traversal_time_indices[1] is never set on 3-marker interval 1.
A defined-C body cannot reach 1744/43: n3a v3 and this wave's P8 measure +592 to +800 bytes and +12 float-constant relocations. The earlier precondition reopen branch (that 2-/3-marker glows never reach the side spline) is refuted, because the marker count is model data from 0 to 5, the callers guard on >1, and the assert checks >1. By this lane's rules the object cannot land until the owner rules. The owner's 2026-09-20 ruling (from Lane A notes, not re-verified this wave) would allow a target-proven uninitialised read once the function is strict exact, which it now is. That supports admitting it.

## reopen_criteria
Land production.patch unchanged once the owner admits these two BUG-commented original defects for get_particle_world_position. The evidence now includes January bytes, the relocation census, and the later first-party /Od build carrying both defects. If the owner declines, the only remaining action is the optional zero-credit park re-baseline to cand_fixed.c. No other source shape or evidence can move this row. Do not repeat these probes:
- the /Od short marker-order local
- the w1/n3a do-not-repeat list
- any defined-C side-vector fill

## task notes
glow.obj is complete in the candidate (25/25) and passes the whole section-4 audit, but the one exact body depends on two authentic original uninitialised reads, so it needs the owner's ruling. Treat it like dead_camera in batch 1. The new evidence strengthens the admission case: the later first-party /Od build has both defects verbatim, and the casts are byte-inert. The patch also adopts authentic HCEX/RTC local names and the /Od PIN statement form; both are byte-inert. No tracked file was edited.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\glow\LEDGER.md
