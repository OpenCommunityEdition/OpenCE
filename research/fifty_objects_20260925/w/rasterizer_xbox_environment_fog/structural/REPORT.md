# rasterizer_xbox_environment_fog structural lane (2026-09-23)

Baseline (candidate = copy of source at worktree HEAD): exact 26 / residual 1 / unwritten 0.
`residual 3904 __rasterizer_environment_fog_screen_begin [size 4000!=3904, relocs 232!=235, sha]`
Baseline frame: January sub esp 0x180, ours 0x1c4.

## Attempt 1: fix shadowed local (REAL BUG, UB)
The BSS split (4f4c8502) renamed the aggregate field to a file static `previous_camera_matrix`,
but screen_begin declares a LOCAL `real_matrix4x3 *previous_camera_matrix = &previous_camera_matrix[...]`
whose initialiser refers to itself (uninitialised read); every later `previous_camera_matrix[...]`
in the pass-0 block hit the local. January references `_previous_camera_matrix` 3x
(T 0xeb `add eax,_previous_camera_matrix` -> [ebp-0x38]; 0x122 loop pointer; 0x23a final csmemcpy); ours 0x.
Fix: local renamed `previous_matrix` (used only by matrix4x3_inverse).
Gate: `residual 3904 __rasterizer_environment_fog_screen_begin [size 4016!=3904, sha]` - relocs now 235==235.
Other 26 rows unchanged (full row diff). KEEP.

## Attempt 2 (v2_windscope.c): wind_matrix/vector/previous_matrix scoped into the wind block (extends through rotation)
Evidence: January wind_matrix -0x7c shares bytes with vsh_constants__texscale (-0x78) and animation_times (-0x58); vector -0x30 shares with phase/point.
Gate: `[size 3984!=3904, sha]`, frame 0x1c4 -> 0x1bc. KEEP (fuzzy).

## Attempt 3 (v3_phasescope.c): phase[] declared inside the first layer loop body
Evidence: January's four phase stores sit in the loop PREHEADER (T 0x515..0x52a, after `jle`, before loop head 0x531) = LICM-hoisted
loop-body stores. Diagnostic (in-place float bits, see D) reproduces the hoist exactly; with csmemcpy(&phase[layer]) the escape blocks LICM.
Gate: `[size 3984!=3904, sha]`, frame 0x1b0. KEEP (fuzzy).

## Attempt 4 (v4_aggs_active.c): matrix/screen_constants/texture_transforms declared at the `if (active)` block
Evidence (/FAsc listing, live frame table copied from Lane C P2 into ./p2): with them in pass-0 scope vsh merges with MATRIX (ours);
January has vsh merged with WIND_MATRIX and matrix/screen/texture unshared -> they interfere with vsh -> enclosing scope.
Diag (in-place bits) after v4: texture -0x194, matrix -0x114, screen -0xe0, wind -0x80 + vsh -0x7c (January's pairing).
Gate: `[size 3984!=3904, sha]`, frame 0x1b0. KEEP (fuzzy).

## Attempt 5 (v5_orshort.c): `short offset` + `base_z < 0.0f || base_z >= 1.0f`
Evidence: T 0x436 `test di,di`; T 0x4b6 `test ah,5 / jnp` (ordered <) then `test ah,1 / jne ok`. Whole block T 0x436..0x501 now aligns.
Gate: `[size 4000!=3904, sha]` (size up 16 but block aligned). KEEP.

## Attempt 6 (v6_period.c): TWO REAL FIELD/PREDICATE BUGS
- loop 1: January `fld [ebx+0x48]; fcomp 0.0; test ah,0x41; jne else` = `screen->animation_period > 0.0f` (ours `!= 0.0f`).
- loop 2 colour branch: January tests `[ebx+0x48]` = animation_period (`test ah,0x44; jnp`) = `screen->animation_period != 0.0f`;
  ours tested `screen->far_density` ([ebx+0x10]) - wrong field.
Gate: `[size 4000!=3904, sha]`; both compare blocks now align. KEEP.
Diag diag_v6.c (v6 + in-place float bits, NOT admissible): `[size 3872!=3904, relocs 230!=235]`, frame 0x194, 1057 vs 1085 insns.

## Attempt 7 (v7_widthheight.c): `short width/height` viewport differences, assigned after the wind block
Evidence: T 0x257..0x27a `mov ax,[x1]; sub ax,[x0]` (16-bit) + `sub ecx,edx; movsx edx,cx` (height), then ONE `fdivr 1.0`
reused (`fld st0; fadd st1` = 2/w, `fld -1.0; fsub st1`) - VC7 CSEs the reciprocal only when the divisor is a short local.
Diag reproduces January's sequence instruction-for-instruction (modulo edi/edx). Gate `[size 4000!=3904, sha]`. KEEP.
Probe p_tancast.c: `(real)tan(...)` reproduces January's `fxch st1; fxch st1` pair (T 0x2b9). KEEP. Multiply order far/aspect
still reversed (ours aspect first); swapping source operand order is INERT (p_tancast2.c) - unresolved, not pursued.

## Attempt 8 (diag_v8.c, NOT kept): inline set_real_point2d at the 3 reseed sites (layers as real_point2d)
Reproduces January's int-copy+lea packet, but VC7 evaluates the inline args left-to-right (first random -> x),
January has first random -> +4. Same negative as the 100K lab; do not repeat.

## Attempt 9/10 (v10_vshscope.c): SEPARATE LOCAL `vsh_constants__texscale_1[VSH_CONSTANTS__TEXSCALE_COUNT - 1][4]`
Evidence: January's own rasterizer_error string stringifies the call argument as `vsh_constants__texscale_1`
("...VSH_CONSTANTS__TEXSCALE_OFFSET+1, vsh_constants__texscale_1, VSH_CONSTANTS__TEXSCALE_COUNT-1)"), and T 0xdd6
`lea ecx,[ebp-0x68]` = a 32-byte object at -0x68..-0x48. This 32-byte object is what lets animation_times(16) and
vsh(48)/wind(52) coalesce into one slot entry (merge rule new<=2*entry: 16 -> 32 -> 48 -> 52). vsh_constants__texscale
(48) moved into the `if (pass == 0)` texscale block (it must be scope-disjoint from vsh_..._1).
Diag diag_v10.c (+ in-place bits): wind@-7c vsh@-78 vsh_1@-68 anim@-58 = JANUARY'S EXACT OFFSETS; texture -0x184 (Jan -0x180),
frame 0x184 vs Jan 0x180 (+4 only). Admissible v10: `[size 3984!=3904, sha]`, frame 0x1a0 (the +0x1c is the csmemcpy bit locals, ruling D).
Full row diff vs baseline: only the screen_begin row changed. KEEP -> candidate.c.

## Attempt 11 (v12_clearalpha.c): REAL BUG - Clear flags
January T 0xa3b `or ecx, 0x80` = D3DCLEAR_TARGET_A (alpha only; the pass writes COLORWRITEENABLE_ALPHA); ours `or ecx,0xf0` = D3DCLEAR_TARGET.
Fix: `D3DCLEAR_TARGET_A | (clear_z_buffer ? D3DCLEAR_ZBUFFER : 0)`. Gate `[size 3984!=3904, sha]`, rows otherwise unchanged. KEEP.
Probe diag_v11b (clear_z as one `pass == 0 && (b0 ? !b1 : ...)` expression): gives January's zero-extended word load but not its
branch graph (January re-tests flags&1 after the b1 test); NOT kept. Residual: January loads `fog.screen->flags` ONCE as a word
(T 0x99a `mov cx,[edx]`) for both local flags where ours issues two byte loads - likely TEST_FLAG macro spelling (cseries.h), not pursued.

## Attempt 12 (v13_skinorder.c): skinning count stored before pointer
Evidence: January T 0xe79 loads node_matrix_count ONCE (`mov cx,[esi+0x64]; test cx,cx; ... mov [ebp-0x3c],cx`) and stores it
before the pointer in BOTH arms (0xe82/0xe91). With pointer-first our count is reloaded after the store; count-first restores
the CSE. Gate `[size 3984!=3904, sha]`, sha 0cb6e6b0.. KEEP (fuzzy).

## Alpha-inline site budget re-measured (probe_site.py, diag_v13): budget still 14 (probe_h14 inlines, h15 calls),
unchanged from Lane C; real_alpha_to_pixel32 costs 15-17 -> ruling A still blocks. None of the structural fixes moved it.

## FINAL (candidate.c == v13_skinorder.c + line wrap, byte-identical sha 0cb6e6b0)
`residual   3904  __rasterizer_environment_fog_screen_begin  [size 3984!=3904, sha]`   == exact 26 residual 1 unwritten 0
Provider link: `no surplus external definitions: PASS (nothing to link)`. Full row diff vs baseline: only this row changed.
Relocation multiset now differs ONLY by the two held rulings:
  A (inline real_alpha_to_pixel32): Jan +2 strings, +1 __real@0, +1 __real@3f800000, +1 display_assert, +1 system_exit; ours +1 _real_alpha_to_pixel32
  D (float bits read in place): ours +5 _csmemcpy.
Frame: admissible 0x1a0 (was 0x1c4); diagnostic with D applied (diag_final.c) 0x184 vs Jan 0x180, and every aggregate
at January's offset +/-4 (wind -0x7c, vsh -0x78, vsh_1 -0x68, anim -0x58 exact). Remaining +4 = a lone inverse_depth slot
(January shares -0x3c with skinning/alpha-scale; the alpha inline A changes the small-slot colouring).
Unresolved non-ruling residuals: TEST_FLAG word-vs-byte flag load (T 0x99a), clear_z branch graph, far/aspect multiply
order, reseed arg order (left-to-right inline eval), constant-1 pinned in edi (LAW Z), chicago pointer = verify result (ebx).

## Attempt 13 (v16/v17 -> candidate.c): nested chicago branch, chicago = shader_get_and_verify_type(...)
Evidence: January T 0xd30 `mov ax,[ecx+0x24]; cmp ax,4; jne 0xe3c`, then `call _shader_get_and_verify_type; mov ebx,eax` and ebx
(the VERIFY RESULT) supplies flags (+0x28), map.index (+0xb0) and u/v scales (+0x9c/+0xa0); the else-if at 0xe3c tests only 3 and 8
(never 4 again) and the no_fog path jumps straight into the shared B tail at 0xe48. Our flat form re-read group->shader and
re-tested type==4. A switch spelling gives `sub eax,3/dec/sub` (diag_v15, rejected). The nested form reproduces T 0xd20..0xe5c
instruction-for-instruction, incl. January's constant-1 pinned in edi. NOTE for review: B (PSTextureModes(1)+ALPHAKILL disable)
appears in two source arms (VC7 cross-jumps them into January's single block) - flagging in case the owner treats that as duplication.
vsh_constants__texscale_1 declared inside the pre-existing bare block (byte-identical to declaring it in the if-body).
Gate: `residual   3904  __rasterizer_environment_fog_screen_begin  [size 3936!=3904, sha]` (1083 vs 1085 insns), frame 0x1a0.

## FINAL (supersedes the earlier FINAL block): candidate.c == v17_blockdecl.c
`residual   3904  __rasterizer_environment_fog_screen_begin  [size 3936!=3904, sha]` ; `== exact 26  residual 1  unwritten 0  (of 27 listed)`
Provider link: `no surplus external definitions: PASS (nothing to link)`. Rows: only screen_begin changed (residual->residual), 26 EXACT kept.
Reloc multiset delta = ruling A (inline alpha helper) + ruling D (5 csmemcpy) only. Diag diag_final.c (candidate + D): 3808 B, frame 0x184, 1051 insns.
Held for owner: A (header cost of real_alpha_to_pixel32: site budget 14, cost 15-17), D (in-place float-bit reads; diag proves
frame/LICM consequences). No header/config proposal.
