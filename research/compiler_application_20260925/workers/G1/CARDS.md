# G1 cards and outcomes (compiler-application campaign, wave 3: parenthesised-FP / hidden-temp transfer)

Worker G1, solo, patch-only, never commits. Worktree C:\halo-worktrees\claude-compiler-application-20260925, HEAD
09f5208f (= brief base 455dffad + lead batches 2..5). Credit is measured against published canonical a8854940.
Governing texts (read in full 2026-09-26 01:28-01:33 -0700): HALO_HOUSE_RULES_20260926.md (sha256 d367f4ad...),
AGENT_BRIEF_v2.md (910a024a...), AGENT_BRIEF.md (hold list), W6/CARDS.md (13fed45f...), OWNER_PACKET.md, LEDGER.md.
Cards: cards/<id>.txt, clock-read, written before the compile/trace they predict, never edited. Outcomes appended here.

## OWNER RULING FOR G1 (received 2026-09-26 ~01:33 -0700, binding; recorded before ANY G1 compile)
1. Compiler mechanisms (hidden 0x267 temps, region caps, slot-ID sorts) are DIAGNOSTIC ONLY; never permission to
   manufacture hidden temporaries.
2. The /Od evaluation order ALONE does not uniquely authenticate parentheses or a macro.
3. Every grouping change needs BOTH (a) meaningful source (the expression reads as natural, meaningful code) AND
   (b) January corroboration (January's own bytes). A change that merely reproduces a schedule does not qualify.
4. No decorative parentheses, no filler, no invented macros.
State of G1 at receipt: census tools written (tools/parscan.py, tools/od_expr.py), baseline gate runs only
(base/*.gate_all.txt, unmodified sources); NO card written, NO candidate compiled. Every card below is written under
this standard and names, per candidate, its meaningful-source evidence (S) and its January evidence (J) separately
from the /Od reading (D).
Operational reading of (b) that I apply (stated so the owner can correct it): J corroboration of a GROUPING must be a
January byte fact that the grouping itself determines, independent of scheduling - e.g. a constant folded from the
group (one fmul by K = a/b instead of two ops), reciprocal-multiply vs divide, the operation count/type, a shared
subexpression computed once, or operand-rank facts that only the tree can produce. A strict-exact schedule after
the change is NECESSARY for credit but is NOT by itself J corroboration of the grouping (ruling item 3).

## Baseline at HEAD 09f5208f (clock 2026-09-26 01:28-01:31 -0700; base/*.gate_all.txt; unmodified sources)
physics 14/3 (compute_ground_plane, compute_new, update_old); decals 32/1 (new_from_collision); actor_moving 32/4
(test_avoidance_vector, move_update, vector_avoidance, path_refresh); weather_particle_systems 19/1; hud_weapon 14/2
(crosshairs_draw, render_weapon_hud); flags 15/1; geometry 27/3; render_cameras 20/1; player_effects 26/3;
collisions 19/1.

TIME NOTE (2026-09-26 02:08 -0700): the outcome sections below were appended in ONE batch at 02:06:58 (clock-read).
The compile/trace times in their headings are the file mtimes of the objects/manifests (date -r), each later than
its card's clock-read time. An earlier draft of these headings carried estimated times; corrected here.

## Census tools (read-only; research/.../G1/tools)
parscan.py (grouping-paren classes in OUR source), od_expr.py (symbolic FP expression trees of the first-party /Od
build, SSE+x87), odours.py (our VC7 /Od trees vs first-party /Od trees, shape-aligned), lst.py (/FAs listing of one
function, identity-checked against the plain compile), alnsum.py / moves.py / xmoves.py (normalised alndiff summaries,
pure-reorder and int<->x87 transposition census), strip_sweep.py (diagnostic decorative-paren strip test),
regions.py (reader for W6 w6trace 'region' runs), odtu.py / odfind.py (locating /Od bodies; odfind's constant
fingerprint proved too weak - not used for any claim).

## C01 outcome (compiles 01:50:01; appended 02:06:58 -0700; objects scratch/campaign/workers/G1/pe_C01{A,B}.obj)
- C01A (decorative '(1.0f - w)' stripped) and C01B (sibling spelling P*w + (1.0f - w)): BOTH keyed-identical to HEAD
  (0 changed / 0 added / 0 removed; gate 26/3 unchanged). A's EXACT prediction (p 0.55) FAILED; B's 'same bytes'
  prediction held. The decorative group is inert at this site.

## C02 outcome (compiles 01:52:44; appended 02:06:58 -0700; objects pe_C02a/b/c_diag.obj)
- C02a (named 'periodic_value' local after scale, per the /Od user-slot order) and C02b (its strip) are keyed-identical
  to HEAD: VC7 forwards the single-use local. C02a's EXACT prediction (p 0.7) FAILED.
- C02c (DIAGNOSTIC, never a candidate: the same local at camera_effect_matrix's site) changes only that function
  (1280 -> 1296 padded, 375 -> 390 insns = J's count; J 1312) - recorded as an observation.

## T01 outcome (trace 01:53:56; appended 02:06:58 -0700; run runs/T01_pe_HEAD_sched, stock_equal true)
- After the periodic call (cycle 131) the ready list holds fmul [esi+0x60] (+0x2c 0xe800), add esp,8 (0x1400) and
  fld 1.0 (0xcc00); the FP slot takes the fmul. fsub 0xd400, faddp 0x3800, fmul scale 0x9c00, fstp 0x0c01.
  No 0x267 record in the block (prediction CONFIRMED). January needs the (1-w) chain to outrank the product at that
  cycle - a key fact, not a grouping fact.
- VERDICT _player_effect_add_continuous_effect (314 B): NEGATIVE after 5 source shapes + 1 trace (stop rule).
  Reopen: a January-attested source fact that raises the (1 - w) chain's +0x2c above the product's (e.g. a second
  consumer of 1 - w or of the periodic value, or a different successor structure of the magnitude store); the J/our
  per-site contrast with get_camera_effect_matrix is recorded above.

## S01 outcome (sweep 01:58:5x-01:59:11; appended 02:06:58 -0700; base/S01_strip_sweep.txt, objects scratch/campaign/workers/G1/strip/)
- 25 decorative arithmetic groups in 13 eligible residual functions. Inert: 16. CHANGED: add_continuous none;
  camera_effect_matrix L837 (worse, 37 -> 40 blocks); nav_point L956 and L960 (each ALONE -> STRICT EXACT);
  ballistic L2624 (changed, same block count; Lane A's landed zero-credit parenthesisation IS load-bearing);
  sun_glow L535 (worse, 112 -> 147); widget_instance_render_recursive L5285 (changed, 29 = 29);
  biped_update_physics L2730 (changed, 330 -> 331); vector_avoidance L1666 (changed, 222 = 222).
- INVALID rows (tool bug, disclosed): decal_new_from_collision L2121 '(u & 0x8000)' were misclassified as LEAF because
  parscan.is_binop_prev does not know '&'; removing those parens changes the parse. Those two rows are void.
- Predictions: '2-5 of 25 change bytes' FAILED (8 valid rows changed); '0 become EXACT (p 0.9)' FAILED (two single
  strips in nav_point); 'ballistic pair load-bearing (p 0.7)': half - L2624 changed, L2625 inert.

## C03 outcome (compiles 02:01:29; appended 02:06:58 -0700; objects nav_N3.obj, nav_N4.obj)
- N3 (both bitmap_extent decorations removed) and N4 (all five decorative groups removed) are NOT exact and identical
  to each other: ours 'push 0; push 0; mov eax,[ebp-0x10]; mov ecx,[ebp-0x14]; push 0' vs J 'push 0; mov eax; mov ecx;
  push 0; push 0'. Prediction 'N3 two-push form (p 0.6)' CONFIRMED.
- CONCLUSION: the single-strip EXACT is a one-record COUNT COINCIDENCE. Stripping one of two identically written
  statements would be a count choice made for bytes (rules 'do not tune counts', owner ruling items 1/3):
  NOT A CANDIDATE. Owner FYI only.

## T02 / T03 outcomes (traces 02:02:26 / 02:03:49 / 02:04:05; appended 02:06:58 -0700; runs T02_{HEAD,N1,N3}_region, T03_{HEAD,N3b}_sched, all
## stock_equal true; T03_N3_sched was an instrument failure (empty trace, exit 1) and was re-run as T03_N3b_sched)
- The +0x58A window is the FIRST region (81 records) of the basic block that starts at +0x467 ('distance *= 3.048').
  HEAD: region 29 = 81 records ending in a hidden op-0x29d record (stmt 0xc5, the fmod/fast_ftol statement); push0#1
  opens region 30 and the two loads are hoisted to that region's head. N1: push0#1 is record 81 of region 29 = J.
  N3: push0#1 and push0#2 are records 80/81.  Prediction (a) CONFIRMED.
- Hidden records in HEAD's region 29: six op-0x267 temps - stmt 0xb4 (x extent) x2, 0xb6 (offset.x) x1, 0xb8
  (y extent) x2, 0xba (offset.y) x1 - plus the op-0x29d record. N3: four 0x267 (one per statement) + 0x29d.
  So every parenthesised FP group we write - NEEDED '(x1 - x0)', the decorative outer pair, and the cast operand of
  '(short)(long)(...)' - is one hidden record, and January's first 80 records of this block hold exactly ONE hidden
  record fewer than HEAD / one more than N3.
- Consequence: no x/y-symmetric spelling can reproduce January (symmetric edits move the count by 2). The /Od (2020)
  statement structure (extent inline in the offset expression: (short)((real)point.x + 0.33f * arrow_scale *
  ((real)width * (x1 - x0) * 0.5f))) keeps 3 groups per axis (= HEAD's 6) - predicted HEAD bytes, not compiled.
- VERDICT _custom_render_nav_point (1618 B, parked): NEGATIVE for admissible source; mechanism fully located.
  Reopen criterion (sharper than the park's): first-party evidence of an ODD difference in parenthesised FP groups
  (or other hidden records) among statements 0xa4..0xc5 (distance*=3.048 .. the fmod/fast_ftol statement) - e.g. an
  authentic x/y-asymmetric spelling - or evidence that January's block starting at +0x467 begins one record later.

## FP arithmetic-count census (appended 2026-09-26 02:16:30 -0700; tools/fpops.py at 02:11:32, read-only on build/base)
- base/fpops_head.txt: per residual row, x87 arithmetic counts by class (add/sub/mul/div, fchs/fabs/fsqrt, literal
  loads) January vs ours. 38 of 50 rows are ARITHMETIC-EQUAL: their residue is order/allocation/scheduling only, so a
  parenthesis/grouping repair can at most act through hidden records (int slots, region caps) or operand sort keys.
- Arithmetic DIFF rows (J-visible grouping/tree facts exist): physics_compute_new, physics_update_old (both HEAD;
  equal in the LD2 state, below), actor_move_test_avoidance_vector (inline vs transform call, W3 G1), 
  render_camera_build_frustum (fchs J27/O29, fmul J122/O121, one J 0.0f load), observer_update_positions (parked,
  credibility blocker), __rasterizer_model_draw (parked), __rasterizer_environment_fog_screen_begin (held D/E),
  biped_update_turning/moving/physics (bipeds holds), update_alien_scout_physics (protected).

## C04 outcome (compile 02:13:31; appended 02:16:30 -0700; objects rc_C04a.obj, rc_C04b.obj)
- C04a and C04b identical: fchs J27/O27 (prediction CONFIRMED), function still residual (CONFIRMED), blocks 169 -> 172
  and 1092 -> 1091 insns vs J 1080 ('blocks drop' FAILED). 20 other rows exact; keyed diff 1 changed / 0 / 0.
  January's arithmetic for the four far values is reproduced by the two named extents; the schedule is not.
## C05 outcome (compiles 02:14:50/52; appended 02:16:30 -0700; objects rc_C05a/b/c.obj)
- C05a and C05b (flat chains for n.i / n.j, /Od order and our order) are keyed-IDENTICAL to HEAD (0 changed): VC7
  still forms and CSEs inverse_plane_z * projection_scale. Prediction 'fmul = J122 (p 0.75)' FAILED; 'C05a = C05b'
  held trivially. C05c = C04a (fchs fixed, fmul still J122/O121). January's per-component recompute stays unexplained.
- VERDICT _render_camera_build_frustum (3370 B, parked): improved-not-exact at the ARITHMETIC level only (C04a: two
  J-evidenced named extents; zero credit; schedule/blocks slightly worse). 5 shapes: STOP. Reopen: a first-party
  source form that makes VC7 recompute inverse_plane_z * projection_scale per component (J +0x874/+0x886) - e.g. an
  intervening aliasing store or a different operand order per statement with D/J support - together with the
  0x364 statement-order block (J 1 insn vs ours 69 at that alignment point) that dominates the residual.

## Unregistered RE-MEASUREMENTS (no card, no prediction, no claim beyond the numbers; disclosed)
- W3's weather candidates on HEAD (02:0x): W1 (r0 body) 530/530 insns, 6 normalised x87 blocks; W2 (W1 + /Od
  plane3d_distance_to_point helper) 7 blocks: two dot-product operand/term-order sites (+0x2a7..+0x2be,
  +0x363..+0x373) = the M7/M8 sort class, not a grouping class.
- Decals-physics lane LD2 transplanted onto HEAD's physics.c (keeping W6's landed V2a vehicle_collision), objects
  ph_LD2u.obj (update_old only) and ph_LD2uc.obj (update_old + compute_new), 02:15:46/47: 14 exact kept, keyed diff
  only the transplanted rows. update_old 5168 = J size, 139 normalised blocks, ARITHMETIC-EQUAL. compute_new: 2944 =
  J size, relocs = J, 911 vs 910 insns, 23 normalised blocks, ARITHMETIC-EQUAL; residue = operand-order sites inside
  inlined add_vectors3d / cross products (M7/M8 class) + two register choices + one fmulp/fxch pair at +0x928.

## S01 correction (appended 2026-09-26 02:19:13 -0700, clock-read)
- strip_sweep.py fixed (binop_at: binary '&' / '*' detection). Re-count: 22 VALID decorative arithmetic groups in 11
  functions. VOID rows (misclassified '&' groups whose removal changes the parse): decal_new_from_collision L2121 x2
  AND bsp3d_test_pill_recursive L2059 (its 'inert' row is void too). Valid: 8 load-bearing (camera_matrix L837,
  nav L956, nav L960, ballistic L2624, sun_glow L535, widget_render L5285, biped_update_physics L2730,
  vector_avoidance L1666), 14 inert.

## CENSUS TABLE (G1, 2026-09-26; ranking by evidence strength for the parenthesis/hidden-temp lever, then size)
Legend: FP-arith = tools/fpops.py (J vs ours arithmetic counts at HEAD); decor = decorative arithmetic groups in OUR
function (S01 strip result); /Od = odours.py/od_expr.py tree comparison; all rows eligible (not owner-held, not a
wave-3-owned unit) unless marked.
| # | function | unit | B | FP-arith | decor (strip) | /Od grouping vs ours | residue class | verdict |
|---|---|---|---:|---|---|---|---|---|
| 1 | _custom_render_nav_point (parked) | hud_nav_points | 1618 | equal | 5: L956, L960 each alone EXACT; both -> 2-push form; 3 inert | later revision (extent inline in offset expr) | ONE hidden record in the 81-record region at +0x467 (T02/T03) | NEGATIVE: count coincidence, not a candidate; owner FYI |
| 2 | _player_effect_add_continuous_effect | player_effects | 314 | equal | 1 inert | later formula (distributed, local) | +0x2c key tie after the call (T01) | NEGATIVE (5 shapes + trace) |
| 3 | _render_camera_build_frustum (parked) | render_cameras | 3370 | DIFF fchs/fmul/0.0 | 0 | later formula; locals for negated extents | statement order (0x364), allocation | improved at arithmetic level only (C04a fchs = J), zero credit; STOP |
| 4 | _physics_compute_new (parked) | physics | 2930 | HEAD diff / LD2 equal | 0 | - | LD2-on-HEAD 23 blocks: M7/M8 operand order in inlined helpers | not a paren target; closest physics row (M7 work, not G1) |
| 5 | _ai_test_ballistic_line_of_fire (parked) | ai | 930 | equal | 2: L2624 load-bearing (Lane A landed decoration), L2625 inert | - | dead-param home + z-row reload | low; owner FYI (item 4) |
| 6 | _player_effect_get_camera_effect_matrix (parked) | player_effects | 1303 | equal | 1 load-bearing (strip worse) | later formula | x87 storage/order | low |
| 7 | _rasterizer_sun_glow_draw (parked) | rasterizer_xbox_lights | 2338 | equal | 5: L535 load-bearing (strip 112 -> 147 blocks) | - | frame + x87 order | low; FYI |
| 8 | _widget_instance_render_recursive | ui_widget | 743 | equal | 1 load-bearing (same blocks) | - | 28 blocks | low |
| 9 | _actor_move_vector_avoidance | actor_moving | 4130 | equal | 1 load-bearing (cast-constant parens) | helper scale_vector3d, K*emergency | 178 blocks | low |
| 10 | _physics_update_old | physics | 5163 | HEAD diff / LD2 equal | 0 | (g*mgd)/gd vs (g/gd)*mgd (recorded INERT, decals-physics G1) | LD2-on-HEAD 139 blocks | low |
| 11 | _rasterizer_environment_specular_spot_light_begin (parked) | rasterizer_xbox_environment | 949 | equal | 0 | '(radius * 0.5f)' J-CORROBORATED by CSE (consistent) | allocation | none |
| 12 | _weather_particle_system_render | weather_particle_systems | 1665 | equal | 0 | helper plane3d_distance_to_point | W3 W2: 2 M7 operand-order sites | none (M7) |
| 13 | _crosshairs_draw | hud_weapon | 2247 | equal | 3 inert | later formula | BODY-FIRST, 152 blocks | none |
| 14 | _flag_update (parked) | flags | 1170 | equal | 0 | (1.0f/delta) consistent with J CSE | stack homes | none |
| 15 | _bsp3d_test_sphere_recursive (parked) | collision_bsp | 792 | equal | 0 | - | pure reorder: dot-term order in inline helper (M7) | none |
| 16 | _player_effect_update_camera_impulse (parked) | player_effects | 738 | equal | 0 | - | dot/cross operand order (M7) | none |
| 17 | _collision_move_point | collisions | 4744 | equal | 0 | - | pure reorder (W3: held valid_real_plane3d route) | none |
| 18 | _actor_move_test_avoidance_vector | actor_moving | 743 | DIFF (call vs inline) | 0 | /Od point_from_line3d | W3 G1: spill materialisation | none |
| 19 | _decal_new_from_collision | decals | 6162 | equal | 0 | later revision | frame/slot order | none |
| 20 | render_weapon_hud, geometry x3, sphere_intersects_cluster_portal, area_of_effect, draw_bitmap_in_rect, motion_sensor x2, action_charge_perform, aiming_vector_test_blockage, draw_layer_int, player_teleport_internal, ai_communication_event, actor_look_update, actor_path_refresh, compute_ground_plane | various | - | equal | 0-1, inert | - | order/allocation/cross-jump | none |
Excluded or held (not worked): _actor_move_update, _actor_action_handle_vehicle_entry (Q13); bipeds rows (holds;
fpops shows arithmetic DIFF in update_turning/moving/physics); _update_alien_scout_physics (protected);
_observer_update_positions (parked credibility blocker); __rasterizer_model_draw; __rasterizer_environment_fog_screen_begin
(held D/E); owned/released units per the brief.

## N1 re-gate (appended 2026-09-26 02:20:59 -0700)
- nav_N1 (only L956's decorative pair removed; hud_nav_points_N1_DIAGNOSTIC_NOT_A_CANDIDATE.patch, sha256 ff5c9031...):
  gate 32/32 exact (base/nav_N1.gate_all.txt), keyed diff 1 changed (EXACT vs January) / 0 added / 0 removed
  (base/nav_N1.keyed_diff.txt). s04 (only L960's pair) is ALSO strictly exact: two different texts, identical bytes -
  the bytes cannot identify which (if either) line January decorated. Kept as mechanism evidence only.

## SUMMARY OF VERDICTS (G1, 2026-09-26 02:20:59 -0700)
| unit | function | meaningful B | verdict | artefact | cards |
|---|---|---:|---|---|---|
| hud_nav_points | _custom_render_nav_point (parked) | 1618 | NEGATIVE for admissible source. Mechanism located: ONE hidden 0x267 record too many in the first 81-record region of the block at +0x467. Single-line decoration strips (either line) are strictly exact; the principled symmetric strip is not. Not a candidate (count choice, ruling items 1/3). Owner FYI. | diagnostic patch (not a candidate) | S01, C03, T02, T03 |
| player_effects | _player_effect_add_continuous_effect | 314 | NEGATIVE (5 shapes + trace; key tie) | - | C01, C02, T01 |
| render_cameras | _render_camera_build_frustum (parked) | 3370 | improved at the ARITHMETIC level only (J-evidenced named far extents; fchs = J), schedule not; zero credit; STOP | render_cameras_C04a_RESEARCH_zero_credit.patch (sha256 3429ada9...) | C04, C05 |
| (census) | 50 x87/scheduler-class rows | - | 38 arithmetic-equal; 22 valid decorative groups, 8 load-bearing; no admissible grouping repair found | tools/*, base/* | S01, census table |

Failed predictions (kept): C01A EXACT (p .55); C02a EXACT (p .7); S01 '2-5 of 25 change bytes' (8 of 22 valid) and
'0 become EXACT' (two single strips did); C04 'blocks drop'; C05a 'fmul = J' (p .75).
Confirmed: C01B inert; C02b = C02a; T01 no 0x267 in block + key explanation; C03 N3 two-push form; T02 region shift (a);
C04 fchs = J; function stays residual (C04/C05).
Instrument notes: T03_N3_sched empty trace (exit 1) re-run as T03_N3b_sched; S01 '&' misclassification (3 void rows)
fixed in strip_sweep.py; odfind.py's constant fingerprint too weak (unused for claims).
Unregistered re-measurements disclosed above (weather W1/W2, physics LD2 transplant) - numbers only.

## Re-verification at the lead's newer HEAD fe283cc5 (appended 2026-09-26 02:24:02 -0700, clock-read)
- Commits 09f5208f..fe283cc5 (Q15 i/ii breakable_surfaces, Q16 render_debug, owner-header prototypes in damage.h,
  recorded_animations.h, structures.h) do not touch player_effects.c, hud_nav_points.c, render_cameras.c, physics.c.
  Gate rows unchanged: player_effects 26/3, hud_nav_points 31/1, render_cameras 20/1
  (base/*.gate_all.fe283cc5.txt); nav_N1 still 32/32. No verdict changes.

## OWNER QUESTIONS / FYI (G1)
OQ-G1-1 (FYI; G1 recommends HOLD): _custom_render_nav_point (1618 B) is strictly exact, and hud_nav_points reaches 32/32
  functions, if the decorative outer parentheses are removed from ONE of the two identical bitmap_extent lines (either
  one; both variants byte-identical). Removing both (the principled cleanup under ruling item 4) is NOT exact.
  Disclosures: parse tree unchanged; no new symbols (keyed diff 1 changed / 0 added / 0 removed); mechanism = one hidden
  0x267 record in the first 81-record region of the block at +0x467 (T02/T03, stock-equal traces); J corroborates the
  tree, not the decoration; the choice of line would be made for bytes (rule 'do not tune counts'); object admission
  is blocked independently (fifty-objects review4: consumer-local prototypes; _object_get_bounding_sphere copy differs
  from January's selected copy, LNK2005 both orders).
OQ-G1-2 (ruling item 4 hygiene): load-bearing DECORATIVE parentheses exist in canonical source of zero-credit residual
  rows: ai.c L2624 (landed by Lane A 2026-09-20), rasterizer_xbox_lights.c L535, player_effects.c L837, ui_widget.c
  L5285, bipeds.c L2730, actor_moving.c L1666. Strip them (some move bytes away from January)? Separately: commission a
  strip sweep of EXACT rows (tools/strip_sweep.py; the scanner counts 878 decorative groups board-wide, most integer or
  leaf groups) to find exact functions whose exactness depends on decoration?
OQ-G1-3 (minor): keep render_cameras C04a (J-evidenced named far extents; arithmetic = January, schedule slightly
  worse) as research provenance only (G1 recommendation), or re-baseline the park with it?
OQ-G1-4 (interpretation check): G1 applied ruling item 3's 'January corroboration' as a January byte fact determined
  by the grouping itself (folded constant, CSE, operation counts), never the post-change schedule. Confirm or correct.
