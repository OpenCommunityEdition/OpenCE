# `render_debug.obj` unwritten-function packet (2026-09-12)

## Result

The January object, existing subsystem headers, and the Opus donor at
`608c66380` were used to reconstruct all seven previously unwritten functions
in `source/render/render_debug.c`. The donor was treated as a lead only. In
particular, the private renderers retain January's internal linkage, decal
angles use the first-party aggregate-initializer pattern already present in
`structure_runtime_decals.c`, and typed tag/object access goes through the
project macros.

Focused gate command:

```text
python tools/campaign/gate.py source/render/render_debug --all --forbid-emitted-symbol _point_from_line3d
```

Six functions are exact, adding **2,220 meaningful bytes / 2,256 padded
bytes** with every inherited exact owner preserved:

| function | meaningful | padded |
|---|---:|---:|
| `_render_debug` | 564 | 576 |
| `_render_debug_bsp` | 471 | 480 |
| `_render_debug_camera` | 542 | 544 |
| `_render_debug_input` | 108 | 112 |
| `_render_debug_structure` | 127 | 128 |
| `_render_debug_structure_decals` | 408 | 416 |

The emitted-symbol guard confirms that this translation unit does not emit
`_point_from_line3d`.

## Inline-owner boundary repair

The exact camera and BSP bodies inline three small math operations that the
January split does not assign to `render_debug.obj`.  Narrow controls in the
owning `real_math.h` therefore expose declarations, rather than inline bodies,
for `arctangent`, `dot_product3d`, and `plane3d_distance_to_point` in this
translation unit.  The two call sites retain the observed January arithmetic:
the camera uses the float result of `atan2`, while the BSP uses the ordinary
typed point/plane equation also recovered by the independent Ghidra pass.

The final focused gate forbids all four non-owned helpers:

```text
python tools/campaign/gate.py source/render/render_debug --all \
  --forbid-emitted-symbol _point_from_line3d \
  --forbid-emitted-symbol _arctangent \
  --forbid-emitted-symbol _dot_product3d \
  --forbid-emitted-symbol _plane3d_distance_to_point
```

It retains the same **29 exact / 7 residual / 0 unwritten** target-owner
classification.  `_render_debug_camera` remains exact at 542 meaningful / 544
padded bytes and `_render_debug_bsp` remains exact at 471 meaningful / 480
padded bytes.  All four emitted-symbol guards pass.  The three repaired helper
names are absent from the candidate owner set; ten older inline-helper owners
remain outside this narrowly scoped repair and receive no object-completion
credit here.

## Best complete fuzzy retained at zero exact credit

`_render_debug_player` is a complete and semantically coherent reconstruction,
but it remains explicitly **zero-credit fuzzy**:

| measurement | January | candidate |
|---|---:|---:|
| padded size | 272 | 272 |
| meaningful bytes | 266 | 0 credited |
| relocation count | 16 | 16 |
| normalized SHA-256 | `a45fe6649ea60e23685d2c15640529bc16e9720ff02715c1809b04bd0301f8d0` | `2cb093e3d436cd9fc015b77afe56b67514e29035a6b9069dd45508a2d2756a56` |
| aligned instruction similarity |  | 95.12% |

Both streams contain 82 instructions and have the same calls, branches,
relocations, and gameplay behavior. The remaining difference is stack layout:
January reserves `0x404` bytes and addresses the formatting buffer at
`[ebp-0x404]`; the natural candidate reserves `0x400` bytes and addresses it at
`[ebp-0x400]`. Artificially enlarging the buffer or introducing a dead local
could force the target frame, but would be unsupported source/codegen pressure,
so no such change was admitted.
