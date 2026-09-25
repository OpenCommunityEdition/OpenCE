# r2w2_vehicles LEDGER (lane claude/fifty-objects-r2-20260924, wave 2)
Task: ADMISSION packet for source/units/vehicles under ruling 20260921 #1 (precedent audit: COVERED zero-credit):
fallback_hunks12.patch (define/undef removal + /Od-attested pathfinding pfl3d calls) + symbols_turret_static.patch.
Tree HEAD 434f0151 (= 05255584 + R2 ledger commit). Claimed source/units/vehicles as r2w2_vehicles.

## Read
- WORKER_BRIEF_R2 s0-9; review_vehicles/REVIEW.md (R13 fallback = 37/2 zero credit, rule-6 cleanup); round-1 vehicles LEDGER;
  precedent_audit LEDGER l.49 (scout+fighter HELD_CLASS; fallback_hunks12 + symbols_turret_static COVERED zero-credit);
  lane B HANDOFF l.65-68 (ruling #1 text) + l.156-162 (17 objects incl. vehicles).
- Precedent landings of ruling #1 (plain removal of the TU-private define/undef, no source comment; disclosure in
  commit/ledger): 59925575 action_vehicle, 6099e659 items/projectiles, bf937aac actor_perception, 4f4c8502 lens flares.
- 05255584 storage precedent: rows set "static": true in place per cachebeta publics.

## Packet rebuild at 434f0151
- git apply --check (read-only): 01 fallback_hunks12 OK, 02 symbols_turret_static OK. cand.c = base.c (copy of tree vehicles.c)
  + 01 via GNU patch -o (CRLF restored to match the autocrlf working tree). diff base->cand = exactly the 2 hunks.
- gate base.c: 37/2 (fighter_new [relocs 33!=32, sha], scout [sha]); gate cand.c: 37/2 (fighter_new [sha], scout [sha]).
  Zero exact credit (as round-1 R13), no row lost.
- Relocation census (relocsites.py): January vehicles.obj references _point_from_line3d ONCE, _vehicle_update +0x268 (UNDEF,
  storage 2). base: fighter +0x138 AND vehicle_update +0x268. cand: vehicle_update +0x268 only (== January).
  build/split census: action_charge DEFINES _point_from_line3d; 16 others + vehicles hold UNDEF refs (== the ruling's 17 list).
- _update_turret_physics: January defines it (sec 11, storage 2 in split = csplit default) with ZERO relocations naming it
  anywhere (inlined at its only site; static-emitted law). cachebeta publics: absent (neighbours _vehicle_hover 0x1A4F00,
  _vehicle_is_flipped 0x1A4F70, _set_real_quaternion 0x1A5040 present) => January file-static; ours already `static`.
- Split emulation: build/tools/csplit.exe -i cachebeta.exe -p cfg_X -o split_X (mk_cfg.py in-place line edit).
  split_ctl == build/split (833/833 identical). split_T vs split_ctl: 1 object differs (source/units/vehicles.obj);
  symdiff: 68/68 sections section_infos_equal, 1 symbol row differs: _update_turret_physics storage 2 -> 3. Nothing else.
- gate cand.c vs split_T (with_split.py redirect): 37/2 (fighter_new [sha], scout [sha]); base.c vs split_T 37/2.
- base.obj (gate-compiled tree source) == build/base vehicles.obj: 172/172 sections, 450/450 symbols (symdiff) -> gate
  reproduces production. cand vs base (owner-keyed): ONLY _update_alien_fighter_physics_new bytes differ + new
  _point_from_line3d COMDAT; pathfinding (exact both), scout and every other section identical.
- object_audit: base vs build/split FAIL(3) (fighter, scout, _update_turret_physics storage 2/3); cand vs split_T FAIL(2)
  (fighter, scout only; 66 January sections ok incl. 28 .rdata + .data _vehicle_update_section; 68 symbols, 0 differ).
  Surplus delta vs base: exactly +.text _point_from_line3d (48 B).
- surplus_identity (redirected base-obj=cand): 28 candidate-only code COMDATs, 0 not identical; new one
  _point_from_line3d IDENTICAL vs source/ai/action_charge.obj. (base: 27, 0 not identical.)
- provider_link cand --baseline=base: _point_from_line3d vs action_charge PASS (both orders). Full: all 86 rows PASS,
  SELECTED-PROVIDER LINK: PASS (base full: 85 PASS). No inherited failures remain at this tree.
- pdb_storage: production 1 disagreement (_update_turret_physics split 2 ours 3 PDB-public False); cand vs split_T: 0.
- data: tinfo --data cand vs production identical modulo section numbers and the .debug$S object path; report.json data
  1744/1744. object_audit data sections all ok.
- /W3 (warn.py, unit flags + /Zs /W3): base 27 == cand 27 diagnostics (line-free multiset equal). No new C4133.
- fake_match_scan: base 0 leads, cand 0 leads.
- real_regions: fighter base 369 vs 363 insns, 5 REAL -> cand 363/363, 3 REAL (improved, still residual); scout unchanged 101.
- /Od (odbuild fn 0x8fd970 = pathfinding; od_pathfinding.txt): call 0x42e0d0 at 0x8fda42 args (&origin[ebp-0x444],
  [0xa31e2c] global_up3d, 0.4f @0x93dd48, &origin) and at 0x8fdad2 (&origin, &vector[ebp-0x458], result.t [ebp-0x430],
  position [ebp+0xc]); fn 0x42e0d0 body = result->x = v->i*t + p->x (x3), returns result = point_from_line3d.
  => both hunk-2 calls attested with exact argument order.
- LAB lab_nofighteruse.c (fighter's pre-existing pfl3d statement deleted, lab only): _point_from_line3d STILL emitted,
  section_infos_equal to cand's copy; reloc only at _vehicle_update+0x268. => the COMDAT is anchored by EXACT callers
  (vehicle_update out-of-line + pathfinding inline), not by the non-exact fighter (P1 class does not apply).
- Patches regenerated at HEAD with correct index lines (orig round-1 index lines were stale: symbols pre-image 3da39456 vs
  HEAD f9abb17d; vehicles post-image e6e72f2e vs actual e6710a63). Hunks byte-identical to the originals (patches/orig/).
  git apply outside the repo reproduces cand.c (blob e6710a63) and cfg_T/symbols.json (blob c602dcdc).
- LAB lab_scale.c (/Od 0x8fda65 spelling scale_vector3d(global_down3d, 2.0f, &vector) for add_vectors3d(down,down); /Od
  fn 0x42e220 = scale_vector3d): whole object section+symbol identical to cand.obj (VC7 folds *2 to fadd); /W3 same.
  Offered as patches/optional/03 (NOT part of the covered packet; byte-inert /Od-attested spelling of a pre-existing line).
- LAB lab_f1held.c (round-1 HELD fighter yaw hunk = cross_product2d view casts, on top of this packet): 38/1, fighter
  EXACT (lab only; held by owner 3ba2eb91 + actor_combat NODUP provider RESERVED to Codex). Reopen evidence only.
- nodup_census _point_from_line3d: 26 definers on build/base, NODUP none.
- MANIFEST.md written (precedent section: ruling #1 quote, every condition YES). VERDICT: ADMISSION_FIX_PROPOSED
  (zero credit; object stays 37/39 NonMatching). Claim released.
