# LEDGER - rasterizer_xbox_environment_fog / __rasterizer_environment_fog_screen_begin

Worker: claude-fifty-objects-20260925 lane. Started 2026-09-23 (fresh; no prior slug dir).

## Baseline (e9e62b78)
gate --all: exact 26 / residual 1 / unwritten 0.
residual 3904 __rasterizer_environment_fog_screen_begin [size 4000!=3904, relocs 232!=235, sha]
Frame: Jan 0x180 vs ours 0x1c4 (task statement).

## Prior evidence read
- opus5_100k (0914): facts 1-9: January INLINES real_alpha_to_pixel32 here (header assert
  literal line 291, 255.0f, 32-bit fistp); asserts use bitmaps.count (now renamed in tree);
  %f[%x] reads float bits in place (no csmemcpy temps); offset 16-bit; base-z OR form;
  reseed two-random packets; viewport short diffs; frame map; fchs;fsubr rotation.
- opus5_150k_w1: bss split (landed now); first-time reseed: y store follows 2nd call.
- Do-not-repeat: __forceinline/flags for alpha inline; setter-order labs l1-l4.
- LaneC handoff: January-only 'alpha>=0.0f && alpha<=1.0f' + ..\bitmaps\bitmaps_inlines.h literal.
- Memory law: inline gating per SITE (callee cost vs block-weight budget); loop +3 steps;
  early return lowers budget of the rest of function by one step; && levels cost.

## Discovery: structural lane (claude-structural-20260923, TODAY) already worked this function
Copied its REPORT.md + candidate.c + diag_final.c + probe_site.py to structural/.
Its source base == this tree's source (diff ignoring CR: identical). Its candidate.c
gated here as c0.c: `residual 3904 [size 3936!=3904, sha]`, exact 26/1/0 (relocs 235==235).
Recorded there (DO NOT REPEAT): shadow fix, wind/phase/agg scopes, short offset + OR base-z,
animation_period predicate fixes, short width/height, (real)tan, set_real_point2d inline
reseed (left-to-right negative), vsh_constants__texscale_1 separate local, D3DCLEAR_TARGET_A,
skinning count-first, nested chicago verify-result. Site budget for alpha inline = 14,
real_alpha_to_pixel32 cost 15-17 (probe_site.py). Remaining: ruling A (alpha inline) + D
(5 csmemcpy float-bit temps) + minor: TEST_FLAG word load, clear_z branch graph,
far/aspect multiply order, reseed arg order.

## P0 probe harness (probe.py) - site budget at alpha site in c0
c0.c 12:. 13:. 14:. 15:C 16:C  -> budget 14 (confirms structural lane/Lane C).
Hypothesis source: LAW I3 (research 0916) "block frequency accumulates from upstream
for-increment loops"; Lane C sec.7: predicate forms change weight. Plan: lab-only
ablations of upstream structures in c0 to learn what sets the site weight, then look
for a January-attested structural difference upstream (not a wrapper).

## P1 budget map + calibration (lab, c0)
Site budget at points (probe.py): pass0 early region (vector.i=1 / base-z block) 10-11;
inside color loop 16; before Clear / before csmemset(pixel_shader) / at set_pixel_shader 15;
inside else-arm (texture path, alpha site) 14.  Upstream loop ablations: removing ANY one of
first-time nest / anim loop / color loop -> site 11; only anim loop -> 9; none -> <9
(LAW I3 confirmed: upstream loops raise downstream budget, non-linearly).
Calibration (costlab.py, extra lab call): real_alpha_to_pixel32 INLINES at budget-15 points
(before csmemset, at set_pixel_shader) and in color loop (16); NOT at 14 -> COST = 15 exactly.
=> need exactly +1 step at the else-arm.
Removing either early return (lab) -> site 15. L_nonearret (return deleted, semantically
wrong lab) gate: alpha INLINED (display_assert/system_exit/literals now match), but flag
store refs 4/2 vs Jan 5/3 because the fallthrough overwrote them.
HYPOTHESIS H1 (single-exit law): January's near==far is an if/else (no return); VC7 jump-
threads the constant-FALSE flags past `if (flag||model_flag)` to the epilogue, which looks
exactly like an early return in the bytes. Test with the semantically-equivalent if/else.

## P2 c1.c = c0 + near==far as if/else (no early return)  ** KEY FINDING **
Evidence: single-exit law + P1 (early return costs 1 budget step; cost 15 vs site 14).
Gate: residual [size 4016!=3904, relocs 240!=235, sha]; relocation multiset now differs ONLY
by _csmemcpy 2 vs 7 (ruling D). real_alpha_to_pixel32 INLINED (assert literal
..\bitmaps\bitmaps_inlines.h + alpha>=0.0f&&alpha<=1.0f + display_assert/system_exit 7/7 match).
Bytes of near==far path in c1 (0x9fa..0xa0e): pop esi; pop ebx; mov [flag],0; mov [model],0;
pop edi; mov esp,ebp; pop ebp; ret == January 0x97f..0x993 exactly (VC7 jump-threads the
constant-FALSE flags through `if (flag||model_flag)` to a duplicated epilogue). So the
if/else is byte-indistinguishable locally AND is what January's inline decision requires.
This retires ruling A (no header change, no forceinline) - it was a .c-local source defect.

## P3 c1d.c = c1 + in-place float-bit reads (DIAGNOSTIC for ruling D; *(long *)& not admissible w/o owner)
Gate: [size 3888!=3904, sha] relocs 235==235, frame 0x184 vs 0x180, 1072 vs 1085 insns,
169 significant alndiff blocks (mostly small-slot permutation). Precedent for D: production
units.c:11494.. uses `*(long *)&unit->object.position.x` in an error() hex dump, and the /Od
build attests that idiom (fn 0x8f02b0 @0x8f05d5: in-place dword loads, no temps). /Od has no
fog-screen body (dx9 stub) so per-site /Od attestation for fog is impossible.
Small-slot map (slots.py): Jan vector/phase/point share -0x34..-0x28; ours vector -0x1c,
phase -0x40, point -0x3c. Jan -0x3c = inverse_depth + alpha scale + skinning.count;
ours inverse_depth alone at -0x80 (the +4 frame).
Color loop (sbs 0x792..0x952): Jan computes &colors[layer] LATE (lea between test ah/jnp),
never homes it; ours homes `color` ptr in [ebp-0x20] (0x7fb). Jan CSEs animation_times[index]
on x87 across the three colour stores (fld once, fld st(1)); ours reloads it 3x (store via
color-> pointer may alias the address-taken local array).

## P4 colour-pointer placement (diag on c1d)
a1d: direct global stores colors[layer].red... -> relocs 240 (6 per-field relocs) vs Jan 1 lea -> REJECT
     (January has ONE relocated lea to the colour array -> a pointer).
a2ad: `color = &colors[layer];` assigned right before the if -> 1082 insns, 179 blocks.
a2bd: `real_rgb_color *color = &colors[layer];` declared in EACH arm -> lea hoisted between
      test ah,44 / jnp exactly like Jan 0x879 (a2bd 0x86d); 1082 insns (was 1072), 178 blocks.
Remaining in arm: Jan loads animation_times[index] ONCE (x87 register value t) and computes
1-t, 2t (fadd st0,st0); ours fsub [mem] and reloads t 3x -> hypothesis: a register local t.
a3d (a2bd + `real animation_time = animation_times[index];` register local in the arm):
  [size 3872!=3904, sha], 1070 insns; t now loaded ONCE and 1-t via fsub st(1) (Jan 0x885..);
  residual inside arm: Jan keeps extra inv copy (fld st0;fld st0) and green = (t+t)*inv*density,
  ours reassociates *2 to the end. /FAsc layout (lay.py): all aggregates +4 vs Jan; small
  entries: ours vector(12) merged with the double temp + animation_time; Jan merges vector
  into the phase/point entry (-0x34, high-aligned) and keeps dbl temp+animation_time as an
  8-byte entry (-0x1c) -> the +4. prev_matrix/projection_scale share in Jan (-0x38), ours not.
Reseed: offset sites in Jan = set_real_point2d-inline packet (int copy of first random via ecx
  before fstp [p], lea eax=p, [eax+4]); recorded negative (structural diag_v8) - not repeated.

## P5 c2.c (ADMISSIBLE) = c1 + colour pointer declared per arm + `real animation_time` local
Gate: [size 4000!=3904, relocs 240!=235, sha]; alpha still inlined; multiset delta = csmemcpy only.
Frame-table instrument (Lane C P2 via structural lane) copied to p2/ and retargeted; ft_a3d.txt
= live table for a3d (ranks/refs). vector (rank 38, 10 refs) merges into dbl-temp entry
(rank 36) because phase/point (ranks 39/40) come later; Jan's layout needs vector's entry to be
joined by point+phase and the dbl temp to stay with animation_time.
Next probe (P6): flags local - Jan 0x99a loads fog.screen->flags ONCE as a word (mov cx,[edx])
and tests cl for both flags BEFORE the screen_flag store; ours reloads the byte (store to the
global may alias *screen) -> hypothesis `word flags = ...->flags;` in the else arm.
P6 a4d/c3: `word flags = global_window_parameters.fog.screen->flags;` in the else arm ->
  flags block now Jan's shape (single word load, cl tests, store after test cl,2); only the
  pointer register differs (Jan edx, ours ecx). a4d [size 3888!=3904, sha]; c3 [size 4016, relocs 240 (csmemcpy only)].

## P7 clear_z decoded (a5d diag / c4 admissible)  ** size now equal in diag **
January 0x9e7..0xa2b decode: pass!=0 -> FALSE; A=flags&1 (word, CSE'd in eax, tested TWICE),
B=flags&2 (cl): `!(A && B) && (A || (B && count>0) || (water && draw_water))` - the re-test of
A at 0xa00 is the second evaluation of A in the second conjunct. Semantically identical to the
old ?: form. Spelled with TEST_FLAG(screen->flags, ...) x4.
a5d (diag): `residual 3904 [sha]`  <- size 3904==3904, relocs 235==235. Block 0x9e7..0xa2b now
identical to January except the Clear flag register (Jan ecx, ours edx).
c4 (admissible): [size 4032!=3904, relocs 240!=235 (csmemcpy only), sha].

## P8 frame-table (ft_a5d.txt) + texture block decode
Entry composition vs Jan: (i) previous_matrix lands in the -4 pool (Jan: with projection_scale
-0x38); (ii) vector joins the dbl-temp entry (Jan: phase/point entry). Everything else same
entries, different placement.
Texture block decode (Jan 0x8cd..0x93f): u loaded FIRST (fld [u]) and stored 4th; v loaded
before [4] and stored after [5]; second row addressed through `lea eax,[ebp+edi-0x170]`
(= &texture_transforms[layer][4]); scale*cos and scale*sin CSE'd and spilled (fst).
HYPOTHESIS H-TV4: two inlined calls of this TU's own set_real_vector4d (January emits
_set_real_vector4d here, 32 B, exact) on real_vector4d rows: load of u/v must precede the
row stores (aliasing), pure x87 args sink into the stores -> Jan's order.
P9 a6d/c5: texture_transforms as `real_vector4d [LAYERS][2]` + two set_real_vector4d(&tt[layer][0|1], ...)
  calls (this TU's own January-emitted helper, inlined) -> a6d [size 3888!=3904, sha], 1076 insns,
  167 blocks (was 182); texture block now Jan's shape (lea eax=row1, fst scale*cos/-0x10, fst
  scale*sin/-4). c5 admissible [size 4016, relocs 240 csmemcpy-only].
P10 colour arm: a7d = no inverse_time local, `(1.0f - animation_time)` written in red and green
  (CSE'd) -> red sequence now Jan-identical (fld st0; fld st0; fmul st1 ... fstp st0).
  Green still reassociated (ours inv*t*density then fadd st0,st0; Jan fld st1; fadd st0,st0 (2t)
  first). a7bd (t*2.0f*inv*density with inverse local) -> no change.
P11 lab g2..g6 (green operand spellings): all identical - VC7 reassociates *2 to the end
  regardless of spelling -> green 2-insn x87 residual is not a spelling lever (do not repeat).
Screen-constants block decode (Jan 0x2ca..0x3c3): [0][0] stored first, then zero stores of
  [0][1],[0][2],[1][0], then [0][3]; [1][1] (-2/h) and [1][3] (1+1/h) stored LAST, after all
  row 2-4 constants and after vector.i/j/k=1,0,0. Ours stores row by row. Next: test rows as
  set_real_vector4d (this TU's helper) - same helper family as P9.
P12 a9d: screen_constants rows via set_real_vector4d -> byte-identical block to a7d (inert, not
  adopted). a10d: vector declared at pass0-block scope -> frame 0x18c (worse; escaped vector
  conflicts with all loop objects) - reverted.

## CHECKPOINT (admissible best = c6.c; diag best = a7d.c; c6 == a7d except the 5 csmemcpy bit temps)
c6.c  gate: residual [size 4016!=3904, relocs 240!=235, sha] - reloc multiset delta = +5 _csmemcpy only.
a7d.c gate: residual [size 3888!=3904, sha] - relocs 235==235, frame 0x184 vs 0x180, 1078/1085 insns.
Changes vs production (all January-byte-attested, see P2/P4/P6/P7/P9/P10):
 1. near==far density: if/else instead of early return (single-exit) -> alpha helper inlines.
 2. colour pointer declared per arm; `real animation_time = animation_times[index];`; (1-t) CSE'd.
 3. `word flags = global_window_parameters.fog.screen->flags;` for the two local flags.
 4. clear_z_buffer = !(A && B) && (A || (B && count>0) || (water && draw_water)).
 5. texture_transforms as real_vector4d[LAYERS][2] via this TU's set_real_vector4d (x2).
 (+ all structural-lane fixes carried in c0: shadow bug, D3DCLEAR_TARGET_A, predicates, scopes...)
Remaining diag residual: frame +4 (entry composition: vector joins dbl-temp entry; prev_matrix
joins -4 pool; scale*cos temp placement), first-time reseed order, green x87 reassociation,
screen-constant store interleave, register picks (edx/ecx/edi).

## P13 reseed decode (not repeating diag_v8)
diag_v8 bytes re-read: set_real_point2d(p, rand(), rand()) evaluates the call args left-to-right
(x=first) and computes p AFTER both calls. January first-time site: call1 -> temp, THEN lea edi=p,
call2 -> fstp [edi] (x), then [edi+4]=temp (y). => source order: first random into a local,
pointer computed, x from second call, y from the local. Offset sites: both calls, index after,
int-copy y via ecx before fstp x, lea eax=p, [eax+4]=y -> inlined setter with locals as args.
R1 lab: first-time body `real v = rand(); ptr = &...layers[layer]; ptr->u = rand(); ptr->v = v;`
R1 (r1d = a7d + first-time body `real v = rand(); layer_state = &...; layer_state->u = rand(); layer_state->v = v;`):
  [sha] (size equal); first-time loop now Jan-identical except `add esp,8`/`fstp [edi]` order;
  wind block registers now match Jan (only slot offsets differ); 150 blocks (was 165).
R2 (pointer local after both calls at offset sites) -> WORSE (stores reordered, [size 3920]) - rejected.
R3 (r3d = r1d + layers typed real_point2d; offset sites `real y = rand(); real x = rand(); short index = ...;
  set_real_point2d(&window->layers[index], x, y);` first-time R1 with ->x/->y) -> [sha]; offset
  sites now Jan-identical except the offset>0 index prologue (Jan mov cx,[esi]/dec cx then
  movsx ecx,[ebx+2]; ours ax + early layer_count load).
P14 rotation: Jan 0x3da `fpatan; fmul mult; fchs; fsubr [rotation]` = rotation -= -(angle*mult).
  rota (`-= atan2(...) * -screen->rotation_multiplier`) and rotb (`-= -(real)atan2(...) * mult`)
  are byte-identical and both reproduce Jan's 4 insns; kept rota (spelling ambiguous; semantics equal).
P15 ft_rota.txt: previous_matrix now merges with projection_scale (Jan -0x38) and the scale*cos
  CSE temp joins {result,z,bitmap_group} (Jan -0x10): both side effects of R1/R3. ONLY remaining
  frame defect: vector (rank 41, 12B) merges into the dbl-temp entry {animation_time, (real)layer
  temp, dbl temp}; Jan founds a vector entry joined by point+phase (-0x34).
P16 residual census (rota, slot-normalised, 53 blocks): frame(vector merge); first-time
  add esp/fstp swap; width/height regs (Jan ecx/edx vs ours edx/edi); projection_scale fmul order;
  screen-constant interleave; offset>0 index prologue regs; point-setup store order; green x87;
  texture loop-bound load position; texscale block store order; skinning regs; stats regs; tail nops.
  (vsh_1 block is identical modulo slots.)
P17 texscale [0][3] dot: Jan evaluates pos.x*fwd.i, pos.z*fwd.k, pos.y*fwd.j (x,z,y); the plain
  expression gives z,y,x. lab_dot.c: `dot_product3d((real_vector3d const *)&gwp.camera.position,
  &gwp.camera.forward)` reproduces Jan's block EXACTLY (b56..b88). => January used the header
  dot_product3d here. The point->vector view cast is a NEW representation cast (rule 24:
  owner-gated, needs per-site /Od attestation which does not exist for fog). Production
  precedent for the same cast idiom: decals.c:891, matrix_math.c:902/914, collisions.c:1400/1420.
  => OWNER ITEM E (diag only; not in admissible chain).
P18 C2.Dll merge rule re-read (disasm 0x1076b3ba, data only): entry scan most-recent-first down to
  [0x1088b57c] (= parameter count, not a scope bound); skip if entry-conflict-bitset has obj rank
  OR obj-conflict-set intersects entry members (0x107159fb) -> conflicts are SYMMETRIC.
  => with our object set Jan's vector/phase/point overlay + separate dbl entry is not derivable;
  January's object set/liveness must differ somewhere. Testing initializer/scope forms of phase/point.
P19 f1d (phase with aggregate initializer {0,0.7135,0.3422,0.5798}) -> layout unchanged (vector still
  in dbl entry). Frame work STOPPED here: vector-merge is the only frame defect; no source form found.

## P20 c7.c (ADMISSIBLE best) = c6 + R1 + R3 (layers real_point2d, set_real_point2d at offset sites) + rota
gate: [size 4032!=3904, relocs 240!=235, sha] - reloc multiset delta = +5 _csmemcpy only.
Surplus COMDATs: production base already emits 13 candidate-only code COMDATs (dot_product3d,
fast_ftol, real_alpha_to_pixel32 ... ); c7 adds _set_real_point2d (provider_link PASS vs
ai/path_obstacles). PRE-EXISTING object blocker: _real_local_random surplus is DIFFERENT from
January's selected provider effects.obj and FAILS the selected-provider link (LNK2005) on the
production base too -> NODUP-provider-link-law item, independent of screen_begin.

## P21 metric.py (slot-normalised LCS; unmatched January instructions of 1082)
production base 277 | c0 (structural lane) 204 | c1 180 | c6 138 | c7 100 | c8 105 (admissible,
no new COMDAT) | diag: c1d 141, a7d 97, r1d 70, r3d 64, rota 63, e1d 58, c8_D 68, c8_DE 63, c7_DE 58.
Production candidate chosen = c8.c (c7 minus set_real_point2d: R3 would add a NEW _set_real_point2d
COMDAT from a non-exact caller - owner ruling 5 requires a strictly exact caller).

## P22 Object-admission blockers independent of screen_begin
(a) object_audit on production base AND c8: 7 January symbols differ in storage (split 2 / ours 3):
    _D3DDevice_SetRenderState, _D3DDevice_SetTextureStageState, _IDirect3DDevice8_SetRenderState@12,
    _IDirect3DDevice8_SetTextureStageState@16, _IDirect3DDevice8_SetVertexData2s@16,
    _IDirect3DDevice8_Begin@8, _IDirect3DDevice8_End@4. pdb_storage.py: none is a cachebeta.pdb
    public -> file-static is correct; the split lacks `"static": true` (same fix precedent:
    rasterizer_xbox_sdk_names_and_private_reciprocal_20260906 + other objects' entries already static).
    Scratch csplit (cfg/ + split/): add `, "static": true` IN PLACE to config/symbols.json lines
    5766,5767,5769,5774,5779,5780,5781 (file_offsets 1396688,1397120,1397232,1397920,1398464,1398480,
    1398496) -> only the fog split object changes (833 objects compared), all section infos equal,
    object_audit symbols 78/78 match; remaining audit failure = screen_begin only.
(b) provider_link: surplus COMDAT _real_local_random DIFFERENT from its NODUP provider effects.obj ->
    LNK2005 (pre-existing on production base; NODUP-provider-link-law owner item).

## FINAL (2026-09-23)
Production candidate: candidate_production.c (== c8.c); patch: production.patch (git-apply verified
against the current production file; result byte-identical to c8.c, CRLF preserved).
  gate --all: exact 26 / residual 1 / unwritten 0; only row changed:
    before: residual 3904 screen_begin [size 4000!=3904, relocs 232!=235, sha]
    after : residual 3904 screen_begin [size 4032!=3904, relocs 240!=235, sha]
  relocation multiset vs January: +5 _csmemcpy ONLY (the float-bit temps, ruling D); every other
  target count equal (alpha helper now inlined: its assert literals, display_assert/system_exit,
  0.0/1.0 literals all match; previous_camera_matrix 3/3).
  slot-normalised unmatched January instructions: 277 (production) -> 105 (c8).
  no new COMDAT/surplus symbol vs production base (41/41 text symbols identical set).
  alpha site budget in c8: 15 (= helper cost 15) -> inlined.
Diagnostics (NOT landable): candidate_diag_DE_R3.c (= c7_DE: + D in-place float bits, + E
  dot_product3d point->vector cast, + R3 set_real_point2d at offset sites) -> [sha] only, size
  3904==3904, relocs 235==235, 58 slot-normalised unmatched insns, frame 0x184 vs 0x180.
Owner items: D (float-bit reads in place: `*(long *)&x` in the %f[%x] dump; units.c precedent,
  /Od-attested there), E (dot_product3d with (real_vector3d const *)&camera.position view cast;
  precedent decals/matrix_math/collisions), R3 (set_real_point2d would newly emit _set_real_point2d
  COMDAT from a non-exact caller - ruling 5 requires strictly exact caller; provider_link PASS).
Remaining machine differences in best diag (c7_DE): frame +4 (vector joins dbl-temp entry instead
  of phase/point entry - no source form found; conflicts are symmetric per C2 0x1076b3ba/0x107159fb),
  width/height/layer_count register picks (Jan ecx/edx, ours edx/edi), projection_scale multiply
  order (tan*far*aspect vs tan*aspect*far; decl order + 3 spellings inert), screen-constant
  zero-store interleave, offset>0 index prologue (cx vs ax), point-setup store order, green x87
  (2t first; 5 spellings inert), texture loop-bound load position, skinning pointer reload (ax vs
  cx), stats-block registers, first-time `add esp,8`/fstp order (setter form inert).
Do-not-repeat (this ledger): L_* loop ablations (lab), lab g2..g6, a9d (screen constants via
  set_real_vector4d, inert), a10d (vector at pass-0 scope, worse), R2 (pointer local at offset
  sites, worse), s4 (bare block over base-z, inert), f1d (phase initializer, inert), m1-m3 (decl
  order, inert), p1-p2 (projection expression shapes, inert), q1d (setter at first-time, inert).
