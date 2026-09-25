# REVIEW LEDGER breakable_surfaces (review-r2w2)

## Setup (HEAD 434f0151 = 05255584 + ledger-only commit; tree clean)
- git apply --check: PACKET/01/02/03 all rc 0; PACKET == cat 01 02 03 (cmp); sha256 match patches_sha256.txt.
- Lab: prod.c (production), cand.c (production + 01). labgate.py = gate.py copy with LAB_DROP (flag removal) + LAB_SPLIT (split root).
- A = prod/prod flags (== build/base except .debug$S), B = prod/default flags, C = cand/prod flags, D = cand/default flags (packet),
  E = cand drop /Ow only, F = prod drop /QIfist only.
- Gates: A,B,C,D,E,F all 12/12 EXACT vs build/split; D 12/12 vs emulated split.
- objcmp: A==F (QIfist dead), D==E (QIfist dead with cand). A vs B: only 7 helper COMDATs differ
  (static hand copy + cross_product3d, plane3d_from_point_and_normal, plane3d_negate, project_point2d, project_point3d,
  vector_from_points3d). B vs D: only the static hand copy (96 B, storage 3) -> _bsp3d_get_plane_from_designator (112 B, ext).
## Emulated split (own csplit run into slug)
- control (unpatched config copy) vs build/split: 833/833 identical. Candidate (03 applied): only breakable_surfaces.obj
  differs; its only symbol-table change is storage 2->3 for _breakable_surface_effect and _globals (no bytes/relocs).
## Audit / storage / surplus / link
- object_audit prod: FAIL(2) storage rows. D vs emu split: PASS, 26 January symbols 0 differ, all 26 sections ok (12 .rdata,
  12 .text, .data, .bss). D vs old split: FAIL(2) (needs 03).
- pdb_storage prod: 2 disagreements; D vs emu: 0.
- surplus_identity prod: 7 not identical; D: 1 (_real_local_random vs effects = B1). _bsp3d_get_plane_from_designator vs
  decals IDENTICAL; decals is the ONLY January definer (items/collision_bsp/collision_features/bipeds reference it undefined).
- provider_link D full: FAIL(1) _real_local_random (same as prod); --baseline=A: _bsp3d_get_plane_from_designator vs decals PASS.
- nodup_census: _bsp3d_get_plane_from_designator 5 definers in build/base, 0 NODUP.
## Precedent verification (special focus)
- P1 = owner ruling 2 of 2026-09-21 (docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md s3; house-rules Rule 6 exception):
  genuine shared-header __inline [bsp3d.h:46, non-static] YES; byte-identical COMDAT [vs decals, the ONLY January definer] YES;
  strict-exact caller [_breakable_surface_effect 4032 EXACT] YES; full ownership/build sweep zero regressions [no header touched;
  only this TU's source+cflags change; emu split changes only this object; ours object differs from prod only in surplus sections]
  YES; selected-provider link both orders PASS; ruling 6 (state every newly emitted symbol) satisfied by MANIFEST; no hand expansion
  (the hand copy is what is REMOVED). New COMDAT is SELECT_ANY; nodup_census 0 NODUP definers.
- First-party /Od attestation: /Od breakable_surface_effect 0x7a0ca0 calls the ONE shared helper 0x5666f0 at 0x7a0e64
  (args bsp, surface->plane_designator, &surface_plane; body = tag block element [& 0x7fffffff], test & 0x80000000 -> call
  plane3d_negate else 16-byte copy, return result); 0x5666f0 has 11 call sites across TUs => genuine header helper, not a TU copy.
- History: hand copy added 888065f3 (2026-07-15) when bsp3d.h still tested `plane_designator<0`; header fixed to `& LONG_MIN` in
  7a3bbb9c (2026-09-13, decals copy exact) => hand copy redundant since then.
- P2 = uniform compile profile: c1620772 (owner removed unevidenced per-unit /QIfist from structure_lens_flares, "A closer object
  produced by the wrong flag is not admissible"), 0eda57d6 (periodic_functions /Op /QIfist override removed), weather doc
  "January build is flag-uniform". Override origin 369b71e7 (Berthalamew WIP, unit NonMatching, no rationale). Conditions hold:
  /QIfist is dead (A==F, D==E byte/symbol identical); /Ow is byte-neutral for all 12 functions + data; only surplus COMDATs move,
  and they move TO the January selected copies. Not stretched. Caveat (disclosure): January-owned bytes cannot discriminate /Ow here,
  so the justification is the uniform-profile default + override without provenance, not per-TU image proof of "no /Ow".
  docs/object_matching_logs/breakable_surfaces_obj.md still lists flags /Ow /QIfist and "Do not repeat: /Ow removal ... compiler-flag
  changes" - that list is about residual-closing levers (A03 byte-neutral); recommend the integrator add a superseding note.
- P3 = symbols.json "static": true from cachebeta publics (house rules 20260830): neither symbol is a public; production already
  static; pdb_storage 0 after.
## Other checks
- /W3: 17 == 17 (same set). fake_match_scan prod+cand: 0 leads. Round trip (LF copies, apply + reverse) PASS for all three files.
- objdiff 3.3.1 mini project (odproj/): prod 5331/5331 code, 12/12 fn, 685/685 data; cand identical.
- Dependency: 01 WITHOUT 02 makes _bsp3d_get_plane_from_designator DIFFERENT vs decals (8 not identical) => land atomically.
- Parks/rejections for the unit: none.
- Incident self-report: ran `python -B -m tools.parked_functions --help` before reading its source (brief s7). git status afterwards:
  clean (only pre-existing untracked research/fifty_objects_r2_20260924/). No tracked file modified.
## VERDICT: APPROVE (as submitted; no amendment needed). Zero credit; object remains blocked by B1 + B2.
