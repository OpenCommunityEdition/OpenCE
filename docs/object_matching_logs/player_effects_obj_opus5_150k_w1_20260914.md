# `player_effects.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

No change to `source/effects/player_effects.c`. Final gate equals baseline: 24 exact / 5 residual / 0
unwritten, guard passes, park drift 0 for `_player_effect_get_camera_effect_matrix`,
`_player_effect_update_camera_impulse`, `_player_effect_start`.

## `_player_effect_get_screen_flash` (800 target / 816 ours)

Every instruction and relocation aligns except one: January clears the just-started bit in place,
`and byte ptr [edi+0xe8],0xfe` (7 bytes), while VC7 emits `mov al,[edi+0xe8]; and al,0xfe; mov [edi+0xe8],al`
(14 bytes); the extra 7 bytes cross the 16-byte alignment. January itself uses load/op/store for the same field
elsewhere, and VC7 emits the in-place form for constant-FALSE `SET_FLAG` in exact functions such as
`texture_cache_bitmap_delete`, so this is a codegen-context choice. Five shapes (`&= ~FLAG`, `&= (unsigned)~FLAG`,
function-scope `effect`, `char flags`, `long flags`) did not change it (`long` also breaks two exact siblings).

Reopen criterion: a measured VC7 law for when a pointer-field byte `&=` is emitted in place.

## `_player_effect_add_continuous_effect` (320, sha only)

Only the x87 schedule of `((1-w) + p*w) * scale` after `periodic_function_evaluate` differs: January computes
`1-w` before `p*w` (`fld 1.0; fsub w; fxch st(1); fmul w`). `p*w + (1-w)`, `w*p + (1-w)` and a named call result
are compiler-identical. The parked camera matrix target uses the p*w-first order for its own term, so the two
January sites differ in expression tree, not commutation.

## `_player_effect_get_camera_effect_matrix` (parked, 1312 target / 1280 park base)

Scratch-only structural progress, no reopen (not exact):

- m1: scripted translation as `set_real_point3d(&matrix->position, j*vertical*intensity, i*horizontal*intensity,
  k*depth*intensity)` reproduces January's right-to-left product evaluation with the third random homed in
  `[ebp-4]` and the x/y/z store order.
- m2/m6: the impulse and shake `effect_scale_value` calls use `temporary_duration` / `camera_shake.duration`
  directly (January `fsubr [ebx+0x50]` / `fsubr [ebx+0x84]` from memory, no `duration` locals).
- m3: the impulse translation as two passes (`position = translation * direction`, then
  `position += jitter * scale`) restores the exact 1312-byte size.
- With m1+m2+m3+m6 (`scratch/workers/player_effects_particles_contrails/pe_m6.c`) the remaining differences are
  two operand loads in the inlined `cross_product3d(global_up3d, &effect->direction, &axis)` (they match without
  m1, so the set_real_point3d expansion flips them) and January keeping the x product on the x87 stack across the
  y/z stores. `scale_vector3d` + `point_from_line3d` (guard-failing, scratch only) does not reproduce that.

Reopen criterion: explain the m1/cross-product coupling and the impulse x-product forwarding, starting from pe_m6.c.

## `_player_effect_start` / `_player_effect_update_camera_impulse` (parked)

- start: the only difference is the i-term operand load of inlined `dot_product3d(&left, &delta)`. Declaration
  order, argument swaps for i and for j/k were compiler-identical.
- update_camera_impulse: the permanent-impulse dot products evaluate their terms in i, k, j order in January
  (ours k, j, i, which is also the order January uses in `player_effect_start`), plus two jitter cross-product
  operand loads; swapping the dot-product arguments was inert.

Parks are unchanged. No orchestrator proposal.
