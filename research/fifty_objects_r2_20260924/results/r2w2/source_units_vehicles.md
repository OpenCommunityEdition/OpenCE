# source/units/vehicles (admit source/units/vehicles)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\cand.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\patches\\01_vehicles_fallback.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\patches\\02_symbols_turret_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\patches\\optional\\03_OPTIONAL_pathfinding_scale_vector3d.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_vehicles\\LEDGER.md"
]

## production_changes
Zero-credit admission fix; the integrator applies it. No new strict function. The object stays 37/39 and NonMatching.

Patches, in apply order (both pass git apply --check at HEAD):
(01) C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\patches\01_vehicles_fallback.patch, sha1 7e45032c:
  - deletes the #define/#undef REAL_MATH_EXTERNAL_POINT_FROM_LINE3D pair around the vehicles.h / real_math.h includes;
  - in vehicle_find_pathfinding_surface_index, replaces the hand expansion with `point_from_line3d(&origin, global_up3d, 0.4f, &origin);` and `point_from_line3d(&origin, &vector, result.t, position);`;
  - drops the `real t` staging local and the stale 'Preserve the January inline schedule' comment.
(02) C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\patches\02_symbols_turret_static.patch, sha1 2a100d69: config/symbols.json line 7052, in place, adds `"static": true` to _update_turret_physics.

The hunks are byte-identical to the round-1 files. I regenerated only the stale index lines: the symbols pre-image was 3da39456 but HEAD is f9abb17d, and the vehicles post-image was recorded as e6e72f2e but is really e6710a63. Applying both patches to the HEAD blobs reproduces cand.c (e6710a63) and the patched symbols.json (c602dcdc).

Integrator sequence: apply 01 and 02, csplit-only regen, then a full ninja. config.json status stays NonMatching.

OPTIONAL, not part of the covered packet: patches\optional\03_OPTIONAL_pathfinding_scale_vector3d.patch (sha1 3639cd62) replaces add_vectors3d(global_down3d, global_down3d, &vector) with the /Od-attested scale_vector3d(global_down3d, 2.0f, &vector). /Od 0x8fda65 calls 0x42e220, which is scale_vector3d. The whole object stays section- and symbol-identical, and the /W3 set is unchanged.

## evidence
Governing ruling, quoted in MANIFEST: HANDOFF_20260920.md:65-68, "a TU may emit it iff January's SAME object REFERENCES it out of line". :156-162 lists vehicles among the 17. Every condition holds:
(1) Out-of-line reference. January vehicles.obj holds an UNDEF _point_from_line3d with exactly one relocation, at _vehicle_update+0x268. The split census matches the 17-object list, with action_charge as definer. The candidate keeps exactly that one relocation; production has an extra one at fighter+0x138.
(2) Form matches the four canonical landings (59925575, 6099e659, bf937aac, 4f4c8502): plain removal of the guard, no source comment.
(3) COMDAT identical. _point_from_line3d is 48/0/9b763841f851 in the candidate and in January's action_charge copy (surplus_identity IDENTICAL; 28 candidate-only COMDATs, 0 not identical).
(4) The emitting caller is strictly exact:
  - _vehicle_update (out-of-line) and pathfinding (2 inline sites) are both EXACT.
  - Lab lab_nofighteruse.c deletes the non-exact fighter's pre-existing use; the COMDAT is still emitted and section-identical.
  - So the P1 class (new header COMDAT from a non-exact caller) does not apply.
(5) Provider link. provider_link --baseline: PASS against action_charge in both orders. Full run: 86/86 rows PASS. nodup_census: 26 ANY definers, 0 NODUP.
(6) Zero regressions:
  - gate --all is 37/2 on base and 37/2 on the candidate; no row changes verdict.
  - The base-vs-candidate owner-keyed diff shows only the fighter bytes changed plus the new COMDAT.
  - No header is touched, and the emulated split changes only vehicles.obj.
(7) Storage per cachebeta publics (05255584 precedent). _update_turret_physics (0x1A4F50) is absent from the publics, while its neighbours _vehicle_hover, _vehicle_is_flipped and _set_real_quaternion are present. No relocation names it anywhere in the split, so it is inlined at its only site. pdb_storage: production 1 disagreement, candidate 0.

Split emulation (csplit -p cfg_X): split_ctl matches build/split on 833/833 files. split_T differs only in vehicles.obj: 68/68 sections equal, and the one changed symbol row is turret storage 2->3.

object_audit:
  - Candidate vs split_T: 66 January-owned sections ok (28 .rdata, .data _vehicle_update_section, 37 .text) and 0/68 symbols differ. FAIL(2) is only the held fighter and scout.
  - Base vs build/split: FAIL(3).
  - Surplus delta: exactly +_point_from_line3d, 48 bytes.

Data: tinfo --data is identical to production apart from section numbers and the .debug$S path; report.json data is 1744/1744.

/Od fn 0x8fd970 (pathfinding) calls 0x42e0d0 at 0x8fda42 with args (&origin, global_up3d, 0.4f, &origin) and at 0x8fdad2 with args (&origin, &vector, result.t, position). 0x42e0d0's body is result = v*t + p, i.e. point_from_line3d.

Other checks:
  - /W3 diagnostics: 27 on base and 27 on the candidate, same multiset.
  - fake_match_scan: 0 leads on both.
  - Fighter: relocations go from 33!=32 to 32/32 and REAL regions from 5 to 3. Scout is unchanged.

All artefacts are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\ (MANIFEST.md, LEDGER.md, cand.c, cand.obj, *.gate.txt, object_audit_*.txt, provider_link_*.txt, surplus_identity_*.txt, pdb_storage_*.txt, splitdiff_*.txt, od_*.txt, lab_*.c/.obj).

## blockers
Nothing blocks the covered packet: every condition of ruling #1 holds, as do the rule-6 COMDAT conditions.

Whole-object completion is still blocked by held classes this packet does not touch:
(a) _update_alien_fighter_physics_new is held:
  - the owner said 'keep the fighter held' (3ba2eb91);
  - the actor_combat NODUP cross_product2d provider it needs is RESERVED to the Codex packet;
  - lab lab_f1held.c shows the held cross_product2d view-cast yaw is EXACT on top of this packet, 38/1.
(b) _update_alien_scout_physics is HELD_CLASS:
  - its leveling point_from_line2d is a count-oracle landing that contradicts /Od 0x8f8b4a;
  - two aggregate 3D->2D view copies are load-bearing, which is the open owner question in structural packet section 5.
(c) Inherited invented private helper owners (_vehicle_{cross,dot,triple}_product3d_target, _vehicle_dot_product3d_test) block admission even at 39/39.

I could not run a full ninja (brief forbids it). It is the integrator's formal confirmation.

## reopen_criteria
Fighter: reopen when the Codex actor_combat cross_product2d provider repair lands and the owner releases the fighter hold. The lab f1held hunk is then EXACT with this packet in place.

Scout: reopen on an owner ruling for structural packet section 5 (aggregate view copies), plus a genuine, /Od-consistent one-temp construct for the S3 x87 leaf.

Whole object: also requires the boat and preprocess functions rewritten onto genuine helpers, removing the invented _vehicle_* helper owners.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_vehicles\LEDGER.md
