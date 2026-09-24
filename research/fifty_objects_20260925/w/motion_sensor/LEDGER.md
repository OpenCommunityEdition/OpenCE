# motion_sensor worker ledger (wave 4, base 16542e46)

Slug: scratch/w/motion_sensor/. Tree clean at start (git status empty).

## Baseline (gate --all on current source copy base.c)
exact 16 residual 3:
- _motion_sensor_update 1168 [size 1136!=1168, relocs 44!=43, sha]
- _render_motion_sensor 768 [size 784!=768, sha]
- _update_motion_sensor 864 [size 816!=864, relocs 37!=39, sha]
(_blip_begin and _render_blip have landed since the n4 ledger.)

## Prior evidence read
- ledgers: jonas_census_tick_one_shot_20260829, opus5_150k_w1, 250k_w3c, next150_n3a, next150_n4
- parks: 3 entries (unclassified). Recorded negatives are in those ledgers - do not repeat.
- n4 m4.c: _motion_sensor_update strict EXACT, owner-gated (uninit camera_positions read + R14 COMDATs).
- w3c u7.c: _update_motion_sensor 864/39 with one 8-byte reassociation hunk (j,i,k vs i,k,j).
- _render_motion_sensor: scheduler interleave of sensor-index int statement vs x87 weight chain + slot swap.

## _render_motion_sensor analysis (probe 0 = baseline base.obj, 784/40)
alndiff residual: (a) T `sub eax,[ebp-0x18]` vs ours `mov edx,[ebp-0x18]; sub eax,edx` with the x87 weight chain
interleaved into the idiv statement; (b) slot swap: January weight [ebp-8] then radius REUSES [ebp-8] (fstp [ebp-8]
after pow) and the inner trip counter lives at [ebp-0x20]; ours radius [ebp-0x20], trip counter merges into weight's [ebp-8];
(c) three alignment pads (0x11a, 0x19a, stack-check `jmp`+lea) all follow code offsets (not check_stack_buffer-shaped).
NEW EVIDENCE (/Od 0x6426c0, od_render_motion_sensor.txt): the weight variable [ebp-0x34] = (10-i)/10; fade [ebp-0x38] =
w*w; then pow(1-w,3.5)*7+1 is stored BACK INTO [ebp-0x34] and [ebp-0x34] is pushed as render_blip's last (radius) arg.
/Od never merges locals => January's source uses ONE variable for weight and radius. Prior waves (v5..v9, rm1..rm5) always
had two locals -> this is not a repeat.
/Od declaration order (slot order): perspective, player, [aspect - later build], relative_scale, history_index(short),
corner(RTC name, real_point2d), sensor_index(short), sensor ptr, weight/radius, fade, blip_index(short, shared by both
inner loops), blip ptr, blip_pos (RTC, block 1), blip_pos (RTC, block 2). sensor_index = (active + 10 - history_index) % 10.
- probe r1 (H1: ONE variable `scale` for weight and radius, /Od-attested; blip_index declared after fade as in /Od):
  gate 784 [sha]; alndiff: the [ebp-8]/[ebp-0x20] SLOT SWAP IS GONE (radius now reuses [ebp-8], trip counter at
  [ebp-0x20] as January). H1 confirmed. Remaining: `mov edx,[ebp-0x18]; sub eax,edx` + x87 interleave + offset pads.
- probe r2 (r1 + /Od operand order (active + 10 - i) % 10): identical residual to r1 (constant reassociated) - inert.
- ORACLE (name-count, K=0..50 extern dummies before the definition, r1 base): sha 69f4678045fb for EVERY K => the render_motion_sensor residual is NOT a TU name-count tie (oracle only, nothing to land).
- LAB div.c: VC7 /O2 compiles (real)(10-i)/10.0f and /10 to the SAME fild+fmul __real@3dcccccd as *0.1f; /Od 0x6427f6 shows divss by 10.0f (0x93dd7c) => the source divides.
- probe r4 ((real)(10-i)/MOTION_SENSOR_HISTORY_COUNT) and r5 (/10.0f): residual identical to r1 - inert (division folded to *0.1f before scheduling).
- probe r6 (r1 + /Od declaration order history_index before corner; HCEX names corner/blip_pos): residual identical - inert for codegen, kept for names.
- probe r7 (r6 + direct pow(1.0f - scale, 3.5): /Od calls a pow<float,double> template => source called pow directly, not power()): residual identical - inert. Kept (attested).
- diagnostics (lab, never landable): e1 (weight := relative_scale, no x87 in the loop-head block) -> `sub eax,[ebp-0x14]`
  FOLDS and `mov eax,[player]` follows idiv exactly as January => the split load is caused by x87 interleaving in the block.
  d1 (sensor ptr after pow) / d2 (weight first) -> 768 but 23 alndiff blocks (pow moves ahead of the int statement; far);
  d3 (sensor ptr assigned between fade and pow) -> 784, 32 blocks. /Od statement order stays best.
- diagnostic e2 (lab: render_motion_sensor with the whole stack-sentinel removed): the sensor-index/weight interleave and the
  split `mov edx,[i]` are UNCHANGED => the render residual does NOT depend on the check_stack_buffer code.

## _update_motion_sensor
/Od 0x642ff0 (od_update_motion_sensor.txt) + RTC names: reference_point (x2, two blocks), unit_center, player_center, point,
dummy (the bounding-sphere radius), customs[16], custom_temp. Statement order == w3c u7 (draw first; bounding sphere;
point.i/j; player_center.x/y = sensor ref, .z = unit_center.z; if (draw && distance_squared3d(&player_center,&unit_center)
<= r*r) tiny_set else {type none; unit_indices NONE}); loop 2: custom_temp.x/y = customs[i], .z = reference_point.z;
customs[i] -= ref; distance_squared3d(&reference_point, &custom_temp).
/Od facts u7 lacks: (1) game_time_get() result stored in a named local [ebp-0xc] (dead); (2) loop-1 counter SHORT [ebp-0x30]
+ a `blips = sensor->blips` pointer local [ebp-0x34]; loop-2 counter a separate LONG [ebp-0x120]; (3) loop-1 declaration
order object_index, unit_center, player_center, point, dummy, draw (u7: object_position, object_radius, displacement,
reference_position, draw); (4) no player_index local (local_player_get_player_index called twice in the ternary).
Hypothesis (LAW M8): the residual (j,i,k vs i,k,j sum order inside the inlined distance_squared3d) follows IL-local ordinals
=> /Od declaration order / local set may flip it.
- probe u7 (w3c u7 body transplanted onto current base): 864/39 [sha]; alndiff = only the loop-1 sum-order hunk 0x16b..0x178 (Jan j,i,k; ours i,k,j) + reloc-name artifacts. Reproduces w3c.
- probe u8 (u7 + /Od loop-1 declaration order unit_center, player_center, point, dummy, draw + /Od/RTC names): residual identical to u7 (sum order unchanged) - inert for codegen, kept for names.
- ORACLES for update_motion_sensor (u8 base): K=0..16 dead assigned locals at function top -> all sha 0dcb6bf20658 (inert); K=0..40 extern names before the definition -> all 0dcb6bf20658 (inert). Not a name-count/dead-local tie.
- HCEX.pdb: update_motion_sensor locals reference_point(0x58), unit_center(0x58, SHARED with the first reference_point =>
  first reference_point block closes before the loop, as u7), reference_point(0x68), customs real_point2d[16](0x80).
  motion_sensor_update: num_recorded_blips short[2], positions real_point3d[2], iter object_iterator,
  local_player_true_index short[2], center real_point3d (HCEX is 2-player; January is 4).
- probe u9 (u8 + /Od local set: no player_index local; short blip_index + blips pointer for loop 1; separate long custom_index for loop 2; no position pointer, customs[i] indexed; custom_temp; HCEX names reference_point x2 / customs): still ONLY the loop-1 sum-order hunk (frame/relocs identical). Kept as the /Od-faithful body.
- probe u10 (u9 + dead named local receiving game_time_get(), /Od [ebp-0xc]): inert.
- lab L1 (point.x/.y removed): loop-1 sum becomes k,j,i (loop-2 order); L3 (z first), L4 (args swapped), L5 (z+0.f), L7 (point.y first), L8 (player_center first, point from it), L9 (y before x): all keep ours i,k,j. Jan j,i,k not reached.
- ORACLE (used copy-chain locals o0=unit_index..ok feeding the NONE test, K=0..10): loop-1 sum order unchanged (i,k,j) - inert.
- ORACLE (real-typed locals: K=0..8 dead at top, K=0..8 used copy-chain feeding player_center.z): sum order unchanged - inert. No count-based lever.
- m4 (n4 _motion_sensor_update body transplanted onto current base): gate exact 17 residual 2 -> STILL strict EXACT at 16542e46 (owner-gated: uninitialised positions[] read + distance_squared3d/vector_from_points3d COMDATs).
- /Od 0x641b60 = motion_sensor_update (od_motion_sensor_update.txt): RTC names positions, local_player_true_index, iter,
  num_recorded_blips, center, dummy. The LATER build ZEROES positions[i] (xorps stores at 0x641cf4..) and re-tests the
  scan player's unit in the object loop (0x641e35..0x641e8c `continue`) => first-party proof that January's uninitialised
  positions[] read is an authentic bug fixed later (owner-gated authentic-bug class, like king/glow/dead_camera).
- DATA/SYMBOL blocker found: object_audit on base: .bss symbol offsets differ (Jan next_vertex_index@0, motion_sensor_globals@4, sweep_theta@8, debug@0xc; ours sweep_theta@0 ... next_vertex_index@8). VC7 .bss law: uninit statics first (name hash), '= 0' after in declaration order => January initialised motion_sensor_globals and sweep_theta. lab/bss1.c (= NULL, = 0.f; house precedent game.c/players.c): object_audit '44 January symbols, 0 differ', gate unchanged (16/3).
- diagnostic e3 (u9 with the stack sentinel removed): loop-1 sum order still i,k,j => update_motion_sensor residual does NOT depend on the check_stack_buffer code.
- probe r8 (r7 + single-exit nested if instead of goto render_motion_sensor_done): byte-identical to r7 (inert). Not proposed (touches the label beside the hud_stack_check lane's block).
- m5 (full candidate, motion_sensor_update without the player_index local as /Od 0x641c98 shows): exact 17 residual 2 (motion_sensor_update still EXACT). cand_full.c := m5 (render r9 + update u9 + motion_sensor_update m4/HCEX names + .bss inits).
- /W3 detector (warn.py): cand_prod / cand_full add no warning (only pre-existing C4244 in blip_type_get area + headers).

## FINAL STATE (wave 4)
Production (landable now): production.patch == cand_prod.c (CRLF), git apply --check OK. Gate exact 16 residual 3 (no
row lost), _point_from_line3d guard passes, object_audit: 44 January symbols / 0 differ (was 2), surplus shrinks by
`_power` (no longer referenced), no new surplus (provider_link --baseline: nothing to link), no new warnings.
  1. .bss: `motion_sensor_globals = NULL`, `sweep_theta = 0.f` (VC7 .bss law; January offsets 0/4/8/0xc).
  2. render_motion_sensor: ONE `weight` local for weight+radius (/Od 0x642868 stores pow result into the weight slot;
     January reuses [ebp-8]) - fixes the [ebp-8]/[ebp-0x20] slot swap; direct pow(1.0f - weight, 3.5) (/Od pow<float,double>);
     HCEX names corner/blip_pos; /Od declaration order history_index before corner. Park re-baseline:
     784/40 sha 69f4678045fb7eb843cd57c0a6715f102f2e7982cdeed68468dda6be4250ed44 (was de9701add36aae94...).
     Remaining: `sub eax,[ebp-0x18]` fold + integer sensor-index statement before the x87 weight chain (scheduler);
     + two offset-driven loop pads + stack-check alignment pad (offset-driven, NOT check_stack_buffer-shaped).
Owner-gated (report only): owner_gated_full.patch == cand_full.c: + update_motion_sensor u9 (/Od body; 864/39, one
8-byte hunk: loop-1 sum association Jan (jj+ii)+kk vs ours (ii+kk)+jj) + motion_sensor_update m4/m5 (STRICT EXACT
1168/43, BUG-commented uninitialised positions[] read; later build fixes it => authentic bug). Gate exact 17 residual 2.
New COMDATs _distance_squared3d/_vector_from_points3d byte-identical to January's action_alert copies, provider link
PASS both orders; admissible only via the exact motion_sensor_update caller (rule i), so the update body can ride only
with the m4 ruling.
Stack-sentinel dependency: NONE of the three residuals depends on check_stack_buffer (e2/e3 diagnostics).
