# damage (source/objects/damage) - fifty-objects wave worker ledger

Base tree 954eebd2. Task: close `_area_of_effect_cause_damage_to_object` (1328) and `_object_damage_body` (1360).

## 0. Baseline (production, gate --all)
exact 32 / residual 2 / unwritten 0.
- AOE: size 1312!=1328, relocs 52!=51, sha (production still has pre-A1 body)
- body: size 1392!=1360, relocs 44!=43, sha (production still has typed unit_get re-lookup)

## 1. Prior work read (do-not-repeat)
- damage_obj_area_of_effect_cluster_20260904, damage_obj_opus5_100k_waveD, _150k_w2, _150k_w3,
  astra_90pct_research8_damage (+independent). Parks: both "unclassified".
- Best prior scratch: opus5-150k-house-clean-20260914/scratch/workers/damage/park_rebase_candidate.c
  (A1 AOE scoping + v1 body child walk + s1 clamp split): AOE 1312/51 (one x87 kill form
  fxch st(1);fstp st(0) vs fstp st(1)); body 1360/43 (4 packets: dying-loop edi reload, eax/ecx
  effect index, ecx/edx resistance reload, integer vs x87 copy of *body_damage).
- DO NOT REPEAT: AOE radius_delta removal, if/else clamp, 2-step PIN, inverted <=, ?:, scale=1 preinit,
  function-scope clamp locals, ?:+PIN, explicit if/else-if clamp, 40+ p*/dvar/km/dp1 probe forms,
  collision_test_line helper (astra: emits surplus COMDAT, same bytes).
  Body: hoist body_vitality, remove region local, block-removal bisections, int-copy probe families,
  if/else actual_damage, param reuse as loop index, while dying loop.
- NOTE body v1 uses (struct unit_datum *)object_get(child) raw downcast: park says rejected under typed
  access; waveD argued same TU object_deplete_body uses same idiom (admitted exact).

## 2. cand0 = production + prior best hunks (A1 AOE scoping, s1 clamp split, v1 body child walk, indent fix)
gate: exact 32 / residual 2. AOE 1312 (relocs 51 OK) one x87 kill-form (T 0x42e fxch st(1); fstp st(0) vs O fstp st(1));
body 1360/43 sha: 4 packets reproduced exactly as in w2/w3 ledgers.

## 3. /Od readouts (odbuild fn 0x76c230 AOE -> od_aoe.txt, fn 0x76ffc0 body -> od_body.txt)
AOE /Od: locals in decl order object, object_definition, damage_effect, can_damage, did_damage, infection_form,
 core-block: x_axis(12), y_axis(12), collision_blocked(byte), axis(12), short ray index (for i<4, switch(i)),
 collision(80), offset(12), offset_point(12); direct branch: collision(80) and collision_test_line(...) with
 can_damage=FALSE directly (no blocked flag in direct branch); infection block: named unit_definition ptr,
 real toughness; clamp: distance, scale, radius_delta; ?: temp for scale then PIN(scale,0,1) (= w2 s4, inert).
 Recursion: child call real, sibling call real (later build /Od, no TCO).
Body /Od: object, damage_amount(=total_damage then *= mult; later build), ignore_difficulty, max, inverse (?: temp),
 actual_damage, difficulty, region, player_controlled, child_object_index, CHILD_OBJECT [ebp-0x2c] (object_get),
 CHILD_UNIT [ebp-0x30] = plain copy of child_object inside the TEST_FLAG(unit mask) block, body_vitality,
 short dying_region_index, region. Later build has 13th param should_do_actual_damage (HCEX confirms) - not January.
 No region_damage local in /Od (re-reads object->region_damage[]).

## 4. cand1 = cand0 + /Od-attested child walk (child_object = object_get(); if unit mask { child_unit =
 (struct unit_datum *)child_object; if (player_index != NONE) {TRUE; break;} } next from child_object)
Hypothesis: replaces the park-rejected raw `(struct unit_datum *)object_get(...)` with the admitted units.c idiom
 (units.c:1723, 8093 unit_update_driver_and_gunner) attested by the /Od frame slots -0x2c/-0x30 for THIS function;
 may also shift IL-local ordinals for the tail ties.
Result: body bytes IDENTICAL to cand0 (same 4 packets, 1360/43). House-rule improvement, no byte change.

## 5. STORAGE EVIDENCE (new): pdb_storage.py source/objects/damage -> 4 disagreements with cachebeta publics:
 _area_of_effect_cause_damage_to_object, _damage_globals, _object_damage_body, _object_destroy_notify_children
 are ABSENT from cachebeta publics => file-static in January (split storage class 2 comes from symbols.json, not
 independent). All other damage.c statics already agree. Prior ledgers never tried `static` on the two residuals
 (astra: "later static linkage was not imported").
Hypothesis: static changes VC7's treatment (private function) -> allocator/x87 decisions in the body tail.

## 6. cand2b = cand1 + `static void object_damage_body(` (defined before its only caller; no prototype needed)
RESULT: gate exact 33 / residual 1. _object_damage_body EXACT (all 4 tail packets closed). object_cause_damage stays exact.
## 6b. cand2 = cand2b + static AOE (prototype + definition): exact 33 / residual 1; AOE unchanged (1312, one x87 kill form).

## 7. AOE x87 kill-form oracle (LAB ONLY, oracle/run.py): 0..8 unused `real` fillers and 0..8 used `long` fillers at
 function top: kill form stays `fstp st(1)` in all 18 -> not an IL-local-count (M8-style) tie at function scope.

## 8. AOE /Od-faithful restructure (byte-inert house-rule cleanups)
- v1_declorder (distance, scale, radius_delta per /Od frame): AOE sha 93606db1 = cand2 (inert).
- v2_noepicenter (drop `epicenter` pointer local; /Od has none; use &damage->epicenter): sha 93606db1 (inert =>
  the epicenter local was an unattested decoration; removed).
- od1 (mk_od.py): /Od names axis/x_axis/y_axis/offset/offset_point/collision; `boolean collision_blocked = TRUE`
  declared in the core block (after x_axis,y_axis per /Od order); natural `short ray_index; for (ray_index = 0;
  ray_index < 4; ray_index++)` (replaces the invented two-counter ray_index/rays_remaining loop - VC7 strength
  reduction produces January's down-counter itself); genuine collisions.h `collision_test_line` inline at BOTH /Od
  call sites (0x76c50b, 0x76c558; replaces hand-expanded vector_from_points3d+collision_test_vector, rule 6);
  direct branch `if (collision_test_line(...)) can_damage = FALSE;` as /Od. RESULT sha 93606db1 = cand2 (inert).
  Still only the x87 kill form differs.
- od2 (?: + PIN per /Od temp [ebp-0x168], decl order distance/scale/radius_delta): sha 93606db1 (inert).
- od3 (od1 + /Od infection block: named `struct unit_definition *unit_definition` in the unit-mask block,
  `(toughness>0 || flag) && damaged_one_object` single if, genuine `real_random()` helper instead of hand-expanded
  real_seed_random(get_global_random_seed_address()), `toughness / 4.f` (/Od divss by 4.0 @0x93df00; VC7 folds to
  fmul 0.25); tail `if (TEST_FLAG(...) && object->first_child != NONE)` and `if (damage_next_object &&
  object->next != NONE)` without child/next locals (/Od frame has none)): sha 93606db1 (inert).
- p1 (PIN inside the then-arm, production form, on od1): fstp st(1) + wrong layout (extra jmp) - as w2 A1.
- LAB lab.py (semantic-changing, never landing): extra distance/delta uses, quotient-first, delta-before-call:
  all keep fstp st(1) or restructure (spills); no flip.
- LAB lab2/lab3 (semantic-changing, never landing): extra uses of delta/distance in the then/else arms or 2-6
  extra uses of distance before/after delta's def (FP-priority hypothesis "D coloured before delta => scale shares
  D's register"): all fstp st(1) (or spills). FP priority-order hypothesis REFUTED in this form.
- LAB lab4 (structural): no-if => delta consumed by fdivp (block-local values are popped at last use; cross-block
  values are killed at block END); else=distance, pre-init w/o else, no PIN, store/PIN in then: all fstp st(1).
- lab5 arm order (natural spellings): `<= 0` first, `!(>0)` first, `?:` with <=: fstp st(1), wrong layout;
  `0.f < radius_delta`: size 1328 by coincidence (fcomp st(1)+jp compare, +2 bytes) but still fstp st(1) - rejected.
- ORACLE tuname.py (LAB, TU declared-name-count law, 0..23 opaque struct tags immediately before AOE; covers the
  measured period 21): kill form fstp st(1) for every k; exact 33 each. The AOE kill is NOT a TU name-count tie.
- od1b (collision/offset/offset_point declared in the loop body): sha 93606db1 (inert; frame identical).
- DBL probes (hs.c double lever precedent): double scale/distance/radius_delta in 7 combos: constants become qword
  (__real@3ff0...) or structure changes; kill still fstp st(1). REFUTED (January constants are dword).
- killscan.py census of January split objects: `fstp st(1)` dead-value kill 36x (35 mid-block), `fxch st(1);
  fstp st(0)` only 3x (autoaim, unit_adjust_projectile_ray, AOE). In the two exact fxch cases the killed value's
  last use was an `fld st(n)` COPY; hud_messaging_update (exact) has the SAME then-arm pattern (fsub; fdivp; fsubr
  1.0; fstp st(1)) with PIN inside the arm and uses fstp st(1).
- ORACLE tuname 24..69: all fstp st(1) (0..69 covered).
- ord1 = od3 with all 34 definitions reordered to January's emission order (law (a); no deferrals needed):
  33 exact, AOE unchanged (fstp st(1)). Definition order is NOT the lever.
- FIX: mk_od.py had emitted the 601 assert file string with single backslashes (C4129) in od1..od3; regenerated.
  strict_eq.py (coff_compare.section_infos_equal) now proves AOE of od1, od2, od3, v1, v2 STRICT-EQUAL to cand2's
  AOE (bytes + relocation targets); body of od3 STRICT-EQUAL to January. (Oracle/lab runs made on the buggy copies
  measured only the x87 kill form, which a string literal cannot affect.)
- LAB agg.py: radius_delta as a real_vector2d member (scalarised aggregate, w2 p8 hypothesis) or distance as a
  member: fstp st(1). Aggregate-member hypothesis REFUTED in AOE context.
- DIAGNOSTIC prag.py (pragmas never land): optimize p/g/s change everything; y/a/w/t keep fstp st(1). No mode
  selects the fxch idiom.
=> AOE STOP RULE: >60 new evidence-based/lab shapes this wave (plus >100 in prior lanes); one x87 kill idiom left.

## 9. FINAL candidates and audit (wave 3)
### final.c (= od3 + static object_destroy_notify_children + static damage_globals + /Od region-damage form)
- gate --all --forbid-emitted-symbol _point_from_line3d: exact 33 / residual 1 (AOE only), guard passed.
- _object_damage_body STRICT-EQUAL to January (strict_eq.py); every other row EXACT; AOE strict-equal to cand2
  (1312 / 51 relocs / normalized sha 93606db11e3fdb8fba3e48d2649b3f193f13850837b1d29e29a133c182cb4b8a).
- rd1 check: dropping the non-/Od `byte region_damage` local keeps the body strict-equal (inert; /Od re-reads the field).
- object_audit: only AOE section + 4 storage diffs (split=2 from symbols.json, ours=3); all .rdata/.bss sections ok.
  => config/symbols.json copy with "static": true on _object_destroy_notify_children, _object_damage_body,
     _area_of_effect_cause_damage_to_object, _damage_globals (scratch/w/damage/config/symbols.json; 4 lines edited in place).
  No other split/base object references those four names (xref4.py).
- surplus (surplus_identity_cand.py): 10 candidate-only code COMDATs, 0 not identical (new: _collision_test_line and
  _real_random, both identical to January's selected action_charge copies). provider_link.py final.obj: PASS (both orders).
- data: build/report.json damage data 760/760 100% (.bss 76, .rdata 684); unchanged by the candidate (audit ok).
- fake_match_scan: 0 leads. CL /W3 /Zs: no TU-local warnings (no C4013).
### minimal.c = production + /Od child walk + 4 statics (no AOE body change): exact 33; body STRICT-EQUAL January;
  AOE STRICT-EQUAL to production base (park unchanged).
### Deliverables: production.patch (full = final.c), production_minimal.patch (= minimal.c), config/symbols.json.
  Both patches `git apply --check` clean and reproduce final.c / minimal.c byte-exactly (CRLF) in an autocrlf=true
  throwaway repo (session scratchpad).
### Parks: retire `_object_damage_body` (strict exact). `_area_of_effect_cause_damage_to_object` stays parked:
  with production.patch re-baseline its base to size 1312 / relocation_count 51 / normalized_sha256
  93606db11e3fdb8fba3e48d2649b3f193f13850837b1d29e29a133c182cb4b8a (objdiff % to be measured by the integrator);
  with production_minimal.patch the park is unchanged.
### Admission blockers still in damage.c for a future whole-object claim (house rule (g)):
  consumer-local prototype `boolean unit_unsuspecting(...)` (owner units.c; no units.h prototype), consumer-local
  `extern boolean debug_damage;` (public in cachebeta, no definer in source), and the AOE x87 kill-form residual.
### Disclosure for review: the body child walk downcast `(struct unit_datum *)child_object` after
  TEST_FLAG(_object_mask_unit, ...) is the admitted units.c idiom (units.c:1723 rider, :8093 child_unit) and is
  /Od-attested for THIS function (0x770491 object_get(child,-1) -> [ebp-0x2c]; unit-mask test; plain copy
  [ebp-0x2c] -> [ebp-0x30] with no verify call; player_index test through [ebp-0x30]; next from [ebp-0x2c]).
  It is NOT the park-rejected repeated raw `(struct unit_datum *)object_get(...)` form. Strip tests: without
  `static` the body is residual (cand1: 4 tail packets); the typed unit_get re-lookup is structural (1392/44).
## STATUS: _object_damage_body EXACT (+1 fn, 1360 padded / 1347 meaningful). AOE blocked (x87 kill idiom). Object incomplete.
