# Lane B worker report: source/physics/collision_debug

## Baseline (real source, verbatim gate output)

    residual   4192  _collision_debug_render  [size 4112!=4192, relocs 275!=278, sha]
    == exact 0  residual 1  unwritten 0  (of 1 listed)

candidate.c at start == byte-identical copy of source/physics/collision_debug.c (cmp SAME).
Workspace already held prior-campaign carry-forwards (not mine): prior_w3c_pF.c, prior_w6_best.c
(+ .obj/.aln), base.obj, base.aln. Both prior bodies re-gated at this HEAD:

    prior_w3c_pF.c   residual 4192 _collision_debug_render [sha]
    prior_w6_best.c  residual 4192 _collision_debug_render [sha]

prior_w6_best.aln: 1227 vs 1227 instructions, frame 0xb528 both. Remaining real differences:
(b) frame cells: 4-byte cell T-0x4c / O-0x5c  vs 16-byte cell T-0x5c..-0x50 / O-0x58..-0x4c
(c) column IV T-0x6c / O-0x70 vs (real)row temp T-0x70 / O-0x6c
(d) spray write loop rows i,j: T "fld row_real; fmul vertical_step.X" vs O "fld vertical_step.X; fmul row_real"
plus the three spray array relocation names (static split vs aggregate _collision_debug_spray_globals).

## Attempts

### A1-A4 (diagnostics on prior_w6_best.c, no candidate change)

- try_1 (w6 best with natural `collision_debug_point = render.camera.position;`): gate `[size 4176!=4192, relocs 277!=278, sha]`, 1219 insns. flags goes to EBX (setne bl directly), ignore_object_index to EDI. January: `xor edx,edx; setne dl; ... push esi; mov esi,edx` = flags in ESI with a byte temp in EDX.
- try_2 (try_1 + `flags = 0; SET_FLAG(flags, _collision_test_front_facing_surfaces_bit, ...)`): identical gate line and same EBX allocation. The flags initialiser spelling is INERT for the register choice.
- Allocator oracle (Lane A res9 tool copied to ./oracle, DIAGNOSTIC): w6 best flags web pri=38 (esi, batch 5, allowed esi,edi), ignore_object_index web pri=18 (ebx). Natural-copy variant: ignore web pri=24 (first term 2x14 instead of 2x11) is coloured BEFORE the pri-22 web and takes edi, flags pri=22 then only has ebx. So the set_real_point3d shape works only by lowering block pressure by 3 integer webs (struct copy temps) at the ignore_object_index definition. Threshold: pressure at that def must be <= 13 (natural = 14).
- try_3a/3b/3c (rename ignore_object_index / row,column): section hash identical to w6 best. Local NAMES are inert (name-hash tie-break hypothesis refuted).
- try_4a/4b/4c (write-loop index types long/short mixes): all worse (real blocks 183-189, size 4128-4176). `short row, column` stays.
- diag_m8_1..8 (dead-local M8 instrument on w6 best): D=1..7 byte-identical diff profile (32 real blocks / 42 rows), D=8 41 rows. The x87 row order (d) is M8-INSENSITIVE.

### A5-A6 (named real local for the row conversion; leaf-age diagnostics)

Evidence: January write loop rows i,j emit `fld T; fmul vertical_step.X` but row k emits `fld vertical_step.k; fmul T` (T = [ebp-0x70], the (real)row value stored in the OUTER loop). w6 best (hoisted compiler temp) emits v-first in ALL rows.
- try_5a: `real y = (real)row;` in the outer loop body, rows use `* y`. gate `[sha]` 1227/1227, real blocks 32 rows 41 (w6: 42). Rows i,j now match January (T first) but row k is now T-first too (January v-first). Frame slots (b),(c) unchanged.
- try_5b: 5a + `real x = (real)column;` : 1228 insns, 41 blocks/67 rows. Worse. Rejected.
- try_5c: 5a with `y * vertical_step.X` operand order: byte-identical to 5a (operand order inert).
- diag_6a (5a, rows reordered k,i,j): all rows still T-first. No last-row/dying-use effect.
- diag_6b (5a, vertical_step set via set_real_vector3d, i.e. fields NOT named before the loop): all rows v-first.
  => LEAF-AGE rule confirmed in this function: in a mem x mem product VC7 loads the YOUNGER leaf first; a field leaf's age is its first NAMED mention, a scalar's age is its declaration. January therefore needs age(v.i),age(v.j) < age(T) < age(v.k): vertical_step.i and .j named BEFORE T's symbol is created, vertical_step.k first named AFTER.
- diag_8a (5a with vertical_step.k zeroed through a cast pointer index): folded by the front end to the same leaf; all rows T-first (not a valid per-leaf test).
- diag_9a (5a; `set_real_vector3d(&vertical_step,0,0,0); vertical_step.j = expr;` so ONLY .j named before the loop): row i v-first, row j T-first, row k v-first. PER-LEAF first-named age CONFIRMED.
- diag_9b (w6 regime, implicit hoisted conversion, vertical_step un-named): v-first in all rows. The implicit hoisted temp is ALWAYS loaded second; January's T-first rows i,j therefore REQUIRE a named real variable holding (real)row.
- diag_7a (`real y;` declared at the top of the if(!repeat) block, assigned in the outer loop): same as 5a, so the scalar's age is its FIRST MENTION in a statement, not its declaration.
- diag_9c (7a + a dead `y = 0.0f;` placed between `vertical_step.j = ...;` and `vertical_step.k = 0.0f;`, DIAGNOSTIC ONLY): rows i,j T-first, row k v-first == JANUARY's exact pattern; real blocks 32 rows 40.
  => Byte fact decoded: in January's source a named real variable that later holds (real)row is first mentioned AFTER vertical_step.i and vertical_step.j are first named and BEFORE vertical_step.k is first named. The dead store itself is inadmissible (inert steering); a natural statement order with that property is required.

### A7 (phantom rows: removing the `real t` carrier via first-mention order)

Evidence: January phantom rows are `fld debug_vector.X; fmul collision.t; fadd debug_point.X` for X=i,j,k AND at 0x672 `fld debug_point.x; fadd debug_vector.i` (debug_point leaf YOUNGER than debug_vector leaf). Leaf-age law => first mention order collision.t < debug_vector.X < debug_point.X.
- try_10a `debug_point.x + collision.t * debug_vector.i`: phantom rows match, but 0x672..0x68b flip (debug_vector became youngest). 37 blocks/49 rows.
- try_10b `debug_point.x + debug_vector.i * collision.t`: 39/51.
- try_10c `point.x = collision.t * debug_vector.i + debug_point.x;` (no `t` local; point_from_line3d's own body with the product operands swapped): phantom rows AND 0x672 site match January. 33 blocks/43 rows: the only new difference vs w6 is the render-loop `lea ecx,[eax+ebx]` (January) vs `[ebx+eax]` (cause e), a local-count-sensitive commutative tie that the `t` local happened to flip.
- try_10d (same on the w3c pF body, shared `short row, column` for both nests): 33/43, lea still `[ebx+eax]`.
- try_11a/11b (chained zero assignments `vertical_step.i = vertical_step.k = 0.0f;` and reverse, on 5a): all rows T-first; chained forms still name both leaves before the loop.
- try_12a/12b (scope of the named `real y`: function scope / outer block): byte-identical profile to 5a (32/41). Scope is inert for slots (c).

### A8-A11 (camera copy device, aggregate control, combinations)

- try_14a (natural `collision_debug_point = render.camera.position;` + `set_real_vector3d(&collision_debug_vector, render.camera.forward.i/j/k)`): 32 blocks/42 rows == w6 profile. try_14b/14c (componentwise named copies of forward / position): 33/43 (render-loop lea flips). So ANY single scalarised camera copy gives January's esi/ebx roles; which one (or what else lowers the integer pressure by one struct copy) is not decidable from the bytes. NEEDS OWNER RULING if used.
- try_15a/15b (natural camera copies, scalarised local copy debug_vector / debug_point via set_real_*): 4176 / 4208, 152-208 blocks. Rejected.
- try_16a (w6 body with the single aggregate `collision_debug_spray_globals`): `[size 4176!=4192, relocs 276!=278, sha]`, 1217 insns, 61 blocks. Three separately addressed arrays are required for 278 relocations (config/symbols.json split dependency, owner-held class).
- try_17a (w6 + no `t` + t-first product + named y) and try_17b (pF + same): both `[sha]` 1227/1227, 33 blocks/42 rows.

## RESUMED RUN (2026-09-21, after usage-limit cut-off)

Unreported tries of the first run re-scored: try_18a/18b (= candidate at resume) `[sha]` 33 blocks/42 rows; try_19a `[size 4176]` rejected; diag_m8b_1..8 (dead-local instrument on named-y body) all 33/42 = insensitive.

### /Od evidence (FIRST consulted in this run; odbuild.func_start misses the function because its prologue is the aligned-stack form `push ebx; mov ebx,esp; ... 55 8B 6B 04`; real range 0x7af800..0x7b1ee1, dumped via od_manual.py -> od_collision_debug_render.txt)
- Last assert line /Od 0x1bc vs January 0x1bb: source is within ONE line of January's.
- RTC locals (first-party names): point, vector, vector_result(1048), collision_point, features(44040), rotation_matrix, rotated_points(96), cube_vector, collision(80), instance(16), **textstring(2048)**, position, velocity, new_position, new_velocity, collisions(704), o, p, i, j, k, bounds(16), v, collision, features, test_center.
- Statement shapes: `flags = 0;` + SET_FLAG for bit 0; ignore index set by a TERNARY (temp slot, two local_player_get_player_index calls, no player_index local); both camera copies are plain struct assignments; `scale_vector3d(&vector, fabs(collision_debug_length), &vector)` (no `length` local); real calls to point_from_line3d (phantom point, rotated points, cube centre `o`, test_center), set_real_vector3d x3 (i, j, k), matrix4x3_transform_vector x3; spray arrays indexed 2D (`imul row,0x1e0; imul column,0xc`); write loop and render loop use DISTINCT short row/column slots; /Od direction order is `column*i + k + row*j` but JANUARY's bytes are `(column*i + row*j) + k` (faddp then fadd k) so the 2020 source differs there.

### B1 try_20a natural camera copies + no `length` local: `[size 4176!=4192, relocs 277!=278, sha]` 1217 insns. Rejected (flags still lands in EBX).
### B2 try_20c ternary for collision_debug_ignore_object_index (per /Od): `[sha]` 33/42, byte profile unchanged -> kept (first-party shape, inert). try_20b = 20c + natural copies: `[size 4176...]` 1219 insns, rejected.
### B3 try_21a separate `short row, column` for the render nest (per /Od distinct slots): `[sha]` 33/42, inert -> kept.
### B4 try_22a **scoped `char textstring[2048]`** replacing the function-scope `char buffer[1000]`
Evidence: /Od RTC `textstring` size 2048 declared after `instance`; JANUARY `lea ecx,[ebp-0x920]; push 0x800` and vector_result at [ebp-0x538] (1048 B) and collisions at [ebp-0x3e0] (704 B) ALL end at -0x120 = three sibling-scope aggregates top-aligned, so January's buffer is a genuine 2048-byte nested-scope local overlapping its siblings. The old "original January bug: 1000-byte buffer, 2048 advertised" comment was a reconstruction artefact.
Gate before `[sha]` 33 blocks/42 rows -> after `[sha]` **2 blocks / 3 rows**. Frame cells (b) -0x4c/-0x5c and (c) -0x6c/-0x70 now match January. Remaining: (d) row k `fld v.k; fmul T` vs ours `fld T; fmul v.k`; (e) render nest `lea ecx,[eax+ebx]` vs `[ebx+eax]`. candidate.c := try_22a.c.
### B5 try_23a 2D spray arrays `[ROWS][COLUMNS]` indexed `[row][column]` (/Od: `imul row,0x1e0; imul column,0xc; lea [a+b+base]`): `[sha]` 2/3, byte-inert -> kept (first-party shape).
### B6 try_24a drop the `length` local (per /Od): `[sha]` 1225 insns 17 blocks. REJECTED: January keeps fabs(length) on the x87 stack (`fld; fabs; ... fmul st(1)` x3; `fstp st(0)`), which needs the named real. January-side proof that `length` exists (2020 source differs).
### B7 try_24b cube colour as an inline ternary argument (no `color` local; /Od uses a compiler temp slot, not a declared local): `[sha]` 2/3 inert -> kept.
### B8 try_24d real `point_from_line3d(&debug_point,&debug_vector,collision.t,&point)` call (first-party per /Od 0x7b0780): `[sha]` **1 block/2 rows** - phantom rows AND the render-nest `lea ecx,[eax+ebx]` (e) now match. try_24e (all four point_from_line3d sites as calls): same 1/2. BUT `--forbid-emitted-symbol _point_from_line3d` FAILS (our build emits the unreferenced COMDAT, section-size 48; systemic per header-inline-linkage-law). Not usable under the lane guard. Saved as try_24d.c / try_24e.c.
### B9 try_25a manual expansion of the helper body with its `t` parameter as a local (`real t = collision.t; point.x = debug_vector.i * t + debug_point.x;` = point_from_line3d's body verbatim): `[sha]` **1 block/2 rows**, guard PASSES. Strip (no `t`, try_24b): lea (e) flips -> the `t` IL value is required; it is the inlined helper's parameter (double attestation: /Od call + January lea/phantom rows). NEEDS OWNER RULING (helper call vs COMDAT guard). candidate.c := try_25a.c.
### B10 try_26a `const real size/step` cube locals (/Od stores two unread real slots): `[sha]` 1/2 inert, no January attestation -> NOT kept.
### B11 try_27a/b/c helper init and/or implicit (real)row on the manual base: `[sha]` 3 blocks/4 rows (rows v,v,v). Rejected there, but see B13.
### B12 try_28a `/ COLLISION_DEBUG_SPRAY_COLUMNS` and `/ ..._ROWS` (first-party: /Od `divss 40.0 / 30.0`): byte-identical to `* 0.025f` (VC7 folds to `fmul __real@3ccccccd`) -> kept.
### B13 DIAGNOSTIC (shift.py / diag_i8_*, diag_h_*): the row product order is an in-function SYMBOL-COUNT cycle of period 32
Implicit-conversion regime, D dead `long` locals: pattern walks vvv -> TTT -> TTv (=January) -> Tvv -> vvv, each window ~8 wide when the step vectors are set through set_real_vector3d. Unreferenced file-scope externs do NOT shift it; statements do not shift it; named locals, field leaves and inline-helper parameters do. So January = implicit `(real)row` (no named `y`, as /Od) + the first-party helper set. Dead locals are the instrument only, never landed.
### B14 try_29b/29c all four point_from_line3d as real calls + set_real_vector3d x3 + implicit conversions (+29c: NATURAL camera struct copies): `[sha]` 1227 insns, 1 block/2 rows (T,v,v). KEY: with the real helper calls the natural `collision_debug_point = render.camera.position;` no longer breaks the allocation (flags stays in ESI) - the set_real_point3d steering device is retired. try_34a (same but the 4 point_from_line3d sites manual): 1219 insns -> the helper calls are load-bearing.
### B15 try_30a magnitude3d(&collision_debug_vector) (first-party /Od fn 0x432950): byte-inert -> kept. try_30b const cube locals: inert, not kept. try_31a `flags = 0; SET_FLAG(bit 0)` (first-party): byte-inert -> kept.
### B16 try_32a `add_vectors3d((real_vector3d const *)&debug_point, &debug_vector, (real_vector3d *)&debug_point)` replacing three `+=` (first-party: /Od `call 0x44e8e0(&point,&vector,&point)`, body a[n]+b[n]->r[n]; cast precedent: collision_features.c:139, device_machines.c:341)
Gate: `[sha]` -> **`[reloc-identity]`**; relocdiff `shape: size 4192/4192, relocations 278/278, sha equal`, 7 differing rows = the three spray arrays vs `_collision_debug_spray_globals`+0/+14400/+28800. try_33a (one aggregate struct): `[size 4176!=4192, relocs 276!=278, sha]` -> three separately addressed statics are code-proven.
### FINAL candidate.c = try_36a.c (32a + brace restyle + magnitude3d + division spelling)
    residual   4192  _collision_debug_render  [reloc-identity]
    == exact 0  residual 1  unwritten 0  (of 1 listed)
unitcheck: gains=0 regressions=0 other=1. fake_match_scan: 0 leads.
Strip tests on the final body (each still `[reloc-identity]`): if/else+player_index vs ternary; named `color` vs inline ternary; `!= FALSE` vs flags=0+SET_FLAG; flat vs 2D arrays; reciprocal vs division; manual sqrt vs magnitude3d -> all inert, first-party spelling kept. Load-bearing: scoped textstring[2048]; `length` local; 4 point_from_line3d calls; add_vectors3d call; set_real_vector3d x3 + implicit (real)row; natural camera copies.
GUARD: `--forbid-emitted-symbol _point_from_line3d` FAILS (COMDAT section-size 48; cc.section_infos_equal == True vs January's single folded copy in action_charge.obj - the same criterion as the `_random_range` exception). newsyms OWNERSHIP FAIL: +_add_vectors3d +_magnitude3d +_magnitude_squared3d +_point_from_line3d +_scale_vector3d +_set_real_vector3d +_square_root (systemic inline COMDATs) +3 spray statics, -_collision_debug_spray_globals.
NEEDS OWNER RULING x2: (1) point_from_line3d COMDAT emission for this TU; (2) config/symbols.json split of `_collision_debug_spray_globals` into static normals(+0)/points(+14400)/hit_flags(+28800). Guard-passing fallback (fake set_real_point3d + named y + `t` local, `[sha]` 1 block/2 rows): try_25a_guard_safe_prev_candidate.c - NOT recommended.
