# LEDGER: source/rasterizer/xbox/rasterizer_xbox_environment (round 2 ATTACK, r2w1)

Base f6d00a8c (worktree HEAD b7c1b7d9 = scratch-only commit on top). Claimed 2026-09-24 as
r2w1-rasterizer_xbox_environment "ATTACK". Inputs: triage scratch/w/triage_rasterizer_b/LEDGER.md P4-P9,P13,P15.

## Probe log
- R0 baseline (f6d00a8c production): gate 42 exact / 2 residual (spot 944!=960 sha; lightmap_draw 3984!=4016 relocs 246!=248).
  Copied triage labs od.c (/Od shape: real_matrix4x3 light_matrix, separate normalize3d stmt, inner_radius local)
  and odz_d2.c (od + 2 initialised dead fillers = M8 oracle instrument). Harness lab/probe.py (gate --all + alndiff,
  real hunks = hunks that differ beyond the split-vs-defined literal annotation). Reproduced triage:
  od = 11 real hunks (0xf3..0x113 x87 j/k flips + rotation 0x12d..0x154 + lea 0x1b5/0x1bd); odz_d2 = 6 real hunks
  (rotation family only). 248/248 insns both.
- R1 NEW EVIDENCE (emission order): January's .text order for this object is: 6 SDK wrappers; then the 18
  non-deferred functions (all *_end, reflection_lightmap_mask_draw, reflection_mirrors_begin, reflections_begin,
  transparent_geometry_begin); then 20 DEFERRED functions in definition order: lightmaps_begin, lightmap_begin,
  lightmap_draw, diffuse_lights_begin, diffuse_light_begin, diffuse_light_draw, diffuse_textures_begin,
  diffuse_texture_draw, specular_lights_begin, **spot_light_begin**, specular_light_begin, specular_light_draw,
  specular_lightmaps_begin, specular_lightmap_begin, specular_lightmap_draw, reflection_lightmap_masks_begin,
  reflection_lightmap_mask_begin, reflection_mirror_draw, reflection_draw, transparent_geometry_submit.
  Ours compiles in plain definition order with lightmap_draw first and spot_light_begin second-to-last (only
  specular_light_begin deferred). By the round-1 deferral law, January's TU had 20 functions calling a
  not-yet-defined TU function, and spot_light_begin was compiled in the deferred group right after
  specular_lights_begin. The allocator arena/free-list state is TU-cumulative (P3 lane-C E6/E8), so the
  rotation may be a compile-ORDER (TU state) effect rather than a defect in the spot body. -> test.
- R2 LAB compile-order test: moved the spot body to January's emission slot (defined immediately before
  _rasterizer_environment_specular_light_begin, so it compiles right after specular_lights_begin): od_mv 11 hunks,
  odz_d2_mv 6 hunks = IDENTICAL to unmoved. Compile position/order is INERT for the rotation.
- R3 LAB TU name-count oracle (P3 lane-C law; 1-name `struct env_probe_i;` units, K=0..45) at two positions
  (immediately before the spot function; at /* public code */ after the globals) on odz_d2: all 92 compiles
  6 hunks, same addresses, 42 exact kept. The rotation is NOT an arena-ordinal/name-count tie (and the triage
  H3 1..16 extern probe is now extended to 0..45 single names at both region boundaries). => the rotation is a
  genuine allocator decision driven by the function's own IL.
- R4 LAB REAL_MATH_EXTERNAL_NORMALIZE3D (P8 fix) on od with M8 fillers D=0..7: identical hunk pattern to od
  (D=2 -> 6 hunks, rotation only). The extnorm object-level fix is byte-inert for spot and does not move M8.
- R5 LAB nested normalize3d(cross_product3d(...)) + half local, D=0..7: identical to separate statement form
  (nesting inert in the half-local context too; /Od separate form stays).
- R6 ALLOCATOR ORACLE (Lane A res9 alloc-oracle copied to lab/oracle, WT retargeted; stock CL under dbg32c,
  nothing on disk patched): odz_d2 spot has only 9 GLOBAL-allocator decisions, all model-consistent (R0-R3 9/9):
  id35 eax pri192 (=&light->forward CSE), id38 ecx pri192 (=&light->up), id13 esi (light), id10 edi (gel),
  id40/36/26 = pixel-shader constant webs 0xff/0xc00/0xd5, id9 eax (definition for gel, crosses blocks), id22 esi.
  The rotating temps (definition reload @0x12d, position.x/y/z, &vertex_constants lea) are NOT global webs:
  they are chosen by C2's block-local allocator, which the Lane A model does not cover.
- R7 Sibling pattern census (January bytes): exact point-light arm of specular_light_begin +0x110..0x132 has
  def=ecx, px=edx, py=eax, pz=ecx, lea edx = EXACTLY our spot pattern; exact diffuse_light_begin +0x2c6..0x2e3
  (no def reload) has px=ecx, py=edx, pz=eax, lea ecx. January spot: def=eax, px=ecx, py=edx, pz=eax, lea ecx.
  In all four, the local temps follow a cyclic eax->ecx->edx order (vc lea reuses px's register), i.e. January's
  spot enters the post-normalize block with the local cursor one step 'behind' ours.
- R8 LAB inner-block scoping (blk1: `{ real radius = ...; real inner_radius = ...; real cone_scale = ...;
  real_vector4d vertex_constants[5]; ... }` after normalize; blk2: only vertex_constants in an inner block like the
  exact diffuse sibling) x D=0..7: identical to od (M8 pattern and rotation unchanged). Scoping INERT.
- R9 LAB statement-order census of the matrix block (F=forward copy, U=up copy, X=cross, N=normalize) at D=2:
  FUXN (od) 6 hunks; UFXN 15; XFUN/XUFN 30 (232 insns); FXUN 35 and UXFN 30 but THEIR post-call temps are January's
  eax/ecx/edx/eax/ecx. => the post-call local-register cursor is set by pre-call IL, but every order other than
  /Od's FUXN breaks the (currently exact) pre-call bytes. Not a landing; mechanism evidence only.
- R10 C2 dive: the local temps are not created through C2 0x10716266 (x87/esp/ebp operand maker, 99 hits) nor the
  0x10723700 fixed-list picker (candidate tables 0x1088e388={ecx,edx}, 0x1088e390={ecx}, 0x1088e394 3-bit map);
  global chooser 0x10716988 fires 9x (= the 9 global webs). Local allocator not located (441 regnode refs in C2).
- R11 LAB public XDK wrapper spellings (IDirect3DDevice8_SetVertexShaderConstant(global_d3d_device,...) as used by
  lightmap_draw; IDirect3DDevice8_SetTextureStageState / D3DDevice_SetTextureStageState for the six stage-1 states;
  both) x D=0..7: all identical to od. Inline params of LATER helpers do not move M8 or the rotation.
- R12 LAB filler POSITION: D initialised dead longs assigned right before the matrix block or right before
  `radius =` (after the cross product) give the SAME M8 pattern as fillers at the function top (D=2 closes the x87
  flips at every position). => the M8 key is the function's total IL-local count, not first-use order.
- R13 /Od census of the DX9 sibling family: 0x7db5d0 (diffuse_light_begin), 0x7e35b0 (specular_light_begin) and
  0x7e4590 (spot) all carry the DX9 `byte [ebp-1]=1` + "### ERROR <fn> failed" success flag; the exact Xbox
  siblings have none (inert there) so it is a DX9 port artifact with no Xbox witness.
  LAB `boolean success= TRUE; ... if (!success) error(_error_silent, "...failed");` = exactly +1 IL local
  (flag + D=1 closes the x87 flips; rotation unchanged). Not enough alone (+2 needed) and not January-attested.
- R14 LAB M8 accounting decoded further (oracle fillers, c = construct count mod 8, od = 0): a scalar local +1;
  a struct local with one member access `.i` +2, `.j` +3; array `a[0]` +2, `a[0],a[1]` +3; vertex_constants via
  named members (real_vector4d/quaternion/plane, plain or union type) c=0, via flat real[20]/[5][4]/.n[] c=5;
  TU-local matrix/vector4d type STRUCTURE inert; cross_product3d body locals inert for M8 (params first). Fillers
  anywhere in the function count equally. No /Od-attested construct found that is +2 (DX9 flag is +1).
- R15 C2 LOCAL ALLOCATOR DECODED (live, dbg32c; nothing on disk patched). Hardware write-watch on the operand
  register field of the px/py/pz dest operands (dynamic watch armed at node-ctor hits #160/#167/#174 found via the
  post-RA tracker 0x1074d885) -> writer 0x107562ca `mov [ebp+0x1c],ecx` after `call 0x10756554` (owner lookup);
  owner table 0x1088b1d8[reg] written by the CHOOSER at 0x1075676d (function ~0x10756684): candidate list =
  [0x1088b96c] (cursor into static list 0x10894708 = {eax,ecx,edx,esi,edi,ebx,ebp,0}); first entry with reg<4 and
  owner==0 (and bitset check 0x1088b0b0[reg]) is TAKEN; then 0x107567db stores the cursor = entry AFTER the taken
  register (wraps to eax). Fixed/global registers are PINNED (owner=regnode self, 0x10756332) WITHOUT moving the
  cursor; cursor reset to eax at function start (0x10757fac). => VC7's block-local temps are ROUND-ROBIN from the
  last TAKEN register.
  Trace for ours (od2): reset; pins(eax); copy1 TAKE ecx; fwd.i edx; fwd.j edx; fwd.k ecx; pin B ecx; copy2 edx;
  up.k edx; [A,B freed]; ARG lea TAKE eax (cursor->ecx); def2 TAKE ecx; px edx; py eax; pz ecx; vc edx.
  January = exactly the same sequence with the normalize3d ARG PINNED instead of TAKEN (cursor stays at eax after
  up.k): def2 eax, px ecx, py edx, pz eax, vc ecx - reproduces all 6 rotation hunks. So January's
  `&light_matrix.left` argument was a GLOBAL/pinned web (a symbol or a benefit>0 constant web), not a local temp.
- R16 **COUNTERFACTUAL EXACT (oracle only)**: od2 (= /Od shape + D=2 M8 fillers) compiled under dbg32c with ONE
  forced cursor store (C2 0x1075680d, 7th local TAKE in the function = the normalize3d argument lea; eax forced
  0x1089470c->0x10894708 so the round-robin cursor does not advance) => `_rasterizer_environment_specular_spot_light_begin`
  STRICT EXACT vs January: 960/960, 64 relocs, normalized sha e0331f6b94a74f9c == January (lab/oracle/forced_7_10894708.obj).
  PROOF: the /Od-attested body is complete; the whole residual = (a) the M8 IL-symbol count (+2 mod 8) and (b) one
  local round-robin step: in January the `&light_matrix.left` argument did NOT consume a local TAKE (it was a pinned
  / pre-assigned register operand). Both are IL facts with no byte signature.
- R17 LAB pinned-argument search (all +M8 oracle D sweep, cursor/regs read back): pointer local
  `left = &light_matrix.left` (early or late), `left = cross_product3d(...)`, `magnitude = normalize3d(...)`, TU-local
  __inline wrapper around normalize3d (with and without REAL_MATH_EXTERNAL_NORMALIZE3D): every one keeps the argument
  as a local TAKE (rotation unchanged); pointer/magnitude locals are +1 M8 each; wrapper params are M8-inert.
  Stop rule reached for the spot row: ~30 evidence-backed shapes, residual proven to be exactly one local-allocator
  IL fact + the M8 count; no genuine January construct found. Preserve the /Od shape at zero credit.
- R18 CANDIDATE (zero credit): cand/cand_spot_odorder.c = production + /Od spot shape with /Od declaration order
  (byte-identical to the od order, measured). gate 42 exact / 2 residual, identical exact-name set (no swaps),
  point_from_line3d guard pass; spot 960/64 sha a0b9f9f4383b30bc (production 944/64). Forced counterfactual re-run
  on THIS shape (+D=2 instrument, one cursor force) = EXACT sha e0331f6b (January).
  patches/p1_spot_od_shape.patch (git apply --check OK). patches/p2_object_prereqs.patch
  (REAL_MATH_EXTERNAL_NORMALIZE3D define/undef + static permutation index; 42 exact kept; removes the Rule-6
  _normalize3d surplus, remaining surplus COMDATs identical to January's selected copies) - prerequisite only:
  alone it ADDS an audit storage row (perm index split 2 / ours 3) until symbols.json lines 5722-5727, 5755, 22727
  get "static": true; apply only with the emulated-split audit when the object becomes admissible.
## FINAL (r2w1)
- spot_light_begin: FUZZY_IMPROVED (not exact). Residual proven = M8 +2 IL-symbol count + one local round-robin
  step (normalize3d argument pinned in January). No genuine construct found after ~30 shapes -> stop rule.
- lightmap_draw: unchanged OWNER hold (astra intensity.c exact, stage-0 per-arm filter witness missing).
- object: blocked (spot, lightmap_draw, 8 storage rows via symbols.json, TU-view packet).
