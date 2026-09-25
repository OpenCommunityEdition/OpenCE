# r2w1-actors LEDGER (source/ai/actors, round 2)
Base f6d00a8c (tree HEAD b7c1b7d9 = baseline commit on top). Claimed source/ai/actors as r2w1-actors.
Read: brief s0-9, triage_ai_a LEDGER, closeout 0919, opus5 w2/w3/w3d/waveD do-not-repeat, Lane A res7 NOTES.

## P0 baseline
- base.c = unmodified source/ai/actors.c. gate --all: exact 74 residual 2
  (_actor_input_update 2384 [sha]; _actors_spawn_from_unit [size 688!=672, sha]).

## P1 two-hunk port of Lane A res9 p1 onto current actors.c (cand.c)
- Hypothesis: spawn residual (688 vs 672, S2 207/203) = rule-6 hand expansion + OR-chain instead of /Od-attested ternary local.
- Change: hunk 1 `long source_actor_index = swarm != NONE ? swarm : actor_index; if (source_actor_index != NONE)`;
  hunk 2 hand-expanded x/y/z replaced by `point_from_line3d(&placement_data.position, &placement_data.forward, 0.3f, &placement_data.position);`.
- Measured: base (production) gate 74/76 (spawn [size 688!=672, sha]); cand gate 75/76 `EXACT 672 _actors_spawn_from_unit`.
  Only row delta is spawn residual->EXACT; _actor_input_update residual [sha] unchanged in both.

## P1 admission checks (cand.obj)
- object_audit: 137 January symbols 0 differ; FAIL(1) = only _actor_input_update .text DIFF; new surplus = +.text _point_from_line3d 48 B
  (base vs cand audit diff: spawn DIFF->ok, +_point_from_line3d surplus). Every other owned section base vs cand: section_infos_equal (180 compared, only spawn differs).
- surplus identity (surplus_cand.py = surplus_identity logic on the candidate path): 17 candidate-only code COMDATs, 0 not identical;
  _point_from_line3d IDENTICAL to build/split action_charge copy. cand COMDAT selection = 2 (ANY); base board has 24 definers, 0 NODUP (nodup_census).
- provider_link --baseline=build/base actors.obj: `_point_from_line3d provider source/ai/action_charge PASS` (both orders inside the tool); full provider_link: PASS (45 PASS rows, 0 FAIL).
- pdb_storage (production): 137 split symbols, 0 disagreements. _point_from_line3d is a cachebeta PUBLIC ([0001:00000F80]) -> external, consistent with non-static header __inline.
- January actors.obj neither defines nor references _point_from_line3d (all-inlined TU) -> this is the ALL-INLINED class admitted by owner ruling 5
  (docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md s3 item 2; house rules l.33-40). Conditions: genuine shared-header __inline (real_math.h l.1080) OK;
  byte-identical COMDAT OK; strict-exact caller OK; provider link OK; zero-regression full sweep = integrator (object-local sweep: only spawn changed).
- Refusal history: Lane A R27 (rejected_hypotheses l.1074/1128-1134) refused ONLY because "_point_from_line3d keeps its explicit guard" + hand expansion refused.
  Current production spawn body is itself that hand expansion (rule 6 debt) -> this patch removes a rule-6 violation.
- Park measurements: cand _actor_input_update normalized sha f6ff0151... == parked.json "base" sha -> its park stays valid; spawn cand sha 38d0b2f1... == target -> park must be removed.

## P2 /Od-attested short counter (probe_short.c)
- Hypothesis: /Od 0x44e008 `mov word ptr [ebp-8],ax`, 0x44e3ce `movsx ecx,word [ebp-8]; add ecx,1; mov word [ebp-8],cx`, 0x44e3de `mov ax,word [ebp-8]`
  => spawned_actor_count is `short` (matches the short return type). Production has `long`.
- Change: `long spawned_actor_count = 0;` -> `short spawned_actor_count = 0;` on top of P1.
- Measured: --fn EXACT 672; --all 75/76 rows identical to P1; every owned section of cand_short.obj section_infos_equal to cand.obj; spawn raw bytes identical.
  => byte-inert; the type is decided by /Od attestation, not bytes. RECOMMENDED variant (removes the long-vs-short disclosure).
- HCEX (hcex_spawn.txt): params unit_index/actor_variant_definition_index/count/velocity; stack locals placement_data, acceleration only
  (enregistered scalars absent) -> no first-party name for source_actor_index (descriptive name) or the counter.
- P1 (a) refresh call (+drop actor_swarm_member_get macro): gate 74/2; input_update sha f6ff0151 == BASE; whole object section-identical to base (only $L label numbers). Byte-inert rule-13 fix.
- P2 (ab) a + add_vectors3d via swarm_center ptr: sha e71700a6, bytediff 575->589; swarm add loop x87 operands reversed (fld center; fadd comp vs Jan fld comp; fadd center) - M8 phase; cross site1 closes, site2 worse; 0x6a6 split unchanged; emits _add_vectors3d COMDAT.
- P3 (ab2) ab without swarm_center ptr local (/Od frame has none): sha 8924c8fd, bytediff 588, same add reversal. 
- P4 (ac) a + /Od threat loop (object_get threat, else-if projectile block with named projectile alias, if/else-if two stores; drop actor_attached_threat_get macro): sha f476fabf, bytediff 575->12, 691 insns (=Jan). BIG MOVE.
- P5 (abc) ab + c: bytediff 35. (ab2c): 47.
- P4 detail (ac): alndiff vs Jan = ONLY cross_product3d site 2 (3 of 6 terms fld/fmul transposed: 0x721 k-t1, 0x72d j-t1, 0x732 j-t2)
  + relocation-name artifacts (symbol vs defined-noncode literal naming, present in every build). The unit live-range split
  at 0x6a6 (lea edx,[ebx+0x1ec]) now MATCHES, join reload gone, [ebp-4]/[ebp-8] slot swap gone, 0x4d3 eax piece matches,
  site 1 fully matches. Mechanism: the threat loop's typed-object/alias shape changes the threat webs; the old single ||
  store merged differently. Residual = pure M8 phase at cross_product3d site 2.

## I1 _actor_input_update: /Od swarm-arm shape (i1.c)
- Hypothesis (triage + /Od 0x4486c0..0x4487db): swarm loop = short counter; component = swarm_component_get(component_indices[i]);
  REAL CALL actor_swarm_component_refresh(unit_indices[i], component_indices[i]) (0x44bb30); REAL CALL add_vectors3d(&component->position,
  &swarm->swarm_center, &swarm->swarm_center) (0x44e8e0 = a+b -> result); scale_vector3d(&center, 1/count, &center). Production hand-expands both
  helpers (rule 13 / rule 6 debt) with a swarm_center alias + long counter + (short) casts.
- Measured: 75/76; input_update 2384 [sha] d8ebea72 (was f6ff0151). The swarm-arm region stays byte-identical to January (VC7 inlines the
  static refresh at this site, consistent with January's missing call reloc), but the residual is NOT closed: the unit [ebp-8]/[ebp-4] slot swap and
  the 0x6a6 unit split remain, and the cross_product3d x87 leaves rotate (different, not fewer). Newly emits _add_vectors3d (P1 class: non-exact caller) -> not landable alone.
- Kept as the /Od-faithful base for further input_update probes.
- /Od local inventory (0x4486c0, declaration order = frame order): -8 actor, -0xc actor_definition, -0x10 swarm, -0x14 short
  component_index, -0x18 component, -0x1c unit, -0x20 vehicle_index, -0x24 vehicle(object), -0x34 center_of_mass,
  -0x3c vehicle alias, -0x40 vehicle_definition, -0x44 firing variant, -0x45 migrate, -0x4c encounter, -0x50 actor_squad,
  -0x54 vehicle_squad, -0x58 prevehicle encounter, -0x5c actor_debug_info, -0x60 encounter_definition, -0x61 mismatch
  flag, -0x68 threat_index, -0x6c threat(object), -0x70 projectile, -0x74 biped, -0x78 gunner vehicle, -0x7c gunner def.
  NO swarm_center pointer, NO stored_prevehicle_encounter local (but Jan reads the field before the 4 stores -> keep).
- P6 (acD) +/Od firing-variant local (assigned after gunner store, Jan store-before-call): bytediff 20 (site1 i-terms break).
- P7 (acE) +/Od prevehicle encounter local: byte-identical to ac (inert).
- P8 (acF) /Od object-typed parent + vehicle alias (drop actor_parent_vehicle_candidate_get): bytediff 17 (site1 k-t1 breaks).
- P9 (acC) /Od short component_index (no casts): bytediff 21.
- P10 (acN) /Od separate cross_product3d; normalize3d(&left) statements (/Od rebuilds the arg from actor): identical to ac.
  => site-2 terms k-t1, j-t1, j-t2 survive every +1; not pure count (D and F both +1, different outcomes).

## I2/I3 separate normalize3d statement (i2.c on cand, i3.c on i1)
- Evidence: /Od 0x4491d5..0x44922d: cross_product3d(global_up3d, &looking, &left) (real call 0x43e710), then normalize3d(&left) with
  the ADDRESS recomputed (push actor+0x164), not the returned pointer => separate statements; production nests normalize3d(cross_product3d(...)).
- Measured: i2 sha == cand sha (f6ff0151) -> byte-INERT; i3 sha == i1 sha -> inert. Not a lever (kept /Od-faithful in later probes only if needed).
- Metric: real_regions cand = 53 reported / 24 REAL.
- P11 (acP) ac minus swarm_center ptr, hand add kept: bytediff 75 (swarm add + site1/site2 disturbed) - rejected (rule 6 anyway).
- P12 (odfull) ac + B2(add_vectors3d, no ptr) + C + D + E + F + N: bytediff 37; SWARM LOOP MATCHES with the /Od add_vectors3d
  call (fld comp; fadd center) - add_vectors3d is compatible with January's bytes at the right phase; site1 k-t1,i-t1 and
  site2 k-t2,i-t2 wrong.
- P13 (acG) /Od named firing_position_type_mismatch flag (if/else-if sets, then if(flag)): encounter-check code IDENTICAL to
  January (flag fully threaded); only x87 phase moves (bytediff 37).
- P14 (acH) /Od migrate order (!= -> migrate=TRUE; else if squad test): identical to ac (inert).
- P15 (acS) /Od single exit (swarm arm if(swarm_unit_index != NONE) sample; else-block for the rest): identical to ac.
- P16 (odall) full /Od-attested inventory = odfull + G + H + S: bytediff 12: swarm OK, site1 OK, site2 k-t1,k-t2,j-t2 wrong.

## I4 FULL /Od-faithful non-swarm arm on top of I1 (i4.c = mkvar.py i1.c a b c d e f g n) -> 76/76 EXACT
- /Od frame (od_actor_input_update.txt) named locals in declaration order (user area above the compiler temps at -0x80..-0x90):
  -8 actor, -0xc actor_definition | swarm: -0x10 swarm, -0x14 short i, -0x18 component | else: -0x1c unit, -0x20 vehicle_index,
  -0x24 parent object (object_get_and_verify_type(idx,-1) via ?: temp -0x80), -0x34 center_of_mass (RTC) | vehicle if: -0x3c typed vehicle alias
  (pure copy of -0x24), -0x40 vehicle_definition | gunner if: -0x44 firing variant definition (named call result) | fake-encounter if:
  -0x45 migrate(byte), -0x4c encounter, -0x50/-0x54 squads | migrate if: -0x58 prevehicle encounter (named; RMW store) | encounter block:
  -0x5c actor_debug_info, -0x60 encounter_definition, -0x61 mismatch flag (byte: =0, then =1 in if / else-if) | -0x68 threat_index,
  -0x6c threat, -0x70 projectile alias | -0x74 biped | -0x78/-0x7c gunner vehicle + definition.
- Edits: a parent object + typed vehicle alias in vehicle block; b named firing_variant_definition; c /Od polarity `!=` -> migrate=TRUE / else squad test;
  d named prevehicle encounter local; e drop production-only stored_prevehicle alias (/Od reads actor->meta flag directly);
  f mismatch flag if/else-if (/Od 0x448d81..0x448dca); g projectile alias + two stores (/Od 0x448f46..0x448f9a); n separate normalize3d.
- MEASURED: gate `== exact 76 residual 0`; _actor_input_update 2384/89 sha bf2e8744 == January target; real_regions 8 reported / 0 REAL (addressing-only).
- NEXT: bisect load-bearing subset; admission (surplus _add_vectors3d identity + provider link; casts; house rules).
- ORACLE (instrument only, never landing): KIND=dead (unreferenced long locals at fn top) D=1..7 ALL byte-identical to ac
  -> the x87 key here is NOT the dead-local count. KIND=init (dead stores) D=2 closes site2-j but not all; D=5/6 site2 OK,
  site1 k-t2 wrong; none closes. => not a single count phase; motivates a bounded search over /Od-attested features only.
- P17 SEARCH (search.py, 48 points, every point composed only of /Od-attested shapes; fixed ON a,c,E,H,N,S; swarm
  hand/addp/add x toggles C,D,F,G): **addG = EXACT 2384** (gate 75/76; only _actors_spawn_from_unit residual).
  addG = a + c + add_vectors3d on &swarm->swarm_center with NO swarm_center pointer local + G named mismatch flag
  + E prevehicle encounter local + H /Od migrate order + N separate normalize3d statement + S single-exit swarm arm.
  long counter (C off), no firing-variant local (D off), no parent/vehicle alias (F off).
  Near misses: addCDG 4 B, addCFG 8 B, addCG 9 B; hand (=ac) 12 B.
- P18 STRIP TESTS (strip/*.c, minimal.py) around addG:
  core = a+c+B2+G: 9 B off (x87). coreE (= core + E) EXACT. coreES EXACT. +H/+N/+S alone on core: identical to core.
  noG (addG minus G): 47 B off. noB2 (hand add + pointer local, rest of addG): 37 B off. noC_threat: 577 B off (split returns).
  noA_refresh (hand-inlined refresh body, rest of addG): EXACT, sha bf2e8744 - (a) is byte-inert as before.
  => load-bearing: c, B2, G, E (each /Od-attested). Inert: a, H, N, S. Omitted /Od features that BREAK exactness: C (short
  counter: 9 B), D (firing-variant local), F (object parent + vehicle alias) -> treated as later-revision edits. DISCLOSE:
  E and G are selected against the x87 leaf tie only (a fitted subset of /Od-attested locals; cf. round-1 action_charge B3).
- LANDING SHAPE chosen for input_update = strip/coreES.c: a (refresh call) + c (/Od threat loop) + B2 (add_vectors3d on
  &swarm->swarm_center, no pointer local) + G (named mismatch flag) + E (named prevehicle encounter local) + S (single-exit
  swarm arm). H and N left at production spelling (byte-inert; optional /Od-faithful alternatives).
- P19 spawn: Lane A res9 p1 two hunks (/Od 0x44dfe0 ternary source_actor_index + point_from_line3d call replacing the rule-6
  hand expansion) applied on production alone (cand_spawn_only): 75/76 (spawn EXACT 672). On coreES (cand_full):
  **gate 76/76 EXACT, residual 0**.

## Bisection of I4 (single edits on I1; leave-one-out from I4)
- singles on I1: c, e inert (sha == I1); g strongest alone (13 REAL); a/b/d/f move bytes but none closes alone.
- leave-one-out from I4: dropping c or n keeps EXACT (inert); dropping a, b, d, e, f, g or the swarm shape (lo_swarm = all edits on cand.c) each
  breaks exactness (4, 4, 3, 2, 8, 22, 6 REAL regions respectively). => load-bearing set = swarm(/Od I1) + a + b + d + e + f + g, every one /Od-attested.
- c (/Od `!=` polarity) and n (separate normalize3d) are byte-inert and /Od-attested -> kept in final for /Od fidelity (droppable, no byte effect).

## FINAL candidate final.c (mkfinal.py) = probe_short.c + /Od-faithful actor_input_update -> 76/76 EXACT
- Typing per /Od: parent object and threat fetched with object_get (mask -1, /Od push -1) as struct object_datum *, typed views taken only after the
  type test ((struct unit_datum *) vehicle view /Od -0x3c; (struct projectile_datum *) projectile view /Od -0x70). The three TU-local polymorphic
  accessor macros (actor_parent_vehicle_candidate_get / actor_swarm_member_get / actor_attached_threat_get) lose all users -> removed.
- Swarm counter named `short unit_index` (file convention l.648/761; /Od short at -0x14).
- Gate: `== exact 76 residual 0 unwritten 0`; input_update sha bf2e8744 == target; spawn 672 EXACT.

## SE1 single-exit swarm/else structure (se1.c -> adopted as final.c; previous final kept as final_v1.c)
- Evidence: /Od 0x44884e `cmp [actor+0x18],-1; je 0x448871` skips the call (source `if (swarm_unit_index != NONE) sample(...)`), then
  0x448871 `jmp 0x4494fa` over the non-swarm arm (source if/else). Production had two early returns + a bare block.
- Change: `if (!= NONE) { actor_input_sample_position(...); }` and `} else {` in place of the bare block (no re-indent needed; one bottom `return;`).
- Measured: 76/76, input_update sha bf2e8744 (identical bytes to final_v1) -> byte-inert, adopted for /Od fidelity + single-exit law.

## SP1 spawn random helper (sp1.c -> adopted as final.c)
- Evidence: /Od spawn 0x44e13e/0x44e317/0x44e341 call fn 0x42f360 with TWO float args; /Od 0x42f360 body = real_seed_random_range(
  get_global_random_seed_address() [0x6c97a0], lo, hi) = real_math.h real_random_range (l.1464). Production hand-expanded it (rule 13 debt).
  actors.obj already emits the _real_random_range COMDAT (used by actor_died l.3330), so no new surplus.
- Change: angle / speed_factor / vertical_factor = real_random_range(lo, hi).
- Measured: 76/76; every owned section of sp1.obj section_infos_equal to previous final.obj -> byte-inert; adopted.

## r2w1b NOTE (second agent under the same label r2w1-actors; claims.log shows two CLAIMs 08:09:30/08:09:31)
- Two workers ran this task concurrently in this slug. r2w1b entries are the P-series (P1-P19, oracle, search.py, strip/,
  cand_full.c); the other worker's are P1/P2(spawn), I1-I4, bisection, FINAL, SE1, SP1 (final.c). File names do not collide
  (r2w1b files: mk.py, a*.c, ac*.c, odfull/odall, search/, strip/, minimal.py, mkfull.py, cand_full.*, r2w1b_*).
- INDEPENDENT CONVERGENCE: both searches reach the January sha bf2e8744 for _actor_input_update from /Od-attested shapes only.
  r2w1b exact point (cand_full / strip/coreES) = threat loop + add_vectors3d(no pointer) + mismatch flag + prevehicle encounter
  local + refresh call + single exit, long counter, NO firing-variant local, NO parent/vehicle alias, stored_prevehicle alias kept.
  Other worker's exact point (final.c) = the COMPLETE /Od inventory (short counter, firing-variant local, parent object +
  vehicle alias, direct stored_prevehicle read, /Od migrate polarity, separate normalize3d) with leave-one-out showing every
  phase-moving feature load-bearing.
  => the x87 leaf tie at cross_product3d site 2 has more than one /Od-composed solution; the complete-/Od set needs NO selection
  among attested features, so it is the stronger landing. r2w1b RECOMMENDS final.c; cand_full is corroboration only.
- r2w1b cross-check of final.c (snapshots r2w1b_twin_final_snapshot.c sha ade5b6cc, r2w1b_twin_final_v2.c sha 508ab4ec incl SP1):
  gate 76/76; every section section_infos_equal to r2w1b cand_full.obj (objcmp: only $L label names differ);
  object_audit PASS; surplus_identity 18 candidate-only COMDATs 0 not identical (_add_vectors3d == actor_combat copy,
  _point_from_line3d == action_charge copy); provider_link --baseline PASS both new symbols; nodup_census 17/24 definers 0 NODUP;
  pdb_storage 0 disagreements; fake_match_scan 0 leads (on cand_full).
- NOTE w3 A5 rejection ("stored_prevehicle_encounter field is read before the four input stores" => local required) is REFUTED:
  the direct-read form in final.c is exact (VC7 hoists that load); both spellings are byte-compatible in context.
- Patches (git apply --check clean at the worktree HEAD): r2w1b_actors_c_v2.patch (from r2w1b_twin_final_v2.c),
  r2w1b_parked_json.patch (drop both actors parks, lines 667-704), r2w1b_config_json.patch (actors.c NonMatching -> Matching).
- r2w1b wrote MANIFEST_r2w1b.md (cross-check manifest; the other worker's MANIFEST.md, if present, is primary). Patches verified
  individually and combined with git apply --check (clean); git status shows no tracked modification. r2w1b releasing claim.

## Admission audit on FINAL final.obj (sha1 ed390f3a; source final.c sha1 0f953992)
- gate --all: 76/76 EXACT (final.gate.txt). relocdiff input_update: 89/89, 0 differing rows. alndiff: only reloc-spelling rows (0 REAL).
- object_audit: OBJECT AUDIT: PASS; 137 January symbols, 0 differ; every January-owned section ok. New surplus vs production:
  +_add_vectors3d (48, 0 relocs) and +_point_from_line3d (48, 0 relocs).
- surplus identity (surplus_cand.py): 18 candidate-only code COMDATs, 0 not identical; _add_vectors3d IDENTICAL to January's selected
  copy (build/split actor_combat), _point_from_line3d IDENTICAL to build/split action_charge. Both selection ANY; base board definers
  17 / 24, NODUP 0 / 0 (nodup_final.txt).
- provider_link --baseline=build/base actors: `_add_vectors3d provider source/ai/actor_combat PASS`, `_point_from_line3d provider
  source/ai/action_charge PASS` (tool links both input orders). Full provider_link: SELECTED-PROVIDER LINK PASS (46 PASS rows, 0 FAIL).
- pdb storage (candidate symbols vs cachebeta publics): 137 split symbols, 0 disagreements; _add_vectors3d and _point_from_line3d are
  publics (external COMDAT correct); _actor_input_update / _actor_swarm_component_refresh absent from publics = static (as in source).
- data: every non-.text owned section section_infos_equal production vs final; .drectve identical; report.json actors data 2288/2288 = 100%.
- rejections: none for actors in config/object_admission_rejections.json. Parks: both actors parks retired (parked_retire.patch, 86 -> 84).
- warnings (/W3 /Zs, warn.py): 6 C4244 (all pre-existing; production had 7 - the spawn `return long->short` one is gone). No C4133/C4013.
  Strip tests: removing the (real_vector3d *) casts in add_vectors3d and the (struct unit_datum *)/(struct projectile_datum *) view casts
  keeps 76/76 with identical sha (only C4133 type warnings) => every cast is byte-inert.
- fake_match_scan: 0 review leads (production also 0).
- git apply --check clean for actors_final.patch, parked_retire.patch, config_status.patch at HEAD b7c1b7d9 (= f6d00a8c + scratch-only).
- Prior-negative relation: opus5 w3 (0914) kept the stored_prevehicle alias because "the field is read before the four input stores";
  in the /Od-faithful context VC7 hoists the read anyway (still exact) and the alias is load-bearing AGAINST exactness (lo_e breaks).
- Objdiff note (not a blocker, pre-existing): _actor_handle_unit_effect (448, strict EXACT at base too) scores 96.8% in objdiff 3.3.1 because
  of 5 compiler-local jump-table labels (internal relocs equal on both sides) -> optional semantic_matches.json entry; it is NOT this lane's delta.

## SD1 /Od declaration order in the spawn loop (sd1.c -> adopted as final.c)
- Evidence (/Od 0x44dfe0 frame, declaration order = slot order, cf. input_update where it matches production order exactly):
  loop body placement_data [ebp-0xb4] (RTC 136), spawned_unit_index [ebp-0xbc], angle [ebp-0xc0] (initialised by the real_random_range
  call BEFORE object_placement_data_new -> declaration initialiser); throw block acceleration [ebp-0xd8] (RTC 12), speed [ebp-0xe0],
  vertical [ebp-0xe4]. Production declared angle first and acceleration last.
- Change: reorder those two declaration groups to the /Od order.
- Measured: sd1/sd2/sd3 all 76/76; sd1 every owned section identical to previous final -> byte-inert; adopted (reviewer law: do not
  contradict the /Od declaration order).

## r2w1-actors (primary worker) CLOSEOUT + concurrency note
- Two agents ran under the label r2w1-actors in this slug (claims.log CLAIM 08:09:30 and 08:09:31). My first ledger section
  ("E0 evidence re-check") was overwritten by the other agent's header; recovered here:
  E0: /Od 0x42e0d0 body = result = v*t + p (point_from_line3d; od_42e0d0.txt, 40+ /Od callers); /Od 0x44dfe0 ternary temp [ebp-0xe8] ->
  [ebp-0x10], cmp -1, actor_get(unit+0x1b8), position.z += 0.3f then REAL CALL 0x42e0d0(&pos,&fwd,0.3f,&pos); real_math.h l.1080
  non-static __inline (REAL_MATH_EXTERNAL_POINT_FROM_LINE3D guard used by 7 other TUs, not actors); house rules l.33-40 exception.
- Label collision: the other agent's "P1 (a)...P19" rows inside my P2 section are ITS probes (mk.py/a*.c/ac*.c/odfull/strip/cand_full);
  my probes are P1/P2 (spawn), I1-I4, b_*/lo_* bisection, FINAL, SE1, SP1, SD1 (mkvar.py/mkfinal.py/run.sh).
- FINAL = final.c sha1 d9dc3764 (= SP1 + SD1 /Od declaration order), final.obj sha1 50d847eb, 76/76 EXACT, OBJECT AUDIT PASS,
  full battery re-run on this exact object (see "Admission audit" above; re-run after SD1: audit PASS, surplus 18/0, provider_link PASS,
  pdb 0 disagreements, 6 pre-existing C4244, fake_match_scan 0).
- PRIMARY production patches: actors_final.patch (sha1 691e8fbc; reproduces final.c byte-for-byte), parked_retire.patch (7e937267),
  config_status.patch (0e7d18f0); git apply --check clean together at HEAD b7c1b7d9.
  r2w1b_actors_c_v2.patch = the SP1 state (0f953992) WITHOUT the SD1 declaration reorder; its object is section-identical to final.obj
  (SD1 byte-inert) -> superseded. r2w1b_parked_json.patch / r2w1b_config_json.patch carry the same edits as mine (park hunk aligned
  on the other brace line; same resulting entries).
- Independent convergence (other agent's cand_full.c: a different /Od-attested subset also hits sha bf2e8744) corroborates that the
  closing lever is the /Od threat-loop/typed-view shape; the complete /Od inventory (final.c) needs no selection among attested
  features, so it is the landing.
