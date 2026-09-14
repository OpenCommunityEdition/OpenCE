# `decals.obj`: Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Unit `source/effects/decals.c` -> January `build/split/source/effects/decals.obj`; XDK 3911 CL 13.00.9254 via
  `tools/campaign/gate.py` (PID-safe, guard `--forbid-emitted-symbol _point_from_line3d`).
- Lane `opus/150k-house-clean-20260914` (base `12f7375d4`), worker `w1:decals`. Worker log: `scratch/workers/decals.md`;
  candidates under `scratch/workers/decals/`.
- Orchestrator scope: `_decal_clip_to_surface`, `_decal_sprite_get_bounds`, `_decal_update` (park), skip
  `_render_debug_decals` and `_decal_new_from_collision`.
- Evidence: January COFF (authoritative), HCEA `decal_update.c`, `decal_sprite_get_bounds.c`,
  `decal_clip_to_surface.c` (topology only), projectiles.obj precedent (`seconds_per_tick`), a January-wide x87
  byte-pattern census over `build/split` (`scratch/workers/decals/negscan.py`, `mulneg.py`), and the previous lane's
  k1..k14 clip candidates.

## 2. Baseline and final real-file gate

`== exact 28 residual 5 unwritten 0`, guard passes, both at baseline and final. **The real file is unchanged**
(`git diff --stat` empty). Parked `_decal_update`: no drift. Owner census of the real file equals `build/base`.
Fake scan: the single pre-existing `__asm` lead (`real_a_rgb_color_to_pixel32`).

| function | January padded/relocs/sha16 | baseline candidate | outcome |
|---|---|---|---|
| `_decal_update` (PARKED) | 400/25/`dcc6a629481fabd9` | 384/26/`1ab3a89f52527a51` | **strict EXACT in scratch; park reopen proposed** |
| `_decal_sprite_get_bounds` | 384/16/`e77622d865444b0c` | 384/16/`9658324e079e3b4b` | not landed (x87 NEG hoisting) |
| `_decal_clip_to_surface` | 1776/59/`d875457fa2f02176` | 1776/59/`2cc51c87ea496852` | not landed (scheduling tie) |
| `_decal_new_from_collision` | 6176/210 | 6048/203 | skipped (point_from_line3d policy) |
| `_render_debug_decals` | 432/27 | same sha | skipped (semantic tier) |

## 3. `_decal_update` park reopen (scratch candidate)

Candidate: `scratch/workers/decals/park_reopen_decal_update.c` = the current real file plus only the reopened body and
the data object that body reads. Whole-TU gate `== exact 29 residual 4 unwritten 0`; the only row change is
`_decal_update` residual -> EXACT (400/25/`dcc6a629481fabd9`); guard passes; no other exact row changes.

January facts that refute the park premise or satisfy its criterion:

1. The fade assertion (line 322, `f>=0.0f && f<=1.0f`) pushes the **decals.c** file literal, not
   `..\bitmaps\bitmaps_inlines.h` as the parked body wrote.
2. `sub esp,8` and `fld [ebp-4]; fmul 255.0f; fstp [ebp-4]; fld [ebp-4]; fistp dword [ebp-8]; mov dl,[ebp-8]` are the
   inlined shared `fast_ftol(real)` helper from `cseries.h` (parameter home + dword result). The park's reopen criterion
   is "a natural same-compiler helper/math spelling that emits the dword conversion"; the admitted shared helper is
   that spelling: `decal->intensity = (byte)fast_ftol(f * 255.0f);`.
3. `fld elapsed; fcomp lifetime; test ah,5; jnp` is the ordered `elapsed < lifetime` test. The body is
   `if (decal->lifetime == 0.0f || elapsed < decal->lifetime) { fade } else { expire }`, the same control structure as
   HCEA `decal_update.c`. The parked `!=`/`>=` form emits `test ah,1; jne`.
4. The 1/30 multiplier is read from `.rdata` at `_decal_wrap_parameters+64`: January's non-COMDAT 68-byte section holds
   the four wrap-parameter records followed by `89 88 08 3d`. `static real const seconds_per_tick = 1.0f /
   TICKS_PER_SECOND;` defined right after the table reproduces that section byte-for-byte and the relocation. This is
   the same pattern already admitted in `projectiles.c:370`.

Shapes: u1 (1+2) -> 400/25, residual polarity plus the 1/30 owner; u2 (+3) -> reloc-identity only; u3 (+4) -> EXACT.

Owner census of the candidate against `build/base` and `build/split`:
- a candidate-only `_fast_ftol` select-any COMDAT, the systemic boundary already recorded for hud_draw, hud_unit and
  player_rumble in `config/object_admission_rejections.json`;
- a static label `_seconds_per_tick` inside the January-identical `.rdata` section;
- the `__real@3d088889` COMDAT is no longer emitted (January decals has none).

## 4. Not landed

- `_decal_sprite_get_bounds`: January negates the registration-point field before multiplying:
  `fld field; fchs; fmul st(1)`. VC7 hoists the negation: `fld st(0); fmul field; fchs`.
  - Census over every January split object: `fchs` directly followed by `fmul` occurs in only three functions:
    - this one;
    - units `_code_0019c460`, where the NEG sits under a double conversion: `(real)((-time_extension)*fabs(...))`;
    - ui_widget `_draw_bitmap_in_rect`, which negates a double `_CIfmod` result.
  - `fmul` directly followed by `fchs` occurs in 31 functions, 16 of them already exact in our build.
  - No same-compiler float donor exists. Removing the scale locals, with the products spelled inline, grew the function
    to 400 bytes (rejected).
- `_decal_clip_to_surface`: re-gated the previous lane's k1 (bare-block previous/current points, frame 0x6c). Two
  scheduling differences remain:
  - January creates the zero temp before storing `input_points`;
  - in the loop tail, January schedules the `input_points` store and the iteration load differently.
  New cleanup probe: replacing the block-scope `convex_polygon2d_clip_to_plane` prototype with its genuine owner
  `#include "math/geometry.h"` leaves clip unchanged, but it breaks `_decals_delete_permanent_from_cluster`
  (declaration count). Rejected.

## 5. Do-not-repeat

- sprite_get_bounds: negation respellings (two in the 100K lane) and scale-local removal (this wave).
- clip_to_surface: k2-k14 (100K lane); geometry.h include in decals.c (breaks delete_permanent_from_cluster).

## 6. Orchestrator proposals

- A. Unpark `_decal_update` with the candidate body and `seconds_per_tick` (dedicated commit). Verify that the full
  build's parked/semantic gates accept it.
- B. If whole-object owner accounting requires it, add a decals `_fast_ftol` select-any COMDAT rejection entry to
  `config/object_admission_rejections.json` mirroring the hud_draw/player_rumble entries. No header change is needed.

## 7. Reopen criteria

- `_decal_sprite_get_bounds`: authentic type/source evidence of a non-float intermediate in the extent expression, or a
  same-compiler float donor with NEG before MUL.
- `_decal_clip_to_surface`: authentic declaration/scope evidence for the clip-loop preheader, starting from
  `scratch/workers/decals/k1.c`.
