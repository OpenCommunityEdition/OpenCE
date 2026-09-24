# LEDGER breakable_surfaces_audit2 (wave 2 re-audit of source/physics/breakable_surfaces, base 931ed8dc)

Resumes from scratch/w/breakable_surfaces_audit/LEDGER.md (wave 1). Wave-1 state: 12/12 EXACT; blockers
were 7 NODUP provider conflicts (decals x4, path_obstacles x2, effects x1), _project_point2d and
_real_local_random header-body mismatch, /Ow-caused 5 DIFFERENT COMDATs, static surplus
_breakable_surface_get_plane_from_designator (T1 fix) and _breakable_surface_plane_distance (bsp3d precedent),
symbols.json storage of _breakable_surface_effect and _globals.

## Step 1 - current state at 931ed8dc (production source copied to prod.c)
- gate --all prod.c: 12/12 EXACT (flags /O2 /Oy- /Ow /QIfist from build.ninja).
- object_audit prod.obj: 26 January sections ok; 2 symbol DIFFs unchanged from wave 1: _breakable_surface_effect and
  _globals split storage 2 vs ours 3 (fix = symbols.json "static": true at lines 5362 and 23114).
- surplus_obj prod.obj: 25 external COMDATs, 9 problems: DIFFERENT _project_point3d, _project_point2d, _vector_from_points3d,
  _cross_product3d, _plane3d_from_point_and_normal, _plane3d_negate, _real_local_random; STATIC
  _breakable_surface_plane_distance, _breakable_surface_get_plane_from_designator.
## Probe A1 - wave-1 T1 (static designator helper -> genuine bsp3d.h bsp3d_get_plane_from_designator) x flag variants
Hypothesis (wave 1): /Ow is the only cause of 5 DIFFERENT COMDATs; Layer 2 fixed project_point2d's header body.
- T1 prod flags: 12/12, 9 problems (8 DIFFERENT incl. new _bsp3d_get_plane_from_designator + static plane_distance).
- T1 drop /Ow: 12/12, 2 problems: _real_local_random DIFFERENT (effects), STATIC _breakable_surface_plane_distance.
- T1 drop /QIfist only: identical to prod (9).  T1 drop both: 12/12, 2 problems (same as drop /Ow).
=> Layer 2 CONFIRMED: _project_point2d now IDENTICAL once /Ow is gone. Remaining: _real_local_random, static plane_distance.

## Probe A2 - provider_link on T1 drop-both (T1_none.obj)
- FAIL(1): only _real_local_random vs effects (LNK2005 both orders; effects hand copy is NODUP). All other 26 surplus rows PASS
  (Layer 1/2 removed decals/path_obstacles NODUP copies).
## Finding F1 - January real_local_random body is the NAMED-LOCAL form (double-attested)
- January effects _real_local_random bytes: e8 call get_global_local_random_seed_address; 50 push; e8 call real_seed_random;
  83 c4 04 add esp,4; c3.  Header direct-return body compiles to ... 59 pop ecx; c3 (DIFFERENT).
- Effects NODUP hand copy `real result = real_seed_random(...); return result;` reproduces add esp,4 (IDENTICAL).
- /Od build: real_local_random = 0x573ae0 (calls seed getter 0x6c9790 then real_seed_random 0x6c9a30) has push ecx slot +
  fstp [ebp-4]/fld [ebp-4] = named local; /Od real_random 0x42e1c0 (global seed, 0x6c97a0) has NO slot = direct return.
  January real_random (action_charge copy) = pop ecx = header direct-return form (IDENTICAL). Same pairing in both builds.
- real_math.h real_local_random_range already uses the named-local form (`real result= ...; return result;`) and is IDENTICAL
  to January bored_camera copy; /Od 0x521410 shows the same slot.
## Finding F2 - effects genuinely used real_local_random (why January's copy sits in effects)
- January effects.obj has the _real_local_random COMDAT but NO relocation to it (every use inlined; VC7 still emits the
  out-of-line copy of a used __inline - vc7-static-comdat-law).  Link order (config index desc = image order): effects 324 >
  weather 312 > first_person_weapons 262 > breakable 145 > fog 120, so effects is the first user => selected copy. Consistent.
- /Od effect_generate_particles (0x56f580): alpha else-arm = call real_local_random (0x573ae0) at 0x56fdee; rotation =
  call real_local_random_range (0x521410) at 0x56fd6d.  Our effects.c spells both as real_seed_random*(get_global_local_random_seed_address(), ...)
  and keeps an uncalled NODUP hand copy of real_local_random (with #define rename of the header inline).
## Probe S1 - shadow whole-board sweep tool (sweep.py) control
- sh/source = copy of source/; compile all 447 build\base\source units exactly as build.ninja with cwd=sh (same __FILE__ text).
- control pristine vs build/base: 447/447 identical (non-.debug sections + symbol tables).
## Probe S2 - mod1 = real_math.h real_local_random named-local body + effects.c hand copy/#define/#undef removed
- 441 identical, 6 different: breakable_surfaces (_real_local_random only), weather_particle_systems (_real_local_random only),
  effects (_real_local_random NO LONGER EMITTED - nothing in effects calls it), physics _physics_update_old (residual both),
  rasterizer_frame_statistics _rasterizer_frame_statistics_draw EXACT->residual (4176), bitmap_drawing _bitmap_copy EXACT->residual (2784).
=> header edit has a declaration-count blast radius (2 exact functions lost) and effects loses its January-owned COMDAT.
## Probe O1-O2 / P2-P3 / R1Px (LAB ORACLE ONLY - dummy declarations are never landable)
- O1 rlr_header alone: same flips as mod1 (rasterizer_frame_statistics_draw, bitmap_copy lose EXACT; physics_update_old residual moves).
- O2 +1 dummy `void name_count_probe(void);` (no header body change): SAME victims (+ bitmap_2d_alpha_bleed, residual).
  => the named local's cost is pure declaration count (TU name-count oracle), not the body.
- Scan: rasterizer_frame_statistics_draw breaks at every tested offset -2,-1,+1,+2,+3,+4,+5 (knife edge at 0);
  bitmap_copy breaks at -2,-1,+1 but survives +2,+3.
- V2 = rlr_header + drop cseries.h one-line duplicate `void stack_walk_disregard_symbol_names(boolean disregard);`
  (cseries_windows.h keeps the house-style owner copy; no callers anywhere): both victims still flip (param name counts too:
  net -1), plus collisions _collision_move_point, shell_xbox .rdata. Not a compensator.
- L2rev (e9e62b78 real_math.h + random_math.h): victims unchanged => Layer 2 was count-neutral for them because they include
  real_math.h but NOT random_math.h: -2 (`v` locals in distance2d/3d) +2 (lock/unlock prototypes moved in). Model confirmed.
- real_math.h census (inline_locals.py): nonuniform_cubic_spline (v9,v10,v11; reads uninitialised v9) and
  nonuniform_cubic_spline_vector3d (v10..v20) are decompiler-named junk bodies ("TODO: doesn't match"); glow.c keeps clean
  NODUP hand copies (assert line 1530 real_math.h). Genuine repair there is -14 names: the header count is known to be far
  from January's, so the victims' exactness sits on a coincidental total. No evidence-backed -1 compensator exists.
=> _real_local_random fix is EVIDENCE-COMPLETE but OWNER-GATED: the genuine /Od+January-attested header body costs
   2 exact functions (4,176 + 2,784 B) by declaration count alone.
## Probe E1/E2 - effects side of the real_local_random packet (effects.c only, with rlr_header)
- effects_nodup + /Od-attested spellings in effect_generate_particles: alpha else-arm `real_local_random()` (/Od 0x56fdee) and
  rotation `real_local_random_range(0.0f, 2.f*_pi)` (/Od 0x56fd6d). Effects stays 38 exact / 3 residual; its _real_local_random
  row stays EXACT (now the genuine header SELECT_ANY copy, from a real use); effect_generate_particles byte-identical to
  pristine (the call spelling is inert after inlining). Without the call restore (mod1) effects stops emitting the section.
## Rule 22 flag proof (production source, only this TU's flags change; SWEEP_DROP lab)
- F1 drop /QIfist: breakable object IDENTICAL to pristine (every non-debug section + symbol table) => /QIfist is dead here
  (all 4 real->long conversions go through the shared fast_ftol helper; no C cast is compiled differently).
- F2 drop /Ow: all 12 owned functions + data byte-identical; only 7 helper COMDATs change (_cross_product3d,
  _plane3d_from_point_and_normal, _plane3d_negate, _project_point2d, _project_point3d, _vector_from_points3d,
  static designator helper) - and with /Ow dropped the 6 external ones become IDENTICAL to January's selected copies.
- F3 drop both: 12/12 EXACT; surplus problems left = _real_local_random + the two statics (T1 removes one).
- /Ow appears nowhere else in config.json; no exported function changes, so every caller (other TUs, own flags) sees the same
  ABI/bytes. Override has no target evidence (369b71e7) and is contradicted by the helper-identity audit.
## C1 = admissible packet (T1 source + config override removed + symbols.json 2x "static": true)
- scratch config/symbols.json (2 in-place line edits: 5362, 23114) -> csplit to split/: ONLY breakable_surfaces.obj differs
  from build/split (833 compared).
- fnstat 12/12 EXACT; audit2 (AUDIT_SPLIT_ROOT=split) OBJECT AUDIT PASS 26/26 symbols; storage vs cachebeta publics: 41 defined
  symbols, 0 disagreements; data 685/685; fake_match_scan 0 leads; no parks / rejections for the unit.
- surplus_obj: 26 external COMDATs, 2 problems: _real_local_random DIFFERENT (effects), STATIC _breakable_surface_plane_distance.
- provider_link: FAIL(1) _real_local_random (effects NODUP); all other rows PASS.
- whole-board sweep C1full vs pristine: 446 identical, only breakable_surfaces differs (designator helper swap + 6 COMDATs).
## C2 = C1 + owner-gated real_local_random packet (real_math.h body + effects E2)
- breakable 12/12, audit PASS, surplus 1 problem (static plane_distance only), provider_link PASS (incl. _real_local_random
  vs effects E2 and _real_local_random_range vs bored_camera).
- board: rasterizer_frame_statistics_draw (4176) and bitmap_copy (2784) EXACT->residual (declaration count); physics_update_old
  residual moves; weather_particle_systems _real_local_random surplus becomes the January form.
## Probe PD (NEGATIVE) - header plane3d_distance_to_point without outer parentheses (hypothesis from bsp3d_obj.md B24)
- pd_noparen + T12 (genuine helper at the 4 breakable sites) + default flags: _breakable_surface_effect residual (4032, sha);
  board: 7 other TUs change (lens_flares, render_cameras, physics, collision_bsp x2, geometry, items). Parens are not the
  j+k association control here; the static breakable_surface_plane_distance stays (owner-landed 95d674d0; bsp3d precedent).
## FINAL (wave 2)
Admissible packet (zero board regressions, shadow whole-board sweep 446/447 identical, only breakable changes):
  patches/breakable_surfaces.c.patch  (T1: static hand copy of bsp3d.h helper deleted; genuine bsp3d_get_plane_from_designator)
  patches/config.json.patch           (per-TU "options" /Ow /QIfist override removed - rule 22 proof above)
  patches/symbols.json.patch + config/symbols.json (full copy; lines 5362, 23114 gain "static": true)
  gate (lab_gate --drop /Ow --drop /QIfist, split with fixed symbols): 12/12 EXACT (gate_C1.txt); audit PASS.
Remaining whole-object blockers after that packet:
  B1 _real_local_random surplus DIFFERENT vs effects (and effects' copy is a NODUP hand copy): genuine fix is known and
     double-attested (patches_owner_gated/: real_math.h named-local body + effects.c NODUP removal + /Od-attested calls) but
     costs rasterizer_frame_statistics (10/10 -> 9/10, _rasterizer_frame_statistics_draw 4176) and bitmap_drawing
     (7/7 -> 6/7, _bitmap_copy 2784) purely by declaration count (dummy-declaration oracle reproduces the same flips).
     No evidence-backed compensating -1 declaration found (duplicate-prototype census: only CRT dups + stack_walk (-2) /
     bitmap_delete (-2), both measured still broken). OWNER RULING needed.
  B2 static candidate-only _breakable_surface_plane_distance (32 B): owner-landed 95d674d0 grouped j/k association; precedent
     bsp3d (Matching, same TU-local plane-distance helper) vs bipeds park (declined the same lever). Explicit owner ruling needed.
Reopen criteria: B1 - an evidence-backed January declaration difference that restores the net name count before
  _rasterizer_frame_statistics_draw/_bitmap_copy (or owner acceptance of the count-coincidence loss), or a scorer/flag change;
  B2 - January source for the j/k grouping, or an owner ruling accepting the bsp3d precedent for breakable.
Stop rule: B1 is a declaration-count tie with no attested compensator; B2 was exhausted by 70+ prior probes + PD negative.
