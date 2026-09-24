# LEDGER - bounding_sphere (_object_get_bounding_sphere provider conflict; render_objects, object_lights)

Base tree 931ed8dc. Worker dir scratch/w/bounding_sphere/. Tools written here: strcensus.py (raw
string census), symcensus.py (COFF symbol census), relrefs.py (relocations to a symbol), pathgroups.py.

## F1 - January per-TU __FILE__ census (evidence, no compile)
- January literal symbols for objects.h: `??_C@_0BF@EIEPJPCC@?4?4?2objects?2objects?4h?$AA@` ("..\objects\objects.h")
  DEF action_vehicle, UNDEF render_objects; `??_C@_0CB@NDAECIIM@c?3?2halo?2source?2objects?2objects?4h@`
  ("c:\halo\source\objects\objects.h") DEF object_lights ONLY, referenced by NO relocation anywhere (orphan).
- `_object_get_bounding_sphere`: DEF action_vehicle (selected copy, "..\" relocs x2); UNDEF (out-of-line
  callers, so each emitted its own COMDAT copy in January) actor_moving, ai_debug (ai), hud_nav_points,
  motion_sensor (interface), object_lights (objects), render_objects (render), bipeds, vehicles (units).
  Our base emits the COMDAT in exactly the same 9 TUs.
- render_objects (January) DEFINES `_object_get_render_bounding_sphere` whose 2 asserts reference the
  "..\" literal (objects.h line 551/552).
- Whole-board January path-literal census (all `??_C@` names with c:\halo\source / ..\ prefixes):
  main .c files are always "c:\halo\SOURCE\<dir>\x.c" (296 strings, upper-case SOURCE); header literals
  are "c:\halo\source\<dir>\h.h" (lower-case) EXACTLY when every user TU lives in the header's own
  directory (ai\path.h, ai\encounters.h, ai\actor_type_definitions.h, sound\sound_*.h, hs\hs_library_*.h,
  objects\widgets\widget_types.h, objects\objects.h in object_lights) and "..\<dir>\h.h" when the TU is
  in another directory (math\real_math.h from ai/camera/effects/widgets/physics, bitmaps\bitmaps_inlines.h
  from effects/interface/rasterizer, objects\reference_lists.h from structures, objects\objects.h from
  ai/render). => January used __FILE__; a header found beside the TU's own .c got the absolute directory
  spelling, a header found through the project /I list (..\<dir>) got the relative one.
  objects.h is the ONLY header with BOTH spellings in January (object_lights is the only objects/ TU that
  uses an asserting objects.h inline).
- /Od build (later): string 0x93eb44 "D:\P4\Halo1\source\objects/objects.h" referenced by fn 0x443f50 =
  object_get_bounding_sphere (assert center line 0x2a9, radius 0x2aa, `push -1; call object_get_and_verify_type`,
  copy center + radius), plus 12 other objects.h assert sites. => objects.h's inline used plain
  `assert()` (i.e. __FILE__), confirmed first-party.
- Consequence: January's object_lights COMDAT copy referenced the c:\ literal (it is the only possible
  referrer; VC7 emits a literal only when referenced; /OPT:NOREF kept the orphan when the linker discarded
  object_lights' duplicate COMDAT in favour of action_vehicle's). January's 7 other non-selected copies
  used "..\" and were byte+reloc identical to action_vehicle's selected copy.
- Current tree: objects.h hard-codes "c:\\halo\\source\\objects\\objects.h" (right for object_lights only)
  and action_vehicle carries a NODUP hand copy under `#define object_get_bounding_sphere
  object_get_bounding_sphere_inline` with "..\". => 7 TUs (render_objects, actor_moving, ai_debug,
  hud_nav_points, motion_sensor, bipeds, vehicles) emit a copy that is NOT identical to January's selected
  copy AND collides (LNK2005) with the NODUP hand copy.

## Tooling
- tgate.py: compiles a unit from a FULL shadow copy of source/ (cwd = shadow root) with build.ninja's
  exact cflags, so shadow HEADERS are honoured by every include (gate.py --source does not see them).
  Validated: tree0 (unmodified copy) -> render_objects/object_lights/action_vehicle objects IDENTICAL
  to build/base (objcmp.py: every section, flags, symbol).
- objcmp.py (object vs object, owner-keyed), surplus_cand.py (surplus identity for a candidate obj
  + COMDAT selection type from the aux record), plink.py (explicit-object LNK2005 probe both orders),
  provider_link_cand.py (provider_link with --provider=<unit>::<obj> override).

## P1 - full __FILE__ model (tree1)  [hypothesis: F1 - one genuine inline body + per-TU __FILE__]
- Change: objects.h macros section gains
  `#ifndef OBJECTS_H_FILE / #define OBJECTS_H_FILE "..\objects\objects.h" / #endif` (+ comment); all 4
  objects.h assert sites (535/536 object_get_bounding_sphere, 551/552 object_get_render_bounding_sphere)
  use OBJECTS_H_FILE. object_lights.c defines OBJECTS_H_FILE "c:\halo\source\objects\objects.h" before
  its includes. action_vehicle.c: `#define object_get_bounding_sphere ..._inline` / `#undef` and the
  NODUP hand copy removed (genuine header inline used).
- Result (per-function rows identical to base in every unit; aggregate): action_vehicle 17/17,
  actor_moving 31/36, ai_debug 59/60, hud_nav_points 31/32, motion_sensor 16/19, object_lights 43/43,
  render_objects 22/22, bipeds 42/51, vehicles 37/39 (all == base counts).
- objcmp vs base: action_vehicle IDENTICAL (the genuine inline COMDAT has the same bytes/relocs as the
  hand copy; selection changes NODUPLICATES -> ANY); object_lights IDENTICAL (c:\ copy kept);
  the other 7 differ ONLY in `_object_get_bounding_sphere`'s 2 DIR32 literal targets and the literal
  section (c:\ -> ..\).
- surplus identity: _object_get_bounding_sphere now IDENTICAL to January's selected copy in actor_moving,
  ai_debug, hud_nav_points, motion_sensor, render_objects, bipeds, vehicles; object_lights DIFFERENT
  (by design: January's own object_lights copy referenced the c:\ literal - F1).
- links: base render_objects vs base action_vehicle = LNK2005 both orders (current blocker). tree1: every
  one of the 8 users pair-links clean with tree1 action_vehicle in both orders; all 9 together link clean
  both orders. provider_link_cand (all surplus symbols, action_vehicle provider = tree1 obj): PASS for
  render_objects, object_lights, actor_moving, ai_debug, hud_nav_points, motion_sensor, vehicles,
  action_vehicle; bipeds FAILs only on unrelated `_cross_product2d` (actor_combat NODUP) and
  `_object_get_type` (items NODUP).
- object_audit: render_objects PASS 41/41, object_lights PASS 71/71, action_vehicle PASS 31/31.

## P1 board sweep (sweep.py: all 447 cl units recompiled from the shadow tree, objcmp vs build/base)
- tree0 control: 446/447 identical (d3d_intimacy.cpp is not a C unit; COMPILE FAILED in both sweeps - tool limit).
- tree1: 9 changed units = the 7 expected (literal + COMDAT relocs only) PLUS
  physics/physics `_physics_update_old` and physics/collisions `_collision_move_point` - BOTH ALREADY RESIDUAL
  in base (no exact row lost). Base-vs-tree1 alndiff: each is ONE register-reload moved one slot
  (physics_update_old @0xe3a/0xe3d `mov esi,[ebp-0x80]`; collision_move_point @0x17b/0x17f
  `mov edi,[ebp-0x1c]`). vs January: collision_move_point 39 -> 41 diff blocks (the 0x17b/0x17f coin
  flips away), physics_update_old 311 -> 311 (frame-structural anyway).
- MECHANISM: the only preprocessed change in those TUs is the literal text inside the UNREFERENCED
  object_get_bounding_sphere inline body: the TU now sees ONE distinct objects.h literal (shared with
  object_get_render_bounding_sphere) instead of two. => distinct string literals count toward the
  TU name-count/ordinal currency (the same 0x17b/0x17f "count coin" the collisions worker recorded).
  January's own collisions/physics TUs also saw ONE distinct spelling ("..\" for both inlines), so tree1 is the
  January-faithful preprocessed state.
- COMBINED with the pending collisions packet F (scratch/w/collisions/final: real_math_planes.h inline
  valid_real_plane3d, matrix_math.c, collisions.c block-scope position): treeC (packet only) collisions 20/20;
  tree2 (packet + P1) collisions 20/20 EXACT. So P1 does not disturb packet F's closure.

## R1 - render_objects `_lighting_storage` storage (pdb_storage: split 2 / ours 2 / PDB-public False)
- Evidence: cachebeta publics lack it (January static); no UNDEF reference in any January split or base object;
  HCEX.pdb `-sym object_get_cached_render_lighting`: "Static Local, Type: struct render_lighting,
  lighting_storage" (HCEX render_object_globals is a File Static, so HCEX distinguishes the two).
  January .bss (1150 B): lighting_storage@0 (120), render_object_globals@120, debug_inactive_objects@1148,
  reported_rendered_object_overflow@1149.
- Probes (gate.py --source, production headers):
  L1 file-scope `static struct render_lighting lighting_storage = { 0 };` -> 22/22, .bss identical, storage 3.
  L2 function-scope `static struct render_lighting lighting_storage;` (no initialiser) in
     object_get_cached_render_lighting -> .bss IDENTICAL layout (uninitialised static placed first),
     symbol `?lighting_storage@?1??object_get_cached_render_lighting@@9@9` storage 3; 21/22 with the only
     residual `_object_get_cached_render_lighting [reloc-identity]` (sha equal; 3 DIR32 rows differ by NAME only).
  L3 function-scope `static ... = { 0 };` -> .bss 1148 B, lighting_storage moved to 1032 (declaration order,
     after the file globals) -> REFUTED by January's layout.
- L2 + scratch split (symfix.py cfg_ro: line 23251 `_lighting_storage` -> `?lighting_storage@?1??object_get_cached_render_lighting@@9@9`
  + "static": true; precedent: 6 existing `?x@?1??fn@@9@9` function statics in symbols.json): csplit changes ONLY
  render_objects.obj; tree3 (tree1 + L2) render_objects 22/22 EXACT vs scratch split, object_audit PASS 41/41,
  pdb_storage 0 disagreements.
- Verdict: HCEX's function-static is VERIFIED as byte- and layout-consistent in its UNINITIALISED form only
  (L2); L1 is the fallback if the owner declines the symbols.json rename (bytes equal, contradicts HCEX scope).

## C1 - object_lights combined with data_gap's pending object_lights packet (tree4 = tree3 + data_gap patches/object_lights.patch)
- data_gap patch applies on top of P1 (hunks offset 3 lines only). vs data_gap scratch split cfg_lights_split:
  object_lights 43/43 EXACT, object_audit PASS 71/71, pdb_storage 0 disagreements, provider_link_cand PASS
  (action_vehicle provider = P1 candidate), surplus identity: all 12 surplus COMDATs identical EXCEPT
  _object_get_bounding_sphere (c:\ literal) - which is January's own object_lights spelling (F1).
- render_objects / render (the other object_lights_rendering.h consumers) byte-identical with and without the
  data_gap patch.
- => object_lights' ONLY remaining admission question is the authentic-but-non-identical surplus COMDAT
  (owner ruling, see F1); the data entry for _lights_section is data_gap's (unchanged by P1: object_lights.obj
  under P1 is objcmp-IDENTICAL to production).

## P2 control - Design 1 only (objects.h -> "..\" everywhere, NO object_lights define; tree5)
- object_lights 43/43 but object_audit FAIL: MISSING the January-owned 33-byte .rdata
  `??_C@..c?3?2halo?2source?2objects?2objects?4h@`; surplus identity 0 not identical; mini objdiff 3.3.1:
  .rdata 1100 -> 96.93%, matched_data 1104 -> 4 (-1,100 data credit, .rdata is all-or-nothing).
  => the per-TU c:\ spelling in object_lights is load-bearing for January-owned data: the header cannot be
  made single-spelling without a data regression. Hence P1's OBJECTS_H_FILE override is required.
- mini objdiff (proj_ro, objdiff 3.3.1) for the full candidates: render_objects (vs cfg_ro split) 22/22,
  data 4678/4678 (.bss/.data/.rdata 100); action_vehicle 17/17, data 116/116; object_lights 43/43,
  data 1104/2645 (.data 99.74 = data_gap's '$' defect entry, unchanged).

## FINAL PACKET (scratch/w/bounding_sphere/final/*, patches/*.patch; all `git apply --check` clean, also together
## and in either order with scratch/w/data_gap/patches/object_lights.patch and symbols_json_all.patch)
- patches/objects_h.patch        OBJECTS_H_FILE macro (default "..\objects\objects.h") + 4 assert sites.
- patches/action_vehicle.patch   remove `#define object_get_bounding_sphere object_get_bounding_sphere_inline`/`#undef`
                                 and the NODUP hand copy (genuine objects.h inline emits the selected COMDAT).
- patches/object_lights.patch    `#define OBJECTS_H_FILE "c:\halo\source\objects\objects.h"` before includes.
- patches/render_objects.patch   lighting_storage -> uninitialised function static (HCEX) in object_get_cached_render_lighting.
- patches/symbols_json.patch (= config/symbols.json full copy, one line op at 23251):
  `_lighting_storage` -> `?lighting_storage@?1??object_get_cached_render_lighting@@9@9`, "static": true.
- OPTIONAL (zero credit, PDB-proven): patches/optional_action_vehicle_with_static_evaluate_seat.patch (REPLACES
  action_vehicle.patch) + patches/optional_symbols_json_with_evaluate_seat.patch (REPLACES symbols_json.patch;
  full copy config_with_evaluate_seat/symbols.json): `static boolean action_vehicle_evaluate_seat` (cachebeta
  non-public, zero January UNDEF refs) - byte-inert (objcmp: only storage 2->3), action_vehicle 17/17, audit PASS
  and pdb_storage 0 vs scratch split cfg_all (csplit changes only action_vehicle.obj + render_objects.obj).
- treeF full-board sweep (447 cl units): changed = the 7 literal-only units + render_objects (+lighting static)
  + physics_update_old / collision_move_point (both residual before and after; one reload swap each).
  Per-function rows identical to base in action_vehicle 17/17, actor_moving 31/36, ai_debug 59/60,
  hud_nav_points 31/32, motion_sensor 16/19, object_lights 43/43, render_objects 22/22 (vs cfg_ro split),
  bipeds 42/51, vehicles 37/39, physics 13/17, collisions 19/20. ZERO exact rows lost.
- fake_match_scan: 0 leads on the 4 candidate files.

## Per-object verdicts
- render/render_objects: COMPLETE CANDIDATE with packet (objects.h + action_vehicle + render_objects + symbols.json):
  22/22 strict, object_audit PASS 41/41, pdb_storage 0, surplus 9/9 identical, provider_link PASS (with the
  packet's action_vehicle; FAILS against the current NODUP action_vehicle, so both must land together),
  mini objdiff 3.3.1 code 5437/5437 data 4678/4678.
- objects/object_lights: function/data-exact but BLOCKED on an owner ruling: its surplus _object_get_bounding_sphere
  COMDAT must reference the c:\ literal (January-owned 33-byte .rdata; without it .rdata falls to 96.9% and
  -1,100 data credit) and therefore can never be section-identical to January's selected action_vehicle copy.
  This non-identity is January's own (F1). Links clean both orders with the packet's action_vehicle. Also
  needs data_gap's packet (4 statics + should_render_lights + _lights_section entry).
- ai/action_vehicle: already Matching; packet keeps 17/17 and an objcmp-IDENTICAL object (selection NODUP -> ANY),
  retiring the last hand copy in the unit. Optional evaluate_seat storage fix above.
- actor_moving, ai_debug, hud_nav_points, motion_sensor, bipeds, vehicles: bounding-sphere surplus now identical to
  January's selected copy (blocker removed); they remain incomplete on their own residuals (bipeds also has
  _cross_product2d/actor_combat and _object_get_type/items NODUP conflicts).

## Owner question (object_lights) and reopen criteria
- Q: admit a surplus header-inline COMDAT that differs from January's selected copy ONLY in its __FILE__
  literal target, when January's own object for that TU proves the same difference (orphan January-owned
  literal, no other possible referrer)? Rule 6 as written ("byte-identical to January's selected provider")
  cannot be met by object_lights in any source: the literal is January-owned data of object_lights and VC7 emits
  it only when this COMDAT references it. Link is clean both orders (selection ANY on both sides).
- If the owner prefers a different __FILE__ model (e.g. every objects/ TU defining OBJECTS_H_FILE, or a build-level
  per-directory define), all variants are byte-identical to P1 for every current unit (only object_lights uses an
  asserting objects.h inline among objects/ TUs).
- Reopen: a new asserting objects.h inline used by another objects/ TU (it must also define OBJECTS_H_FILE);
  any header whose January literal census shows both spellings (today only objects.h, see F1 census).
- Side finding for whoever owns collisions/physics residuals: distinct string literals parsed in UNREFERENCED header
  inline bodies are part of the TU ordinal currency (flip the collision_move_point 0x17b/0x17f coin and a
  physics_update_old reload); P1 is the January-faithful literal set, and packet F stays 20/20 with it.
