# actor_perception worker ledger (lane claude/fifty-objects-20260925)

Start: fresh (slug dir did not exist at resume). Base e9e62b78.

## Baseline (gate.py --all at HEAD)
exact 42 residual 2 (of 44):
- residual 400  _actor_perception_aiming_vector_test_blockage [sha]   (parked, unclassified)
- residual 1472 _actor_perception_refresh_danger_zone [size 1504!=1472, sha]

## Prior-art summary (read before probing)
- danger_zone: Lane A 20260920 had a STRICT EXACT candidate (scratch/res6/danger-zone2/candidate_exact.c) needing
  (A) _point_from_line3d COMDAT (drop REAL_MATH_EXTERNAL_POINT_FROM_LINE3D) and (B) #pragma auto_inline(off) on
  actor_get_perception_knowledge. Ruled REJECTED 20260920 on both. Since then owner ruling 20260921 (Lane B)
  relaxed the pfl3d guard for the 17 January objects that reference it out of line; actor_perception IS one of the 17.
  B (pragma) remains inadmissible; dummy `long prop_index = NONE` also rejected (waveC).
- opus5 w1: a single-exit short-return body of actor_get_perception_knowledge is exact and NOT inlined (claim).
- blockage: parked; 400/10 both sides; residuals: zero-store on out-of-range path, 1/mag scale temp alive past j store.

## Evidence gathered (session 1)
- January danger_zone decoded (jan_danger_zone.txt): early returns at top are genuine (object==NULL has its own
  epilogue at 0x45). Knowledge called out of line at 0x308 (vehicle arm) and 0x4e8 (projectile arm), `push -1` literal.
- /Od 2020 (0x474190, od_danger_zone.txt): RTC locals sense_position(56), animation_state(2), direction(16).
  Vehicle arm topology in /Od: `if (mag<k || dist>r+10) {type=none;} else {...}` (jmp to case end, not direct break);
  `prop_index = NONE; if (driver != NONE) prop_index = prop_get_active_by_unit_index(..); if (prop_index != NONE) {..} else {..}`;
  else-arm has two dead zeroed shorts, encounter ?:, blind=FALSE; if(asleep||(enc&&enc->blind)) blind=TRUE;
  parent -> ultimate parent -> object_get -> location; los; `if (!blind && vis(..knowledge(actor_index,-1)..)>=2) ... else if (aud>=2)`.
  Projectile arm: `if (asleep || (enc && enc->blind)) {} else { def = tag_get(proj); if (def->radius > dist) {...} }`
  (explicit jmp from the then-arm to the case end = EMPTY then-block or if/else form).
- P1 c1_nopfl.c = production minus REAL_MATH_EXTERNAL_POINT_FROM_LINE3D: [size 1552!=1472, relocs 52!=50, sha] (= Lane A cand1).
- LAB wv1..wv3 (lab/mk.py): wrapping the vehicle knowledge site in 1..3 extra `if` levels does NOT flip the
  auto-inline (still 0 knowledge relocs) -> the site budget is not the lever; the CALLEE's pruned cost is.
- Found prior art: opus5 150k w1 (C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/actor_perception/
  knowledge_final.c, P13_knowledge_pfl3d.c) = HCEA-lifted SINGLE-RESULT knowledge body (`short knowledge_type = NONE;`
  ... `if (knowledge_type == NONE) {...}` ... one return). Production later landed a DIFFERENT byte-identical spelling
  (goto done + `if (result != NONE) goto done`), which VC7 auto-inlines at the two NONE sites. January bytes attest the
  sentinel: `cmp ax,0xffff; jne` at 0xcb is the `knowledge_type == NONE` test after the orphan assignment.
- P2 c2_knowledge_sr.c = c1_nopfl + single-result knowledge body (struct actor_datum/prop_datum, prop_acknowledged,
  _ai_unit_effect_*, _actor_knowledge_*, + TU enumerator `_actor_combat_status_investigate = 2`):
  **exact 43 residual 1** - _actor_perception_refresh_danger_zone EXACT 1472/50, _actor_get_perception_knowledge EXACT 240.
  (first try had mangled #line backslashes -> [reloc-identity] on the assert file string; fixed.)
- P2b same with literal 2 instead of the new enumerator: also exact 43 (enumerator is byte-inert).

## _actor_perception_aiming_vector_test_blockage
- January residual (sbs vs c2): (1) normalize2d tail: January `fld [i];fmul st1;fstp [i];fld [j];fmul st1;fstp [j];fstp st0`
  vs ours `...;fmul [j];fstp [j]` (reciprocal consumed); (2) k-range layout: January places the `blockage = 1` arm
  out of line after the error-squared test (jmp back) and returns 0 directly (xor eax,eax) on the out-of-range path.
- /Od readout FOUND at 0x4710a0 (via code refs to 0.36f/1.21f constants; odfloat.py; od_blockage.txt). RTC locals:
  source_planar_direction(8), friend_vector(12), friend_perpendicular_vector(12). Shape (2020):
    short blockage = 0; planar = aggregate copy of aiming i,j;
    if (normalize2d(&planar) > 0.0f) {                                   [call 0x42e020 = normalize2d]
      vector_from_points3d(origin, point, &friend_vector);               [0x42e3a0]
      friend_distance = magnitude2d(&friend_vector);                     [0x42def0 = magnitude2d, 3D obj passed]
      friend_projection = dot_product2d(&friend_vector, &planar);        [0x42dd30 = dot_product2d]
      if (friend_projection > 0.8660254f * friend_distance) {
        point_from_line3d(&friend_vector, aiming_vector, -friend_projection, &perp);   [0x42e0d0]
        if (error) negate_vector3d(&perp, error);                        [0x43e7f0]
        if (perp.k > -0.5 && 0.9 > perp.k) blockage = 2; else if (perp.k > -0.8 && 1.2 > perp.k) blockage = 1;
        else blockage = 0;                                               [/Od 0x4711d0: explicit zero store]
        if (blockage > 0) { hes = magnitude_squared2d(&perp);            [0x42df20]
          if (0.36 > hes) {} else if (1.21 > hes) blockage = 1; else blockage = 0; } } }
    return blockage;   (single exit)
  => production body is a hand-expansion of normalize2d/magnitude2d/dot_product2d/point_from_line3d/negate_vector3d
     (rule 6 debt) and lacks the `if (blockage > 0)` gate that explains January's out-of-line blockage=1 arm.
- P3 b1.c = c2 + /Od-shaped blockage body (header helpers normalize2d/magnitude2d/dot_product2d/point_from_line3d/
  negate_vector3d/magnitude_squared2d, /Od-attested view casts, explicit else blockage = 0, `if (blockage > 0)` gate,
  empty then-arm for hes < 0.36): [size 384!=400, sha]. The k-range/else-if LAYOUT NOW MATCHES January (blockage=1 arm
  out of line with jmp back; 0 path returns xor eax,eax). ONLY residual: normalize2d tail - January
  `fld [j]; fmul st(1); fstp [j]; fstp st(0)` vs ours `fmul [j]; fstp [j]` (-4 bytes: 381 vs 385 raw).
  Hypothesis: M8 (IL-local ordinal mod 8 of scale_vector2d's `c` param) -> probe local-count oracle (lab only).
- LAB bo1..bo7 (dead `long lab_dummyK = K;` locals 1..7, M8 oracle): all inert (384!=400). M8 says j-terms are
  tree-shape ordered, so a count lever was unlikely; oracle negative.
- **P4 b2.c = b1 with the /Od-attested AGGREGATE copy `source_planar_direction = *(real_vector2d const *)aiming_vector;`
  in place of the two field copies: exact 44 residual 0 - WHOLE UNIT EXACT (blockage EXACT 400/10).**
  /Od evidence (0x4710c6..0x4710d1): `mov ecx,[ebp+0xc]; mov edx,[ecx]; mov eax,[ecx+4]; mov [ebp-0x10],edx;
  mov [ebp-0xc],eax` = pointer loaded once, both dwords loaded before both stores = struct copy (a field copy in /Od
  reloads [ebp+0xc] per statement). Field copy (b1) is NOT exact -> the cast is load-bearing: this is exactly the
  owner-packet item 5 class (claude_structural_owner_packet_20260923.md s5) = needs owner ruling.
- Strip test (b2_strip.c: the five pointer view casts removed, aggregate copy kept): code bytes/relocs IDENTICAL to b2
  (objeq.py: only .debug$S and internal $L label numbering differ) -> the pointer casts are byte-inert (rule 24 ok).
  The aggregate copy cast cannot be stripped (no castless aggregate form; field copy = b1 = not exact).
- object_audit b2: PASS (all January-owned sections/symbols ok). NEW surplus vs base: _point_from_line3d (Jan
  actor_perception references it UNDEF -> 17-object class), _normalize2d, _scale_vector2d, _dot_product2d,
  _negate_vector3d (all-inlined class, ruling 5), __real@42340000 literal. surplus_cand.py: all five new COMDATs
  byte-identical (section_infos_equal) to January's selected provider (action_charge / action_obey).
  Pre-existing (in base too): _distance3d surplus NOT identical to action_vehicle's copy - inherited, investigate.
- provider_link.py b2 --baseline=base: all NEW surplus PASS (_point_from_line3d, _normalize2d, _scale_vector2d,
  _dot_product2d, _negate_vector3d, __real@42340000). Full surplus (b2 AND base identically): FAIL (2) =
  _distance3d and _distance_squared2d vs action_vehicle NODUP hand copies (LNK2005 both orders). PRE-EXISTING at base,
  same class as config/object_admission_rejections.json leaf_map / biped_limp_noodle (candidate-only-comdat-owner).
  => whole-object completion blocked by the board-level action_vehicle distance3d/distance_squared2d provider repair.
- pdb_storage.py: 5 symbols absent from cachebeta publics AND no UNDEF ref in any other split object (undef_refs.py)
  => January file-static: _actor_emotion_unopposable_retreat, _actor_perception_qsort_compare_optional_props,
  _actor_perception_refresh, _actor_perception_refresh_test_object, _last_refresh_overflow_warning_time.
- P5 b3_static.c = b2 + `static` on those 4 functions (prototype+definition) and the global: exact 44/0 and code
  sections byte-identical to b2 (objeq) -> storage fix is codegen-inert; needs paired symbols.json "static": true.
- P6 b4a (/Od declaration order: planar then `short blockage = 0`, nested scopes) and b4b (all 7 locals at function
  top in /Od frame order): both exact 44/0 -> declaration order/scope inert for this function.
- HCEX DIA2Dump: blockage params source_position / source_vector / friend_position / friend_direction_to_aiming_vector
  (= actors.h prototype names already), local source_planar_direction (= /Od RTC name). Knowledge: params only.
- P7 blockage_final.txt (b4a + HCEX param names + one-param-per-line): exact 44/0.
  hes test spellings: `if (!(hes < 0.36f)) { if (hes < 1.21f) 1 else 0 }` -> [sha] (43/1); `if (hes >= 0.36f)
  blockage = hes < 1.21f ? 1 : 0;` -> [sha]. Only the /Od-attested `if (<0.36) {} else if (<1.21) 1; else 0;`
  (empty then-arm = /Od explicit jmp at 0x4711fb) is exact -> the empty arm is structural, not decoration.
- FINAL candidates built from production by build_final.py: final_full.c (all packets), final_nostatic.c (no storage
  packet), fallback.c (pfl3d + knowledge only). Gates: final_full 44/0, final_nostatic 44/0, fallback 43/1 (blockage [sha]).
  Patches: production.patch (= final_full.patch), final_nostatic.patch, fallback.patch; symbols_static.patch (5 lines in place).
- csplit regen IN SCRATCH (splitcfg/ with symbols_after.json -> split_out/): only source/ai/actor_perception.obj changes
  among 833 split objects; those 5 symbols become storage 3. object_audit (SPLIT_ROOT=split_out, object_audit_t.py)
  on final_full.obj: **OBJECT AUDIT: PASS**. provider_link --baseline: PASS (all 6 new surplus).
  Remaining whole-object blocker: pre-existing _distance3d / _distance_squared2d SELECT_ANY vs action_vehicle NODUP (LNK2005).
- /Od knowledge 0x470910 (od_knowledge.txt) attests the single-result SENTINEL form: `knowledge = NONE` ([ebp-8] = -1),
  prop block sets 3 / orphan block guarded by `knowledge == NONE`, then `if (knowledge == NONE) { combat_status >= 2 ?
  2 : ... }`, one return. (2020 uses an else-if chain inside the prop block; January bytes are exact with the HCEA form.)
- fake_match_scan final_full.c: 1 review lead = empty-then-else at the hes < 0.36f arm (evidence above: /Od explicit jmp,
  alternatives not exact). Disclose.
- Castless field-copy variants on the final body (lab/F1 i,j; F2 j,i; F3 all-top order): all [size 384!=400, sha]
  -> the /Od-attested aggregate copy is load-bearing (owner item-5 class).
- fallback.obj: blockage / knowledge / emotion_update sections section_infos_equal to build/base -> fallback causes
  no park drift (blockage park measurement unchanged).

## RESULT (session 1)
Verdict: FUNCTIONS_EXACT_OBJECT_BLOCKED.
- final_full.c: 44/44 functions strict EXACT; object_audit PASS against a scratch csplit regen with symbols_static.patch.
- Newly exact: _actor_perception_refresh_danger_zone (1472), _actor_perception_aiming_vector_test_blockage (400).
- Levers: (1) single-result sentinel spelling of actor_get_perception_knowledge (HCEA + /Od 0x470910 + January
  `cmp ax,0xffff` sentinel test) raises its inline cost so VC7 calls it out of line at the two NONE sites as January
  does - this is the source answer to Lane A's "no source spelling" (pragma) claim; (2) drop the TU's
  REAL_MATH_EXTERNAL_POINT_FROM_LINE3D suppression (ruling 20260921 #1: actor_perception is one of the 17 objects);
  (3) /Od-shaped blockage body with real header helpers, `else blockage = 0;`, `if (blockage > 0)` gate, empty
  first arm of the hes chain, and the /Od-attested aggregate planar copy (load-bearing; owner item 5).
- Blockers: (a) owner item-5 ruling on the aggregate 3D->2D view copy (blockage only; fallback.patch lands the danger
  zone without it); (b) PRE-EXISTING _distance3d / _distance_squared2d SELECT_ANY surplus LNK2005 vs action_vehicle's
  NODUP hand copies (class of leaf_map/biped_limp_noodle rejections) - board-level provider repair.
Files: production.patch (=final_full.patch), final_nostatic.patch, fallback.patch, symbols_static.patch,
  final_full.c/.obj, knowledge_single_result.txt, blockage_final.txt, build_final.py, od_*.txt, jan_danger_zone.txt.
