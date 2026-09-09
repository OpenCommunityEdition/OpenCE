# Devices and Rasterizer Lights: exact private-owner packet with retained draw frontier (2026-09-09)

## Result

This packet starts from published canonical commit
`6f8644c0cc7b7748f98b9a4624e89678ff5263c4` and selectively reconciles the
unfinished local lane `agent/environment-fog-next-20260909` at commit
`8358a7cc5c5e1b613bbea6e873e0a780b04c2faf`. The donor remains read-only and
was not merged wholesale.

Two target functions become strict exact with **1,010 meaningful / 1,024
padded code bytes** and zero inherited loss:

| Unit | Function | Meaningful | Padded | Relocations |
| --- | --- | ---: | ---: | ---: |
| `source/devices/devices` | `device_render_debug` | 426 | 432 | 27 |
| `source/rasterizer/rasterizer_lights` | `lens_flare_evaluate_corona_rotation_function` | 584 | 592 | 38 |

The complete `rasterizer_lens_flares_draw` reconstruction is retained as an
honest fuzzy body: 2,226 meaningful / 2,240 padded bytes, 115 relocations and
99.27634 percent objdiff similarity. It receives **zero exact credit** and has
an active hash-pinned record in `config/parked.json`. Neither object is marked
whole-object Matching.

## Source and ownership decisions

`device_render_debug` is the previously proven typed device renderer. It uses
the device accessor, group datum fields, the named debug flag, the original
power/position diagnostics and the genuine render-debug interface. January's
three-component point offset is written as its ordinary scalar expansion so
the object does not materialize the forbidden `point_from_line3d` COMDAT. The
public function and subsystem debug flag declarations now live in `devices.h`;
Object Types' consumer-local declaration is removed.

The lens-flare function name, enum values, tag layouts and behavior are
corroborated by the independently recovered HCEA symbol/type corpus under
`C:/Users/isabe/Documents/Codex/reference/halocea/src/`. January's split COFF
remains the byte and ABI authority. The retained code uses project `real`,
typed tag-block access, named reflection and animation flag bits, and the
authenticated 0xF0 lens-flare definition and 0x80 reflection layouts.

The private corona evaluator is naturally removed by VC7 when it has no
caller. Retaining the complete genuine draw caller makes the evaluator's
ordinary static ownership and exact January body observable. The draw body is
not distorted to close its remaining instruction-scheduling differences.
It uses no address names, raw field offsets, pointer punning, fake branch,
helper alias, declaration trick, forced inline, compiler steering or assembly.

The natural math calls cause VC7 to retain 64-byte `cross_product3d` and
48-byte `subtract_vectors3d` COMDAT copies which the linked January split
assigns elsewhere. They are not credited and keep Rasterizer Lights outside
whole-object admission. A scalar-expansion trial removed those owners but
made the exact evaluator 128 padded bytes too large and was rejected. The
explicit campaign guard confirms that `point_from_line3d` is not emitted.

The public lens-flare draw and occlusion declarations are moved from Render's
consumer-local prototype block to the narrow `rasterizer_lights.h` interface.
`rasterizer_sun_glow_draw` is declared there because its Xbox implementation
has no dedicated header. `uncompress_int8_to_real` is exposed through the new
narrow owner header `rasterizer_geometry_compression.h`, included by both its
implementation and this consumer. Putting that declaration in broad
`rasterizer_geometry.h` changed the otherwise exact 480-byte
`rasterizer_dynamic_geometry_initialize`; the narrow genuine owner interface
restores it with no loss.

## Rejected exact-looking Devices candidate

`device_add_scenario_information` remains its existing natural 288-byte fuzzy
body. Widening the private `device_group_new` flag formal from the datum's
`word` to `unsigned long` makes the caller exact, but January reads and stores
only a word and ordinary argument promotion already occupies a four-byte stack
slot. HCEA and the independent PC reconstruction also use 16-bit flags. With
no new January type evidence, the width change would repeat the documented
optimizer-only inference rejected in
`devices_obj_opus1_four_function_reconciliation_20260906.md`; it is not
admitted or credited here.

The incomplete occlusion-test body and the semantically incorrect reset
variant from earlier donor experiments are likewise not imported.

## Verification

- Isolated strict gates: Devices 28 exact / 2 residual / 0 unwritten;
  Rasterizer Lights 9 exact / 3 residual / 1 unwritten.
- Both complete candidate objects pass
  `--forbid-emitted-symbol _point_from_line3d`.
- A dependency audit compiled all 56 initially affected transitive consumers.
  Replacing the broad geometry declaration with the narrow owner interface
  reduced the final changed-header set to 12 consumers; the previously
  perturbed Draw Primitives owner was rebuilt explicitly as a sentinel.
- Whole-tree stable verdicts: 6,695 to 6,697 strict functions, exactly the two
  intended gains, 1,024 padded bytes and **zero regressions** across 8,245
  target owners.
- Park validation: 351 active, zero stale, zero invalid.
- Fake-match scan over all changed source and owner-interface files: zero
  review leads.
- `git diff --check`: clean.

The root integrator must still run the full Ninja, semantic, test, admission
and publication gates on the combined batch. This worker does not push.
