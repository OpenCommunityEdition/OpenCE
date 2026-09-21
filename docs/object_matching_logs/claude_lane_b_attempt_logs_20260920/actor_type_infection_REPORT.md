# Lane B worker log: source/ai/actor_type_infection

Resumed run (an earlier interrupted run left candidate.c == real source, ref_* copies, v0.obj/shape1.obj, no REPORT.md).

## Baseline (verbatim: python tools/campaign/gate.py source/ai/actor_type_infection --all)

    EXACT       304  _infection_decide_action
    EXACT       688  _infection_swarm_aim_jump
    residual   3616  _infection_swarm_control  [size 3360!=3616, relocs 103!=104, sha]
    EXACT        96  _infection_wander_move_time
    EXACT        96  _infection_wander_pause_time
    == exact 4  residual 1  unwritten 0  (of 5 listed)

## Prior research re-gated in this worktree (measurement base only, not my work)

- ref_prior_v0.c (n3a/n4 v0.c): residual 3616 _infection_swarm_control [size 3600!=3616, sha]; 4 siblings EXACT.
- ref_shape1_flags.c (Astra research7 32-bit flag accumulator): same gate line; 4 siblings EXACT.

Remaining clusters at v0/shape1 (reloc-spelling + branch-immediate noise filtered; aln_v0_real.txt, aln_shape1_real.txt):

1. 0x20f inlined magnitude_squared3d on the x87-stack delta: January jj,kk,ii; ours kk,jj,ii.
2. 0x874 inlined normalize3d #1: January `fld [dir.k]; fmul st(1)` keeps the scale live to the join; ours `fmul [dir.k]` + `jmp` over the pop.
3. three inlined cross_product3d(&up,&direction,..) sites (0x8e6, 0x9c0, 0xb59): only the up.i products commute.
4. 0xbfd..0xcce: four per-exit `mov al,1`, else-arm sink of `direction = forward`, should_jump in AL / flags in CX, pre-csmemset conversion (carries the 16-byte padded gap).

## Attempts

### D0 (diagnostic only, never a landing): LAW M8 dead-local sweep on shape1 base
- Hypothesis: cluster 3 (up.i-only commutation) is an IL-local-count (mod 8) defect.
- Instrument: D = 1..8 dead `long lab_mod8_K = 0;` at function top of ref_shape1_flags.c (diag/m8_D.c).
- Result: every D gives the identical region list (39 regions, same offsets) and gate `[size 3600!=3616, sha]`. VERDICT: MOD8-INSENSITIVE. Cluster 3 is not an M8 count defect (consistent with M8's own limit: frame-local aggregates are not governed by it).

### Evidence found this run: HCEA 2020 debug (unoptimised, RTC) build decompile `ref_hcea_ghidra.c` + raw asm `hcea_ghidra.asm` + HCEX line table `hcea_lines.txt`
The debug build is a statement-level oracle (no optimisation): it shows every local and helper call. HCEX line numbers 494/518 sit right after January's assert anchors 493/516 (0x1ED/0x204), so the 2011 source text is line-compatible with January's in this function. Facts read from it that v0 does not carry:
- should_jump is FOUR separate `should_jump = TRUE;` statements in an if / else-if chain (HCEX lines 681,683,685,687,689,691,693,695,697), and swarm_component->flags is RE-READ from memory at every test (no `component_flags` local).
- helpers: distance3d(&prop->body_position,&swarm_component->position); negate_vector3d(&movement_vector,&movement_vector) x3; point_from_line3d(&swarm_component->position,&movement_vector,K,&behind_point); set_real_vector3d(&control_data.throttle,1,0,0); dot_product3d(&movement_vector,&up_vector); dot_product3d(&movement_vector,&vector_to_friend).
- final block: `if (normalize3d(&left_vector) == 0) movement_vector = forward; else cross_product3d(...)`.
- a second pointer local `biped` = unit inside the biped-type block and inside the attacking block; two dead locals in the combat block (long = NONE, boolean = FALSE).
- a dword control_flags accumulator (Astra shape1) is confirmed.

### E5 distance3d() in the prop scan (HCEA call evidence) - REJECTED
- gate: `residual 3616 _infection_swarm_control [size 3600!=3616, sha]` before and after; region list identical (byte-inert for cluster 1).
- newsyms: `+ _distance3d .text VIOLATION: January does not define it` -> OWNERSHIP FAIL. Reverted.
### E4 point_from_line3d(&position,&direction,-0.2f,&probe) (HCEA call evidence) - REJECTED
- function bytes identical (VC7 folds `v*-0.2f + p` into `fmul 0.2; fsubr p`, which is January's spelling), but the object newly emits `_point_from_line3d` -> FORBIDDEN-EMITTED-SYMBOL guard fails. Reverted. (try saved: work_e4.c)
### D1 (diagnostic) cluster-1 census: January's jj,kk,ii on-stack order occurs at only 2 sites in all of build/split (this one and EXACT `_actor_perception_unit_from_swarm` @0xb0, spelled distance_squared3d(&component->position,&position->body_position)); M8 dead-local sweep on the distance3d spelling is also insensitive (D=1..8 identical).

### E7 should_jump as four statements + flags re-read, on the shape1 base - ACCEPTED (progress)
- Evidence: HCEA debug asm 0x483449..0x48349b: `test flags&0x10 -> jump=1` / else-if has_direction && ground_ticks>=45 / member==index -> jump=1 / else-if melee -> jump=1 / else-if facing -> jump=1, and `swarm_component->flags` re-read from memory at each test (no copy local). HCEX line table 681..697 has nine separate lines. w1 `p1` tried this chain on the OLD base (should_jump callee-saved) and saw it merged; Astra's dword control_flags accumulator (which makes should_jump dead before csmemset) is the missing co-requisite.
- before: `residual 3616 _infection_swarm_control [size 3600!=3616, sha]`
- after : `residual 3616 _infection_swarm_control [sha]`  (size now equal; 4 siblings EXACT; saved try_1_e7.c)
- January's four `mov al,1` blocks, AL for should_jump, CX for the flags all reproduce. Left in the tail: else-arm sink of `direction = forward` (block order), an extra `mov cl,[esi+2]` re-read in the attached arm, csmemset argument interleave, animation/aiming byte scheduling.

### E10 final normalize arm order `if (normalize3d(&right) == 0.f) direction = forward; else cross_product3d(...)` - ACCEPTED
- Evidence: HCEA debug asm lays the forward-copy arm FIRST (0x483410..0x483429 `JMP`) and the cross_product3d call arm second (0x48342b); January sinks the forward arm after the next statement's first block (`jmp 0xbfd` back-edge at 0xc23).
- before `[sha]` 34 regions -> after `[sha]` 32 regions; the 0xbfd/0xc23 block-order regions are gone. (try_2_e10.c)
### E11 attached-arm melee state as if/else with two stores (not ?:) - ACCEPTED
- Evidence: HCEA debug asm 0x4834a7..0x4834c9 has two direct byte stores `[unit+0x24d] = 4` / `= 0` under a branch (a ?: would stage a temp). January `and cl,1; shl cl,2` reuses the CSE'd flags word; our ?: spelling re-read `mov cl,[esi+2]`.
- after: `residual 3616 _infection_swarm_control [sha]`, 1127 vs 1127 instructions, 9 regions left: ALL of cluster 4 closed (whole tail 0xbfd..0xdc0 now byte-identical). unitcheck: gains=0 regressions=0. candidate.c := this state. (try_3_e11.c)
- Remaining: cluster 1 (0x20f), cluster 2 (0x874/0x87c), cluster 3 (0x8e6,0x8ee,0x9c0,0x9ce,0xb59,0xb64). All x87, all same-size.

### E1 dot_product3d(&direction,&up) (HCEA arg order) - byte-inert, not adopted (region list identical).
### E12 HCEA declaration order of the loop locals - byte-inert (9 regions identical), not adopted.
### D2 (diagnostic) second M8 instrument `long lab_used_K = actor_index;` D=1..8 on the E11 base: all identical -> clusters 1-3 are NOT IL-count keyed.
### D3 (diagnostic) TU-local cross_product3d body respellings (natural i,j,k temps / no temps / a->i operands swapped): the six up.i terms never move -> operand spelling in the helper body is inert for cluster 3 (header respelling cannot fix it).
### D4 (diagnostic) `up` (and `direction`) promoted to function scope: inert.
- Cluster-3 byte reading: January ranks leaves "symbol+offset before bare symbol, tie -> first argument" (up.j,up.k > dir.j,dir.k > up.i > dir.i) at all 3 `cross_product3d(&up,&direction,&x)` sites; ours ranks "first argument first" always. Only the two a->i products per site differ (6 products, 12 instructions, 0 bytes).

### D5 (diagnostic) cluster-2 census: inlined `normalize3d(&v);` with the result discarded exists in January in both forms - "J" (non-destructive 3rd multiply, one `fstp st(0)` shared at the join; 9 sites incl. this one and EXACT _actor_look_find_random_vector/_king_engine_post_rasterize/_flag_render_proper/_render_sky) and "O" (destructive `fmul m; fstp m; jmp` over the zero-path pop; _flag_update). Our header produces J at the exact donors, so the header is right and cluster 2 is contextual. What follows the normalize (integer copy / call / FP) is inert (diag p2,p3); whether site #3 uses the result is inert (p4).
### D6 (diagnostic) swapped-argument cross product at site 3: ours loads `up` first in EVERY product regardless of argument position -> ours orders by VARIABLE; January orders by SHAPE first (symbol+offset before bare symbol) then variable. Same signature as _breakable_surface_effect @0x37b (frame-local a->i terms) - systemic class.

### D7 (diagnostic) function-size budget theory: 4/16/48/128 extra trailing `if (game_time_get()==K) game_time_get();` blocks after the loop -> clusters 1 and 2 keep ours' form at every size (clus.py byte-pattern check). Not a size/budget threshold.
### E13 prop-scan respellings: (a) `real score = 0.f; ... score += (1 - d/r)*10` (HCEA debug shape) -> WORSE (size 3600, frame/regs move; 22+ regions) REJECTED; (b) `distance = magnitude3d(&delta)` -> byte-identical to current (inert), not adopted.

### E3 negate_vector3d(&direction,&direction) at the 3 negation sites (HCEA call evidence) - function bytes inert; newsyms `+ _negate_vector3d VIOLATION` -> not adopted alone.
### E2 dot_product3d(&direction,&delta) (HCEA arg order, neighbour loop) - byte-inert, not adopted.
### E6 set_real_vector3d(&control.throttle, 1.f, 0.f, 0.f) instead of three member stores - ACCEPTED (closes cluster 1)
- Evidence: HCEA debug asm 0x483649..: three pushes 0,0,1.0f + &control_data.throttle then a CALL (set_real_vector3d) in the has_direction arm; house rule also prefers the real helper call over manual expansion. `_set_real_vector3d` is ALREADY emitted by the baseline object (newsyms: OWNERSHIP PASS, no new code symbol).
- before: 9 regions (0x20f 0x874 0x87c 0x8e6 0x8ee 0x9c0 0x9ce 0xb59 0xb64); after: 8 regions - 0x20f (cluster 1, jj/kk order on the x87 stack 0xAxx bytes away from the edit) is GONE.
- Mechanism: an extra inline instance (its parameter temps) re-keys a distant x87 commutative tie - the per-function inline-temp numbering key (research campaign F1), not a user-local count (M8 sweeps were insensitive).
- gate: `residual 3616 _infection_swarm_control [sha]`; unitcheck gains=0 regressions=0. candidate.c := this state (try_4_e6.c).

### C_AB = E6 base + E3 (negate_vector3d x3) + E4 (point_from_line3d) - STRICT EXACT, but fails the emitted-owner guard
- gate (verbatim): `EXACT 3616 _infection_swarm_control` / `== exact 5 residual 0 unwritten 0 (of 5 listed)`. Saved as exact_needs_ruling.c (NOT candidate.c).
- Every change is HCEA-debug-asm backed (helper calls at 0x48313e point_from_line3d(&swarm_component->position,&movement_vector,K,&behind_point); three negate_vector3d(&movement_vector,&movement_vector) calls; set_real_vector3d for throttle).
- Singles c_a (E3), c_b (E4), c_c (E5 distance3d), pairs c_ac, c_bc: all still 8 regions. Only E3+E4 together (with or without E5) close clusters 2 and 3 -> the inline-instance inventory re-keys the x87 ties (same mechanism as E6).
- BLOCKER: the object then DEFINES `_point_from_line3d` (48 B COMDAT) and `_negate_vector3d`; the lane guard forbids `_point_from_line3d` outside action_charge.obj.

### E14 separate `unit` (unit_get) and block-local `biped = (struct biped_datum *)unit` pointers - ACCEPTED (byte-neutral, authenticity)
- Evidence: HCEA debug asm copies the unit pointer into a second local inside the biped-type block (0x48207a) and inside the attacking block (0x48354a); idiom attested in exact code (units.c:4196, collisions.c:1691). Removes the raw cast of the unit_get() result. 8 regions unchanged. (try_5_e14.c)
### Strip tests (all byte-neutral, so none of the inherited casts is load-bearing): s1 drop `(word)`; s2 `word control_flags`; s3 drop `(char)` x2; s4 `long control_flags`; s5 drop `(short)` on MAX(). exact_stripped.c (exact variant without the casts) is still EXACT.

## FINAL STATE

candidate.c (guard-passing best state) - verbatim gate --all:

    EXACT       304  _infection_decide_action
    EXACT       688  _infection_swarm_aim_jump
    residual   3616  _infection_swarm_control  [sha]
    EXACT        96  _infection_wander_move_time
    EXACT        96  _infection_wander_pause_time
    == exact 4  residual 1  unwritten 0  (of 5 listed)

- _point_from_line3d guard: PASS. newsyms: OWNERSHIP PASS (+__real@bd888889, +__real@bfc90fdb both January-defined; -__real@3f6eeeef). fake_match_scan: 0 leads. CL /Zs /W3: no warning in the .c (12 header warnings identical to baseline, no C4013). unitcheck: gains=0 regressions=0.
- Remaining residual byte ledger (aln_final_real.txt; 1127 vs 1127 instructions, 3616 vs 3616 bytes, net 0):
  * cluster 2 @0x874: January `fld [ebp-8]; fmul st(1)` (5 B) + shared `fstp st(0)` at the join; ours `fmul [ebp-8]` (3 B) + `jmp +2` (2 B). 5 = 3+2.
  * cluster 3 @0x8e6,0x8ee,0x9c0,0x9ce,0xb59,0xb64: six `fld X; fmul Y` pairs with X/Y exchanged (the two up.i products of each cross_product3d(&up,&direction,..)); 6 B each both sides.
- Both clusters are the per-function inline-instance numbering key: they close together, and only, when the function carries January's inline-helper inventory.

exact_needs_ruling.c = candidate.c + negate_vector3d(&direction,&direction) x3 + point_from_line3d(&swarm_component->position,&direction,-0.2f,&probe):

    EXACT       304  _infection_decide_action
    EXACT       688  _infection_swarm_aim_jump
    EXACT      3616  _infection_swarm_control
    EXACT        96  _infection_wander_move_time
    EXACT        96  _infection_wander_pause_time
    == exact 5  residual 0  unwritten 0  (of 5 listed)

- NEEDS OWNER RULING: that object emits `_point_from_line3d` (guard FAIL) and `_negate_vector3d` (newsyms VIOLATION). Both emitted COMDATs are 48 B and byte-IDENTICAL to January's selected copies (action_charge / action_obey split objects); January's selected copies sit at image offsets 5,504 and 24,352, i.e. EARLIER in link order than _infection_swarm_control (160,880), so a discarded later duplicate is first-copy-consistent (same showing that earned `_random_range` its exception).
- exact_needs_ruling_full_hcea.c additionally uses distance3d() (also EXACT 5/5; emits `_distance3d` too).
