# collision_debug wave-2 ledger (slug scratch/w/collision_debug/)

Base tree 931ed8dc. Task: whole-object admission; replace every hand-expanded header helper with
the genuine /Od-attested call; keep _collision_debug_render EXACT; storage fix for
_collision_debug_cube_vertices (static per cachebeta publics).

## Inputs read
- WORKER_BRIEF.md (incl. s.7), campaign_house_rules.md, claude_fifty_objects_20260925.md,
  all 6 docs/object_matching_logs/*collision_debug* ledgers, scratch/w/review_collision_debug/REVIEW.md.
- Production source == wave-1 review cand.c (unchanged since 5671843f).

## F1 - independent /Od call census of _collision_debug_render (od_linear.py 0x7af800 0x7b1ee4 -> od_linear.txt)
Every /Od callee mapped to production calls with equal counts (asserts x3, local_player_get_player_index x2,
player_get, fabs, scale_vector3d, global_collision_bsp_get x2, breakable flags, collision_bsp_test_vector,
point_from_line3d x4, collision_get_features_in_sphere x2, game_time_get x3, fmod x3, render_debug_line x14,
render_debug_point x7, render_debug_vector x6, set_real_vector3d x3, matrix4x3_transform_vector x4, add_vectors3d,
memmove + memset x2 (cseries.h #define -> cs*), ...). Collision-user push/pop is INLINE in /Od too (movsx
[depth]; cmp 0x20; assert line 0x4c=76; store 0x15) = macro-like, same as January; board-wide spelling; not a helper.
TWO sites differ:
 (a) features radius: /Od 0x7b1dc6 call 0x432950 = magnitude3d (-> square_root 0x42e320 -> sqrt(float) 0x42e2f0
     -> sqrtf 0x42e450 -> CRT). Production hand-expands `(real)sqrt(v.i*v.i+...)` (review finding, reconfirmed).
 (b) NEW (the wave-1 review missed it, calling it a <cmath> overload artifact): textstring degrees:
     /Od 0x7b1086 call 0x46d330 -> 0x4625b0 -> 0x46d3e0 (cvtss2sd; call CRT 0x931704) = THREE levels, exactly like
     square_root's chain; bare fabs (0x42dec0->0x42e400->CRT) and fmod (0x57f040->0x581cd0->CRT) are TWO levels.
     Validation of the classification on other January owners: 0x4625b0 (acos(float) overload) is called DIRECTLY
     by 0x8c2c30 "units\bipeds.c" (production bipeds.c:3158 bare `(real)acos(cosine_of_angle)`), while 0x46d330 is
     called by 0x464900 "ai\actor_moving.c" (production actor_moving.c:2731 `arccosine(facing_dot)`) and
     0x8db7f0 "units\units.c" (production units.c:9627 `arccosine(dot_product)`), plus real_math-region
     0x6ca910/0x6caa40/... (real_math.c uses arccosine x6). => 0x46d330 = real_math.h `arccosine`; production's
     `(real)acos(collision.plane.n.k)` is a hand-expanded arccosine (rule 6/13).
 Constants (not helpers, readouts): /Od computes yaw/pitch/roll as fmod((real)game_time_get()*{1.0,-0.7,1.4}*pi/30.f,
     2pi_float) (fmod(float,float) overload), and degrees as arccosine(k)*360.f/2pi_float (0x93e9c8=360.0,
     0x93f798=6.2831855f). January has single folded constants 0.104719758f/-0.073303826f/0.146607652f, 57.295776f
     (0x42652ee0, 1 ULP below round(180/pi)=0x42652ee1) and __real@401921fb60000000 = (double)(float)2pi.

## P1 - helper spellings (hypothesis: /Od-attested magnitude3d + arccosine calls are byte-inert for the render fn)
- base.c (production copy): EXACT 4192. candA_mag (magnitude3d only): EXACT. candC_acos (arccosine only): EXACT.
- candB_mag_acos (both): EXACT 4192, exact 1/1.
- object_audit candB: all 17 January sections ok; 1 symbol differs = _collision_debug_cube_vertices storage 2/3
  (the symbols.json row, fixed below). New surplus vs production: _arccosine 16, _magnitude3d 48,
  _magnitude_squared3d 48, _square_root 16 (.text COMDATs).
- surplus_all.py (every candidate-only external def vs every January split definer): 22 rows, problems 0; all
  IDENTICAL raw+flags. _arccosine == January's selected actor_moving copy (hash bbe13982; ours 5 definers, same hash).
- provider_link.py candB: 22/22 PASS, SELECTED-PROVIDER LINK: PASS. nodup_census: NODUP [] for all four new helpers.
## Storage fix (symbols.json) - measured
- scratch/w/collision_debug/config/ = copy of config/*.json; symbols.json line 19564 edited IN PLACE (byte-level,
  CRLF kept): `{ "file_offset": 2667176, "flags": 0, "name": "_collision_debug_cube_vertices", "static": true },`
  (diff config/symbols.json vs copy = exactly that one line).
- csplit.exe -i cachebeta.exe -p scratch/w/collision_debug/config -o scratch/w/collision_debug/split: rc 0, 833 objs;
  byte cmp vs build/split: ONLY source/physics/collision_debug.obj differs.
- gate_r.py (gate.py + GATE_SPLIT_ROOT) vs regen split: candB EXACT 4192, 1/1; object == as-is compile (timestamp only).
- audit_r.py (object_audit + AUDIT_SPLIT_ROOT) vs regen split: 29 January symbols, 0 differ, OBJECT AUDIT: PASS.
- objeq: base.obj == build/base production (36 sections IDENTICAL); candB == production + exactly 4 new COMDATs
  (_arccosine, _magnitude3d, _magnitude_squared3d, _square_root); _collision_debug_render bytes unchanged.
- pdb_storage (production build): 1 disagreement = _collision_debug_cube_vertices split 2 / ours 3 / PDB-public False
  -> the symbols.json static row resolves it (cachebeta has no public at 0x28B2A8).

## P2 - constant readouts (secondary; house rule 20 implausible literals). Hypothesis: VC7 /O2 (no /Op) folds the
/Od-attested expressions `x * 360.f / (_pi*2.f)` and `(real)t * K * _pi / 30.f` into January's single constants
(float math: 360/2pi_f = 180/pi_f = 0x42652ee0 = January, 1 ULP below round(180/pi); pi_f/30 = 0x3dd67750,
-0.7f*pi_f/30 = 0xbd962051, 1.4f*pi_f/30 = 0x3e162051 - all equal January's literals).
- vD1 `arccosine(k) * 360.f / (_pi*2.f)` (/Od 0x7b1094: mulss 360.0; divss 6.2831855f): EXACT, object IDENTICAL to candB
  => VC7 /O2 folds the reassociated constant; explains January's 1-ULP-off 57.295776f (=360/2pi_f=180/pi_f).
- vD3 `arccosine(k) * 180.f / _pi`: EXACT, IDENTICAL (not /Od-attested; control only).
- vD2 `RADIANS_TO_DEGREES(arccosine(k))` (real_math.h macro, extra (real) casts): residual [sha] - rejected.
- vT0 fmod modulus `_pi*2.f` for the raw double 6.2831854820251465: EXACT, IDENTICAL (review's twopi result reproduced).
- vT1 `(real)game_time_get() * {1.0f,-0.7f,1.4f} * _pi / 30.f, _pi*2.f` (/Od 0x7b0845..0x7b08fa: cvtsi2ss; mulss
  {1.0,-0.7,1.4}; mulss pi_f; divss 30.0; fmod(float,float) with 6.2831855f): EXACT, IDENTICAL.
- vFull (helpers + attested constant expressions) and vFull_nocast (same without the `(real)` cast on
  game_time_get(); plain spelling per strip-test preference): both EXACT, object IDENTICAL to candB.
  /W3: 14 warnings each, all in shared headers (cseries.h:45, real_math.h C4244), ZERO in the TU; no new warnings.

## FINAL candidate = scratch/w/collision_debug/cand_final.c (== vFull_nocast.c)
Changes vs production (3 hunks, source/physics/collision_debug.c only):
 1. radius = magnitude3d(&collision_debug_vector) * 0.5f + ... (/Od 0x7b1dc6 -> 0x432950 magnitude3d)
 2. arccosine(collision.plane.n.k) * 360.f / (_pi*2.f) (/Od 0x7b1086 -> 0x46d330 arccosine; mulss 360; divss 2pi_f)
 3. yaw/pitch/roll = (real)fmod(game_time_get() * {1.0f,-0.7f,1.4f} * _pi / 30.f, _pi*2.f) (/Od 0x7b0845..0x7b08fa)
Admission chain on cand_final.obj:
 - gate.py: EXACT 4192 _collision_debug_render, exact 1/1 (vs build/split AND vs regenerated split).
 - audit_r (regen split with static row): 17/17 January sections ok, 29 January symbols 0 differ, OBJECT AUDIT: PASS.
 - surplus_all: 22 candidate-only external defs, problems 0 (all IDENTICAL raw+flags to January's selected copy):
   code COMDATs _add_vectors3d, _arccosine, _magnitude3d, _magnitude_squared3d, _point_from_line3d,
   _scale_vector3d, _set_real_vector3d, _square_root; 3 D3D SDK tables; 11 literals.
 - provider_link: 22/22 PASS both orders, SELECTED-PROVIDER LINK: PASS. nodup_census: NODUP [] (new helpers).
 - objdiff 3.3.1 one-unit report (cand_final.obj vs regen split): code 4182/4182, data 29278/29278, .bss/.data/
   .rdata/.text all 100.0.
 - fake_match_scan: 0 leads (candidate .c and collision_debug.h).
 - Blast radius: .c-only edit (no header) + a symbols.json row whose csplit regen changes ONLY collision_debug.obj.
Patches (git apply --check OK against 931ed8dc; post-images verified byte-equal to the candidate files outside
the repo): collision_debug.c.patch (final, 3 hunks), collision_debug.helpers_only.patch (fallback: hunks 1+2 with
the readout literals kept; post-image == candB_mag_acos.c), symbols.json.patch (+ full copy config/symbols.json).
Integrator follow-up: config/config.json line 515 collision_debug.c "NonMatching" -> "Matching"; regen split; ninja.

## Not changed (reviewed, out of scope or already attested)
- add_vectors3d((real_vector3d const *)&point, &vector, (real_vector3d *)&point): /Od 0x7b0c42..0x7b0c5c attests
  add_vectors3d(point, vector, point) (wave-1 strip test: casts byte-inert).
- cube vertices: /Od 0x7b097b..0x7b099a passes &cube[i] (0x9bc400+12i) to the SAME 0x6c7160 used by the three
  spray matrix4x3_transform_vector calls; declared type unobservable (HCEX has no collision_debug compiland; its
  globals list only the hs *_definition records). Cast kept as landed/strip-tested in wave 1.
- collision-user push/pop: expanded inline in /Od as in January (assert line 76 both builds); board-wide spelling,
  collision_usage.h has no macro; not a header helper.
