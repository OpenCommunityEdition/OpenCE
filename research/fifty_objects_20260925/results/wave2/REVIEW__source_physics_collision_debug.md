# review source/physics/collision_debug
approve=True

## per_object
null

## checks
All checks were run independently. Notes and scripts are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_collision_debug\ (REVIEW.md, odq.py, surplus_check.py, objeq.py, warn.py, audit_r.py, od_proj\, split\, config\).
(1) Patches: git apply --check passes for collision_debug.c.patch, collision_debug.helpers_only.patch and symbols.json.patch against HEAD 931ed8dc (status clean). I also applied them outside the worktree. Inside a repo subdirectory, git apply silently skips out-of-cwd paths, so that location would not have tested anything. The post-images are byte-equal to cand_final.c, candB_mag_acos.c and the worker's config\symbols.json.
(2) /Od census: I regenerated od_linear.py 0x7af800..0x7b1ee4 and it is identical to the worker's. I mapped all 43 callees in order to cand_final.c calls, and no other hand-expanded helper remains. The two helper sites check out:
 - 0x46d330 is a one-argument float wrapper that calls the acos(float) overload 0x4625b0, which calls 0x46d3e0, which calls CRT acos. Its /Od callers are actor_moving, aim_assist, real_math.c, units.c and collision_debug. Production spells arccosine in all of those TUs, and real_math.h:766 arccosine is the only such wrapper.
 - 0x432950 is magnitude_squared3d followed by square_root, which is magnitude3d.
 - I read the constants from the /Od exe: 360.0f, 6.2831855f, 1.0f, -0.7f, 1.4f, pi_f and 30.0f. The operation order matches the candidate spellings.
(3) gate.py --source cand_final.c --all: EXACT 4192 _collision_debug_render, exact 1/1. The fallback candB is also EXACT 1/1.
(4) symbols.json: my own one-line in-place edit is byte-equal to the worker's copy. csplit regen returns rc 0 with 833 objects. Only collision_debug.obj differs, by one byte (the storage class of _collision_debug_cube_vertices goes 2 to 3). Against the regenerated split, _collision_debug_render is section_infos_equal True (278 relocs), and object_audit gives 17/17 sections, 29 symbols, 0 differ, PASS.
(5) cachebeta_publics.txt has no public at [0028B2A8]; the 0x60-byte gap sits between 0028B298 and 0028B308. pdb_storage reports this as the only disagreement, and no other object references the symbol.
(6) Surplus: my own check compares every candidate-only external definition against every January split definer, using section_infos_equal, raw bytes and section flags. Result: 22 definitions, 0 problems. A board hash census of the 4 new helpers (5/63/93/83 build/base definers) matches January's single copy. provider_link gives 22/22 PASS in both orders, SELECTED-PROVIDER LINK PASS. nodup_census gives NODUP [] for all 4.
(7) objeq against build/base production: identical except .debug$S (compile path only) plus 4 new COMDATs (_arccosine, _square_root, _magnitude_squared3d, _magnitude3d). cand_final and candB are identical except .debug$S.
(8) objdiff-cli 3.3.1 one-unit report against the regenerated split: code 4182/4182, data 29278/29278, and .bss/.data/.rdata/.text all at 100.
(9) Strip tests and controls (gate --edits, then objeq): all EXACT and object-identical.
 - s1: dropped `* 1.0f`
 - s2: `* 180.f / _pi`
 - s3: `TICKS_PER_SECOND` instead of 30.f
 - s4: bare acos (identical apart from the _arccosine COMDAT)
 - s5: `(real)` cast on game_time_get
 - s6: the existing add_vectors3d and cube-vertex view casts removed
 Every changed construct is byte-inert, so there is no decoration.
(10) I re-derived the float numerics: 360/2pi_f = 180/pi_f = 42652ee0 (round(180/pi) is 42652ee1), 3dd67750, bd962051, 3e162051, and (double)2pi_f = 401921fb60000000.
(11) /W3 /Zs: base and candidate have identical warning lists, 12 header warnings each and 0 in the TU. No C4013.
(12) fake_match_scan: 0 leads.
(13) parked.json, rejections and semantic configs have no collision_debug entry. config.json:515 is collision_debug.c index 140 "NonMatching".

## issues
None blocking.
Minor corrections to the worker's record:
(a) config/symbols.json uses LF line endings, not CRLF. The worker's copy keeps the file's LF endings byte for byte, so the patch is correct and only the wording is wrong.
(b) The /W3 count is 12 header warnings, not 14. The worker's own warn files also show 12.
Out-of-scope observation: /Od 0x8d0360, called from bipeds.c 0x8c5090 ("post-limp-noodle") and probably in biped_limp_noodle.c, calls bare acos 0x4625b0 three times. Production biped_limp_noodle.c spells arccosine. So the worker's claim that the three-level classification holds on other owners is not universal. It does not affect this site: collision_debug calls the separate wrapper 0x46d330, which is arccosine. This may be worth a look in a biped_limp_noodle lane.
Style choice the bytes cannot decide: `_pi / 30.f` and `_pi / TICKS_PER_SECOND` give byte-identical output (s3). The /Od-attested 30.0f literal is acceptable.
Apply order for the integrator:
1. Apply scratch\w\collision_debug\collision_debug.c.patch (post-image cand_final.c). If only the helper fixes are wanted, apply collision_debug.helpers_only.patch instead (post-image candB_mag_acos.c, byte-identical object).
2. Apply symbols.json.patch: line 19564 gets `, "static": true`.
3. Regenerate the split, then run a full ninja and the stable sweep. Expect 0 moves; collision_debug stays EXACT.
4. Flip config/config.json line 515 to "Matching".
The admission record should list 8 surplus code COMDATs, 3 D3D SDK tables and 11 literals, all identical to January's selected copies and all pair-linking cleanly.
