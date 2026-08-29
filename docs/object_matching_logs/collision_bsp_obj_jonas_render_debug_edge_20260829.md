# `collision_bsp.obj` Jonas render-debug edge (2026-08-29)

This additive ledger records the first bounded production candidate for
`_render_debug_collision_edge` in `source/physics/collision_bsp.c`. January COFF
is authoritative; the HCEA body is supporting semantic provenance only.

## Scope and provenance

- Integration base: `34c5255de7c3b0adc7c2301ce48c1f909a7b94eb`.
- Baseline/final `collision_bsp.c` blobs: `7c37697266fd67b180faa787fc0c396a28d0223d`
  / `3d9ad556a9f49f26dd1dbdda7b3165a78c65bcb7`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `collision_bsp.obj` SHA-256:
  `ebed4b7ca91a0f2e3cd73a5f286c6d3cdcc33cf2127996b1d3f3afe56b2cf879`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the unchanged XDK 3911
  and repository source include paths.
- HCEA source oracle: `afc14e5b1d016560ac5808a7da5aa9f2adb2b885`,
  `src/halo/physics/collision_bsp.c` blob
  `4b5b30271497014aedac9bcfd293a5f981f04487`.

A search of the exact-reconstruction repository's complete history, current
object ledgers, and retained worktrees found no prior production body or
bounded experiment for this symbol. Earlier exact commits contain only the
prototype required by the already exact callers. Retained non-exact trees hold
copies of the same HCEA lift, not an independently validated exact candidate.

HCEA identifies the edge and endpoint lookup topology and the optional pair of
transforms. January independently proves the typed edge block at `+0x48` with
stride `0x18`, `vertex_indices[0]` and `[1]` at offsets `+0x00` and `+0x04`,
the vertex block at `+0x54` with stride `0x10`, and two 12-byte transform
scratch points. It also corrects HCEA's stale return-type annotation: January
keeps each `_matrix4x3_transform_point` return value as the point subsequently
passed to `_render_debug_line`.

## Validated baseline

Before this lane the object had 15/30 strict functions, 2,664/10,292 meaningful
code bytes, and 2,800/10,544 padded code bytes. The target edge renderer has
121 meaningful bytes, 128 padded bytes, six relocations, and normalized SHA-256
`1f53e20af0f11a0cf1c1510213dc1d02820b206984e5623cc760a35d252cbcfd`.

The target has `REL32` calls at `+0x17`, `+0x27`, and `+0x35` to
`_tag_block_get_element_with_size`, at `+0x4C` and `+0x59` to
`_matrix4x3_transform_point`, and at `+0x6B` to `_render_debug_line`. The
target owns 176 non-code bytes; 16 bytes were already exact before this lane.
No data definition or ownership changed, and no additional data credit is
claimed.

## Accepted control and experiment

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Typed edge lookup; typed endpoint vertex lookups in index order; optional transforms assigned from the helper returns; one forwarded debug-line call | `128/128` | `6/6` | `1f53e20af0f11a0cf1c1510213dc1d02820b206984e5623cc760a35d252cbcfd` | none | all 15 prior exact functions remain exact | retained |

This was the only production candidate. All declarations occur at the start of
the function's C89 block, all structure access uses named fields and types, and
the function explicitly returns. It contains no layout cast, raw byte offset,
instruction steering, compiler control, or object manipulation.

## Do not repeat and residuals

- Do not replace `bsp->edges`, `bsp->vertices`, or their named element types
  with base-plus-offset arithmetic.
- Do not swap `vertex_indices[0]` and `[1]`; January preserves endpoint order.
- Do not discard either transform helper's return and assign the scratch
  address separately; January retains the helper returns in `EBX` and `ESI`.
- Do not retry alternate declaration, transform, or point-selection forms for
  this already exact body.
- Do not use assembly, `volatile`, register keywords, pragmas, intrinsics,
  barriers, undefined aliasing, object patches, or compiler-flag changes.

The edge renderer has no residual. Fourteen other functions remain absent and
were not investigated by this lane; their classification is unchanged and this
note does not park them.

## Disposition and validation

`_render_debug_collision_edge` is strict exact and eligible for ordinary
function credit. `collision_bsp.obj` advances to 16/30 strict functions,
2,785/10,292 meaningful code bytes, and 2,928/10,544 padded code bytes. It
remains `NonMatching`.

- Hardened comparator: exact normalized bytes and exact relocation type,
  address, target, and addend for all six calls.
- Disassembly gate: all 64 instructions and the seven-byte padded tail agree.
- Whole-TU gate: 16 exact, zero residual, 14 unwritten; all 15 prior exact
  siblings remain exact.
- Aggregate build/report gates: pass (`halobetacache_build`, `libcmt_build`,
  `semantic_progress`, and `progress`) in four incremental actions.
- Semantic report: 470 units, 4,857 functions evaluated, 4,730 semantic exact,
  129 hidden exact / 76,388 bytes, one ordinary-only function, one ordinary
  structural function, zero rejected functions, and zero unit errors.
- Progress report: 382/833 complete objects; global code 596,314/2,198,102
  bytes and 4,701/11,060 functions; Halo code 583,400/1,770,166 bytes and
  4,534/7,574 functions; data 1,856,018/4,176,062 bytes.
- Python tool tests: 205/205 pass.
- Admission audit: zero candidates, contradictions, or revocations.
- Parked audit: 12 active ledgers, zero stale, zero invalid.
- `git diff --check`: pass.
