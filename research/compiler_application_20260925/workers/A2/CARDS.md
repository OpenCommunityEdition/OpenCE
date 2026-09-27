# A2 (wave 3) CARDS / LEDGER - render_debug + breakable_surfaces admission

Started 2026-09-26 00:36:02 -0700. HEAD 455dffad (canonical cc608036 + batch 1). Worktree clean for both units.

## S0 state at HEAD (measured, 2026-09-26 00:36:02 -0700)
render_debug:
- gate --all 36/36 EXACT. object_audit PASS. surplus_identity 16 COMDATs 0 not identical. provider_link PASS.
- Review items (a) point_from_line3d x3, (b) vehicle view -> struct vehicle_datum, (c) REAL_MATH_EXTERNAL defines,
  (e) six debug_* tentative definitions: ALREADY LANDED in 1cd5e4ed (R2 batch 2). r2 reviewer F1 extras
  (point_from_line2d, arctangent, plane3d_distance_to_point, set_real_point3d) also present at HEAD.
- OPEN: (d) render_debug_globals_definition aggregate with opaque_after_* pads (source lines 252-282, 330).
  No ruling found on owner-queue Q2d (D0/D1/HOLD) in docs/, research/, this lane's OWNER_PACKET/LEDGER.
- January map atlas 4cc87b45...jsonl: text-only (0x400600..0x642994); no data names for render_debug. (new check)
breakable_surfaces:
- gate --all 12/12 EXACT.
- object_audit FAIL(2): _breakable_surface_effect, _globals storage split 2 / ours 3.
  pdb_storage: both ABSENT from cachebeta publics -> January static; ours right, split (symbols.json) wrong.
- surplus_identity: 26 COMDATs, 7 DIFFERENT: _project_point3d, _project_point2d, _vector_from_points3d,
  _cross_product3d, _plane3d_from_point_and_normal, _plane3d_negate, _real_local_random.
- provider_link FAIL(1): _real_local_random vs effects (LNK2005 both orders; effects NODUP hand copy).
- candidate-only STATIC code: _breakable_surface_get_plane_from_designator (96, hand copy of bsp3d.h inline),
  _breakable_surface_plane_distance (32, TU-local j+k association helper, 95d674d0).
- history: fifty-objects wave1/wave2 (w/breakable_surfaces_audit{,2}); r2 lead declined the /Ow removal
  ("on precedent alone, without image proof, and earns nothing").

## BS-F1 outcome (2026-09-26 00:38:56 -0700) - card cards/BS-F1_flag_override.txt - ALL PREDICTIONS CONFIRMED
- tools: A2/tools/lab_gate.py (= tools/campaign/gate.py + --drop/--split-root/--tmpdir; diff verified),
  objcmp.py, surplus_cand.py (= surplus_identity logic on a candidate obj; reproduces production's 7 DIFFERENT).
- controls: lab_gate production breakable/render_debug objects == build/base (objcmp IDENTICAL).
- P1 drop /QIfist: 12/12, objcmp IDENTICAL to production.
- P2 drop /Ow: 12/12; CHANGED exactly _breakable_surface_get_plane_from_designator (static), _cross_product3d,
  _plane3d_from_point_and_normal, _plane3d_negate, _project_point2d, _project_point3d, _vector_from_points3d;
  January-owned sections, data and symbol table unchanged.
- P3 drop both == P2 (objcmp IDENTICAL). surplus: 26 COMDATs, 1 not identical (_real_local_random vs effects);
  the 6 /Ow-affected COMDATs are IDENTICAL to January's selected copies.

## BS-S1 symbols.json storage (2026-09-26 00:45:40 -0700) - closes BOTH object_audit findings (LEAD-OWNED patch)
- Evidence: cachebeta.pdb publics contain neither _breakable_surface_effect nor _globals (J: January was static);
  our source already declares both static; the split (symbols.json rows 5362, 23118) marks them external.
  _globals is attested by January's own assert literals "globals"/"!globals" in this object.
- Census: no other January split object defines or references either name.
- csplit emulation: control cfg (unmodified copies) -> 833/833 byte-identical to build/split.
  cfg_BS (2 in-place '"static": true' edits, CRLF kept, no reserialisation) -> ONLY breakable_surfaces.obj differs.
- Against split_BS: production object object_audit PASS (26 January symbols, 0 differ); lab_gate 12/12.
- Patch: LEAD_breakable_surfaces_symbols_json.patch (LF, git apply --check OK at HEAD 455dffad).
- Tools: tools/object_audit_split.py (SPLIT_ROOT env; diff vs scratch/tools/object_audit.py = 3 lines),
  tools/splitcmp.py, tools/mkpatch.sh.

## BS-T1 outcome (2026-09-26 00:46:33 -0700) - card cards/BS-T1_designator_helper.txt - PREDICTIONS CONFIRMED
- copies/breakable_surfaces_T1.c (CRLF kept): local static copy deleted; genuine bsp3d_get_plane_from_designator.
- production flags (/Ow /QIfist): 12/12; January-owned sections unchanged; static 96-B copy gone; NEW external COMDAT
  _bsp3d_get_plane_from_designator (112) DIFFERENT vs January's selected copy (decals) -> 8 not identical.
  => T1 is NOT admissible alone under /Ow (rule 42 needs a byte-identical helper copy).
- default flags (no override): 12/12; _bsp3d_get_plane_from_designator IDENTICAL to decals; surplus 27 COMDATs,
  1 not identical (_real_local_random); 1 static (_breakable_surface_plane_distance). provider_link FAIL(1) only
  _real_local_random vs effects; _bsp3d_get_plane_from_designator vs decals PASS both orders.
=> T1 and the flag fix are a COUPLED repair (rule 24): each has independent evidence (/Od shared call; flag census).

## RD-R1 outcome (2026-09-26 00:49:11 -0700) - card cards/RD-R1_rotate_vector2d.txt - ALL PREDICTIONS CONFIRMED
- copies/render_debug_R1.c: 36/36 EXACT; objcmp vs production: ONLY +_rotate_vector2d (48, external COMDAT).
- surplus_cand: 17 COMDATs, 0 not identical (_rotate_vector2d == January's path_obstacles copy); 0 static surplus.
- provider_link --baseline (new surplus only): _rotate_vector2d vs path_obstacles PASS both orders.
- strip test copies/render_debug_R1_strip.c (casts removed): object IDENTICAL to R1; /W3 adds exactly 2 C4133
  (point2d->vector2d) - the casts are type bridges only (byte-inert). Cast form: /W3 census unchanged vs production.
- sweep harness validated: tools/sweep.py control (copy of source/, 447 units, cwd=shadow root) == build/base 447/447.

## BS-B1 outcome (2026-09-26 00:53:38 -0700) - card cards/BS-B1_real_local_random_board.txt - PREDICTION PARTLY FAILED
- sweep variants (tools/sweep.py, shadow copies of source/, production flags): ctrl == build/base 447/447.
- b1h (real_math.h named-local body only): 442 identical, 5 different:
    breakable_surfaces _real_local_random, weather_particle_systems _real_local_random (surplus bodies, intended);
    rasterizer_frame_statistics _rasterizer_frame_statistics_draw 4176 EXACT -> residual;
    bitmap_drawing _bitmap_copy 2784 EXACT -> residual;
    ai_communication _ai_communication_update_speech_timers 672 EXACT -> residual   <- NEW vs 931ed8dc.
- b1 (+ effects.c: rename/#undef + NODUP copy removed, /Od-attested real_local_random() and
  real_local_random_range(0.0f, 2.f*_pi) restored): same 5 + effects: _real_local_random NODUPLICATES -> ANY
  (bytes unchanged, row EXACT 38/38 unchanged), NEW surplus COMDAT _real_local_random_range (ANY) in effects.
- Victims do not use real_local_random (grep): distant compiler-state flips.
- LAB ORACLE b1d (one dummy prototype before real_local_random, never landable): flips
  _rasterizer_frame_statistics_draw, _bitmap_copy, __rasterizer_model_draw, _bitmap_2d_alpha_bleed (NOT
  ai_communication). => C1-number class (vc7-c1-number-mod64-law); a prototype and a body-local shift different
  number streams, so the victim sets differ.
- FAILED PREDICTION: I predicted the same 2 victims (+physics_update_old moving); actual = 3 victims, the third
  (_ai_communication_update_speech_timers) new since 931ed8dc; physics_update_old unaffected.
- Cost of the genuine B1 fix at HEAD: 3 exact functions, 7,632 padded bytes. OWNER RULING REQUIRED (rule 62).

## BS-T2 outcome (2026-09-26 00:54:34 -0700) - card cards/BS-T2_genuine_plane_distance.txt - PREDICTION CONFIRMED (measure-only)
- copies/breakable_surfaces_T2.c (T1 + genuine plane3d_distance_to_point x4, local helper deleted), default flags:
  11/12; _breakable_surface_effect residual [sha] 4032/4032, 1156/1156 instructions, relocations equal.
- Differences = exactly 4 sites x 2 loads: January loads y*n.j ([esi+4]) before x*n.i ([esi]); ours the reverse
  (x87 commutative operand-order class, vc7-x87-operand-sort-decoded). No shape search run (70+ prior shapes;
  rule 64). B2 stays an owner question: TU-local association helper (bsp3d precedent) vs genuine helper.

## HEAD moved to 42fa975e (2026-09-26 01:01:31 -0700): +343a3f82 (hud_weapon), +42fa975e (decals.c). Neither touches my units,
   real_math.h, effects.c, symbols.json or config.json. build/base decals.obj verified current (objcmp vs fresh
   compile IDENTICAL). Brief v2 updated: owner rulings 2026-09-26 binding; "no C4013" = no NEW C4013; Q5 accepts
   descriptive names for unknown-name entities.
## BS-B1 re-measured at 42fa975e: identical to 455dffad result (ctrl2 == build/base 447/447; b1h2/b1_2 = same
   5/6 changed TUs, same 3 victims).
## BS-FIN combined candidate finB (2026-09-26 01:01:31 -0700) = ctrl2 + B1 (real_math.h + effects.c) + copies/breakable_surfaces_final.c,
   breakable flags without /Ow /QIfist (MSYS_NO_PATHCONV=1 required; a first run without it silently kept /Ow -
   caught by objcmp, rerun).
- board: 441 identical; changed = breakable (intended), weather/effects (intended), and the 3 B1 victims.
- breakable: 12/12; object_audit vs split_BS PASS (26/0); surplus 27 COMDATs, 0 not identical; provider_link
  (providers from finB) PASS incl. _real_local_random vs effects and _bsp3d_get_plane_from_designator vs decals.
  Same object vs PRODUCTION effects: FAIL(1) _real_local_random (NODUP) -> the effects.c part is required.
- effects: 38 rows unchanged; new surplus _real_local_random_range IDENTICAL (bored_camera), link PASS.
- weather_particle_systems: 19 rows unchanged; _real_local_random now IDENTICAL to January's effects copy.
- ONLY remaining breakable item: static candidate-only _breakable_surface_plane_distance (B2, owner).

## RD-D1 (owner-conditional) rebased at HEAD (2026-09-26 01:06:17 -0700) - measure only (lab form of owner-queue Q2d D1)
- copies/render_debug_R1_D1.c = RD-R1 + aggregate/pads/8 offset checks removed, 7 '= 0' file statics with
  descriptive render_debug_cache_* names + disclosure comment; 32 uses renamed mechanically.
- cfg_RD1 symbols.json: row 23256 _render_debug_globals -> _render_debug_cache_strings + 6 static rows
  (+0x400 4952128, +0x7400 4980800, +0x7404 4980804, +0x7408 4980808, +0x740A 4980810, +0x740B 4980811).
  csplit: ONLY render_debug.obj differs (833 compared).
- vs split_RD1: R1_D1 36/36 EXACT, object_audit PASS (69 January symbols, 0 differ).
  Aggregate source (R1) vs split_RD1: 33/36 (3 reloc-identity) -> source and symbols.json must land together.
- strip test (initialisers removed): gate 36/36 but object_audit FAIL(7): .bss 29715 B, name-hash order ->
  the '= 0' initialisers are layout-load-bearing with these names (same as the fifty-objects lab).
- no new first-party names: January atlas text-only; HCEX.pdb + HCEX_Release.pdb have NO render_debug.c symbols
  (DIA2Dump -g: no render_debug_triangle/add_cache*/cache names); cachebeta publics none; 2001 maps functions only.
- patches: render_debug.patch (R1, vs HEAD), OWNER_D1_render_debug_separate_statics.patch (applies AFTER
  render_debug.patch), OWNER_D1_symbols_json.patch (lead). Sequential git apply post-images == measured copies
  (mod EOL); breakable_surfaces.patch post-image == copies/breakable_surfaces_final.c (mod EOL).

## RD-C4 outcome (2026-09-26 01:15:22 -0700) - card cards/RD-C4_implicit_declarations.txt - PREDICTION FAILED (better than predicted)
- render_debug.c + ai/ai_debug.h + physics/collision_debug.h includes (my unit): object IDENTICAL to RD-R1; C4013 5 -> 3.
- LEAD header packet c4b: render_debug_object_damage -> objects/damage.h (damage.c's own duplicate local prototype
  removed), render_debug_recording -> cutscene/recorded_animations.h, render_debug_fog_planes ->
  structures/structures.h; render_debug.c additionally includes cutscene/recorded_animations.h + objects/damage.h.
  render_debug /W3: 0 C4013 (5 before); damage/recorded_animations/structures /W3 unchanged.
- 447-unit sweep c4b vs ctrl2: 445 identical; render_debug (only +_rotate_vector2d = RD-R1); rasterizer_xbox_models:
  PARKED residual __rasterizer_model_draw moves 89b0d7ea -> cbfa8585 (5168 B / 348 relocs unchanged; strict
  12/12 unchanged). damage.obj IDENTICAL. ZERO exact-row changes. Cause: rasterizer_xbox_models.c includes
  structures/structures.h directly (+1 C1 number). Same sha the fifty-objects queue recorded for a +1 shift.
- mini objdiff 3.3.1 project (one unit): control reproduces 95.08453 (== build/report.json == parked.json);
  c4b = 95.07629. Park re-baseline patch: LEAD_parked_model_draw_rebaseline_IF_C4.patch (history note appended).
- FAILED PREDICTION: I predicted >= 1 exact loss; actual 0 (one parked residual moved).
- Patches: render_debug.patch (R1 + 2 includes + comment placeholders -> real names; object == R1),
  render_debug_includes_after_LEAD_prototypes.patch, LEAD_owner_prototypes_c4013.patch,
  LEAD_parked_model_draw_rebaseline_IF_C4.patch, OWNER_D1_render_debug_separate_statics.patch (rebased onto
  render_debug.patch), OWNER_D1_symbols_json.patch, LEAD_render_debug_status_matching_IF_D_RULED.patch.
  Apply orders A (own), B (+C4), C (+D1), D (+C4+D1) all reproduce the measured copies (mod EOL).

## FINAL BATTERY (2026-09-26 01:18:50 -0700, HEAD 42fa975e) - tools/battery.py, reports in battery/*.txt
- battery validated on production render_debug (CONTROL_render_debug_production.txt): reproduces 36/36, audit PASS,
  pdb 0, surplus 16/0, link PASS, protoscan clean, 5 C4013, fake scan 0.
- RD1 own patch (vs build/split): 36/36 | audit PASS | pdb 0 (63) | surplus 17/0 | link PASS | protoscan clean |
  /W3 C4013 3 (inherited; 5 at HEAD) | fake 0.
- RD2 own + C4 (shadow c4b): same, /W3 C4013 0.
- RD3 own + D1 (vs split_RD1): 36/36 | audit PASS | pdb 0 (69) | surplus 17/0 | link PASS | protoscan clean | C4013 3 | fake 0.
- RD4 own + C4 + D1 (shadow rdall, vs split_RD1): all PASS, C4013 0.
- BS0 production vs split_BS: 12/12 | audit PASS | pdb 0 | surplus 26/7 DIFFERENT + 2 statics | link FAIL(1) | fake 0.
- BS2 T1 + no override, WITHOUT B1: 12/12 | audit PASS | pdb 0 | surplus 27/1 (real_local_random) + 1 static | link FAIL(1).
- BS1 T1 + no override + B1 (shadow finB, providers finB): 12/12 | audit PASS | pdb 0 | surplus 27/0 + 1 static
  (_breakable_surface_plane_distance, B2) | link PASS | protoscan clean | /W3 unchanged (C4244 x15, C4305, C4146) | fake 0.
- lead review_patch.py: A2_render_debug 0 gains/0 losses, +_rotate_vector2d; A2_breakable_surfaces 0/0,
  +_bsp3d_get_plane_from_designator, -static copy (under production /Ow flags; coupled to the flag patch).
- D0 option: OWNER_D0_render_debug_disclosure.patch (comment only, object IDENTICAL; applies after render_debug.patch
  with or without the C4 include increment).

## Re-verification at HEAD 09f5208f (2026-09-26 01:23:21 -0700) using the PATCH FILES themselves (git apply into CRLF shadow copies)
- HEAD moved (+physics.c, main.c, parked.json -1 entry). All my patches still apply (incl. the parked rebaseline).
- ctrl3 == build/base 447/447. c4_3 (render_debug.patch + includes increment + LEAD_owner_prototypes): 445 identical;
  render_debug (+_rotate_vector2d), model_draw parked move only. b1_3 (both OWNER_B1 patches): same 6 TUs / 3 victims.
- all_3 (every patch incl. D1 + breakable + B1, breakable flags /Ow /QIfist dropped): 438 identical, 9 different =
  union of the above PLUS one INTERACTION: physics/collisions _collision_move_point (residual, NOT parked) bytes move
  7270ae9f -> 2cded988 at 4752 B / 226 relocs; objdiff 3.3.1 99.60962 -> 99.623764 (fuzzy only, zero strict effect).
  It appears only when C4 and B1 land together (c4_3 and b1_3 alone leave it identical).
- all_3 breakable obj == finB obj; all_3 render_debug obj == rdall obj (objcmp IDENTICAL) -> batteries BS1/RD4 hold.
- Row changes in all_3: exactly the 3 B1 victims EXACT -> residual; 0 other status changes.

## MANIFEST (sha256 prefix; all LF; git apply --check clean at HEAD 09f5208f unless marked increment)
MY UNITS (source):
  3b87976628adccd9 render_debug.patch  (RD-R1 rotate_vector2d genuine call w/ 2 ruling-3 view casts; +ai/ai_debug.h,
                   +physics/collision_debug.h owner includes; header-comment _code_/_bss_ placeholders -> real names)
  db2e15f9af25446f render_debug_includes_after_LEAD_prototypes.patch (increment; ONLY with LEAD_owner_prototypes_c4013)
  b5e3e0df8804261a breakable_surfaces.patch (BS-T1 genuine bsp3d_get_plane_from_designator; dead #undef removed)
                   COUPLED: land only with LEAD_breakable_surfaces_config_flags.patch (else new COMDAT non-identical)
LEAD-OWNED:
  46f714aaae9c0865 LEAD_breakable_surfaces_symbols_json.patch (2 x "static": true; closes both object_audit findings)
  210f706b36fea775 LEAD_breakable_surfaces_config_flags.patch (per-TU /Ow /QIfist override removed; run configure.py)
  0883fb5802a1a791 LEAD_owner_prototypes_c4013.patch (optional hygiene: 3 owner-header prototypes + damage.c dup removal)
  c45cc661b45b5932 LEAD_parked_model_draw_rebaseline_IF_C4.patch (with the previous one only)
  0643363d0cc5a64e LEAD_render_debug_status_matching_IF_D_RULED.patch
OWNER-GATED:
  388507f4b74b018f OWNER_D1_render_debug_separate_statics.patch (increment after render_debug.patch) +
  8280a6ab5d24e166 OWNER_D1_symbols_json.patch                   (must land together)
  472f4cfab0eba664 OWNER_D0_render_debug_disclosure.patch (comment only; object identical)
  0d86021a2592e78b OWNER_B1_real_math_real_local_random.patch +
  73696f6b9cbb09d8 OWNER_B1_effects_real_local_random.patch      (must land together; 3 exact losses)

## OWNER QUESTIONS (every side effect disclosed)
OQ1 render_debug (d) aggregate [= fifty-objects owner-queue Q2d, still unanswered]: D1 / D0 / HOLD?
  D1 = OWNER_D1 pair: 7 '= 0' file statics named render_debug_cache_* (descriptive, disclosed; no first-party name
  exists: cachebeta publics, HCEX.pdb, HCEX_Release.pdb (no render_debug.c symbols at all), 2001 maps, January atlas
  (text-only) all checked). symbols.json: 1 rename + 6 new static rows. Initialisers are layout-load-bearing (strip
  test FAIL(7)). New symbols: none external; 7 static .bss names replace 1. Conflicts with the 2026-09-15 preference
  against descriptive static names in symbols.json (env_fog/models held); Q5 (09-26) accepted descriptive names for
  live temporaries only.
  D0 = keep the aggregate + OWNER_D0 disclosure comment (invented aggregate with 2 opaque pads).
  Either -> +1 object (render_debug) after LEAD_render_debug_status_matching; 0 functions/bytes (already 36/36,
  data full; objdiff unchanged).
OQ2 breakable per-TU flags: approve removing "/Ow /QIfist" (config.json)? r2 lead declined 09-24 ("precedent alone,
  without image proof, earns nothing"). Facts: the override is image-UNOBSERVABLE both ways (all 12 January-owned
  sections + data identical with/without; /QIfist object-identical); added by PR #35 (369b71e7) with no evidence;
  flag census: January flag-uniform /O2 /Oy- where observable; W10a 67-state sweep: /Ow gains nothing board-wide;
  with the override 6 surplus helper COMDATs are NOT identical to January's selected copies (surplus gate fails) and
  the /Od-attested genuine designator helper's COMDAT is not identical either; without it all are identical.
  Rules 25/50: per-TU flags need target evidence. It is required for breakable admission (not sufficient alone).
OQ3 B1 real_local_random: land the double-attested named-local header body (J: January effects copy add esp,4; D:
  /Od slot) + effects.c NODUP removal with /Od-attested calls? COST at HEAD (re-measured 455dffad, 42fa975e,
  09f5208f): _rasterizer_frame_statistics_draw 4176, _bitmap_copy 2784, _ai_communication_update_speech_timers 672
  EXACT -> residual (C1-number flips; none uses real_local_random). Other effects: weather's _real_local_random
  becomes January-identical; effects emits a new identical _real_local_random_range COMDAT (link PASS); effects rows
  unchanged. Combined with the C4 packet: _collision_move_point (not exact, not parked) 99.60962 -> 99.623764.
  Required for breakable admission (surplus + link).
OQ4 B2 breakable_surface_plane_distance: admit the TU-local static association helper (landed 95d674d0; bsp3d
  precedent, Matching, same class; bipeds park declined it) for WHOLE-OBJECT admission? Genuine /Od-attested helper
  (4 real calls to plane3d_distance_to_point 0x56d580) makes _breakable_surface_effect residual [sha]: 4 sites, January
  adds y*n.j before x*n.i (x87 operand-sort key class; 70+ prior shapes; not re-searched, rule 64). Candidate-only
  static code 32 B; January's object has no such section.
