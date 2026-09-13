# `rasterizer_xbox_environment.obj` lightmap-draw reconstruction

## Admission decision

`_rasterizer_environment_lightmap_draw` is now a coherent reconstruction, but
it is **not exact and receives zero matching credit**.  January's function is
4,008 meaningful bytes / 4,016 padded bytes with 248 relocations.  The retained
candidate is 3,984 padded bytes with 246 relocations and a different normalized
hash.

The candidate replaces an unwritten owner with the full vector-debug and normal
environment-lightmap passes: texture and render state, the environment vertex
shader, animated self-illumination colors, the pixel-shader programs, drawing,
and frame-statistics accounting.  Structure and field names are supported by
the Halo CE Anniversary reconstruction at
`research/halocea/src/_rasterizer_environment_lightmap_draw_pp.c`, the symbol
build, January's field relocations and assertion strings, and donor commit
`74e015673` from the Opus rasterizer lane.  January remains byte authority where
the PC-family source differs.

## Inline-schedule evidence

January has two `_point_from_line3d` and four `_scale_vector3d` call
relocations.  The project already supported the former through the opt-in
`REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` declaration.  The analogous narrowly
scoped `REAL_MATH_EXTERNAL_SCALE_VECTOR3D` option was added to `real_math.h` and
enabled only for this translation unit.  This emits the four independently
proven calls without defining a `_point_from_line3d` COMDAT or fabricating a
foreign prototype.

The scale schedule improves the candidate from 4,016 padded bytes / 242
relocations / 2,745 differing normalized byte positions to 3,984 / 246 / 1,219.
It does not make the function exact.  The remaining structural boundary is two
January pixel-shader-field relocations plus broader local initialization and
source-order differences.  No compiler steering, hand-transcribed target
instructions, fake alias, or credit exception was used.

## Shared-owner correction

January relocations prove that offsets `0x24`, `0x28`, and `0x2C` in
`rasterizer_frame_statistics` are respectively the lightmap dynamic vertex,
triangle, and draw counters.  `rasterizer_frame_statistics.h` now names those
three fields while preserving every later offset and the record's total layout.

All eleven direct includers of that header and the separate Xbox model
statistics consumer were gated.  No inherited exact owner was lost.  The fog
lane independently gained two exact owners while these checks were running;
those gains are not attributed to this packet.

## Verification

- Final environment gate: 40 exact, 3 residual, 1 unwritten of 44; all 40
  inherited exact owners remain exact.
- `_rasterizer_environment_lightmap_draw`: residual, candidate 3,984/246 versus
  January 4,016/248; zero exact bytes credited.
- `_IDirect3DDevice8_SetTexture@12`: still unwritten.  The lightmap body does not
  naturally instantiate it, so no synthetic wrapper was added.
- `_point_from_line3d` emitted-symbol guard: pass for the environment object.
- All 64 direct `real_math.h` includers were compiled before and after the
  opt-in guard.  Exact-owner sets were unchanged.  Sixty-two pass the point
  guard; `action_charge.obj` and `vehicles.obj` have the same pre-existing
  `_point_from_line3d` COMDAT failure on both sides and were additionally
  compared without the guard.
- Final isolated object SHA-256:
  `EAC0CE738A922704540A8D723C9136E716594800E4802755BF03259A7E31C49C`.
- `git diff --check` passes for the three production files.
