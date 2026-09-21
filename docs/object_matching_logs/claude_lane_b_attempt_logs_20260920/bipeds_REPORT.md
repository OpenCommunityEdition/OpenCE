# bipeds worker REPORT (Lane B)

## Baseline (python tools/campaign/gate.py source/units/bipeds --all) - verbatim

```
EXACT        16  _animation_left_foot_frame_index
EXACT        16  _animation_right_foot_frame_index
residual    480  _biped_accelerate  [sha]
residual     96  _biped_adjust_placement  [sha]
EXACT       304  _biped_apply_rotation
EXACT        48  _biped_approximate_surface_index
EXACT       432  _biped_build_flying_axes
EXACT       240  _biped_bumped_object
residual    176  _biped_check_discard  [sha]
EXACT        16  _biped_delete
EXACT        48  _biped_disconnect_from_structure_bsp
EXACT       240  _biped_exit_seat_end
EXACT       192  _biped_export_function_values
EXACT       464  _biped_falling_damage
EXACT       320  _biped_falling_danger
EXACT       384  _biped_find_ground_surface
residual    544  _biped_find_nearby_support_surface  [sha]
EXACT       448  _biped_find_pathfinding_surface_index
EXACT      1232  _biped_fix_position
EXACT        80  _biped_flying_through_air
residual    336  _biped_get_autoaim_pill  [sha]
EXACT       192  _biped_get_physics_pill
residual    416  _biped_get_sight_position  [sha]
EXACT       432  _biped_jump
EXACT       272  _biped_make_footstep
EXACT       192  _biped_new
EXACT        48  _biped_place
EXACT        32  _biped_preprocess_node_orientations
EXACT       272  _biped_render_debug
EXACT        96  _biped_reset
EXACT      1360  _biped_snap_facing
EXACT       272  _biped_start_landing
EXACT       128  _biped_start_limp_body_physics
EXACT        80  _biped_stop_limp_body_physics
EXACT        32  _biped_stop_melee_attack
EXACT       416  _biped_try_to_make_footsteps
EXACT      1120  _biped_update
EXACT       352  _biped_update_airborne
EXACT       208  _biped_update_dead
EXACT       512  _biped_update_jumping
EXACT       176  _biped_update_landing
residual   4080  _biped_update_moving  [size 4000!=4080, sha]
residual   5376  _biped_update_physics  [sha]
EXACT       128  _biped_update_slipping
residual   1712  _biped_update_turning  [size 1760!=1712, sha]
EXACT       432  _biped_vehicle_speech
EXACT       256  _biped_verify_object_vectors
EXACT        16  _bipeds_dispose
EXACT        16  _bipeds_dispose_from_old_map
EXACT        16  _bipeds_initialize
EXACT        16  _bipeds_initialize_for_new_map
== exact 42  residual 9  unwritten 0  (of 51 listed)
```

candidate.c created earlier as byte-exact copy of source/units/bipeds.c (cmp SAME at resume, no prior REPORT.md).

## Prior-ledger facts that bound this packet (read before any compile)

- `_biped_check_discard`: wave D cd1 proved strict EXACT with `ai_debug_describe_actor(...)` moved LAST in the error() varargs;
  January's pushes are `(2, fmt, name, x, y, z, describe)` against `"%s (%s) ... (%.1f %.1f %.1f)"` = original varargs bug (UB).
  Policy item, NEEDS OWNER RULING; not re-spent.
- `_biped_update_turning`: w3 reached 557/557 insns 1712/49 only with `(real_vector2d *)&v3` casts (REJECTED ON SIGHT in the brief)
  plus a real_math.h rotate_vector2d body change (header, loses _bitmap_copy). Not reachable inside this packet's rules.
- `_biped_update_physics`: January reads `collisions[best_collision_index]` with index -1 (OOB, policy).
- `_biped_update_moving`: donor `opus5-100k-consolidated/scratch/workersD/bipeds/um1.c` (frame 0x67c).

## Attempt 1 - _biped_update_moving: splice wave-D um1 body into HEAD (try_1.c)

- evidence: um1 has January's frame 0x67c (HEAD body 0x68c) and the long actor_is_leaping argument (January +0x9c2 pushes a dword).
- before: `residual   4080  _biped_update_moving  [size 4000!=4080, sha]`  (172 REAL regions, rr.py)
- after : `residual   4080  _biped_update_moving  [size 4096!=4080, relocs 96!=97, sha]` (75 REAL regions)
- all 42 exact rows kept. Used as the working base for update_moving (not exact; no credit).

## _biped_adjust_placement (96) - fresh pass, 5 shapes, UNCHANGED

Byte ledger (January vs ours, 88 real bytes both, 39/39 insns): the ONLY difference is the register of the `data` parameter web:
January `fld [eax+42Ch]; mov ecx,[ebp+0Ch]; fld st0; fmul [ecx+40h] ...` (9 modrm bytes ecx) vs ours `mov eax,[ebp+0Ch]` (eax).
Allocator reading (eax-first min-cost): January's `data` web must INTERFERE with an eax-preferring web (definition, copy of tag_get's result),
i.e. `data` is referenced in IL before definition's last reference - yet its machine load follows the radius `fld`.
- ap1 `real_point3d *position = &data->position;` declared before height_offset: register becomes ECX (interference confirmed) but
  `mov ecx,[ebp+0Ch]` is emitted BEFORE the fld (2 real regions: a pure 2-insn order swap). Not January.
- ap2 `real_vector3d *up = &data->up;` same as ap1.
- ap3 (diagnostic only, unnatural) `data->up.i*(height_offset = definition->...)`: identical to base (eax).
- ap4 (diagnostic) `x = x + up.i*(h = ...)`: identical to base.  ap5 `x = x + ...` spelling: identical to base.
Gate line unchanged every time: `residual     96  _biped_adjust_placement  [sha]`.
Blocker (specific): need an IL where the first `data` reference precedes `definition->biped.collision_radius` but whose param load is
emitted after the x87 load. No natural spelling found; prior ledgers cover flags-local removal, inline point_from_line3d, no biped local, field re-reads.
Reopen: a same-compiler exact donor showing `fld [eax+K]; mov ecx,[ebp+N]` with eax free (param load after an x87 load taking ecx).

## _biped_get_autoaim_pill (336) - fresh pass, 1 shape, UNCHANGED

Ledger: 131/131 insns; only the spherical arm's duplicated `*width = definition->biped.autoaim_width` tail differs (3 rows):
January `mov eax,[esi+458h]; mov ecx,[ebp+14h]; ... mov [ecx],eax` (value=eax, ptr=ecx: the SAME colours as the non-spherical tail),
ours hoists `mov edx,[ebp+14h]` into the preceding 12-byte struct copy (before `add esp,0Ch`) and uses value=ecx.
- aa1: explicit `*width = ...` at the end of each of the three arms (own resolution per arm): byte-identical to base.
Reading: in ours the width-pointer web interferes with the struct-copy webs (eax src / ecx dst) so it takes edx and is hoisted; in January it does not.
Blocker: what keeps January's `width` parameter web from overlapping the `*height = *global_zero_vector3d` copy. Prior: ap1 head-node local (worse).
Reopen: a donor showing a param-pointer load NOT hoisted into a preceding struct copy in a tail-duplicated exit.

## _biped_find_nearby_support_surface (544) - fresh pass, 9 measurements, UNCHANGED

Ledger: 175/175 insns, 1 real region / 2 rows: January `fld [ebp-14h] (plane.i); fmul [ebp-44h] (base.x)`, ours `fld base.x; fmul plane.i`.
January term order K(base-first), I(plane-first), +, J(base-first), +, fsub d.
- NOTE (house rule): the landed source line is a hand-reassociated expansion `base.y*n.j + (n.i*base.x + base.z*n.k) - d`, not the helper.
- fn1 genuine `plane3d_distance_to_point(&surface_plane, &base)`: 544/23, 2 real regions - emits K,J,+,I,+ all base-first (term ORDER differs too).
  Control: exact `_bsp3d_test_point` uses the same helper through pointers and emits J,K,+,I,+ plane-first - the helper's order is context-keyed.
- M8 instrument on fn1 (D=1..7 filler locals, diagnostic only): all 7 identical to fn1 -> M8-insensitive (frame aggregates; agrees with research campaign row).
- fn2 `surface_plane` declared at function scope before `base` (symbol ordinal) + helper: identical to fn1.
Blocker: January's tree pairs {K,I} first with a plane-first I term; neither the helper nor ordinal/local-count moves it. No natural spelling known.
Candidate keeps the landed line untouched (immutable policy for non-closing edits).

## _biped_accelerate (480) - fresh pass, 7 measurements (instrument), UNCHANGED

Ledger: 166/166 insns; 2 of the 6 products in the inlined `cross_product3d(global_up3d, acceleration, &axis)` load the other leaf:
term a.j*b.k: January `fld [edi+8]; fmul [eax+4]` (b.k first) / ours a.j first;  term a.k*b.i: January `fld [eax+8]; fmul [edi]` (a.k first) / ours b.i first
(+1 consequential `push edx` position). January's rank order is {b.k,b.j} > {a.j,a.k} > b.i ; ours {a.j} > {b.k,b.j,b.i} > {a.k,a.i}.
- M8 instrument (D=1..7 filler locals, diagnostic only): all 7 byte-identical to base (4 regions) -> NOT an IL-local-count defect (pointer parameter + global pointer leaves).
- Prior (wave D): if/else halving (worse), scale_vector3d halving (identical), `velocity.i += acceleration->i` (flips adds AND cross the other way: age-driven).
Blocker: leaf-age ranking of `acceleration->i` vs `global_up3d->j/k` inside the inline; no genuine statement in the body is known that ages `acceleration->i` last.
Reopen: a predictive A22/A35 leaf-age model (research lane), or an exact donor calling cross_product3d(global pointer, pointer param).

## _biped_get_sight_position (416) - fresh pass, 2 shapes + board scan, UNCHANGED

Ledger: 149 vs 147 insns, ONE root site: January `fld [edi+4]; fst [ebp+0Ch]; fmul st(2)` vs ours `mov ecx,[edi+4]; mov [ebp+0Ch],ecx; fld [ebp+0Ch]; fmul st(2)`
(+3 bytes; the later `mov ecx/edx,[ebp-4]` row is the consequence of ecx being spent on the copy). Same slot, same reloads.
- gs1 flat scope (both reals declared at block top, assigned in sequence): byte-identical to base (scope is dead here, as the law says).
- gs2 (diagnostic) first product reads `desired_gun_offset->j` directly: worse (no CSE with the int copy).
- Board scan `scan_fldfst.py` (25 January sites of `fld [reg+d]; fst [ebp+d]`): exact donors are (a) rumble_player_impulse `real longest = player->impulse_time[0]`
  saved around an assert call, (b) valid_realcmp inline params (address-taken bits test), (c) update_moving `physics.movement_desired.j = frame_info->dy`
  (a STRUCT FIELD store forwarded to the next x87 use). All three are memory-resident values with x87 store->load forwarding; a plain scalar `real` local
  that VC7 declines to enregister (ours) is copied with integer moves and is not forwarded.
Blocker: January's sideways value behaves as a memory-resident object with forwarding; the natural scalar local / inline `t` parameter (wave D gs1) does not.
Reopen: evidence of what the sideways value is stored INTO in the original (a field of a local aggregate would reproduce the forwarding).

## _biped_update_moving - NEW MECHANISM FOUND (not closed): struct-copy scheduling is ALIAS-driven (labs lab1..lab13, diagnostic only)

Byte fact: every 12/16-byte aggregate copy between `biped->...` and `physics.*` in January is PIPELINED
(`L0 S0 L1 L2 .. S1 S2`, e.g. +0x53..+0x76, +0x90, +0xc2, +0x151, +0x933, +0xc34) while ours (HEAD and um1) is SEQUENTIAL (`L0 S0 L1 S1 L2 S2`)
with the SAME registers. ~12 of um1's 75 real regions are exactly this.
Lab results (same compiler, appended lab functions, `disfn.py`):
- dest = member of a local struct, src = `param->member` (pointer PARAMETER): PIPELINED (lab_j/k/i), even when `&physics` goes to biped_update_physics.
- src pointer = CALL RESULT (`biped_get`) and the local's address reaches ONLY a static callee whose body does not leak it (lab_r): PIPELINED = January's shape.
- src pointer = call result and ANY address of the local (whole or member, const or not) reaches an extern or a leaking static (lab_a/c/s/t/u/v/w/x): SEQUENTIAL = ours.
- In the real function the flip to pipelined needs ALL of: unit_get_aiming_vector(&physics.aiming), biped_get_physics_pill(&physics.position..),
  the melee block's `&physics.position` externs, biped_bumped_object(&physics.velocity) AND biped_update_physics(&physics) neutralised (lab10d; any one left = sequential, lab11/lab12x*).
- Flag probes (diagnostic): `/Oa` loses 20 baseline-exact rows, `/Ow` loses 11 -> January's bipeds.obj is NOT an /Oa or /Ow object.
Reading: VC7 sequences an aggregate copy when the source pointer may alias the destination local (escaped local x unknown-provenance pointer).
January's compiler believed `biped` could not alias `physics` although January passes `[ebp-0DCh]` (= physics+8) to externs. The source fact that gives
January that knowledge is NOT identified (not pointer-local spelling lab_g/h, not const, not member-vs-whole, not flags).
This blocks `_biped_update_moving` independently of every other site. Reopen: identify what makes a call-result object pointer non-aliasing in VC7
(candidates not yet measured: `biped` reached through an inline helper's parameter; a differently-typed/declared object accessor).

## _biped_update_turning (1712) - fresh pass: donor re-measured at HEAD, NOT adopted (policy), candidate UNCHANGED

- Frame evidence: January `sub esp,30h`, HEAD body `sub esp,38h` (+8 = the typed `real_vector2d forward_2d/turn_axis_2d` copies HEAD uses to avoid casts).
  January's frame has no room for ANY 2D copy of forward/turn_axis -> the original operated on 2D views of the 3D vectors in place.
- try_t1.c = HEAD + w3 donor body `opus5-150k.../w3_bipeds/t8m2.c` (9 `(real_vector2d *)&vector3d` casts into rotate_vector2d/normalize2d/dot/cross 2D helpers,
  plus w3's genuine facts F/E/G/D/K/B), NO header change:
  gate `residual   1712  _biped_update_turning  [sha]` (size now 1712==1712, frame 0x30==0x30, 49/49 relocs), 3 REAL regions, all 42 exact rows kept.
  Remaining 3 regions are pure x87 leaf-order swaps in the two inlined cross_product3d bodies and one dot term, always `biped->object.up.*`:
  +0x193 `fld [ecx+8]; fmul [esi+30h]` vs ours swapped; +0x349 and +0x35d likewise (up.i/up.k against desired_facing).
- M8 instrument on try_t1 (D=1..7): all identical -> not an IL-local count defect (pointer-field leaves).
- NOT adopted: the brief rejects representation-punning casts on sight. NEEDS OWNER RULING: `(real_vector2d *)&real_vector3d` into 2D helpers
  (HCEA's biped_update_turning spells exactly this; January's 0x30 frame proves no typed copies exist). Even with the ruling it is 3 leaf-age swaps from exact.
Blocker (specific): (1) cast policy, (2) leaf age of `biped->object.up` in three products.

## _biped_check_discard (176) - re-measured, NOT adopted: NEEDS OWNER RULING (original varargs bug)

January bytes (+0x4c..+0x93): `call _ai_debug_describe_actor; fld [esi+14h]; add esp,14h; push eax; mov eax,[esi]; sub esp,18h; fstp qword [esp+10h]; ...;
push name; push fmt; push 2; call _error` -> the describe result occupies the slot ABOVE z: the argument list is (2, fmt, name, x, y, z, describe)
against `"WARNING: biped %s (%s) is in a bad place (%.1f %.1f %.1f), erasing"`. cdecl positions are ABI-fixed, so this is a genuine original
format/argument mismatch (undefined behaviour when the silent error is formatted).
- try_cd1.c (describe argument moved last): `EXACT       176  _biped_check_discard` (single-function gate). NOT in candidate.c.
- candidate keeps the correct argument order: `residual    176  _biped_check_discard  [sha]`.
No natural defined-C spelling can produce January's push order. Flag: NEEDS OWNER RULING (same item raised in opus5 100K/150K ledgers).

## _biped_update_moving (4080) - summary of this pass

Adopted in candidate.c (four byte-evidence corrections to the HEAD body; all 51 gate rows textually unchanged, unitcheck NO CHANGE):
1. `fabs(crouch_delta) > 0.01` -> `0.01f` (candidate.c:3903). Evidence: January relocates `__real@3f847ae140000000` = (double)0.01f; ours had
   `__real@3f847ae147ae147b`. After: relocation-target multiset symbolically EQUAL to January (97/97) - verified with relocdiff --count-by-target.
2. `actor_is_leaping((short)biped->unit.actor_index)` -> no cast (candidate.c:3862). Evidence: January +0x9c2 `mov eax,[ebx+1A4h]; cmp eax,-1; push eax`
   (dword); actors.h prototype takes `long`; the cast made ours `movsx`.
3. store order `ground_tangential_velocity_max = 0.1f;` before `ground_tangential_angle = 0.5f;` (candidate.c:3864-3865). Evidence: January +0x9da stores
   [ebp-88h]=0.1f then +0x9e4 [ebp-84h]=0.5f.
4. `if (fabs(dyaw) >= 0.0001f)` -> `if (!realcmp(dyaw, 0.f))` with the TU-private `#define realcmp(a, b) (fabs((a) - (b)) < _real_epsilon)` (candidate.c:358, 3661),
   verbatim the macro already landed in source/ai/actors.c:468 and actor_looking.c:236. Evidence: January +0x490 `fcomp qword (double)0.0001f; test ah,5; jnp`
   (= `!(x < eps)`); `>=` compiles to `test ah,1; jne`. Strip test: the undecorated `>=` spelling does NOT produce January's branch; the macro is a genuine
   first-party idiom, not a decoration. Measured on try_3: the +0x498 region disappears.
Gate line before and after: `residual   4080  _biped_update_moving  [size 4000!=4080, sha]` (REAL regions 172 -> 171). No credit claimed.

Not adopted (kept as scratch donors):
- try_4.c = HEAD + wave-D um1 body + realcmp + parenthesised radius: `[size 4096!=4080, relocs 96!=97, sha]`, frame 0x67c == January, 75 REAL regions before the
  radius change (93 reported after it only because alndiff re-aligns; the melee call block then matches January's x87 staging instruction-for-instruction
  except the `movsx edx/ecx` register). um1 contains bare scoping blocks (`{ unsigned long flags = ...; }`, `{ word dead = ...; }`) that I would not defend
  as natural source, so it is NOT the candidate.
- Radius accessor showing (owner-unblocked lever) measured on the HEAD body and it FAILS the required showing:
  plain `target->object.bounding_sphere_radius` -> integer `mov eax,[edi+0Ch]; push eax` (not January);
  `(target->object.bounding_sphere_radius)` -> January's `push ecx` reserve + `fld [edi+0Ch]; fstp [esp]`, reserve placed as January (`push ecx; fstp; mov word`);
  `#define OBJECT_BOUNDING_SPHERE_RADIUS(datum) ((datum)->object.bounding_sphere_radius)` -> byte-identical to the single-paren form;
  DOUBLE parens `((...))` -> x87 staged but the reserve push moves (`mov word; fstp; push ecx`): the decoration is GRADED, not idempotent.
  => does not meet "measurably idempotent"; NEEDS OWNER RULING before use. Not in candidate.c.
Root blocker (new, see the alias section above): January schedules every biped<->physics aggregate copy, the `cmp word [ebx+460h],1` / `cmp word [ebx+80h],-1`
hoists, the `physics.out_flags` byte kept in cl across stores through `biped`, and the `(1.f - crouch)` value kept on the x87 stack across stores to
physics.movement_desired as if `biped` cannot alias `physics`. Our compiler does that only when no address of `physics` reaches unknown code. About 30 of
um1's 75 regions are this one fact. Remaining independent sites on the um1 body: x87 leaf order in `movement_desired.* *= movement_scale` (+0x464),
old_cross/new_cross slot swap (+0x51d), player speed block (January hoists `1.f - crouch` between `fcomp` and `fnstsw` = the subexpression lives in BOTH
arms of the throttle test, no `uncrouch` local before the `if`), `mov ax,[ebx+0B6h]; and ax,4` (16-bit masked damage_flags value, +0xb17),
`mov eax,[ebp-48h]; cmp eax,-1` (support index read once, +0xb6f), `mov eax,[ebp-0E0h]; and eax,10h; test ax,ax` (+0xf40).

## _biped_update_physics (5376) - fresh pass, no shape spent, UNCHANGED

- NEW measured structural fact the packet triage missed (it reported frame none/none because the frame is set through __chkstk):
  January `mov eax,0AFACh; call __chkstk`, ours `mov eax,0AFB4h` -> ours is 8 bytes LARGER. Slot census over `[ebp-X]` operands: January 61 distinct cells,
  ours 63 (three byte cells each side: January -1/-9/-31h, ours -1/-11h/-29h) -> exactly TWO surplus 4-byte scalar cells in ours. Frame law: structural.
- First divergences: January keeps the zero-extended `in_flags` in EAX and homes it at [ebp-48h]; ours takes EBX (`push ebx; xor ebx,ebx; mov bx,[esi+4]`);
  January holds `&physics->velocity` in EBX and `&physics->forward` in EDI (ours EDI/EBX) - consequences of the two extra long-lived values.
- Known policy boundary unchanged: January reads `collisions[best_collision_index]` with the index still -1 (+0xe8a, opus5 150K w3) - out-of-bounds read,
  NEEDS OWNER RULING; the landed guard `best_collision_index != NONE &&` makes strict equality impossible while it stands.
Blocker (specific): (1) two surplus scalar frame cells (identify which named locals/temps are not in January), (2) the OOB stick test policy.
284 REAL regions; not attempted further inside this packet's budget.

## Final checks on candidate.c

- `gate.py source/units/bipeds --source scratch/lane_b/w/bipeds/candidate.c --forbid-emitted-symbol _point_from_line3d --all`:
  guard passed; `== exact 42  residual 9  unwritten 0  (of 51 listed)` (full listing in final_gate.txt; every row textually equal to baseline).
- `unitcheck.py`: `NO CHANGE  SUMMARY gains=0 regressions=0 other=0`.
- `newsyms.py`: `OWNERSHIP PASS` (+ `__real@3f847ae140000000` literal which January references; - `__real@3f847ae147ae147b` which January lacks).
- `fake_match_scan.py`: 0 review leads.
- `CL /Zs /W3`: one C4013 in the TU: `unit_get_local_player_index` at candidate.c:767 inside `_biped_bumped_object`, which is baseline-EXACT (immutable; not touched).
  No C4013 in any of my nine residual functions.

Addendum to the alias section (labs 14-16, all SEQUENTIAL = not the January fact): pointers bound through an `__inline` helper's parameters (lab_y),
a pointer loaded from a static global (lab_z), `struct biped_physics physics[1]` (lab_ab), `object_get` instead of `biped_get` (lab_ac),
plain-struct vectors instead of the real_vector3d union (lab_aa). Only a pointer PARAMETER source (lab_i/j/k) or a local whose address never reaches
unknown code (lab_r, lab10d) pipelines. Lab files lab1.c..lab16.c and disfn.py are in this directory; none of it is in candidate.c.
