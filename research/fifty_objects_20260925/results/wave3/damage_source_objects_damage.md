# source/objects/damage

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[
 "_object_damage_body"
]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/production_minimal.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/final.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/minimal.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/final.obj",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/minimal.obj",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/od_aoe.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/od_body.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/killscan.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/strict_eq.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/LEDGER.md"
]

## production_changes
There are two alternative patches, and each one applies with `git apply --check` clean. Each also reproduces its candidate file byte for byte (CRLF) in a throwaway repo with autocrlf=true. Pick ONE of them.

(A) PREFERRED: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/production.patch, which gives final.c. Contents:
  (1) `static void object_damage_body(`.
  (2) The body's deathless-player child walk rewritten in the /Od-attested form: `struct object_datum *child_object = object_get(child_object_index); if (TEST_FLAG(_object_mask_unit, child_object->object.type)) { struct unit_datum *child_unit = (struct unit_datum *)child_object; if (child_unit->unit.player_index != NONE) { player_controlled = TRUE; break; } } child_object_index = child_object->object.next_object_index;`. This replaces the extra typed unit_get re-lookup.
  (3) `static` on object_destroy_notify_children, damage_globals, and the AOE prototype and definition.
  (4) The body drops the non-/Od `byte region_damage` local and re-reads object->object.region_damage[] as the /Od build does. This is byte-inert.
  (5) AOE rewritten in the /Od-faithful form. This is byte-inert: AOE is STRICT-EQUAL to the prior best park_rebase_candidate body, with 1312 bytes and 51 relocations. The changes:
     - /Od names axis/x_axis/y_axis/offset/offset_point/collision;
     - `boolean collision_blocked = TRUE` in the core block;
     - a natural `short ray_index; for (ray_index = 0; ray_index < 4; ray_index++)` loop instead of the invented two-counter loop;
     - the genuine collisions.h `collision_test_line` at both /Od call sites instead of the hand-expanded vector_from_points3d+collision_test_vector;
     - the direct branch as `if (collision_test_line(...)) can_damage = FALSE;`;
     - no `epicenter` local;
     - an infection block with a named `struct unit_definition *unit_definition`, the genuine `real_random()` helper instead of the hand-expanded real_seed_random(get_global_random_seed_address()), and `infection_form_toughness / 4.f`;
     - the child and sibling recursion without the child/next locals.

(B) MINIMAL: scratch/w/damage/production_minimal.patch, which gives minimal.c. It contains only items (1), (2) and (3). AOE bytes are unchanged, so the AOE park is untouched.

config/symbols.json: a full edited copy is at C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/config/symbols.json. Four lines are edited in place, adding `, "static": true`:
  - line 5108 _object_destroy_notify_children
  - line 5126 _object_damage_body
  - line 5129 _area_of_effect_cause_damage_to_object
  - line 23108 _damage_globals
This needs a csplit-only regen of source/objects/damage. No other split or base object references these names.

config/parked.json:
  - Retire the `source/objects/damage:_object_damage_body` entry; the function is strict exact.
  - With patch (A), re-baseline the `_area_of_effect_cause_damage_to_object` base to size 1312 / relocation_count 51 / normalized_sha256 93606db11e3fdb8fba3e48d2649b3f193f13850837b1d29e29a133c182cb4b8a. The integrator needs to measure objdiff_percent.
  - With patch (B), that park stays unchanged.

No shared header is edited.

## evidence
Baseline gate: 32 exact / 2 residual.

NEW FACT: pdb_storage.py reports 4 disagreements with the cachebeta publics. _object_damage_body, _area_of_effect_cause_damage_to_object, _object_destroy_notify_children and _damage_globals are absent from the publics, so they were file-static in January. The prior lanes never tried `static` on the two residuals.

Object damage body:
- cand1 applies the /Od-attested child walk. The /Od build at 0x76ffc0 has frame slots [ebp-0x2c] = object_get(child,-1) and [ebp-0x30] = a plain copy of it inside the unit-mask test. This is the admitted units.c idiom (units.c:1723 and :8093) and is not the park-rejected repeated raw object_get downcast. It reaches 1360/43 but leaves 4 tail packets (edi reload, eax/ecx, ecx/edx, x87-vs-integer copy).
- Adding `static` (cand2b) makes the body STRICT-EQUAL to January. _object_cause_damage stays exact.
- Strip test: without `static` the body is residual again.

Final candidate:
- gate --all --forbid-emitted-symbol _point_from_line3d: 33 exact / 1 residual, guard passed.
- object_audit shows only the AOE section plus the 4 storage flags that symbols.json fixes.
- Data is 760/760 (100%) in build/report.json.
- surplus_identity on the candidate object: 10 COMDATs, all identical to January's selected copies. The new _collision_test_line and _real_random are the action_charge copies.
- provider_link: PASS.
- fake_match_scan: 0 leads. /W3: no TU-local warnings.

AOE:
- The /Od readout of 0x76c230 was applied as byte-inert house-rule cleanups. strict_eq.py (section_infos_equal) shows these variants equal to cand2: od1, od2 (the ?: form plus PIN, with /Od declaration order), od3, v1 and v2.
- The one remaining difference is at +0x42e. January uses `fxch st(1); fstp st(0)` and we emit `fstp st(1)`. Those 2 bytes push the section across the 16-byte pad: 1312 vs 1328.
- More than 60 new probes left it at fstp st(1):
  - arm-order and compare spellings;
  - variable reuse (the result in distance or radius_delta) and 1-to-3-statement splits in the real TU;
  - double types (these turn the constants into qword literals);
  - scalarised-aggregate members;
  - extra references to distance or delta (FP-priority hypothesis);
  - REAL_MATH/COLLISIONS extern switches;
  - the full TU reordered to January's emission order;
  - diagnostic optimize pragmas;
  - filler-local oracles;
  - the TU declared-name-count oracle for k = 0..69 (3x the measured period).
  Controls and census:
  - A census of the January split objects (killscan.py) finds `fstp st(1)` 36 times and the fxch idiom only 3 times: autoaim, unit_adjust_projectile_ray and AOE. In the two exact fxch cases, the killed value's last use was an `fld st(n)` copy.
  - hud_messaging_update is exact and has the identical then-arm pattern; January also emits fstp st(1) there.

## blockers
1. _area_of_effect_cause_damage_to_object is not exact. The single difference is January's `fxch st(1); fstp st(0)` against our `fstp st(1)`, which kills the dead radius delta at the end of the falloff then-arm (+0x42e). It is robust to every local spelling, TU name count, definition order, extern-helper switch and type that was tried. The mechanism is inside the C2 x87 stackifier, and no source lever has been found. The methodology stop rule fired.

2. Whole-object admission items in damage.c that remain even after AOE:
   - a consumer-local prototype `boolean unit_unsuspecting(...)`. Its owner is units.c, and units.h has no prototype. Moving it would be a shared-header edit with declaration-count blast radius.
   - a consumer-local `extern boolean debug_damage;`. The symbol is public in cachebeta, but no source defines it. This is the same class as the held render_debug debug_* externs.

3. Patch (A) changes AOE's parked bytes, so the park entry must be re-baselined in the same batch. Otherwise the park drift check and progress fail.

## reopen_criteria
Reopen AOE only on one of the following:
(a) a decoded C2 x87-stackifier rule, for example a live trace with the Lane A dbg32c tooling, that says when a dead cross-block value at st(1) is freed with `fxch st(1); fstp st(0)` rather than `fstp st(1)`, together with the source feature that selects it;
(b) a same-compiler exact donor that shows that idiom at a block end without an `fld st(n)` copy as the last use;
(c) authoritative January source or local records for the falloff clamp.

Do NOT repeat any of the following:
- the ledger's decl-order, arm-order, ?: and PIN, variable-reuse, statement-split, double, aggregate-member, extern-switch, definition-order, pragma or name-count probes;
- the prior lanes' dvar/km/p* families.

## task notes
The main result is a new strict function: _object_damage_body (1360 padded / 1347 meaningful). It closed because of January's storage fact: the function is absent from cachebeta publics, so it is `static`. The /Od-attested child walk was also needed. The whole object is still incomplete because of the AOE x87 kill idiom and two consumer-local declaration items.\n\nFor the integrator:\n- Run the usual full chain. Watch the rename-stable diff for _object_damage_body becoming strict and for zero moved rows elsewhere.\n- After the csplit regen, object_audit shows only the AOE difference.\n- The four storage corrections are zero-credit ownership fixes backed by the cachebeta publics.\n- Surplus COMDATs _collision_test_line and _real_random are new with patch (A). They are byte-identical to January's action_charge copies, and provider_link passes in both orders.\n- An earlier mk_od.py bug put single backslashes in one assert string. It was fixed and the files regenerated. The final strict-equality claims were re-verified after the fix.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/damage/LEDGER.md
