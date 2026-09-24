# source/effects/player_effects

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\cm_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\optional_add_continuous_periodic_local.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\ac1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\cm2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\cm2b.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\cm2e.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\shadow\\cseries\\cseries.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\sweep_pin.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\player_effects\\LEDGER.md"
]

## production_changes
One optional zero-credit patch: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_effects\production.patch. `git apply --check` is clean against 16542e46. The full candidate file is scratch/w/player_effects/cm_final.c.

The patch changes only the body of player_effect_get_camera_effect_matrix. Every change is attested by the /Od build at 0x57ba70:
(a) The single `effect_matrix` plus the invented `source` pointer become the RTC-named `real_matrix4x3 impulse_matrix` (declared inside the impulse if-block) and `real_matrix4x3 shake_matrix` (declared inside the shake if-block).
(b) The impulse branch now does `*matrix = impulse_matrix;` and the else-branch does `*matrix = *global_identity4x3;`.
(c) The invented `real duration` locals are dropped. The effect_scale_value calls read effect->camera_impulse.temporary_duration and effect->camera_shake.duration directly, as /Od does.
(d) Declarations follow the /Od order: impulse_matrix, scale, axis; then shake_matrix, scalars, and continuous last (still initialised, per house rule 17).

What it does not change: the translation keeps production's single-pass arithmetic and the scripted branch keeps its direct stores, so no new header COMDAT is emitted.

Measured effect:
- Whole-TU gate: exact 26 / residual 3, unchanged. Only this row's section bytes change; the symbol set is identical.
- provider_link: nothing new to link. fake_match_scan: 0 leads.
- Row moves from 1280 [size] to 1296 [size], 49 relocs, normalized sha d4c8a437224a8ecdf17ee890798e95e93e1a546a772b17ca024e02e09a558110.
- alndiff difference lines drop from 97 to 65 (375 to 385 of 390 instructions).

Park impact: the unclassified park for _player_effect_get_camera_effect_matrix must be re-baselined to size 1296 / 49 / the sha above. Land only if re-measured objdiff is above 91.115486.

Optional and byte-inert: scratch/w/player_effects/optional_add_continuous_periodic_local.patch (`git apply --check` OK). It adds the /Od-attested named `real periodic = periodic_function_evaluate(...)` local between scale and magnitude, and the section sha is identical to production. It matters only as the prerequisite for the PIN route described below.

No header, config or symbols.json change is proposed.

## evidence
Full log: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_effects\LEDGER.md. Baseline gate: exact 26 / residual 3. _player_effect_start was already exact via 417fc10e.

(1) _player_effect_add_continuous_effect
- /Od readout (0x57b490): the frame has a named local at [ebp-0x14] between scale [ebp-0x10] and magnitude [ebp-0x18] that receives the raw periodic_function_evaluate result. The PIN and MAX compiler temps sit after the named locals, so this is a named local, not a temp.
- In a lab TU the named local reproduces January's `fld 1.0; fsub w; fxch st(1); fmul w; faddp` order. In the real TU it does not.
- Bisecting cseries.h line by line pins the flip to line 133, `PIN ... CEILING((n),(ceiling))`. Marathon's first-party cseries.h spells it `CEILING(n,ceiling)`.
- With a shadow cseries.h (sgate.py), ac1 (named local) plus the Marathon PIN makes the function strict EXACT, with the TU at 27/2. Either change alone stays residual.
- Board sweep (sweep.py, all 84 units that expand PIN or interpolate_scalar, SHADOW-SEEN on every compile): exact 2576 -> 2571, 0 gains. Losses: _path_state_traverse, _rasterizer_transparent_geometry_group_draw (15120), _unit_preprocess_node_orientations, _update_human_boat_physics, _update_human_plane_physics.
- Other PIN spellings also lose functions. s1 `CEILING(n,(ceiling))` makes ac1 exact but loses 3; s2 `CEILING((n),ceiling)` leaves ac1 residual and loses 2.
- Conclusion: the macro's parenthesisation acts as a hidden per-function ordering key, and the production spelling is the board's local optimum.
- Inert, measured (production spelling unless noted):
  - ac2 (declare periodic/magnitude, assign later)
  - ac4 (scale/periodic/magnitude all declare-then-assign)
  - ac5 (drop the parens on 1-w)
  - ac6 (scale* first)
  - ac7 (p*w first)
  - ac8 ((real)game_time_get())
  - ac9 (staged +=; VC7 re-forwards it into one tree)
  - dead-local sweeps D=0..16
  - prototype sweeps N=1..32

(2) _player_effect_get_camera_effect_matrix
- /Od readout (0x57ba70; RTC impulse_matrix/axis/shake_matrix): scale_vector3d + point_from_line3d for the impulse translation, set_real_point3d for the scripted translation, and two matrices.
- cm2 (fully /Od-shaped): 1312/49, and cluster B (x product forwarded on x87) is SOLVED by scoping each matrix to its own block. The remaining difference is the inlined cross_product3d leaf loads: 22 diff lines here, and 8 (2 transposed pairs) in the hand-expanded twin cm2b.
- cm2's new _point_from_line3d and _set_real_point3d COMDATs are IDENTICAL to January's selected copies (ai/action_charge, ai/ai_debug) and pair-link in both orders.
- cm2e (without set_real_point3d): the cross product becomes exact, but the scripted-branch store interleave is wrong. This is the n3a exclusive-or, now without cluster B.
- Inert on cm2/cm2b:
  - M8 initialised-dead-local sweeps at the top and at 4 scopes
  - camera_impulse/camera_shake pointer locals (-> 1296, the A46 RMW flip)
  - no rotation/translation locals
  - no camera_shake_ticks
  - the ?: scale initialiser
  - /Od declaration order
  - Marathon PIN

(3) _player_effect_update_camera_impulse
- /Od readout (0x57dea0, RTC names normalized_direction/facing/normalized_facing/facing/perp/permanent_impulse): the calls and their argument order already equal ours.
- The residual is the 2 jitter cross_product3d leaf loads plus January's i,k,j term order in BOTH permanent dot products, against our k,j,i.
- A lab TU shows the order is intrinsic to the permanent block. Removing the whole temporary block leaves k,j,i. Inert: product-chain order, argument swap, triple_product3d, decl order, preceding functions, dead locals.
- ikjscan.py board census: January emits ikj 19 times; the byte-identical examples involve freshly written or nested-inline operands, and no such construct appears in the /Od permanent block.
- /Od-attested variants u1 (game_time local), u2 (decl order) and u12 are byte-identical; the M8 sweep D=0..8 is insensitive.

## blockers
The object cannot complete this wave.
- _player_effect_add_continuous_effect: EXACT is reachable only with a shared cseries.h PIN respelling, which regresses 5 strict-exact functions board-wide (not landable).
- _player_effect_get_camera_effect_matrix: 2 to 5 x87 leaf-order flips remain in the inlined cross_product3d(global_up3d, &effect->direction, &axis). They are coupled to the /Od-attested set_real_point3d scripted translation. The genuine /Od form (cm2) also emits new _point_from_line3d and _set_real_point3d COMDATs from a non-exact caller, which is owner-gated (P1 class) until the row is strictly exact.
- _player_effect_update_camera_impulse: January's i,k,j order in both permanent dot products and 2 jitter cross-product leaf loads, with no source lever found.

## reopen_criteria
- add_continuous_effect: a TU-local genuine construct that reproduces the ordering key flipped by the Marathon PIN spelling (ac1 + Marathon PIN is strict EXACT), or a PIN spelling proven on all 84 PIN-expanding units with 0 losses (production, Marathon, s1 and s2 all lose at least 2).
- camera_effect_matrix: a source cause for the cross_product3d leaf order under set_real_point3d, starting from cm2.c (22 diff lines, /Od-faithful) or cm2b.c (8). Once strictly exact, cm2's COMDATs are already measured identical and link-clean, so the owner-gated part disappears.
- update_camera_impulse: evidence for January's i,k,j dot-term order (both permanent dots) and the 2 jitter cross-product leaf loads. Not local count, argument order, product-chain order or preceding-block context (all measured inert).

## task notes
- Tree was clean at the start and at the end. No tracked file was edited, and no ninja, configure or git-mutating command was run.
- Tools written in the slug:
  - sgate.py: a copy of gate.py whose temp .c sits next to --source, so a shadow header dir wins the quoted-include lookup.
  - sweep.py: before/after board sweep.
  - lab.py, m8cm.py, m8pos.py, cmcombo.py, ucivar.py, permorder.py, ikjscan.py, dotctx.py.
- New cross-unit observations for the lane:
  - (a) cseries.h's PIN parenthesisation acts as a hidden x87 ordering key. Marathon's first-party `CEILING(n,ceiling)` fixes add_continuous_effect but costs 5 exact functions board-wide, including _unit_preprocess_node_orientations and a 15,120-byte function. Do not attempt a PIN header packet.
  - (b) The /Od RTC two-matrix block scoping fixes the camera-matrix x-product forwarding (cluster B). The escape scope of the address-taken matrix is the lever.
  - (c) For the M8 instrument, uninitialised dead `real` locals are completely inert. Only initialised `long x = 0;` locals exercise the mod-8 key, so earlier "insensitive" readings are only valid if they used initialised locals.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\player_effects\LEDGER.md
