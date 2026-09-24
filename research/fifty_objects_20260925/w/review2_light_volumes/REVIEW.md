# Adversarial review: light_volumes OBJECT_COMPLETE_CANDIDATE (wave 2 review2)

Reviewer slug: scratch/w/review2_light_volumes/. Tree 931ed8dc. Worker slug scratch/w/light_volumes/.

## R1 patch application
- git apply --check clean for production.patch, symbols.patch, config_parks_status.patch against current tree.
- Applied all three (git apply, outside the worktree) to copies of the tracked files: results are byte-identical to the
  worker's full copies (candidate/source/objects/widgets/light_volumes.{c,h}, config/symbols.json, parked.json, config.json).

## R2 independent compile (candidate .c AND candidate .h together)
- scratch/w/review2_light_volumes/rv_compile.py: gate.py's exact cflags from build.ninja, but the candidate tree
  (tree/source/objects/widgets/light_volumes.{c,h}) is compiled IN PLACE with /I<tree/source> PREPENDED so the candidate
  header shadows the tracked one (gate.py copies the .c to scratch/ and appends --cflag /I AFTER the source root, so it
  cannot test a header edit). Result: EXACT 10/10 (render 912 strict section_infos_equal). Surplus .text:
  _dot_product3d, _point_from_line3d, _vector_from_points3d. rv_cand.obj == worker cand_final.obj (20/20 sections+storage).
## R3 split regeneration + object audit
- csplit.exe run into scratch/w/review2_light_volumes/split with config copy + the patched symbols.json (only change).
  splitdiff.py over all 833 objects: ONLY light_volumes.obj changes, ONLY _light_volume_globals storage 2->3.
- object_audit (SPLITDIR copy object_audit_env.py) vs regenerated split: PASS, 14/14 January sections+symbols.
  vs the current build/split it FAILS only on that storage class (expected until the symbols.json regen lands).
- Candidate-only surplus: .drectve, "definition" literal, __real@00000000, __real@3f800000 (all pre-existing in
  build/base) + the three new helper COMDATs; _power (16 B) surplus from base is gone.
## R4 storage truth
- cachebeta publics: _light_volume_globals ABSENT (as are _pow1/_light_volume_interpolate_frames, the known statics).
  Every public/absent verdict agrees with candidate storage: 0 disagreements.
- xref over build/split: _light_volume_globals is named only by light_volumes.obj (no external January user).
  Control: neighbours _damage_globals/_lightning_globals/_telnet_console_globals are also absent+own-object-only
  (consistent: unfixed statics); the public _glow_globals is referenced from glow.obj (COMMON), as expected.
- HCEX DIA2Dump -sym light_volume_globals: "File Static, Type: struct <unnamed-tag>" (re-run, confirmed).
## R5 surplus
- surplus_identity (CAND env copy) on rv_cand.obj: _point_from_line3d == action_charge, _vector_from_points3d ==
  action_alert, _dot_product3d == action_charge: 3/3 IDENTICAL.
- provider_link.py source/objects/widgets/light_volumes rv_cand.obj: all 6 surplus externals PASS (both orders).
- nodup_census: _dot_product3d 71 / _point_from_line3d 23 / _vector_from_points3d 76 definers, NODUP [] for all three.
## R6 header consumers
- light_volumes.h includers: light_volumes.c, widgets.c only (grep). light_volume_get / light_volume_globals used nowhere else.
- widgets.c compiled with the overlay header (preprocessor #line confirms the overlay header was used): 11/11 EXACT and
  20/20 sections+storage identical to build/base widgets.obj. (Production-header compile also identical: control.)
## R7 objdiff 3.3.1 (scratch od_proj, relative paths)
- cand vs regenerated split: code 1900/1900 100%, data 284/284 100%, 10/10 functions.
- control base vs current split reproduces build/report.json (995 code / 88.430885 fuzzy / data 284) -> harness valid.
- cand vs CURRENT split also 100/100 (objdiff ignores the storage class), i.e. credit does not even depend on the regen.
## R8 /Od attestation re-read (od_render.txt re-checked line by line, od_pow1.txt / 0x525c50 / 0x525d60 / 0x69fab0 re-dumped)
- Every statement in the new render body maps to the /Od: brightness=1.0 at 0x79f036 right after the marker call;
  vector_from_points3d(0xecfe0c camera.position, marker+0x60, &eye[ebp-0x98]); ABS(dot(camera.forward, marker+0x3c)) as
  a 3-call macro expansion; external_scale=1.0 at 0x79f0dd; far-fade PIN(dot(forward,&eye)-far)/(near-far) as 3 calls,
  brightness*=; angle PIN((1-par)*def+0x3c + par*def+0x40); if(ofv(obj, src-1, &external_scale)) brightness*=;
  short count/short i for-loop; pow1((real)i/(real)(count-1), ...) x4; point_from_line3d(marker+0x60, marker+0x3c,
  of*len+offset, &position); rgb_colors_interpolate; alpha two statements; draw_sprite3d. Argument orders all match.
- /Od loop-local frame order (-0xc0 of, -0xd0 position, -0xd8 rf, -0xdc radius, -0xe0 cf, -0xe4 bf, -0xf8 color)
  = the candidate's interleaved declaration order (inert in /O2 per worker s10, but attested).
- pow1 0x79fb90: ucomiss e,1.0; test ah,44h; jnp -> value arm; fall-through pow arm (= `e != 1 ? pow(v,e) : v`);
  calls 0x525c50 = float(float,float)->0x525d60 = cvtss2sd x2 + CRT pow + fstp dword: the C++ math.h pow(float,float)
  -> powf chain. 0x69fab0 is a separate float wrapper -> 0x525c50 whose ONLY caller is 0x6a3c30 (weapons.c,
  "magnification" asserts) = real_math power. So pow1 spells C `pow` directly: CONFIRMED.
- Side note (not this unit): /Od breakable_surfaces 0x7a0ca0 also calls 0x525c50 directly at both sites that our
  breakable_surfaces.c spells power(); byte-equivalent at plain assignments, so no action here.
- /Od extras NOT in candidate and correctly absent in January: object_get_marker_by_name result stored to a short
  [ebp-0x88] (dead in /O2), `if (!rasterizer_widget_set_texture(...))` gate (January render has no such test: the
  candidate is exact without it -> later-revision difference).
- Declaration-order tension: /Od lays brightness (-0xa0) BETWEEN eye_to_marker_vector (-0x98) and parallel_factor
  (-0xa4); the candidate declares brightness in the enclosing block and the other three in an inner block. The inner
  block is forced by January's own bytes ([ebp+8] shared by external_scale and the loop temp; worker s5 measured) and
  brightness must outlive the loop, so the later build's flat order cannot be January's; recorded, not a blocker.

## R9 January bytes re-check of the escape-scope claim (gate --disas of build/split render)
- `mov edi,[ebp+8]` at +0xa (object_index enregistered), `mov [ebp+8],3f800000h` at +0xd3 (external_scale=1),
  `lea eax,[ebp+8]` +0x189 (&external_scale to object_get_function_value), `fld [ebp+8]` +0x19e, then in the loop
  `mov [ebp+8],ecx` +0x232 / `fild [ebp+8]` +0x23c (count-1 temp) and offset_fraction fstp/fld [ebp+8]. CONFIRMED.

## R10 reviewer strip tests (mkvar.py -> tree/source/objects/widgets/var_*.c, compiled with rv_compile.py + overlay header)
| variant | change from candidate | result |
|---|---|---|
| st1_if_pow | pow1 = `if (e != 1.f) value = pow(value, e); return value;` | render 928 residual -> ternary LOAD-BEARING (/Od-attested ternary) |
| st2_ternary_power | ternary with real_math `power` | render 848/43 + _pow1 residual (+_power surplus) -> pow LOAD-BEARING |
| st3_ternary_realcast | `e != 1.f ? (real)pow(v,e) : v` (float ternary) | same as st2 -> the DOUBLE type is the mechanism (C semantics, not decoration) |
| st4_ternary_eq | `e == 1.f ? v : pow(v,e)` | EXACT (inert; /Od jnp->value arm, fall-through pow arm attests the `!=` sense used) |
| st10 | `(real)(e != 1.f ? pow(v,e) : v)` | EXACT (inert; would silence C4244 - not needed) |
| st5_flat | no inner block (all four locals at the if-block level, /Od order) | residual [sha] -> a scope block IS required (escape-scope law) |
| st7_long_index | `long sprite_index` | residual -> short index LOAD-BEARING (/Od movsx word) |
| st11 | `(real)(definition->count - 1)` | residual -> `count - 1` LOAD-BEARING (/Od uses the count local) |
| st8 | loop-local aggregates declared last | EXACT (inert) |
| st9 | alpha one statement | EXACT (inert) |
| **st6** | /Od order eye_to_marker_vector, brightness, parallel_factor at the if-block level; ONLY `{ real external_scale; external_scale = 1.f; ... }` inner | **EXACT** |
| **st6b** | as st6 with `real external_scale = 1.f;` (declaration+initialisation, rule 17) | **EXACT** |
| **st6c** | eye_to_marker_vector, brightness outer; `{ real parallel_factor = ABS(...); real external_scale = 1.f; ...}` | **EXACT** |

FINDING F1 (source-authenticity, byte-inert): the submitted grouping declares `brightness` BEFORE
eye_to_marker_vector/parallel_factor/external_scale (outer vs inner block). The /Od frame (declaration-order) layout is
eye_to_marker_vector -0x98, brightness -0xa0, parallel_factor -0xa4, external_scale -0xac, i.e. eye_to_marker_vector is
declared before brightness. The worker used exactly this /Od frame order as declaration-order evidence for the loop
locals (the odd `position` between offset_fraction and radius_fraction) but contradicted it for these four. st6b keeps
the /Od order, needs the scope block only for the one escaping local (external_scale - the only local whose address
reaches a real call), and combines declaration+initialisation (house rule 17) - and it is equally EXACT. The worker's
s9 negative ("initializer form of external_scale goes residual") was only true inside the worker's own grouping.

## R11 house rules / owner-gated classes on the submitted diff
- void/params/explicit returns: OK (render keeps `return;`, pow1 returns a value). `real` everywhere; PIN/ABS/FLAG macros.
- Helper COMDAT class: owner ruling 5 (20260921, all-inlined COMDAT class admitted) conditions all met: genuine
  real_math.h __inline helpers, byte-identical to January's selected copies, strict-exact caller, affected-TU sweep clean
  (light_volumes + widgets; csplit regen touches one object), provider link PASS both orders, and the change REMOVES the
  hand expansions (rule 6 improvement).
- Header edit removes an extern (the file-static is attested by cachebeta publics + HCEX) - does not keep an extern
  over a static; only consumer widgets.obj is raw-identical. No declaration-count canary includes light_volumes.h.
- /W3 (overlay header): one new C4244 at pow1's return (same class as 11 real_math.h lines); production's C4146 is gone;
  no C4013 (pow declared through cseries.h <math.h>).
- No volatile/pragma/asm/dummy locals/decoration casts or parens. Recorded prior negatives are not contradicted: the w2
  "ternary breaks _pow1" and "pow vs power inert" probes each changed ONE of {ternary, pow}; the combination is new
  (reviewer st1/st2/st3 confirm each half alone fails).
- Park removal must travel WITH the source change (parked bodies whose bytes move fail the build); config.json
  "Matching" last, after the chain passes.

## R12 amended candidate (reviewer) = submitted candidate with the F1 fix (st6b)
- Files: scratch/w/review2_light_volumes/amended/candidate/source/objects/widgets/light_volumes.{c,h}
  (.h identical to the worker's candidate header); patch scratch/w/review2_light_volumes/amended/production_amended.patch
  (git apply --check OK on 931ed8dc; applying it to the tracked files reproduces the full files byte-for-byte).
- Render prologue: `frame = ...; struct object_marker marker; real_vector3d eye_to_marker_vector; real brightness;
  real parallel_factor;` / marker call / `brightness = 1.f;` / vector_from_points3d / `parallel_factor = ABS(...)` /
  `{ real external_scale = 1.f; far-fade; angle; if (ofv(..., &external_scale)) brightness *= external_scale; }`.
- Audit: 10/10 EXACT; amended.obj vs submitted rv_cand.obj: 0 of 20 sections/symbols differ (same object);
  object_audit vs regenerated split PASS 14/14; surplus 3/3 IDENTICAL; provider_link PASS; widgets 11/11 + 20/20
  raw-identical with the header; objdiff 3.3.1 1900/1900 code, 284/284 data; /W3 only the pow1 C4244.
- Why it is preferred: the /Od frame order is eye_to_marker_vector, brightness, parallel_factor, external_scale; the
  amended form keeps that order and scopes ONLY the local whose address escapes to an out-of-line call (external_scale,
  escape law), with declaration+initialisation (rule 17). The submitted form contradicts the /Od order - the same class
  of contradiction that stopped actor_aim_grenade at zero credit (escape-scope law ledger) - and its s9 negative is
  grouping-specific. st6c (`{ real parallel_factor = ABS(...); real external_scale = 1.f; ...}`) is also exact and
  /Od-consistent; st6b is the minimal block.

## VERDICT
approve=false for scratch/w/light_volumes/production.patch AS SUBMITTED (byte-correct, but its block grouping contradicts
first-party /Od declaration-order evidence while an object-identical, fully audited form does not).
The object IS complete-ready: land scratch/w/review2_light_volumes/amended/production_amended.patch INSTEAD of
production.patch, plus the worker's symbols.patch (+csplit regen) and config_parks_status.patch (park removal with the
source; config.json Matching last). All three of those were verified here.
