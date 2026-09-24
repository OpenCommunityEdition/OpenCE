# light_volumes wave-2 worker ledger (claude/fifty-objects-20260925, base 931ed8dc)

Target: `_light_volume_render` (Jan 912/47, frame 0xa4; ours 928/47, frame 0xa8), parked tu-context-optimization.

## Baseline
- gate --all at 931ed8dc: exact 9 / residual 1 / unwritten 0. render `[size 928!=912, sha]`.
- Ledgers read: all 8 light_volumes_obj_* logs. Recorded negatives (do NOT repeat): dot_product3d either order;
  vector_from_points3d alone; (real_vector3d const*) cast; function_value outer-block decl; declaration order;
  function_value scoping; two-stmt vs nested pow1 args; pow vs power in pow1; ternary/result-var pow1 (break _pow1);
  if-negate ABS; static light_volume_globals; distance_fade stmt-position grid (lvG 12 cells); intensity accumulator;
  local-count instruments (mod8); do-while vs for (for+short index = January loop header, w3d v7).
- NEW levers not used by earlier waves (all dated after 20260916): /Od build readout (odbuild.py),
  HCEX DIA2Dump locals, VC7 frame-allocator decode, escape-scope frame law.

## /Od readout (0x79ef80, later build; RTC: marker 108, eye_to_marker_vector 12, external_scale 4, position 12, color 16)
Statement order: interpolate_frames; object_get_marker_by_name; `brightness = 1.0`; vector_from_points3d(&camera.position,
&marker.position, &eye_to_marker_vector); parallel = ABS(dot_product3d(&camera.forward, &marker.forward)); `external_scale = 1.0`;
if (far>0) brightness *= PIN((dot_product3d(&camera.forward,&eye_to_marker_vector)-far)/(near-far),0,1);
brightness *= PIN((1-parallel)*perp + parallel*par, 0, 1); if (object_get_function_value(obj, src-1, &external_scale)) brightness *= external_scale;
if (brightness>0 && (...alpha...) && (...radius...)) { begin; set_texture; short count = def->count; for (short i=0;i<count;i++) {
 offset_fraction = pow1((real)i/(real)(count-1), off_exp); radius_fraction = pow1(of, rad_exp); radius = (1-rf)*hither + yon*rf;
 color_fraction = pow1(..); brightness_fraction = pow1(..); point_from_line3d(&marker.position, &marker.forward, of*length+offset_from_marker, &position);
 rgb_colors_interpolate(...); color.alpha = (1-bf)*h.alpha + y.alpha*bf; color.alpha *= brightness; draw_sprite3d(...) } end }
HCEX: marker, external_scale, position, color locals (enregistered scalars absent). January frame 0xa4 = marker108+color16+radius+
color_fraction+divisor+pow1#1 exp+delta/position12 (shared)+i+brightness+pow1#2-4 exp/bf; external_scale in [ebp+8].

## Probe v1 (/Od shape: brightness accumulator, external_scale=1 before far-fade if, eye_to_marker_vector in inner block,
vector_from_points3d + 2x dot_product3d, for/short, point_from_line3d CALL instead of hand expansion, alpha two statements)
Hypothesis: /Od statement shape + real helper calls (never tried together; point_from_line3d never tried) fixes the pre-loop contest.
Result: 928/47, frame 0xb0, 294 insns, 61 hunks. PRE-LOOP (0x0..0x245) and loop tail (0x301..end) are now
INSTRUCTION-IDENTICAL to January modulo slot offsets (delta homed, 1.0 immediate store between fsub/fstp, parallel on x87,
external_scale 1.0 at +0xd3). Remaining: the four inlined pow1 joins (ours memory phi; January x87 phi: eager `fld value`
before jnp, fstp at join) + exponent-home copies (#2 integer copy vs Jan fld/fst). Surplus COMDATs _point_from_line3d,
_vector_from_points3d, _dot_product3d: provider_link PASS.

## Probe v2 = v1 + `real external_scale;` declared in the inner eye_to_marker_vector block, with the
object_get_function_value if moved inside that block.
Hypothesis: January [ebp+8] hosts external_scale (lea at +0x189) AND the count-1 temp (+0x232) -> by the escape-scope law
external_scale's declaration scope closes before the loop.
Result: 928/47, frame 0xac (from 0xb0), external_scale now in [ebp+8] exactly like January. Pre-loop still identical.
Remaining 8 frame bytes = pow1 exponent/result homes (ours: e2 own slot, bf own slot; Jan: e2/e3/e4/bf share [ebp-4]).

## Oracle: dead-declaration ordinal count (instrument only, never landable) on v2, D=0..8 at function top
Result: MOD8-INSENSITIVE (all D: 928, 61 hunks, frame 0xac). The pow1 phi residual is not an IL-ordinal effect.

## Lab (scratch/w/light_volumes/lab/p.*): pow1 spellings compiled out-of-line
Out-of-line byte-identical to January _pow1: if-form (current), multi-return `if (e!=1) return power(v,e); return v;`,
`if (e==1) return v; return power(v,e);`, inner `result` then `value = result`. NOT identical: ternary (both senses),
`result = value` pre-init, if/else result (confirms w2). In a small loop lab ALL spellings inline as MEMORY phis,
so pow1 spelling alone does not produce January's x87 phi.

## Probe v2p5/v2p6/v2p8 (pow1 spellings that keep out-of-line _pow1 exact), render context
- p8 (inner result then value=result): byte-identical to v2 (canonicalised).
- p5 `if (e!=1.f) return power(v,e); return v;` and p6 `if (e==1.f) return v; return power(v,e);`: 944 B, frame 0xa8,
  305 insns, 54/57 hunks. Exponent handling becomes January's (#1 int copy, #2/#3/#4 fld/fst sharing [ebp-4]) but each
  pow1 becomes a two-arm diamond: then `fld v; fld e; call; fstp dest; jmp`, else `mov r,[v]; mov [dest],r` (int copy).
  Observation: January's x87-phi shape == p5's diamond with the else copy done through x87 (fld v / fstp dest) and the
  common `fld v` head hoisted + common `fstp dest` tail merged. So the question becomes: what makes the else-arm copy go
  through x87 instead of integer registers.

## Board census (scratch/w/light_volumes/cipow_scan.py): inlined `test ah,0x44; jnp; ...; call __CIpow` diamonds in all
January split objects. X87PHI-with-eager-load after fcomp occurs ONLY in _light_volume_render (4/4). The only other site
(_breakable_surface_effect, exact) is the discard/reload x87 form. No exact donor exists for January's pow1 inline shape.
Lab: `static __inline` pow1 = byte-identical to v2 (inert). pow/power/(real)pow/(double) variants canonicalise.
Probe v3 = v2 with loop scalars declared in the enclosing if-block and assigned in the loop: byte-identical to v2 (inert).
Probe v4a (early return for the NONE test instead of nesting): byte-identical to v2 (block weight inert here).
HCEA decompile (research/halocea/src/pow1.c) renders pow1 as `if (e == 1) return z; return pow(z,e);` (decompiler, not source).

## KEY FINDING: pow1 is `return exponent != 1.f ? pow(value, exponent) : value;` (C pow -> double ternary)
Evidence: (1) /Od pow1 0x79fb90 = ternary result temp [ebp-4] with the pow arm as fall-through and `jnp` to the value arm
(=> `exponent != 1.f ? pow(...) : value`). (2) /Od callee chain: pow1 -> 0x525c50 (float(float,float) -> 0x525d60) ->
0x525d60 (cvtss2sd both, CRT pow, fstp dword) = C++ math.h pow(float,float) -> powf -> pow(double). The real_math.h
`power` wrapper is a DIFFERENT function 0x69fab0 (-> 0x525c50) whose only caller is weapons.c magnification, which our
source spells `power(...)`. So pow1 calls C `pow` directly, not `power`. (3) Lab (lab/r.*): with C `pow` the ternary is
DOUBLE-typed; its out-of-line body is byte-identical to January's _pow1 AND its inlined form is January's x87 phi exactly
(`fcomp; fld v; fnstsw; test ah,44h; jnp; fld e; call __CIpow; fstp dest` and #2 `fld e; fst home; fcomp; fld v ...`).
The recorded w2 negative "ternary breaks _pow1" used `power()` (float ternary) - different expression type.
Mechanism: the double-typed return temp cannot coalesce with the float destination, so it is x87-allocated.

## Probe v5 = v2 + pow1 body `return exponent != 1.f ? pow(value, exponent) : value;`  ==> EXACT
gate --all: exact 10 / residual 0 / unwritten 0. _light_volume_render EXACT 912 (strict section_infos_equal).
Candidate: scratch/w/light_volumes/v5.c / v5.obj.
/W3 check (scratch/w/light_volumes/warn.py): v5 has no C4013; only the new C4244 double->real at the pow1 return (same class
as real_math.h's own power/sine/... returns). Production has no C4013 either (w3d's hud_draw.h note is stale).

## Strip tests from v5 (each reverts ONE element to the production spelling)
- s1 point_from_line3d -> hand expansion (named offset local): residual [sha] => helper call LOAD-BEARING (and rule 6 forbids hand expansion anyway).
- s2 vector_from_points3d -> hand expansion: residual [sha] => LOAD-BEARING.
- s3 both dot_product3d -> hand expansion: residual 291->293 insns => LOAD-BEARING.
- s6 color.alpha one statement `(...)*brightness`: EXACT (inert; /Od two-statement form kept).
- s4 distance_fade/intensity pair instead of the /Od `brightness` accumulator: 928/48 => accumulator LOAD-BEARING.
- s5 external_scale declared at the outer level (call after the inner block): frame 0xa8 => inner scope LOAD-BEARING (escape-scope law).
- s7 do{}while loop (production form): residual => /Od `for (short i...)` LOAD-BEARING.
- s8 `real brightness = 1.f;` as a declaration initializer: residual => the /Od statement position (after marker) is LOAD-BEARING.
- s9 `real external_scale = 1.f;` initializer: residual => /Od statement position (after parallel_factor) LOAD-BEARING.
- s10 loop-local declaration order (aggregates last) vs /Od order: raw-identical (inert). /Od order kept.

## Whole-object audit on the exact candidate (scratch/w/light_volumes/light_volumes.c == v5.c, cand.obj)
- gate --all: exact 10/10. object_audit: PASS (14 January sections/symbols equal).
- surplus: _dot_product3d, _point_from_line3d, _vector_from_points3d (NEW, all IDENTICAL to January's selected copies:
  action_charge / action_charge / action_alert); _power is NO LONGER emitted (pow1 now calls C pow). provider_link PASS both orders.
- data: build/report.json light_volumes data 284/284 (100%) already; no data entry needed.
- pdb_storage: `_light_volume_globals` is ABSENT from cachebeta publics (split+ours say external). Not referenced by any
  other January split object (xref.py). => January file-static. Needs symbols.json "static": true + static definition +
  removal of the header extern (owner-gated otherwise). Only includers of light_volumes.h: light_volumes.c, widgets.c.
- HCEX DIA2Dump -sym light_volume_globals: "File Static, Type: struct <unnamed-tag>" => second attestation that the
  global is file-static (HCEX even uses an anonymous struct; not adopted - would remove the named struct + asserts from the header).
- Static fix measured: .c `static struct light_volume_globals light_volume_globals = {0};` + header extern removed (scratch
  header copy via a test-only include path): light_volumes 10/10 exact; widgets.c 11/11 exact and ALL 20 widgets.obj
  sections/symbols raw-identical to build/base (objcmp.py). csplit regenerated into scratch/w/light_volumes/split with the
  edited symbols.json: only light_volumes.obj changes (833 objects compared), only _light_volume_globals storage 2->3.
  object_audit vs regenerated split: PASS; pdb_storage vs regenerated split + static candidate: 0 disagreements.

## FINAL (OBJECT_COMPLETE_CANDIDATE)
Files:
- scratch/w/light_volumes/production.patch  (source/objects/widgets/light_volumes.c + light_volumes.h; git apply --check OK)
- scratch/w/light_volumes/symbols.patch + full copy scratch/w/light_volumes/config/symbols.json (line 23105 only: "static": true)
- scratch/w/light_volumes/config_parks_status.patch + full copies config/parked.json (park removed), config/config.json (Matching)
- full candidate sources: scratch/w/light_volumes/candidate/source/objects/widgets/light_volumes.{c,h}
- candidate object: scratch/w/light_volumes/cand_final.obj; regenerated split: scratch/w/light_volumes/split/
Audit (all on cand_final.obj): gate 10/10 EXACT; object_audit vs regenerated split PASS; pdb_storage 0 disagreements;
surplus _dot_product3d/_point_from_line3d/_vector_from_points3d IDENTICAL to January selected copies, nodup_census: 0 NODUP
definers board-wide (71/23/76 SELECT_ANY), provider_link PASS both orders; _power surplus REMOVED; data 284/284 already 100%;
widgets.obj (only other includer of light_volumes.h) raw-identical 20/20; fake_match_scan 0 leads; /W3: one new C4244
(double->real at pow1's return, same class as real_math.h). No admission rejection entries for the unit.
Reusable law: an inlined `if (c) v = f(v); return v;` / `c ? power(v,e) : v` helper coalesces its float return temp with the
caller's float destination -> MEMORY phi; `c ? pow(v,e) : v` with C pow is DOUBLE-typed, cannot coalesce, and is
x87-allocated -> eager `fld v` after the compare + `fstp dest` at the join. Out-of-line bytes are identical for both.
