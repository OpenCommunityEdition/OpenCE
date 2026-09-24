# REVIEW actor_perception (adversarial)
Start: Wed Sep 23 20:05:16 PDT 2026. Fresh (dir did not exist).

## 1. Reproduction of gates (independent)
- Tracked source/ai/actor_perception.c and config/symbols.json clean at e9e62b78.
- Applied production.patch / fallback.patch / final_nostatic.patch to fresh copies of production (patch -s --binary):
  all apply cleanly and are byte-identical to the worker's final_full.c / fallback.c / final_nostatic.c.
- gate.py --all: base 42/2 (blockage [sha], danger_zone [size 1504!=1472, sha]);
  apply_production 44/0; apply_final_nostatic 44/0; apply_fallback 43/1 (blockage [sha] only). Reproduced.
## 2. Object audit / surplus (independent)
- object_audit (current split): apply_final_nostatic PASS; apply_production FAIL(5) = exactly the 5 storage 2/3 rows that
  symbols_static.patch + csplit regen address; apply_fallback FAIL(1) = blockage only; base FAIL(2).
- surplus_rev.py (own script; candidate vs January selected copy, NEW vs build/base): NEW surplus = _scale_vector2d,
  _normalize2d, _dot_product2d (action_charge), _point_from_line3d (action_charge; Jan actor_perception has it UNDEF),
  _negate_vector3d (action_obey), __real@42340000 (actions). ALL section_infos_equal to January's selected copy.
  Pre-existing (identical in base): _distance3d NOT identical to action_vehicle's copy.
- Note: action_charge/action_obey hand copies were removed at 5e81dabf, so these providers now emit SELECT_ANY header inlines.
- provider_link --baseline=build/base (own run): production candidate PASS for all 6 new surplus; fallback PASS
  (_point_from_line3d, __real@42340000). nodup_census over build/base: zero NODUP definers of any of the 6 symbols.
## 3. Knowledge body review
- Semantics: new body == old production body (prop_acknowledged = state in [2,3]; unit_effect 1/2 = shooting/death_scream;
  state.mode>=combat(3) = old view "combat_status>=3"; state.combat_status>=investigate(2) = old "artificial_combat_status>=2";
  final bool = guard/noncombat). `_actor_combat_status_investigate = 2` matches the same TU-local enumerator in
  action_obey.c, actor_stimulus.c, encounters.c (and implicit =2 in actors.c/actor_combat.c/actor_looking.c).
- /Od 0x470910 (worker dump, re-read): sentinel [ebp-8] = -1, single return; BUT the prop block in /Od is an ELSE-IF chain
  (three separate `mov 3` stores each jmp to 0x470a18), the orphan arm uses a named local + ?: temp ([ebp-0x14] = 3/2),
  and the final arm is `if (cs>=2) 2; else if/else on mode (jge -> 1-store placed last)` with direct stores (no ?: temp).
  The candidate instead uses one combined || condition (the HCEA-lift form). Must test whether the /Od-shaped chain
  also flips the inline decision (if only the || form works, the || form is steering the inliner).
- LAB (review, over fallback.c): k_chain (/Od else-if chain in prop block) 43/1, knowledge EXACT 240 + danger_zone EXACT 1472;
  k_orphan (/Od named orphan local + ?: definite/searching) 43/1 same; k_od (full /Od shape: else-if chain, orphan ?:,
  final `else if (mode < combat) noncombat; else guard;`) 43/1 same (first k_od try had mangled #line backslashes ->
  reloc-identity on the assert file string only; fixed). => the inline flip is carried by the single-result SENTINEL
  structure itself, robust across every /Od-attested internal spelling; the candidate's || / `+` spelling is not steering.
  (Optional: k_od is the more /Od-faithful spelling; both exact.)
- January knowledge bytes (dis_range, own run): `cmp ax,0xffff; jne` at 0xcb right after the orphan `setne/add 2`
  = the sentinel test, confirmed; defined-then-tail-duplicated `mov eax,3` returns at 0x97.
## 4. Blockage /Od re-read (odbuild fn 0x4710a0, own dump od_blockage_rev.txt)
- Confirmed: blockage=0 init; 0x4710c6 aggregate 8-byte integer copy (pointer loaded once, both loads then both stores);
  normalize2d(&planar) > 0; vector_from_points3d(src,friend,&fv); magnitude2d(&fv); dot_product2d(&fv,&planar);
  proj > k*dist; point_from_line3d(&fv, source_vector, -proj, &perp); if (dir) negate_vector3d(&perp, dir);
  k-range else-if chain with explicit `xor; mov [ebp-0x18]` zero store at 0x4711d0; `test; jle` = if (blockage > 0);
  magnitude_squared2d(&perp); `comiss 0.36,hes; jbe; jmp end` at 0x4711f9/0x4711fb = EMPTY then-arm; else-if 1.21 -> 1; else 0.
  RTC: source_planar_direction(8), friend_vector(12), friend_perpendicular_vector(12). Candidate body matches this shape.
## 5. Storage (Packet C) + data
- cachebeta publics: all 5 proposed statics absent; every other unit symbol's public presence == storage (public<->2,
  absent<->3) including the pre-existing statics. No UNDEF reference in any other build/split or build/base object and
  no textual reference in any other source/ or libs/ file. symbols_static.patch applies in place (5 lines, CR-insensitive
  diff shows only `, "static": true` additions) and equals worker's symbols_after.json.
- Worker scratch csplit regen (split_out/): 833 objects, only source/ai/actor_perception.obj differs; its only change is
  storage 2->3 on the 5 symbols; all sections section_infos_equal. object_audit (SPLIT_ROOT=split_out) on MY
  apply_production.obj: 121 January symbols, 0 differ, OBJECT AUDIT: PASS. (Integrator must redo the csplit regen itself.)
- Data: all 76 January data sections ok in production/nostatic/fallback audits; build/report.json data 2068/2068 100%.
## 6. Blockage strip tests (review lab, over final_nostatic)
- b_strip_casts (5 pointer view casts removed): 44/0 EXACT -> casts byte-inert (rule-24 strip test PASSES; /Od attests
  each site passes the 3D object: magnitude2d/dot_product2d(&fv), point_from_line3d(&fv,..,&perp), magnitude_squared2d(&perp)).
- b_fieldcopy (castless `planar.i = src->i; planar.j = src->j;`): [size 384!=400, sha] -> aggregate view copy LOAD-BEARING
  (owner packet 20260923 item 5 class; correctly held for ruling).
- b_noempty (`if (!(hes < 0.36f)) { if (<1.21) 1; else 0; }`): [sha] -> the empty then-arm is LOAD-BEARING; its only
  support is the /Od explicit `jbe; jmp end` at 0x4711f9/0x4711fb (first-party, later revision). Disclosed by worker.
- b_noelse0 (drop `else blockage = 0;`): [size 384!=400, sha] -> the redundant zero store is LOAD-BEARING; independent
  evidence = /Od 0x4711d0 `xor eax,eax; mov [ebp-0x18],ax` in the else arm of the k-range chain.
## 7. Rulings context
- Lane A 20260920 ruling rejected the danger-zone package (pfl3d emission + #pragma auto_inline). Superseded for pfl3d by
  Lane B owner ruling 20260921 #1 (actor_perception is in the 17 objects that reference _point_from_line3d out of line) and
  by the committed house-rules exception (3c74fa36, docs/campaign_house_rules.md: byte-identical, strict-exact caller,
  sweep, provider link). The pragma is NOT used; the worker's source lever replaces it. Lane A's "no source spelling"
  proof assumed the post-prune callee cost is pinned by its 240-byte body; refuted (goto vs sentinel bodies are
  byte-identical at 240 yet inline differently) - reproduced here with 3 more /Od spellings.
- Relocation census: Jan/cand/base _point_from_line3d refs: emotion_update only / emotion_update only / +danger_zone;
  __real@42340000: danger_zone x3 / x3 / none; knowledge calls from danger_zone: 2 / 2 / 0. Candidate == January.
- Pre-existing whole-object blocker confirmed: provider_link (no baseline) FAIL(2) _distance3d/_distance_squared2d vs
  action_vehicle NODUP hand copies (identical in base; nodup_census: action_vehicle is the only NODUP definer).
- fake_match_scan: production candidate 1 lead (blockage empty-then-else, Packet B, /Od-attested); production file 0.
## 8. Extra artefacts produced by the review
- lab/k_od.txt: fully /Od-shaped knowledge body (else-if chain, named orphan local + ?:, final if/else). Swapped into
  production and final_nostatic candidates: both 44/0; into fallback: 43/1 (danger zone + knowledge EXACT). Optional,
  equally exact, closer to first-party /Od than the HCEA-lift `||` / `searching + (x != FALSE)` spelling.
- A_plus_C.patch / A_plus_C.c (review-built): Packet A + Packet C WITHOUT B (the mechanical path if the owner holds item 5;
  the worker shipped only A, A+B, A+B+C). Gate 43/1 (blockage [sha] only); code sections identical to fallback.obj;
  object_audit vs scratch regen split: 121 symbols, 0 differ, only the blockage section DIFF. Applies cleanly (patch --binary).
- Packet C inert: apply_final_nostatic.obj vs apply_production.obj code sections identical.
- /Od helpers verified: 0x42df20 = i*i+j*j (magnitude_squared2d), 0x42def0 = sqrt(0x42df20) (magnitude2d),
  0x42dd30 = 2-term dot (dot_product2d). HCEX DIA2Dump: params source_position/source_vector/friend_position/
  friend_direction_to_aiming_vector + local `union real_vector2d source_planar_direction` (= actors.h + candidate).
## 9. VERDICT: APPROVE (verdict FUNCTIONS_EXACT_OBJECT_BLOCKED is accurate)
- Every worker claim re-measured independently and reproduced (gates, audit, surplus identity, provider link, data,
  storage, strip tests). Changes are TU-local (actor_perception.c) + 5 in-place symbols.json lines; no header/config.json.
- PACKET A (fallback.patch): ADMISSIBLE NOW under the committed rule-6 exception + ruling 20260921 #1. The knowledge respell
  is semantically identical to production, /Od-attested (sentinel, single return) and January-attested (cmp ax,0xffff at
  0xcb); the inline flip is robust across all /Od spellings tested (not steering). No pragma. Blockage bytes unchanged ->
  no park drift. Optional: use lab/k_od.txt (fully /Od-shaped, equally exact) for the knowledge body.
- PACKET C: ADMISSIBLE (publics + no external refs + codegen-inert + scratch regen touches 1 object). If B is held, apply
  review-built A_plus_C.patch INSTEAD of fallback.patch, plus symbols_static.patch + csplit-only regen.
- PACKET B: HOLD until an explicit owner yes on owner-packet s5. The ruling request must state that THREE constructs are
  load-bearing and carried only by first-party /Od attestation: (1) aggregate view copy (field copy -> [size 384!=400]);
  (2) empty then-arm of the hes<0.36 test (plain `!(hes<0.36f)` -> [sha]); (3) explicit `else blockage = 0;` (drop ->
  [size 384!=400]); and name every new COMDAT (_normalize2d, _scale_vector2d, _dot_product2d, _negate_vector3d; all
  byte-identical, provider link PASS). On yes: production.patch, retire the parked.json blockage entry.
- Whole object stays blocked by PRE-EXISTING _distance3d/_distance_squared2d vs action_vehicle NODUP (LNK2005).
- Integrator: full ninja + stable per-function sweep after applying; csplit-only regen after symbols.json, then full
  ninja before stable_verdicts; do not run --forbid-emitted-symbol _point_from_line3d on this unit.
