# `light_volumes.obj` interpolator recovery (2026-08-26)

## Scope

This wave resumes `source/objects/widgets/light_volumes.obj` from the retained
8/10 checkpoint documented in
`docs/object_matching_logs/light_volumes_obj_jonas_partial.md`.

- Integration base: `21b70d6a`.
- January authority: `build/split/source/objects/widgets/light_volumes.obj`.
- Starting state: 8/10 strict functions, 476/1,900 meaningful code bytes,
  560 strict padded code bytes, and 284/284 target non-code bytes.
- Residuals at the start: absent `_code_00124490` and absent
  `_light_volume_render`.

No protected Units/game-engine source, configuration status, semantic
allowlist, parked manifest, compiler flags, or target artifact changed.

## Evidence reviewed

The complete local history and the prior Jonas checkpoint were read before the
candidate was frozen.  The strongest Xbox donor is the source foundation from
`dca60279`; it preserves the original same-frame interpolation quirk and places
the helper and renderer in the same translation unit so VC7 can recover the
private calling convention.

The user-supplied `surreptitiousresearch/halocea` repository independently
provides:

- `src/blam/effects/light_volume_interpolate_frames.c` at
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, confirming that both blend
  endpoints intentionally read frame zero;
- `src/blam/effects/light_volume_render.c`, confirming the renderer's field,
  loop, exponent, and intensity topology;
- `src/headers/light_volume_frame.h`, confirming the 0xB0 frame layout.

January disassembly resolved cross-build differences.  In particular, the
January renderer calls `rasterizer_widget_set_texture` and immediately loads
the sprite count; it does not branch on the return value as the later HCEA body
does.

## Frozen compile and retained strict gain

The Xbox donor's static helper and renderer were restored together.  The helper
is exact only in this natural same-TU context: making it externally reachable
in the older isolated attempt changed its private ABI.

`_code_00124490` now compares strict exact:

| Measurement | Target | Candidate |
| --- | ---: | ---: |
| Meaningful bytes | 519 | 519 |
| Padded bytes | 528 | 528 |
| Relocations | 27 | 27 |
| Normalized SHA-256 | `99092a055fef...` | `99092a055fef...` |

The complete normalized SHA-256 on both sides is
`99092a055fef75f7f2f093807906ddf277d3d7b31d1a7235ddaddf2eedc9cae5`.
Every relocation address, COFF type, symbolic destination, addend, and section
owner is exact.  All eight previously accepted functions and all 284 non-code
bytes remain exact.

## Renderer measurement and bounded retry

The faithful renderer is retained as a nonexact source reconstruction.  It has
the same 47 relocation identities as January but remains 16 padded bytes long:

| Measurement | Target | Candidate |
| --- | ---: | ---: |
| Meaningful bytes | 905 | 927 |
| Padded bytes | 912 | 928 |
| Relocations | 47 | 47 |
| Normalized SHA-256 | `2e43dbc111ad...` | `a4e273c79398...` |

HCEA's later `distance_fade` lifetime supplied one genuinely new bounded retry.
Moving the initialization after marker acquisition did not reduce the
candidate: it remained 928 padded bytes, increased to 48 relocations, and
materialized `1.0f` through x87 instead of January's immediate integer store.
That form was rejected and the stronger 47-relocation Xbox donor shape was
restored.  No other renderer permutation was compiled.

## Widget API correction

The donor initially compiled with constraint-violation warnings because three
shared rasterizer declarations still described obsolete one-argument wrappers.
January's private callees read the following stack slots, and HCEA independently
supplies the same types:

- `rasterizer_widget_begin(short type, word flags)`;
- `rasterizer_widget_set_texture(short stage_index, long bitmap_group_index,
  short sequence_index)` returning `boolean`;
- `rasterizer_widget_draw_sprite3d(real_point3d const *point, real radius,
  real_vector2d const *scale, real rotation, unsigned long color)`.

The public/private declarations and forwarding definitions in
`source/rasterizer/rasterizer.h` and `source/rasterizer/rasterizer.c` now use
those signatures.  The renderer then compiles without warnings or incompatible
calls.  All three public rasterizer wrappers remain strict-exact 16-byte,
one-relocation tail jumps, so this correction changes source truth without
regressing machine code.

## Current unit and campaign result

`light_volumes.obj` is now 9/10 strict:

- 995/1,900 meaningful code bytes exact;
- 1,088 strict padded code bytes;
- 284/284 target non-code bytes exact;
- only `_light_volume_render` remains nonexact.

The campaign gain is 519 meaningful bytes and one strict function.  Regenerated
progress reports 509,924/2,198,102 code bytes and 4,224/11,060 functions overall;
Halo CE reports 497,010/1,770,166 code bytes and 4,057/7,574 functions.  Complete
objects remain 377/833 overall and 275/468 for Halo CE.

## Validation

- Full `halobetacache_build`, `libcmt_build`, ordinary progress, and semantic
  progress complete successfully.
- Semantic audit: 470 units, 4,332 functions evaluated, 4,189 semantic exact,
  4,250 accepted exact, and zero unit errors.
- Object admission: zero candidates, zero revocations, and only the inherited
  `shell_xbox` contradiction.
- Parked validation: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests pass.
- `git diff --check` passes.

The post-commit whole-TU regression snapshot/check covers both
`source/objects/widgets/light_volumes` and `source/rasterizer/rasterizer`.
