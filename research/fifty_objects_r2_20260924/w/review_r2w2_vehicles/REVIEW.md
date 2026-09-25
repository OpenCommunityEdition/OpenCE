# REVIEW r2w2 vehicles (adversarial) - reviewer label review_r2w2_vehicles_rv1

Tree: HEAD 434f0151 (= 05255584 + ledger-only commit), clean except untracked research dir.
Packet under review: scratch/w/r2w2_vehicles/patches/01_vehicles_fallback.patch (7e45032c) +
02_symbols_turret_static.patch (2a100d69); optional 03 (3639cd62).

VERDICT: APPROVE 01 + 02 as submitted (zero credit; object stays 37/39 NonMatching). No amendment needed.
Optional 03 independently verified byte-inert and /Od-attested (integrator's choice).

## Independent checks (all artefacts in this directory)
1. git apply --check at HEAD: 01 OK, 02 OK. Applied outside the repo (scratchpad, non-git) to HEAD blobs:
   vehicles.c -> e6710a63 (== worker cand.c), symbols.json -> c602dcdc. Hunks byte-identical to the round-1 files
   (patches/orig == claude-fifty-objects-20260925 review_vehicles/fallback_hunks12.patch and vehicles/symbols_turret_static.patch).
2. gate.py --all: my_base.c 37/2 (fighter [relocs 33!=32, sha], scout [sha]); my_cand.c 37/2 (fighter [sha], scout [sha]).
   No row changes verdict. my_base.obj == build/base vehicles.obj (objcmp.py: 157 sections / 156 symbols, 0 diff).
   my_cand.obj == worker cand.obj (0 diff). base vs cand: ONLY +sym:_point_from_line3d and the fighter bytes (33->32 relocs).
3. Relocation sites: January vehicles.obj: _vehicle_update+0x268 -> _point_from_line3d (UNDEF). Production: that + fighter+0x138.
   Candidate: _vehicle_update+0x268 only, now to the local COMDAT (storage 2, selection ANY).
4. build/split census: 17 objects hold UNDEF _point_from_line3d == ruling #1's 17-object list verbatim (incl. vehicles);
   action_charge is the sole definer.
5. Lab lab_nofighter.c (fighter's pfl3d statement deleted, lab only): COMDAT still emitted and section-identical, referenced
   only by _vehicle_update (EXACT). The COMDAT is anchored by the exact out-of-line caller; the P1 class does not apply.
6. Split emulation (my own cfg_R = config/ copy + patched symbols.json; build/tools/csplit.exe -> split_R): 833 files; only
   source/units/vehicles.obj differs from build/split; within it 68/68 sections equal and 1 symbol row changes:
   _update_turret_physics storage 2 -> 3.
7. object_audit my_cand.obj vs split_R: 66 January-owned sections ok, 2 DIFF (held fighter, scout), 68 symbols / 0 differ,
   FAIL(2) residual-only. Production vs build/split: FAIL(3) (same + turret storage). Surplus delta: exactly +.text
   _point_from_line3d (48).
8. pdb_storage: production 1 disagreement (_update_turret_physics split 2 ours 3, not public); candidate vs split_R 0.
   cachebeta publics: 0x1A4F00 _vehicle_hover, 0x1A4F70 _vehicle_is_flipped, 0x1A5040 _set_real_quaternion present;
   0x1A4F50 absent. No relocation anywhere in build/split names _update_turret_physics. The source is already `static`.
9. surplus_identity (candidate, split_R): 28 candidate-only code COMDATs, 0 not identical; _point_from_line3d IDENTICAL
   vs action_charge.
10. provider_link --baseline=production: _point_from_line3d vs action_charge PASS (both orders). Full run: 85 provider rows
    PASS (production 84; the only new row is _point_from_line3d). nodup_census: 26 definers, NODUP none.
11. Data: every non-debug section other than the fighter .text and the new COMDAT is section_infos_equal to production
    (objcmp), so data is unchanged. objdiff 3.3.1 mini-report (odproj/): prod reproduces build/report.json (34 fns,
    9436/16908 code, 1744/1744 data); candidate vs split_R has identical counts, data 1744/1744, fuzzy 96.871 -> 97.187.
    The ONLY per-function change is the fighter, 95.008 -> 99.912. _update_turret_physics stays 100 after the storage change.
12. /W3 (/Zs): 27 == 27, identical multiset (no new C4133). fake_match_scan: 0 leads for both.
13. /Od (odbuild fn 0x8fd970, od_pathfinding_mine.txt):
    - 0x8fda42 call 0x42e0d0(&origin [-0x444], [0xa31e2c] -> (0,0,1) = global_up3d, 0.4f @0x93dd48, &origin);
    - 0x8fda65 call 0x42e220([0xa31e38] -> (0,0,-1) = global_down3d, 2.0f @0x93dd64, &vector [-0x458]);
    - the surface_index store (0x8fdaa9) precedes 0x8fdad2 call 0x42e0d0(&origin, &vector, result.t [-0x430] passed
      directly, position).
    0x42e0d0 is x3 (v*t+p) returning result, i.e. point_from_line3d. The /Od frame has no `t` staging local. The candidate
    matches statement order and argument order.
14. vehicles.c asserts use literal line numbers (match_assert ..., 308 / 372), so the -10 line shift is inert (bytes confirm).

## Precedent scrutiny (SPECIAL FOCUS)
- Ruling #1 read at source: claude_lane_b_HANDOFF_20260920.md:65-68, "a TU may emit it iff January's SAME object
  REFERENCES it out of line"; :156-162 lists vehicles. The ruling was requested for exactly this object (lane B admission
  manifest l.1119-1120: owner ruling on emitting the byte-identical header-inline COMDATs _point_from_line3d ... from
  vehicles.obj).
- House-rules rule-6 exception (docs/campaign_house_rules.md:32-40): byte-identical copy (yes); its caller strictly exact
  (the out-of-line caller _vehicle_update is EXACT, lab 5); zero-regression sweep (only vehicles.obj changes in base and
  split, no row lost; the integrator's ninja confirms); selected-provider link (PASS). The same paragraph says "Do not
  hand-expand or hand-copy the helper to evade this check": production's pathfinding hand expansion with its "without
  owning point_from_line3d" comment is a live violation that this packet removes.
- Precedent commits 59925575, 6099e659, bf937aac, 4f4c8502: all are plain define/undef removal with no comment (verified in
  their diffs). Difference: each precedent also gained strict functions, and this one is zero-credit. The ruling does not
  condition on a gain, so this is the ruling's literal case, not a stretch.
- Owner "keep the fighter held" (3ba2eb91) concerns landing the fighter (rule-24 cross_product2d view casts plus the
  _cross_product2d emission vs actor_combat NODUP, RESERVED). This packet does not touch fighter source, emits no
  _cross_product2d, and the fighter is not parked; its bytes only move toward January.
- Storage precedent 05255584: in-place "static": true per cachebeta publics. Same form here (line 7052, order preserved).

## Non-blocking notes
- MANIFEST count wording: "86/86 rows PASS" is 85 provider rows plus the summary line (production: 84 plus summary).
  The substance is unchanged.
- Optional 03 (scale_vector3d(global_down3d, 2.0f, &vector)): applies on top of 01 (blob a9e6fee5), gate 37/2, whole object
  0 diff vs my_cand.obj, /W3 identical, attested by /Od 0x8fda65. Recommended, not required.
- Out of scope, pre-existing: /Od RTC names the pathfinding locals collision_origin / collision_vector (production uses
  origin / vector); /W3 shows pre-existing C4013 implicit declarations (matrix3x3_multiply,
  matrix3x3_rotation_to_quaternion) in vehicles.c.
