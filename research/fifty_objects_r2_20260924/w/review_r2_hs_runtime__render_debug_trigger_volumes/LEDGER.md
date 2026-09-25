# REVIEW LEDGER: hs_runtime::_render_debug_trigger_volumes (1088 B), round 2 adversarial review
Reviewer slug: scratch/w/review_r2_hs_runtime__render_debug_trigger_volumes/ (':' sanitized). Tree HEAD b7c1b7d9 (= f6d00a8c + baseline docs).
Worker slug: scratch/w/hs_runtime__render_debug_trigger_volumes/.

## R0: context read
- Brief sections 0-9 read. Worker MANIFEST/LEDGER/production.patch read.
- Prior negatives: opus5_150k_w1 (tv1, t6/t7 point_from_line3d spelling rejected ONLY for COMDAT rule), opus5_250k_w3c
  (342/342 insns, 130 slot renames + 4 x87 order; ranking input invisible in emitted stream).
- Owner ruling 5 (2026-09-21, memory owner-rulings-20260921-lane-b) admits the all-inlined header-inline COMDAT class under
  5 conditions; docs/campaign_house_rules.md lines 33-40 carry the exception.

## R1: patches + independent gate
- git apply --check at worktree HEAD (b7c1b7d9; source/config/tools identical to f6d00a8c): production.patch, production_crlf.patch,
  park_retire.patch, park_retire_crlf.patch all clean.
- GNU patch -p1 of production.patch onto `git show f6d00a8c:source/hs/hs_runtime.c` (head_lf.c) == worker cand.c (cmp equal).
- gate --all on my applied file: 65 exact / 1 residual (_hs_evaluate_begin_random [sha]) / 0 unwritten; _render_debug_trigger_volumes EXACT 1088.
  (rev_cand_gate.txt). Baseline head_lf.c: 64/2/0 (rev_base_gate.txt). rev_base.obj == build/base production obj except .debug$S;
  rev_cand.obj == worker cand.obj except .debug$S (objeq).

## R2: admission battery (independent)
- object_audit rev_cand.obj: FAIL(13) = begin_random + 12 converter storage rows; baseline FAIL(14); only surplus delta is
  +.text _point_from_line3d 48 B.
- surplus (worker surplus_cand.py read first, read-only): _point_from_line3d NEW IDENTICAL vs action_charge.obj; 4 old IDENTICAL.
- provider_link --baseline: _point_from_line3d vs source/ai/action_charge PASS (both orders). nodup_census: 24 definers, 0 NODUP.
- COMDAT selection 2 (ANY), storage external; cachebeta publics line 6312 lists _point_from_line3d (external agrees).

## R3: /Od re-read (od_linear 0x606c80..0x607462 -> od_tv.txt; RTC desc 0x607464)
- Side arm: pfl3d(&matrix.position[-0x18], &world_extent[-0x74], 1.0, &corner[0]); sides[0].n[(axis+1)%3] = -local_extent..;
  sides[1].n[(axis+2)%3] = -..; transform x2; pfl3d(corner0,sides0,1.0,corner1); pfl3d(corner1,sides1,1.0,corner2);
  pfl3d(corner2,sides0,-1.0,corner3); jmp join. Else arm: corner[0]=matrix.position; +/- copies; transform x2; SAME 3 pfl3d calls.
  => duplicated tail is /Od source text (confirmed). Floats 0x93dd5c=1.0, 0x9401b4=-1.0, 0x93dd50=0.5 (odbuild.float_at).
- String tail: two render_debug_string_at_point calls (0xa1c244 / 0xa1c22c) under if/else (confirmed). color copy + alpha in else (confirmed).
- /Od frame (declaration order): matrix -0x40, volume_index -0x48, scenario -0x4c, volume -0x50 (=NULL before loop), local_extent -0x60,
  world_extent -0x74, edge_index -0x7c, corner -0xb0, sides -0xd0, side -0xd8, axis -0xdc, color -0xf0, center -0x104, ray -0x118, result -0x170.
- FINDING: the candidate declares `matrix` FIRST IN THE VOLUME-LOOP BLOCK, textually AFTER the outer `short volume_index;
  struct scenario *scenario`. /Od puts matrix BEFORE volume_index. None of the worker's 7 variants placed matrix before
  volume_index (checked fn_od_order.txt, fn_od_outer_only.txt, fn_bareblock.txt), so the MANIFEST claim "full /Od frame order"
  is inaccurate. Precedent: wave2 REVIEW light_volumes rejected a grouping contradicting /Od declaration order when an equally
  exact /Od-consistent grouping existed. Probe next.

## R4: reviewer probes (mkprobes.py; all full-TU gate --all)
- rv_matrix_outer.c (cand + `real_matrix4x3 matrix;` moved to the top of the if-block, BEFORE `short volume_index;` = /Od order):
  65/1/0, trigger_volumes EXACT; objeq_strict vs rev_cand.obj: 0 non-debug sections differ, symbols equal.
- rv_full_od.c (rv_matrix_outer + `{ real_point3d center; real_vector3d ray; struct collision_result result; ... }` block after
  the edge loop = complete /Od declaration order matrix..result): 65/1/0 EXACT; identical object (0 non-debug diffs, symbols equal).
- rv_volume_outer.c (rv_matrix_outer + volume declared in the outer scope, assigned in loop): EXACT, identical object (info only; not adopted:
  /Od's `= NULL` is a dead store with +1 assert-anchor drift, and rule 17 favours decl+init).
- rv_ternary.c (STRIP TEST of the if/else render_debug_string_at_point pair -> production ternary): RESIDUAL [sha]; alndiff 342/342,
  25 hunks incl. the string-call tail (January computes name/center once, 2 color loads, cross-jumped call). The if/else pair is
  load-bearing AND /Od-attested (two calls 0x607410/0x607431) -> double attestation (worker had not strip-tested it).
- single_tail.c (worker strip test) re-gated: residual [sha] (confirmed).

## R5: HCEX evidence re-read
- DIA2Dump -sym printed no children for this function (both PDBs); raw CodeView scan of HCEX_Release.pdb (hcex_locals.py):
  S_GPROC32 render_debug_trigger_volumes -> S_REGREL32 matrix@0xa0, local_extent@0x50, world_extent@0x60, sides@0x70,
  result@0xe0, ray@0x88; no S_BLOCK32. Worker's HCEX local list confirmed.
- S_COMPILE2 of RELEASE_CACHE\halo\hs_runtime.obj: language 0 (C), VC 16 -> C89 declarations. Record order is NOT first-use
  (result precedes ray); light_volume_render's records (marker, external_scale[block], position, color[later nested block]) follow
  textual order. Both later builds place ray/result after sides; with January's per-edge `sides[2] = {0}` (edge-loop block) a C89
  source can only do that with a block after the edge loop. Corroboration, not proof.

## R6: amended patch (primary = rv_full_od)
- amended/production_amended.patch (LF) + amended/production_amended_crlf.patch (CRLF content, LF headers): git apply --check clean
  at worktree HEAD; GNU patch --binary reproduces amended/hs_runtime.c (LF) and hs_runtime_crlf.c (CRLF) byte for byte; git apply of
  the LF patch outside the repo (autocrlf=true) == amended CRLF. NOTE: the worker's production_crlf.patch (and mine) need
  `patch --binary` under GNU patch ("different line endings" otherwise); git apply is unaffected.
- amended CRLF file gate: 65/1/0 EXACT; object identical to rev_cand (non-debug). Compose with hs_runtime_converter_storage.patch:
  order-independent, gate 65/1/0, object_audit FAIL(1) = begin_random only, provider_link (all surplus) PASS, composed.obj identical
  to worker compose.obj (non-debug).
- /W3 /WX on amended: clean. --forbid-emitted-symbol _point_from_line3d flags the COMDAT as expected (ruling-5 class; opt-in guard,
  no build/test enforces it for hs_runtime).
- Fallback (if the owner rejects the bare block): fallback/production_matrix_only.patch (= rv_matrix_outer), git apply --check clean,
  identical object.
- park_retire.patch: applied to a copy, valid JSON, 86 -> 85 entries, only {source/hs/hs_runtime,_render_debug_trigger_volumes} removed,
  all other entries unchanged; begin_random park (line 801) kept.

## Verdict
- approve=false for production.patch AS SUBMITTED (matrix grouping contradicts /Od declaration order; equally exact /Od-consistent
  spelling exists - light_volumes wave2 precedent). APPLY amended/production_amended.patch (or _crlf) + worker park_retire.patch.
