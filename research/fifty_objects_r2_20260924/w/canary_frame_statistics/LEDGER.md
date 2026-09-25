# LEDGER - declaration-count canary research (r2w2-canary)

Unit claimed: source/rasterizer/rasterizer_frame_statistics (label r2w2-canary). LAB ONLY.
Tree: 05255584 (clean). Slug: scratch/w/canary_frame_statistics/.
Goal: genuine source reason _rasterizer_frame_statistics_draw (4,176 B) is count-coincident, so the
genuine real_math.h bodies (real_local_random named local; valid_real_plane3d __inline) can land.

## Prior evidence read
- P3 (lane C FINDINGS.md): currency = declared names (tag/member/typedef/extern/local/param = 1; proto void f(void)=2);
  text/type-size inert; plateaus + period (21 x 3-name probe = 63 units?); for draw the count region was
  "after the private_globals extern, before draw"; names before it never substitute (0..45), removing 6 before
  never compensated (0..30 after). draw window 1 unit wide.
- Since P3, 4f4c8502 closed draw by genuine source (height_adjust/line_advance locals, count_static_vertices
  else-branch, eat_my_shorts else-if, extern->definition, reserved744 removed).
- r1 collisions: broad real_math.h valid_real_plane3d inline loses draw + bitmap_copy; draw exact only at +0;
  -6..+30 compensations failed; stack_walk dup removal failed. bitmap_copy restorable by bitmap_delete dup removal.
- r1 breakable audit2: real_local_random named local loses draw + bitmap_copy; -2..+5 scan knife edge.

## Tooling (all under slug; lab only)
- sh/ = pristine copy of source/ at 05255584; lab.py (compile with build.ninja flags, cwd=tree; strict rows vs build/split),
  sweep_pos.py (K dummies before a marker), scan_i.py (1 dummy at arbitrary file-scope line of a preprocessed TU),
  mkvar.py (variant trees var/<name> = sh + named edits), netcount.py (draw as a mod-64 counter).
- Control: sh compile of rasterizer_frame_statistics = 10/10 EXACT, draw sha 747b937d70 (== January).

## F1 - PERIOD IS EXACTLY 64, window 1 (sweep_pc.txt, sweep_top.txt)
- K opaque tags at `/* public code */` (after globals): draw EXACT at K = 0, 64, 128 only (0..140).
- K opaque tags at the top of the .c (before #includes): draw EXACT at K = 0, 64, 128 only (0..140).
  (P3's "names before the extern never work 0..45" was a range artifact: +64 works anywhere.)
- K=59 at either position gives normalized sha 747b93 but NOT strict-exact (relocation identity) - irrelevant.

## F2 - FILE RECORDS ARE PART OF THE CURRENCY (new)
- `#include` of a NEW empty header at public code = +1 (fingerprint == 1 opaque tag); the same header twice = +1;
  a new header holding one tag = +2; re-including an already-included guarded header (cseries.h,
  rasterizer_frame_statistics.h) = 0. /E output compiled as C is equivalent to the .c build plus 7 names at the
  top (== -57): each first `#line "file"` makes a file record (dropping all #line changes the state).
  fs_i57.c (57 dummies + .i) reproduces base byte-for-byte except .debug$S.
- Macros (#define) are NOT names (2 extra #defines: draw stays EXACT).

## F3 - draw's RELEVANT SYMBOLS ARE ALL INSIDE THE .c GLOBALS BLOCK AND LATER
- scan_i.py: one dummy at 24 file-scope positions from line 3 of the .c through every SDK header, cseries,
  errors, players/game/real_math/integer_math, interface, main_runtime, rasterizer, rasterizer_debug_options,
  rasterizer_frame_statistics.h, rasterizer_geometry, rasterizer_xbox, draw_string and the .c struct/verify-typedef
  region (.c line 193): fingerprint 7aafc0 at EVERY position (== +1 at top). One dummy at /* public code */ = b9bb05.
  => for draw, every declaration from the top of the TU to the verify typedefs is interchangeable currency:
  only NET COUNT (mod 64) before the globals block matters. Header externs/prototypes that draw uses are not webs.
- Variant trees: rlr (real_local_random named local) = net +1; vp (valid_real_plane3d proto -> __inline) = net +1;
  rlr+vp = net +2 (netcount.py: exact at K=63/63/62 top dummies). Matches r1 flips.
=> CONSEQUENCE: any GENUINE net -1 per +1 anywhere in the TU before the globals block restores draw. The question
   becomes a board-wide one: find genuine compensators whose effect is net 0 in every TU that holds a
   count-sensitive exact function.

## F4 - THE CURRENCY IS C1's SYMBOL TABLE, not just declared names (netcount.py probes before the globals block)
- extern var 1; static var with numeric init 1 (numeric constants cost 0: 123457, 1234.5f, 0 all free);
  static var with a string init 2 => a STRING LITERAL is a record; two vars sharing one literal = 3 (literals are
  DEDUPED per TU); two different literals = 4.
- static fn definition (void) = 3; with 1 param = 4; + one `if` (with or without braces) = 5 => an if costs 1
  (compiler label record); an extra { } block costs 0; fn with match_assert("f", 1530, x>0) = 7
  (= 3 + param + if + 2 new literals).
- stack_walk_disregard_symbol_names duplicate removal: cseries.h copy -2, cseries_windows.h copy -2.
- spline repairs (glow.c January-exact bodies, glow _nonuniform_cubic_spline 192 B / _vector3d 176 B strict EXACT):
  spl1 (nonuniform_cubic_spline) net 0 in this TU (-3 locals, + if, + #expr literal, + && label; the file literal
  dedups with uniform_cubic_spline's); spl3 (vector3d) net -11 (11 junk locals); both -11.
  (Probe bug caught: a mis-escaped file literal "..\math\real_math.h" made spl1 +1 - literal text matters only via
  dedup.)

## F5 - WHOLE-BOARD SWEEPS (board.py; 447 units in 6 s; control: sh sweep == build/base for all 447 units,
##      per-function size/sha/relocs)
Canary set found (count-coincident exact functions reached by these edits):
  rasterizer_frame_statistics::_rasterizer_frame_statistics_draw 4176, bitmap_drawing::_bitmap_copy 2784,
  units::_unit_preprocess_node_orientations 1920, bitmap_utilities::_bitmap_2d_alpha_bleed 560 (+ _uncompress_from_mipmap 96
  under spl3), decals::_decals_delete_permanent_from_cluster 400, rasterizer_xbox_models::__rasterizer_model_transparent_geometry_submit
  1296, error_geometry::_error_geometry_polygon 496, game_engine_race::_race_engine_player_update 160 / _race_touch_flag 336,
  geometry::_get_edge_vertex 416, collisions::_collision_move_point 4752 (the gain; itself count-sensitive).
Uniform-shift windows (window.py: K dummies at top of the .c; accepted nets): draw {0} only; bitmap_copy {0,1,6,7,8,-3..-16,-25..-31};
  units {-5,-4,0..31,...}; alpha_bleed {-1..15,19}; decals {-3..6,16..31,-28..-32}; models {-16..6 minus -12,...};
  error_geometry, race, geometry wide. Canaries whose TU includes real_math.h helpers they inline are ALSO position-
  sensitive (units: rlr +1 breaks it although +1 at the top is accepted) - their relevant symbols include inlined
  helper params/locals declared inside real_math.h, so only compensation adjacent to the edit is uniform for them.
Single-edit sweeps vs base: RLR (rlr) lost draw, bitmap_copy, units; VP (vp+vpd+vpc) +move_point, lost draw, bitmap_copy, units;
  RV (both) +move_point, lost draw, bitmap_copy, units, alpha_bleed; SPL (spl1+spl3) lost 5; SWCS lost 5 (draw, bitmap_copy,
  race x2, geometry); SWCW lost draw only; BDI / BDB lost bitmap_copy only.
GRID g1 (grid_g1.txt): rlr+vp+vpd+vpc x {0,swcs,swcw} x {0,bdI,bdB} x {0,spl1,spl3,spl1+spl3} - 36 whole-board sweeps.
  ZERO-LOSS combos: {swcw, bdB} and {swcw, bdB, spl1}: +_collision_move_point 4752, lost 0.
  Everything with spl3 loses 4-5 (decals/models/bitmap_copy/draw); swcs never zero-loss (race x2, and move_point not gained);
  bdI leaves units lost; no stack_walk removal leaves draw lost.
Packet PA = rlr vp vpd vpc swcw bdB; PAE = PA + effects.c owner-gated half (NODUP removal + /Od calls); PAES = PAE + spl1:
  all three: gained 1 (+4752), lost 0; data sections identical as multisets in every unit (only COMDAT index shifts);
  CL /Zs /W3 warning census identical to base (177 lines, no new C4013); surplus COMDATs == January selected copies:
  breakable/weather _real_local_random a8ad3cb3 (== January effects), effects _real_local_random_range 7ed2e50e
  (== January bored_camera), collisions/render_cameras _valid_real_plane3d 5a34cbcd (== January matrix_math),
  matrix_math _valid_real_plane3d stays EXACT, effects _real_local_random stays EXACT.
  Parked drift: __rasterizer_model_draw (parked, zero credit) 5168/348 sha 89b0d7ea -> cbfa8585 (header-inline
  allocation hash, same class as its recorded P6-P9 re-baselines) - integrator must re-baseline its parked.json measurement.

## F6 - period 64 is universal (sweep_pos at top, K=64..69): bitmap_copy E at 64,67,68,69 / r at 65,66 (== K 0,3,4,5 / 1,2);
   units E at 64,68,69 / r at 65-67 (== 0,4,5 / 1-3); alpha_bleed E at 64,65 / r at 66-69 (== 0,1 / 2-5).

## F7 - single-half grids (grid_g2rlr.txt, grid_g2vp.txt): rlr alone or vp alone x {0,swcs,swcw} x {0,bdI,bdB}:
   NO zero-loss combination. Best is {swcw,bdB}: loses only draw (net -1 in its TU). The two held real_math.h bodies
   are each +1 in frame_statistics' TU; together +2 == exactly one duplicated one-parameter prototype (-2).
   => B1 (rlr) and collisions (vp) must land TOGETHER.

## F8 - PACKET (ready, lab-measured; real_math.h RESERVED for Codex, so NOT proposed for landing now)
   packet/A_*.patch (CRLF-preserving, apply with patch -p1 --binary; verified on a pristine copy == var/PKT_A):
     A_real_math.h.patch        rlr named local; valid_real_plane3d prototype (l.672) -> __inline after valid_real_normal3d
     A_matrix_math.c.patch      plain valid_real_plane3d definition removed (header inline becomes the provider)
     A_collisions.c.patch       loop-site match_vassert uses valid_real_plane3d(&collision->plane) (/Od one call);
                                `position` stays FUNCTION scope (/Od-attested; reviewer requirement)
     A_cseries_windows.h.patch  duplicate stack_walk_disregard_symbol_names prototype removed (cseries.h copy kept)
     A_bitmaps.h.patch          duplicate bitmap_delete prototype removed (bitmaps_internal.h copy kept)
     AS_real_math.h.patch       same + nonuniform_cubic_spline junk body -> glow.c January body (count-neutral here)
     B1_effects.c.patch         breakable-audit owner-gated effects half (NODUP removal + /Od calls), unchanged
   rerun_packet.py (whole board, 447 units): PA / PAS / PAE: +_collision_move_point 4752 B strict, 0 lost; PAI
   (bitmaps_internal.h copy removed instead) loses units::_unit_preprocess_node_orientations 1920.
   Per-object: collisions 20/20, matrix_math 35/35, breakable 12/12, units 189/189, frame_statistics 10/10,
   bitmap_drawing 7/7, bitmap_utilities 48/48, bored_camera 9/9 (weather 19/20, render_cameras 20/21, effects 38/41 unchanged).
   object_audit: collisions PASS; matrix_math/breakable/effects fail lists IDENTICAL to their base audits (pre-existing
   storage/residual items). Pair links (pairlink.py, both orders): collisions+matrix_math(pkt) PASS,
   render_cameras+matrix_math(pkt) PASS, breakable+effects(pkt) PASS, weather+effects(pkt) PASS,
   effects(pkt)+bored_camera(base) PASS; collisions(pkt)+matrix_math(BASE) FAIL and breakable(pkt)+effects(BASE) FAIL
   => matrix_math.c and effects.c halves are mandatory. W3 census unchanged (no new C4013).
   Integrator obligations: re-baseline parked __rasterizer_model_draw measurement (sha 89b0d7ea -> cbfa8585, 5168/348);
   surplus _valid_real_plane3d in collisions/render_cameras and _real_local_random_range in effects need owner COMDAT
   admission (all byte-identical to January's selected copies).

## F9 - GENUINENESS (the owner question; why this is RESEARCH_ONLY + owner ruling, not a landing)
   - Both copies of each duplicate are CAMPAIGN-authored; no January evidence decides which copy January had:
     stack_walk: cseries.h one-liner added 5a89849d (2026-01-18); cseries_windows.h section copy added 5d3444f7 (2026-09-13).
     bitmap_delete: bitmaps_internal.h (campaign-invented header, c0fcc65f 2026-08-31); bitmaps.h (HCEX-stub public header,
     e9a1b0c7 2026-09-02).
   - The zero-loss packet removes the LATER-added copy in both cases (one uniform rule), but that rule was noticed after
     measurement, and for bitmap_delete it removes the declaration from the HCEX-attested public owner header and keeps
     the invented bitmaps_internal.h copy (ownership argues the opposite: bdI, which loses units 1920 B).
     swcs (ownership-natural for stack_walk) loses race x2 + bitmap_copy + draw and blocks the move_point gain.
   - Measured facts for the owner: PA = +4752 B / 0 lost; PAI (ownership-principled) = +4752 B / -1920 B (units canary).
   - Root cause stands: every canary is a whole-TU record count coincidence (mod 64). Known genuine header deviations in
     frame_statistics' TU (verify typedefs +6 campaign scaffolding, spline junk +11, missing rlr/vp -2, stack_walk dup +2)
     sum to +17 vs January; our count is still == January's mod 64, so ~17 records (mod 64) of January content are
     missing elsewhere (real_math.h assert anchors: January had ~114 more lines between Jan 880 and Jan 1508 than ours
     between 1036 and 1550; /Od (later build) anchors 871/882-883/902-903/1549-1550/1571 match OUR lower half). That is
     the only fully-genuine route and it is not recoverable from current evidence.
