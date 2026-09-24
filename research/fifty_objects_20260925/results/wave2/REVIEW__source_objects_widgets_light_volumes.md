# review source/objects/widgets/light_volumes
approve=False

## per_object
[
 {
  "unit": "source/objects/widgets/light_volumes",
  "approve": false,
  "issues": "Reject the submitted production.patch. Its block grouping (brightness declared before an inner {eye_to_marker_vector, parallel_factor, external_scale} block) contradicts the /Od declaration order (eye_to_marker_vector, brightness, parallel_factor, external_scale). Reviewer variant st6b follows that order, scopes only external_scale (whose address escapes to object_get_function_value) as `{ real external_scale = 1.f; ... }`, and produces an identical object (0/20 differences). Its full audit passes: 10/10 EXACT; object_audit PASS against the regenerated split; surplus IDENTICAL; provider_link PASS; widgets.obj identical; objdiff 1900/1900 code and 284/284 data. Land C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\review2_light_volumes\\amended\\production_amended.patch instead. symbols.patch and config_parks_status.patch are approved as submitted. Remove the park together with the source change, and set Matching last."
 }
]

## checks
All checks were run independently. Notes and scripts are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_light_volumes\ (REVIEW.md).
(1) Patches: `git apply --check` is clean on 931ed8dc for production.patch, symbols.patch and config_parks_status.patch. Applied outside the worktree, they reproduce the worker's five full-copy files byte for byte.
(2) Compile: I did not use gate.py, because it appends --cflag /I after the source root and so cannot test a header edit. rv_compile.py uses gate's exact build.ninja flags, compiles in place and puts the candidate header first on the include path. A /E check confirmed the overlay header was picked up. Result: 10/10 EXACT, with render at 912 under strict section_infos_equal. rv_cand.obj matches cand_final.obj in all 20 sections and symbols.
(3) Split: I ran csplit with the patched symbols.json into my slug. splitdiff over all 833 objects shows only light_volumes.obj changes, and only _light_volume_globals' storage class (2 to 3). object_audit against that split: PASS, 14/14 sections and symbols.
(4) Storage: _light_volume_globals is absent from cachebeta_publics. Only light_volumes.obj names it (xref over build/split). I re-ran DIA2Dump on HCEX: "File Static". The storage check against publics shows 0 disagreements.
(5) Surplus: _point_from_line3d, _vector_from_points3d and _dot_product3d are IDENTICAL to January's copies in action_charge, action_alert and action_charge. provider_link passes for all 6 surplus externals in both orders. nodup_census finds 0 NODUP definers. The _power surplus is gone.
(6) widgets.c, the only other includer of the header: with the new header it is 11/11 EXACT, and 20/20 sections are identical to build/base.
(7) objdiff 3.3.1, one-unit project in my slug: code 1900/1900 and data 284/284 against the regenerated split. As a control, the base build reproduces build/report.json (995 code, 88.430885).
(8) /W3: one new C4244 at pow1's return, the same class as 11 real_math.h lines. There is no C4013.
(9) /Od re-read: I re-dumped 0x79ef80, pow1 at 0x79fb90, and 0x525c50, 0x525d60 and 0x69fab0. Every statement, argument order, the short loop counter and the `!=`-ternary pow1 shape are attested. pow1 calls the math.h pow(float,float) chain directly. The real_math power wrapper 0x69fab0 has one caller, in weapons.c (magnification).
(10) January bytes: [ebp+8] holds external_scale (stored at +0xd3, lea at +0x189) and later the count-1 fild temp and offset_fraction. Confirmed.
(11) Strip tests, 13 variants:
- st1, if-form pow1 with pow: 928, residual.
- st2, ternary with power: breaks _pow1 and render.
- st3, `(real)pow` ternary: same as st2. The double-typed ternary is the mechanism, so it is not decoration.
- st4, `==` ternary: inert.
- st10, `(real)(...)` around the ternary: inert.
- st5, flat scope with no block: residual.
- st7, long index: residual.
- st11, `definition->count - 1`: residual.
- st8 and st9, declaration order and one-statement alpha: inert.
- st6, st6b and st6c: EXACT (see the issue below).
(12) Config: parked.json and config.json copies parse. Only the light_volumes park is removed, and none remain for the unit. No other config, tool or test references the unit.
(13) Amended candidate: I gave it the same full audit. EXACT 10/10, and amended.obj matches rv_cand.obj in all 20 sections and symbols. object_audit PASS; surplus IDENTICAL; provider_link PASS; widgets 11/11 and 20/20; objdiff 1900/1900 and 284/284; only the same C4244 at /W3.

## issues
REJECT production.patch as submitted. The object is still complete-ready: land the amended patch below instead. It produces an identical object.

F1: the block grouping in the submitted patch contradicts the /Od declaration order, while an equally exact, fully audited grouping does not.
- The /Od frame (declaration order, not first-use order) is eye_to_marker_vector -0x98, brightness -0xa0, parallel_factor -0xa4, external_scale -0xac. brightness is stored before eye_to_marker_vector is first used, yet sits below it, so eye_to_marker_vector is declared first.
- The submitted source declares brightness in the enclosing block and the other three in an inner block. That puts brightness before eye_to_marker_vector, contradicting the /Od.
- The worker used this same /Od frame order as evidence for the loop locals (`position` between offset_fraction and radius_fraction), but broke it here.
- January's bytes only require a scope around the one local whose address reaches an out-of-line call (external_scale, escape law). eye_to_marker_vector only goes to inlined helpers.
- A /Od-order contradiction of a scope is what stopped actor_aim_grenade at zero credit.
- The worker's strip test s9 ("initializer form of external_scale goes residual") only holds inside the worker's own grouping.

Reviewer variant st6b is EXACT and consistent with both January's bytes and the /Od:
- The if-block declares `struct light_volume_frame *frame = ...; struct object_marker marker; real_vector3d eye_to_marker_vector; real brightness; real parallel_factor;`.
- Statements: the marker call; `brightness = 1.f;`; vector_from_points3d; `parallel_factor = ABS(dot_product3d(...));`.
- Then `{ real external_scale = 1.f; <far-fade PIN>; <angle PIN>; if (object_get_function_value(..., &external_scale)) brightness *= external_scale; }`.
- It also follows rule 17 (declaration plus initialisation). st6c (`{ real parallel_factor = ABS(...); real external_scale = 1.f; ... }`) is also exact and consistent with the /Od; st6b is the minimal block.

APPLY INSTEAD of scratch\w\light_volumes\production.patch:
- Patch: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_light_volumes\amended\production_amended.patch. It passes `git apply --check` against 931ed8dc and reproduces the full files exactly.
- Full files: ...\review2_light_volumes\amended\candidate\source\objects\widgets\light_volumes.c and light_volumes.h. The .h is identical to the worker's.
- The only difference from the worker's .c is the render prologue above.

APPROVED unchanged:
- symbols.patch (line 23105 gets "static": true), followed by a csplit regen. Verified: one object and one storage class change.
- config_parks_status.patch. Remove the park together with the source change, because parked bodies whose bytes move fail the build. Set config.json to Matching last, after the full chain passes.

Non-blocking notes:
- A new /W3 C4244 at pow1's return (double to real), the same class as real_math.h.
- The /Od extras (the marker-count short, the set_texture result gate) are later-revision additions, and January's bytes show neither. They are correctly omitted.
- Side observation, not for this unit: /Od breakable_surfaces 0x7a0ca0 calls the pow overload directly at the two sites that our breakable_surfaces.c spells power(). This is byte-equivalent at plain assignments.

Everything else was verified and is clean: exactness, storage, surplus identity, provider link, consumer TU, objdiff credit, house rules, and the COMDAT class under owner ruling 5.
