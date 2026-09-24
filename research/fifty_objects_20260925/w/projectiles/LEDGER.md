# projectiles LEDGER (lane claude/fifty-objects-20260925)

## 0. Baseline (fresh run, no prior slug dir existed)
- gate.py source/items/projectiles --all: exact 29 / residual 1 / unwritten 0 (of 30). Only `_projectile_new` 432 [sha].
- alndiff vs build/base: 142/142 insns; only real difference = two scheduling swaps at 0x13b..0x148:
  JAN: fld st(0); fmul [esi+0x24]; lea eax,[esi+0x50]; push eax; lea ecx,[esi+0x48]; fadd [esi+0x18]; push ecx
  OURS: fld st(0); lea eax,[esi+0x50]; fmul [esi+0x24]; push eax; lea ecx,[esi+0x48]; push ecx; fadd [esi+0x18]
  (rel-type rows symbol vs defined-noncode for __real@ literals are the same target - not real.)
- Frame 0xc identical; 16 relocs identical.
- Prior negatives (closeout 20260919, w2/w3 20260914): direct SET_FLAG(..., scenario_location_underwater(...)), block scoping,
  expression order, 287 preserved candidates -> byte-inert. Do NOT repeat.

## 1. /Od readout (halo_cache_symbols.exe) - found via 'cont' imm32 compare
- projectile_new = 0x69b3a0..0x69b79c (dump: scratch/w/projectiles/od_projectile_new.txt).
- Velocity update is a REAL CALL to fn 0x42e0d0 = point_from_line3d (body: v->i*t + p->x ... returns result):
    push &obj.velocity(+0x24 /Od = Jan +0x18)  [result]
    push scale (= /Od-only field +0x218 * definition.initial_velocity; January has no +0x218 field -> scale = definition->projectile.initial_velocity)
    push &obj.forward (+0x30 /Od = Jan +0x24)  [v]
    push &obj.velocity                          [p]
  i.e. point_from_line3d((real_point3d *)&velocity, &forward, initial_velocity, (real_point3d *)&velocity).
- No `initial_velocity` local in /Od (scale computed inline in the arg). Locals: projectile [-4], definition [-8], detonation ticks [-0xc], arming ticks [-0x10], short attachment index [-0x14], attachment ptr [-0x18].
- Later-build-only extras (NOT January): +0x218 scale via 0x699b60, flag bit 0x40 else-if, trailing switch on 0x6aee30 with 0x695540 calls.
- Precedent for the cast: ai_debug.c / actor_firing_position.c use point_from_line3d((real_point3d *)&vec, ...).
HYPOTHESIS H1: January inlined point_from_line3d here (header __inline). Inline-param binding changes the IL order of the x87 work vs
the scenario_location_underwater arg leas, which is exactly the residual (2 schedule swaps). Hand-expanded statements in base = rule-6 violation anyway.

## 2. Probes
- P1 (H1) cand_h1.c: velocity update -> inline point_from_line3d((real_point3d *)&vel, &forward, definition->projectile.initial_velocity, (real_point3d *)&vel),
  drop `initial_velocity` local. RESULT: byte-identical to base (same 2 swaps). Inline-vs-open-coded is INERT here (but /Od-attested, rule-6 compliant: keep).
- P2 LAB ORACLE lab_double.c: `double initial_velocity` -> ISA changes (fld mem; fmul st(1); extra fstp) 140 insns. Not the lever; FP-type slot law does not apply (float*double converts).
- Reading of residual: Jan = push0, COPY(fld st0), MUL, lea, push, lea, ADD, push, STORE; ours = push0, COPY, lea, MUL, push, lea, push, ADD, STORE.
  i.e. our whole x87 chain from MUL is one integer slot later; as if in January COPY+MUL are one node (value CSE'd, stackifier dup) and in ours the copy is its own node.
- P3 LAB lab_nolocal.c: field read 3x (no local) -> VC7 reloads definition per component (alias), size 448, frame 8. NEGATIVE; January used ONE value (local or inline param).
- SCAN scan_copy.py over all January objects: 'fld st(0); x87; int-filler' (Jan form here, "B") vs 'fld st(0); int-filler; x87' ("A").
  Counter A/exact 45, A/nonexact 13, B/nonexact 10, B/exact 9 (scan_copy.txt). Closest analogue: ai_debug.c code_00039990 +0x227 =
  inline point_from_line3d(&point, global_up3d, <t>, &point) then call pushes; January = B. Its header comment records:
  bare `point_height+0.15f` -> residual; `(point_height+0.15f)` -> EXACT (landed via named macro ai_debug_attractor_label_height).
  => the B/A emission choice is sensitive to the SHAPE of the inline's t-argument expression node.
- P4 LAB ORACLE lab_paren.c: H1 + t-arg `(definition->projectile.initial_velocity)` -> ALL 30 EXACT. Mechanism confirmed: a paren/rvalue node on the
  inline t-argument moves the x87 chain one slot earlier. Decoration-only parens FAIL the strip test (owner ruling 20260921) -> NOT landable.
  Need a NATURAL January spelling that yields the same IL node.
- P5 LAB lab_cast.c `(real)field` t-arg: residual (A). P6 LAB lab_local.c: inline + `real initial_velocity` local as t: residual (A).
- HCEX DIA2Dump -sym projectile_new: only param projectile_index (enregistered) - no local evidence.
- /Od 0x699b60 = later-build difficulty/owner velocity scale (flag 0x4000, 1.5/1.0) -> the /Od product scale is a LATER feature; January t = the field.
HYPOTHESIS H2 (M8-type IL-local count): the paren oracle may act by adding one IL temp. /Od local set = projectile, definition, detonation ticks,
arming ticks, short attachment index, attachment ptr. Ours has 2 extra locals vs /Od: `attachments` block ptr and `underwater`. Remove both (/Od-faithful).
- P7 cand_h2.c (H1 + no `attachments` local + no `underwater` local, /Od-faithful): byte-identical residual (same 2 swaps). H2 INERT (but loop w/o local stays byte-exact).
- P8 LAB lab_dummy1..7.c: 1..7 declared-UNUSED real locals added -> all residual (same bytes). Declaration-count/name-count oracle INERT here.
- P9 LAB lab_nocast/castp/castr (pointer-cast presence on p/result args of the inline): all residual. Casts INERT.
- P10 LAB lab_pbase `(definition)->projectile.initial_velocity`: residual. lab_pfull `((definition)->projectile.initial_velocity)`: EXACT.
  => the OUTER paren around the whole t value is the trigger (any function-like macro expanding to `((d)->...)` would do it).
- P11 LAB paren elsewhere (lab_parm arming RHS, lab_pp p-arg, lab_pv v-arg, lab_pdet `*= (TICKS_PER_SECOND)`): all residual.
  => NOT a numbering/count effect; the trigger is specifically an outer paren node on the inline's t argument.
- P12 LAB lab_localrhsparen: `initial_velocity = (definition->projectile.initial_velocity);` then pass the local -> ALL 30 EXACT.
  lab_localargparen: `(initial_velocity)` at the call -> residual. => the paren must wrap the FIELD LOAD itself (propagates through a local copy);
  wrapping a variable use does nothing. January's source therefore had the load of initial_velocity inside a parenthesised expression
  (explicit paren, or a function-like macro expansion `((d)->projectile.initial_velocity)`).
- P13 LAB lab_mul1 `field*1.f`, lab_ternary `TRUE ? field : 0.f`: both fold to the residual A form. Only an explicit paren node wraps the load.

## 3. Data audit (.data 1548 B shown unmatched in build/report.json at 99.096%)
- January .data sec [54] `_projectile_update_section` 1548 B / 6 relocs; base identical under coff_compare.section_infos_equal (True) and
  section_info_resolved (size 1548, relocs 6, normalized_sha256 7bfaf5fb0b0c9e0593153885a81788bc8b118a9651a60a8a94e5ffd9fd203cf2) for split, base AND candidate.
- Cause of the objdiff 3.3.1 undercredit: relocs @1528/@1544 target `??_C@_06IKLLLDHL@normal?$AA@` / `??_C@_07GFBFDLBM@gravity?$AA@`
  which January's split leaves UNDEFINED (folded literal owned elsewhere) while ours defines them locally; plus the 3.3.1 '$'-literal name defect.
  Byte+relocation identical => scorer defect; fix = single-section config/semantic_data_matches.json entry (integrator).
- .rdata 544 B already 100%.
- P14 LAB lab_m8a (extra used `arming_time` local), lab_m8b (extra used `ignore_object_index` local): same residual (4 swap hunks). M8/IL-local count INERT.
- CHECK: cand_h1.obj and cand_h2.obj are byte-identical to build/base (only .debug$S differs; projectile_new sha 9d8477d2 = park base).
  lab_pfull / lab_localrhsparen: projectile_new sha 85fac0f0 = January target; nothing else changes.
- object_audit on lab_localrhsparen.obj: PASS (55 January symbols, 0 differ; only candidate-only surplus = helper/literal COMDATs incl. _point_from_line3d
  which base already emits; projectiles is one of the 17 owner-relaxed objects with an out-of-line _point_from_line3d reference).

## 4. Object-level audit (on exact LAB candidate lab_pfull.obj and on production base)
- object_audit: PASS. pdb_storage: 55 split symbols, 0 disagreements.
- surplus_identity: 24 candidate-only code COMDATs, 1 NOT identical: _distance3d (ours = real_math.h inline body; January's selected copy in
  action_vehicle.obj has a different x87 order: fld st(2)/fmulp st(3)... vs fld st(0)/fmul st(1)...). January projectiles.obj DOES reference
  _distance3d / _valid_real_vector3d_axes2 / _normalize3d / _point_from_line3d as undefined externals.
- provider_link: SELECTED-PROVIDER LINK FAIL (3) in BOTH base and candidate: _distance3d (action_vehicle), _real_random (action_charge),
  _valid_real_vector3d_axes2 (bored_camera) - all three providers hold NODUP hand copies (nodup_census). PRE-EXISTING, provider-side.
- Data: see section 3; proposed entry scratch/w/projectiles/semantic_data_entry.json (single-section, allow_incomplete_unit like leaf_map).

## 5. Deliverables
- production.patch (CRLF-preserving; `git apply --check` OK) = cand_h1: replaces the hand-expanded point_from_line3d (house rule 6 violation)
  with the /Od-attested inline call; byte-identical to build/base (only .debug$S differs). Zero credit, quality fix only.
- cand_h2.c: byte-identical alternative that additionally drops the `attachments` and `underwater` locals (/Od-faithful local set).
- HELD (owner ruling, NOT landable under strip test): lab_pfull.c / lab_paren.c / lab_localrhsparen.c close _projectile_new EXACT
  (sha 85fac0f0) only with an outer paren around the initial_velocity load. No first-party name for an accessor macro exists.
## 6. Stop rule fired: H1 inline, H2 /Od local set, pointer casts, M8 extra used locals, unused-declaration oracle, fold oracles
  (x*1.f, TRUE?x:0), (real) cast, local-through: all inert. Only the paren node on the load reaches January.
