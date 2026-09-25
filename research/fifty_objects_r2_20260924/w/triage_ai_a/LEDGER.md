# Triage ai_a ledger (round 2)
Base f6d00a8c. Objects: actor_perception, actions, actors, action_charge, path_obstacles.
Read-only triage; no candidate source edits; no claims.

## actors :: _actors_spawn_from_unit (672)
- Prior refusal: Lane A R27 (claude_lane_a_ai_core_rejected_hypotheses_20260920 l.1074/1133) and
  actors_obj_large_closeout_20260919: exact only via point_from_line3d; refused ONLY because
  `_point_from_line3d` emission was guarded at that date.
- House rule 6 exception documented 3c74fa36 (2026-09-23, AFTER both refusals): header __inline
  COMDAT incl. _point_from_line3d may be emitted when copy byte-identical to January's selected
  copy + caller strictly exact + zero-regression sweep + provider link; "January's per-object folded
  survivor is not an absolute emission boundary". build/base already has 24 DEF of _point_from_line3d
  (split: 1 DEF action_charge, 17 UNDEF) incl. non-17 objects (biped_limp_noodle, actor_type_infection,
  collision_debug, breakable_surfaces, leaf_map ...) => practice confirmed.
- /Od 0x44dfe0 (od_spawn_from_unit.txt): ternary temp [ebp-0xe8] = swarm!=-1 ? swarm : actor ->
  [ebp-0x10]; `if (x != -1)` then actor_get(unit->actor_index); REAL CALL fn 0x42e0d0 with
  (&position, &forward, 0.3f, &position) = point_from_line3d. Both Lane-A p1 hunks /Od-attested.
- MEASURED: gate of UNMODIFIED Lane A file res9/spawn-unit2/v/p1.c at f6d00a8c headers:
  `EXACT 672 _actors_spawn_from_unit` (other residuals = that old floor's 3). Emitted
  _point_from_line3d .text 48 B byte-identical to January action_charge copy (hex compared).
- Diff current actors.c spawn fn vs p1: exactly 2 hunks (ternary source_actor_index; pfl3d call
  replacing the current hand expansion, which itself violates rule 6).
- VERDICT: ATTACK (landing-ready), p~0.85; integrator must gate on current file (75/76 expected),
  remove park entry, provider_link, full sweep.
## actor_perception :: _actor_perception_aiming_vector_test_blockage (400)
- Round-1 Packet A + C landed (43/1, pdb 0 disagreements, link PASS on board; distance3d provider
  blocker cleared by Layer 2). Only Packet B remains, held on structural owner packet s5.
- MEASURED: unmodified round-1 final_full.c (claude-fifty-objects-20260925/scratch/w/actor_perception)
  gated at f6d00a8c: `== exact 44 residual 0`. Differs from production ONLY in the blockage fn.
- Load-bearing (fail strip test; /Od 0x4710a0-attested only): aggregate 2D view copy (field copy 384!=400),
  empty then-arm of hes<0.36 ([sha] otherwise), explicit else blockage=0 (384!=400). New COMDATs
  _normalize2d/_scale_vector2d/_dot_product2d/_negate_vector3d identical + link PASS (round-1 review).
- Production blockage body currently hand-expands point_from_line3d/negate/dot (rule 6/13 debt).
- No castless January spelling found in any ledger; odbuild confirms aggregate integer copy.
- VERDICT: OWNER (s5 aggregate view copy + two /Od-only constructs). No worker route.
## actions / action_charge premise re-measure at f6d00a8c
- round-1 actions cand_exact.c (owner-gated TU-private ((d)*(d)) macro): `== exact 63 residual 0`.
  cand_fuzzy.c: 62/1 (vehicle_entry [sha]); production 62/1.
- round-1 review no_a4_vec.c (action_charge, load-bearing C1/C2 aggregate copies + /Od-only dead
  target_point + fitted named-result subset): `== exact 22 residual 0`; production 21/1 [size 3264!=3248].
- HCEX DIA2Dump action_charge_perform locals: vector_to_target, horizontal_velocity, vertical_velocity,
  aim_vector, minimum_velocity, target_alignment_vector, alignment_vector. NO target_point, NO success,
  no scalar result names -> HCEX does NOT attest the dead target_point block that closes the M8 count.
- provider_link on laneA_p1_at_f6d00a8c.obj --baseline build/base actors.obj: _point_from_line3d
  provider action_charge PASS; SELECTED-PROVIDER LINK PASS. COMDAT selection ANY (2).
- /Od note: spawned count is a SHORT local ([ebp-8] word) in /Od; production uses long. p1 exact
  with long -> keep; disclose.

## actors :: _actor_input_update (2384)
- Prior: 0902 wave, opus5 waveD/w2/w3/w3d (0914-0915), actors closeout 0919, Lane A res7 (0920):
  residual = `unit` live-range split at aiming join (Jan lea edx,[ebx+0x1ec] vs ours add ebx,0x1ec +
  reload) + pre-threat-loop unit piece eax vs ebx + 3-5 cross_product3d x87 leaves. Inert/lateral:
  decl order, scope hoist, arm inversion, -1 local count (x2), single-exit swarm arm, pointer/value/
  component/helper/control spellings (8), gunner_def fold (lateral). NONE of these ledgers read /Od.
- NEW /Od readout (0x4486c0, od_actor_input_update.txt): swarm loop CALLS fn 0x44bb30 (body =
  unit_get(u,3); component get; surface=-1; if biped surface=+0x494; object_get_origin; store) =
  actor_swarm_component_refresh(unit_indices[i], component_indices[i]); and CALLS 0x44e8e0 =
  add_vectors3d(&component->position, &swarm_center, &swarm_center). Production hand-expands both
  (rule 13 manual inlining of a TU static + rule 6 hand-expanded header helper).
- January: _actor_swarm_component_refresh (80 B) exists; setup calls it out of line (@0x65 reloc);
  input_update has no call -> VC7 inlined it at the loop site (consistent with per-site gating).
- Also /Od threat loop: projectile arm uses a named alias local [ebp-0x70] and TWO stores to
  delayed_attached_projectile_index (if flag / else if danger-zone) vs production's single || store.
  January bytes show one merged store at 0x55d (cross-jump possible either way).
- Link: _add_vectors3d has 17 base definers, 0 NODUP; but new COMDAT from a NON-exact caller is P1
  class -> the add_vectors3d half lands only if the function goes exact.
- Mechanism link is indirect (unit split was count-insensitive per S4/S7). VERDICT: ATTACK, p~0.15.

## path_obstacles :: _obstacles_test_circle (160)
- Exact only with parenthesised field operand (bare parens / TU-private accessor) = brief s2 HELD item;
  09-20 accessor ruling requires a first-party-attested name: /Od strings (odbuild str disc) and atlas
  have no disc-radius accessor. Natural spellings exhausted (0906, 0919 HCEA forms, Lane B 6 shapes,
  M8/H3 sweeps insensitive).
- Extra admission prerequisite found: pdb_storage -> _global_obstacle_colors split 2 / ours 2 but ABSENT
  from cachebeta publics (January file-static); no other object references it (sym_census). Needs
  `static` + symbols.json "static": true + csplit-only regen (name not first-party; atlas/HCEX silent).
- VERDICT: OWNER (held paren/accessor ruling); workers must not retry.

## action_charge :: _action_charge_perform (3248)
- Round-1 review HOLD_FOR_OWNER (B1 dead target_point filler-indistinguishable; B2 load-bearing C1/C2
  aggregate view copies; B3 fitted named-result subset; full /Od set residual). no_a4_vec still 22/22
  at f6d00a8c. HCEX lists no target_point -> no new attestation. VERDICT: OWNER.

## actions :: _actor_action_handle_vehicle_entry (960) + data 2404
- cand_exact 63/63 at f6d00a8c; needs 09-20 squaring-macro ruling extended to actions.c (structural
  packet s7; (d)*(d) by hand also matches = weaker class) + grouped '$'-defect data entry needing the
  semantic_progress.py verifier change (main ledger held table) or objdiff 3.6.0.
- No first-party squaring macro: source/ grep (only actor_perception's admitted one), Marathon 2
  source none, /Od strings none. VERDICT: OWNER.
