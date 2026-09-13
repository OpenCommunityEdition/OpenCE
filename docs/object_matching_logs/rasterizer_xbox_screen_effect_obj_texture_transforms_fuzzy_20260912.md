# `rasterizer_xbox_screen_effect.obj` texture-transform owner reconciliation

## Result

The January private symbol `_code_0015fc90` is now identified as
`rasterizer_screen_effect_set_texture_transforms` and marked file-static in the
symbol atlas.  The name and operation are independently attested by the later
Halo CE Anniversary source reconstruction of the corresponding screen-effect
helper.  The retained Xbox implementation builds the four pairs of texture
coordinate transform constants for convolution, scanline, noise, and screen
stages, applies blur/warp/video-pass offsets, and uploads eight vertex-shader
constants.

This is a **coherent fuzzy reconstruction with zero exact credit**:

| metric | January | candidate |
|---|---:|---:|
| padded code bytes | 2,128 | 2,080 |
| emitted target owner | `_code_0015fc90` | semantic alias used for the audit gate |

The whole-TU gate reports `10 exact / 3 residual / 0 unwritten`, preserving all
ten inherited exact owners.  Relocation count and identity do not differ in the
gate verdict.  The prior accounting classified the function as unwritten only
because the complete source body emitted under the descriptive but unattested
name `rasterizer_screen_effect_set_vertex_shader_constants` while the target
atlas still used an address placeholder.

## Admission boundary

The production source now uses the cross-build semantic name directly; the
temporary gate alias exists only because the checked-in split target is not
regenerated until the batch-wide configure/build.  No body changes, compiler
overrides, inline controls, volatile/register forcing, barriers, assembly, raw
offsets, or undefined behavior were introduced.  The emitted-symbol guard confirms that
the candidate does not define `_point_from_line3d`, and the fake-match scan has
zero findings.

Do not tune the remaining 48-byte schedule/size difference without new
authoritative January source or local-variable evidence.  After the next full
configure/build, record the semantic owner in `config/parked.json` using the
fresh candidate hash and objdiff percentage; it remains worth zero exact bytes
until the strict gate closes.
