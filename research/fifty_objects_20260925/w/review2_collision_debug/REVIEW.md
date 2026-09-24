# REVIEW2 collision_debug (adversarial review of OBJECT_COMPLETE_CANDIDATE, wave 2)

Tree HEAD 931ed8dc, git status clean at start. Worker evidence: scratch/w/collision_debug/.
Read: WORKER_BRIEF.md (all, incl s.7), campaign_house_rules.md, claude_fifty_objects_20260925.md,
6 docs/object_matching_logs/*collision_debug* ledgers, wave-1 REVIEW (scratch/w/review_collision_debug/REVIEW.md),
worker LEDGER.md.

## R1 patch application
- git apply --check: collision_debug.c.patch, collision_debug.helpers_only.patch, symbols.json.patch all clean vs 931ed8dc.
- Applied outside the repo (scratchpad; NOTE: `git apply` inside a repo subdirectory silently skips paths
  outside cwd, so the check must be done outside the worktree): post-images byte-equal to cand_final.c,
  candB_mag_acos.c and scratch/w/collision_debug/config/symbols.json. diff config/symbols.json vs copy = line 19564 only.

## R2 /Od call census (independent; od_linear.py 0x7af800 0x7b1ee4 regenerated == worker's od_linear.txt)
- 43 distinct callees mapped in order to cand_final.c calls: assert x3 (0x54cbc0+0x54ecf0), local_player_get_player_index x2,
  datum_get (player_get), fabs 0x42dec0, scale_vector3d 0x42e220, breakable_surface_flags_get 0x7a2b80,
  global_collision_bsp_get 0x865d10 x2, collision_bsp_test_vector 0x7abf40, point_from_line3d 0x42e0d0 x4,
  collision_get_features_in_sphere 0x7a4b00 x2, game_time_get 0x5c9930 x3, fmod(float,float) 0x57f040 x3,
  render_debug_string_at_point, matrix4x3_rotation_from_angles, matrix4x3_transform_vector 0x6c7160 x4,
  render_debug_line 0x848550 x14, add_vectors3d 0x44e8e0, collision_test_point 0x7a7630 x2, render_debug_point x7,
  collision_test_sphere, render_debug_sphere x2, collision_test_vector 0x7a78e0 x2, render_debug_vector x6,
  collision_model_instance_new, tag_block_get_element x3, render_debug_collision_surface, 0x46d330 (arccosine),
  material_get_name, snprintf-CRT 0x43e920 (January reloc = __snprintf, decisive), render_debug_string,
  collision_move_sphere/pill, csmemmove + csmemset x2, render_frustum_get_projection_bounds, set_real_vector3d x3,
  magnitude3d 0x432950, render_debug_collision_features, RTC prologue/epilogue. No other hand-expanded helper remains.
- 0x46d330 body = push float; call 0x4625b0 (acos(float) overload) -> 0x46d3e0 (cvtss2sd; call CRT acos): a one-arg
  float wrapper one level above bare acos. /Od callers of 0x46d330: actor_moving 0x464900, aim_assist 0x588610/0x588920,
  real_math.c 0x6ca9xx..., 0x6d28d0, units.c 0x8db7f0, collision_debug 0x7b1086 -- production spells arccosine in
  actor_moving:2731, aim_assist:359/408, real_math.c x6, units.c:9627. Direct callers of 0x4625b0 include bipeds.c
  0x8c2c30 (production bipeds.c:3158 bare acos). real_math.h:766 arccosine is the only acos wrapper. => ATTESTED.
- 0x432950 = push v; call magnitude_squared3d 0x42df70; call square_root 0x42e320 = real_math.h magnitude3d. ATTESTED.
- Constants read from the /Od exe: 0x93e9c8=360.0f, 0x93f798=6.2831855f (0x40c90fdb), 0x93dd5c=1.0f,
  0x9bc538=-0.7f (TU-local literal pool), 0x9421ac=1.4f, 0x93fac4=pi_f (0x40490fdb), 0x93dd84=30.0f.
  /Od 0x7b0845..0x7b08fa: push 2pi_f; call game_time_get; cvtsi2ss; mulss {1.0|-0.7|1.4}; mulss pi_f; divss 30.0; call
  fmod(float,float). /Od 0x7b1094: fstp arccosine; mulss 360.0; divss 2pi_f. Candidate spellings follow these left-to-right.

## R3 gate (independent compile)
- cand_final.c: EXACT 4192 _collision_debug_render, exact 1 residual 0 unwritten 0.
- candB_mag_acos.c (fallback): EXACT 4192, exact 1/1.

## R4 storage / split regeneration (independent)
- cachebeta_publics.txt: every external collision_debug_* global and _collision_debug_render is a PDB public; NO public at
  [0028B2A8] (publics jump from bsp3d nodes* @0028B298 to count<=14 @0028B308 = exactly the 0x60-byte cube array).
  pdb_storage.py: 1 disagreement, _collision_debug_cube_vertices split 2 / ours 3 / PDB-public False. Source already
  declares it `static const`. No other object (build/split or build/base) references the symbol.
- My own byte-level in-place edit of a config copy (one line, 19564) is byte-identical to the worker's
  scratch/w/collision_debug/config/symbols.json. symbols.json is LF-only (the "CRLF kept" wording in the proposal is
  inaccurate but harmless: the copy preserves the file's LF endings exactly).
- build/tools/csplit.exe -i cachebeta.exe -p review2/config -o review2/split: rc 0, 833 objects; byte-compare vs build/split:
  ONLY source/physics/collision_debug.obj differs, and by exactly ONE byte (symbol 8 storage class 2 -> 3).
- cand_final.obj _collision_debug_render vs REGEN split: section_infos_equal True (4192 / 278 relocs).
- object_audit (copy with AUDIT_SPLIT_ROOT) vs regen split: 17/17 January sections ok, 29 January symbols 0 differ,
  OBJECT AUDIT: PASS. (vs as-is build/split: FAIL(1) = only the storage row, as expected.)

## R5 surplus / link
- own surplus_check.py (every candidate-only external def vs EVERY January split definer: section_infos_equal +
  raw bytes + section flags): 22 defs, problems 0 (8 code COMDATs, 3 D3D tables, 11 literals; each one January definer).
  _arccosine == actor_moving copy; _magnitude3d/_square_root == action_charge; _magnitude_squared3d == action_alert.
- Board hash census: all build/base copies of the 4 new helpers (5/63/93/83 definers) share January's single hash.
- provider_link.py source/physics/collision_debug cand_final.obj: 22/22 PASS, SELECTED-PROVIDER LINK: PASS (both orders).
- nodup_census: NODUP [] for _arccosine, _magnitude3d, _magnitude_squared3d, _square_root.
- objeq vs build/base production: 36/37 sections identical (+ .debug$S path only) + 4 new COMDATs
  (_arccosine 16, _square_root 16, _magnitude_squared3d 48, _magnitude3d 48). cand_final vs candB: identical except .debug$S.

## R6 scorer
- objdiff-cli 3.3.1 one-unit report (own od_proj, cand_final.obj vs regen split): code 4182/4182, data 29278/29278,
  .bss/.data/.rdata/.text all 100.0 (same as build/report.json for production).

## R7 STRIP TESTS / spelling controls (all via gate.py --edits on cand_final.c; objeq vs cand_final.obj)
- s1 drop `* 1.0f` (yaw): EXACT, object identical -> the /Od-attested `* 1.0f` is not load-bearing.
- s2 degrees `* 180.f / _pi`: EXACT, identical.
- s3 `_pi / TICKS_PER_SECOND` (cseries.h enum = 30) for `_pi / 30.f`: EXACT, identical (byte-undecidable style choice).
- s4 `(real)acos(k) * 360.f / (_pi*2.f)` (bare acos): EXACT, identical minus the _arccosine COMDAT -> helper not load-bearing.
- s5 `(real)game_time_get()` cast restored: EXACT, identical.
- s6 pre-existing add_vectors3d / cube-vertex view casts removed: EXACT, identical (byte-inert; kept for type correctness,
  owner-landed in 5671843f, /Od-attested add_vectors3d(point, vector, point)).
- => nothing in the diff (or the pre-existing casts) buys the match; every changed construct is byte-inert. No decoration.
- Float numerics re-derived: pi_f 40490fdb, 2pi_f 40c90fdb, 360/2pi_f = 180/pi_f = 42652ee0 (round(180/pi) = 42652ee1),
  (1.0f*pi_f)/30 = 3dd67750, (-0.7f*pi_f)/30 = bd962051, (1.4f*pi_f)/30 = 3e162051, (double)2pi_f = 401921fb60000000;
  all equal January's literals / the production readout literals.

## R8 hygiene
- /W3 /Zs: base and candidate both 12 warnings, all in cseries.h:45 / real_math.h (C4146/C4244), ZERO in the TU, identical
  lists (worker's "14" is a miscount; its own warn files also show 12). No C4013.
- fake_match_scan on cand_final.c + collision_debug.h: 0 review leads.
- House rules on the diff: no new locals, no casts added, no prototypes, `real` throughout, genuine real_math.h helpers
  (magnitude3d, arccosine) and real_math.h's own `(_pi*2.f)` spelling; the three implausible readout literals
  (0.104719758f / -0.073303826f / 0.146607652f, 57.295776f, 6.2831854820251465) become the /Od-attested expressions.
  Owner-gated classes (s.7): none touched (no uninit/NULL repro, no new view cast, no header edit, no dead locals, no temps).
- Parks / rejections / semantic entries: none for collision_debug. config.json line 515 = collision_debug.c index 140 "NonMatching".
- Rule-6 exception conditions for the 4 new COMDATs: byte-identical to January's selected copy (yes), caller strictly exact
  (yes), provider links both orders (yes), full sweep zero regressions (integrator; .c-only change, no header, so no other
  TU's codegen can move; the symbols.json row changes only this split object).

## Observations (non-blocking, out of scope)
- /Od 0x8d0360 (called from bipeds.c 0x8c5090 "post-limp-noodle"; likely biped_limp_noodle.c) calls BARE acos 0x4625b0 x3
  and 0x8ceb60 x1, while production biped_limp_noodle.c spells arccosine x3. The worker's "classification holds on other
  owners" generalisation is therefore not universal, but it does not affect this site: /Od 0x7b1086 calls the distinct
  one-arg acos wrapper 0x46d330, whose callers are TUs spelling arccosine, and real_math.h:766 arccosine is the only such
  wrapper (January's actor_moving.obj defines _arccosine, so January's real_math.h had it). Worth a look by a
  biped_limp_noodle lane.
- The top-of-file "symbols in this file" comment still lists csplit placeholder names (_rdata_0028b2a8, _bss_00456ec0);
  pre-existing convention in 69 files, comment only.

## VERDICT: approve = true (OBJECT_COMPLETE_CANDIDATE confirmed)
Every worker claim reproduced independently: patches apply clean to 931ed8dc and their post-images equal the
candidate files; gate EXACT 1/1 against the current AND the regenerated split; object audit PASS 29/29 with the
one-line static row (PDB-proven: no public at 0028B2A8); regen changes one byte of one object; 22 surplus definitions
identical to January's selected copies, pair-link PASS in both orders, NODUP clean; objdiff 3.3.1 100/100 on all four
sections; 0 TU warnings; 0 fake-scan leads. Both helper sites (magnitude3d 0x432950, arccosine 0x46d330) and the
constant expressions are /Od-attested operation by operation, and every changed construct is byte-inert under
strip tests, so nothing in the diff is steering. The wave-1 blocker (hand-expanded magnitude3d) is fixed, and so is the
arccosine hand expansion it missed.
Integrator: apply collision_debug.c.patch (or the helpers-only fallback) + symbols.json.patch; regen split; full ninja +
stable sweep (expect 0 function moves, collision_debug stays EXACT); then flip config.json:515 to "Matching".
Admission record: 8 surplus code COMDATs (_add_vectors3d, _arccosine, _magnitude3d, _magnitude_squared3d,
_point_from_line3d, _scale_vector3d, _set_real_vector3d, _square_root), 3 D3D SDK tables, 11 literals, all identical.
Minor record corrections: symbols.json is LF (not CRLF); the TU /W3 list is 12 header warnings, not 14.
