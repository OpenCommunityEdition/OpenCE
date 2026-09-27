# A4 cards (outcomes appended after each card)

## D1 dynavobgeom dead global flag (card cards/D1_dynavobgeom_dead_flag.txt)  outcome 2026-09-26 00:43:45 -0700
- PREDICTION HELD. gate 17/17 EXACT; keyed_diff: only the .bss key changes (?warned 1 byte EXACT vs January;
  _reported_too_many_transparent_geometry_groups removed); object_audit FAIL(2) -> PASS (31/31 symbols);
  pdb_storage 1 -> 0; surplus_identity 4/4 IDENTICAL; provider_link 19/19 PASS both orders.
  Objects: scratch/campaign/workers/A4/dynavobgeom_D1.obj. Logs: logs/*dynavobgeom_D1*.

## L1 whole-program link probe at HEAD with D1 (card cards/L1_link_probe_head.txt)  outcome 2026-09-26 00:52:43 -0700
- PREDICTIONS HELD (1)-(4). tools/link_probe.py, 621 objects, dynavobgeom replaced by D1, lib dir
  work/data-encoding-final/xbox/lib (Aug-2001 XDK 3911-era). Inputs unchanged: True.
  full: LNK1120, 397 unresolved, 26 duplicate defs = 24 CRT (reconstructed libcmt vs real LIBC) + _plane2d_from_points
  + _real_local_random. diagnostic (/NODEFAULTLIB): 472 unresolved (451 unique names), 2 duplicates (the same two).
  ZERO LNK2005 names any dynavobgeom surplus symbol (3 SDK tables, dot_product3d, vector_from_points3d,
  real_alpha_to_pixel32, real_argb_color_to_pixel32, __real@*, literals).
  Unresolved (unique 451): 143 pooled COMMON records absent from our tree (incl. _global_window_parameters,
  _global_d3d_caps, _pixel_shader, _rasterizer_lights, _dsound_globals), 85 xapilib, 82 libcmt, 46 dsound, 29 xnet,
  9 d3d8, 4 d3dx8, 6 bink, 2 dsstrmh, 2 xkbd, 41 other vendor/import, 2 held Halo (_fast_ftol_C, _main_crash).
  => a COMPLETE ordinary link is structurally unavailable at HEAD, for reasons outside both A4 units.
  Files: scratch/campaign/workers/A4/link_probe_D1/{summary.json,full.log,diagnostic.log,unresolved_classified.json}

## R-A rasterizer_xbox storage/ownership packet (card cards/RA_rasterizer_xbox_storage.txt)  outcome 2026-09-26 01:08:46 -0700
- PREDICTION HELD. root_RA (git archive HEAD + mk_RA.py), split_RA (csplit radius: rasterizer_xbox.obj only).
  revgate vs OLD split 92/2/1 (expected: renamed/static-local names); vs split_RA 95/95 EXACT.
  object_audit PASS (194/194 January symbols; blend .rdata 108 B align 4). pdb_storage 25 -> 1 (_rasterizer_xbox_d3d_globals).
  Surplus 15 external defs, all SELECT_ANY + IDENTICAL; SetFlickerFilter/SetSoftDisplayFilter copies gone.
  provider_link 15/15 PASS both orders. /W3 17 = 17 (no new). fake_match_scan 0.
  Header radius (7 other includers, objeq with $L-label normalisation): 6 IDENTICAL; rasterizer_xbox_models:
  PARKED fuzzy __rasterizer_model_draw 5168/348 sha 89b0d7ea -> cbfa8585 (same drift measured at cdc8ebd3; 12/2 counts
  unchanged; zero exact loss) => needs the lead-owned park re-baseline.

## W1 window owner declaration in rasterizer.h (card cards/W1_window_owner_decl.txt)  outcome 2026-09-26 01:18:30 -0700
- PREDICTION FAILED (exact loss). Sweep of all 64 rasterizer.h includers (root_base vs root_W, objeq --norm-labels,
  control build/base == root_base for all 64): 62 IDENTICAL, including all 23 consumers (views, local renames,
  word->short, 5 new includes all byte-inert). Two changed:
  * source/rasterizer/rasterizer_frame_statistics: _rasterizer_frame_statistics_draw EXACT -> residual (4176 B, same
    size) - this TU does not use the global; the only change it sees is the +1 declared name in rasterizer.h
    (declaration-count tie). EXACT LOSS => W1 as placed is NOT admissible.
  * source/rasterizer/xbox/rasterizer_xbox_models: PARKED fuzzy __rasterizer_model_draw bytes changed (5168/5168).
  Sweep table: scratch/campaign/workers/A4/sweep_W/sweep.tsv
- W1 position control: the extern placed right after the struct definition in rasterizer.h also flips
  _rasterizer_frame_statistics_draw (position-independent). fndiff base vs W: 1178/1178 insns, 11 differ = four
  commutative 'a + b' operand-order swaps (mov ecx,[0xac]/mov edx,[0x9c] -> swapped; lea esi,[ecx+edx] ->
  [edx+ecx]) - the C1 declared-name-count tie family (memory: vc7-commutative-operand-sort, c1-number-mod64-law).

## W2 window owner declaration in rasterizer_xbox_internal.h (same card family)  outcome 2026-09-26 01:21:45 -0700
- FAILED (worse). Declaration in a new globals block at the end of rasterizer_xbox_internal.h, include added to the 16
  consumers lacking it, same consumer reconciliation as W1. Sweep 64 TUs: 61 IDENTICAL; EXACT LOSSES
  rasterizer_xbox_draw_primitives _rasterizer_dynamic_geometry_initialize (480 B) and rasterizer_xbox_screen_effect
  __rasterizer_screen_effect (3888 B); models' parked __rasterizer_model_draw drifts. Sweep: sweep_W2/sweep.tsv.

## W0 genuine-typed consumer-local externs (no owner declaration)  outcome 2026-09-26 01:23:21 -0700
- (card W1 family; W0 = the view-reconciliation half only.) All 10 partial views removed; each of those consumers
  declares 'extern struct rasterizer_window_begin_parameters global_window_parameters;' locally; the 5 non-includers
  gain #include "rasterizer/rasterizer.h"; shadows drops its duplicate profile enumerator and its renamed copy of
  rasterizer_frame_begin_parameters (+ extern) now supplied by rasterizer.h; xbox_debug/text/motion_sensor use
  camera.viewport_bounds.{x0,y0,x1,y1} with the two locals renamed to what they hold.
  Sweep 64 TUs (every rasterizer.h includer): 64/64 IDENTICAL (objeq --norm-labels), control 64/64. Zero exact loss,
  zero park drift. sweep_W0/sweep.tsv
- Include-closure census (incclosure.py, root_base): the only headers common to all 23 consumers are cseries.h,
  integer_math.h, real_math.h + CRT headers, all also in rasterizer_frame_statistics.c's closure => no existing header
  can carry a single owner declaration to all consumers without either reaching frame_statistics (+1 name, W1 loss)
  or adding includes to consumers (W2 losses).

## R-B owner declarations in rasterizer_xbox.c (card cards/RB_rasterizer_xbox_owner_decls.txt)  outcome 2026-09-26 01:25:49 -0700
- PARTIALLY HELD. All three items together flip __rasterizer_initialize (2352 B, sha) - a declaration-count tie.
  Bisect (on R-A): debug-options view -> rasterizer_debug_options.h: IDENTICAL 95/95; redundant pixel_shader extern
  removed: IDENTICAL; debug+pixel: IDENTICAL 95/95; bitmaps_internal.h include for global_vector_palette: flips
  __rasterizer_initialize (alone and in every combination) => NOT taken; the local palette extern stays as disclosed
  debt. Final R-B = debug+pixel: object IDENTICAL to rx_RA (259 sections), 95/95 vs split_RA, audit PASS.

## R-C (OWNER-GATED: queue item 2, 9 descriptive .bss names) re-verified at HEAD on R-A+R-B  2026-09-26 01:27:10 -0700
- (No new card: this is the fifty-objects packet's aggregate hunk, rebased; prediction = their measurement.)
  Held: split_RC radius 1 (rasterizer_xbox.obj); 95/95 vs split_RC; object_audit PASS 216/216; pdb_storage 0/216;
  surplus 15/15 SELECT_ANY+IDENTICAL; provider_link PASS; /W3 17=17; fake_match_scan 0; protoscan clean.
  Comment amended per RULING.md so the 9 descriptive names are labelled (rule 15). Strip test (initialisers removed ->
  audit FAIL 23 rows, name-hash .bss order) is the fifty-objects S1 measurement on identical lines; not re-run.

## L2 link probe with both unit candidates (D1 + R-C objects)  2026-09-26 01:33:31 -0700
- Same as L1: full LNK1120 397 unresolved, 26 duplicates (24 CRT + _plane2d_from_points + _real_local_random);
  diagnostic 472 / 2. ZERO LNK2005 on any surplus symbol of either unit (3 SDK tables, plane3d_*, dot_product3d,
  real_*_to_pixel32, vector_from_points3d, literals, __real). Unique unresolved unchanged (static storage + renames
  introduce no new unresolved). scratch/campaign/workers/A4/link_probe_units/.
- review_patch.py: A4_D1_dynavobgeom 0 losses (the ?warned .bss section becomes exact vs January; the stray external
  removed); A4_RA_RB_rasterizer_xbox shows 4 'losses' vs the UNPATCHED split, all symbol-naming consequences
  (initialize's call to the renamed static, the three static-local tables, the renamed static) that require
  LEAD_RA_symbols.json.patch + csplit; its object (compiled with the UNPATCHED header) is IDENTICAL to rx_RB and gates
  95/95 vs split_RA => the header hunk is byte-inert for rasterizer_xbox (its only effect: the models park drift).
