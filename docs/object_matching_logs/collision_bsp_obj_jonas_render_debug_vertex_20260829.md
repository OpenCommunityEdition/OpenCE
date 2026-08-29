# `collision_bsp.obj` Jonas render-debug vertex (2026-08-29)

This additive ledger records the first bounded production candidate for
`_render_debug_collision_vertex` in `source/physics/collision_bsp.c`. January
COFF is authoritative; the HCEA body is supporting semantic provenance only.

## Scope and provenance

- Integration base: `cb670110d9302906ed9c6101c49cb9eedb08a32b`.
- Baseline/final `collision_bsp.c` blobs: `9b1c969689d8da289762cd1a9a95f7f943f2f9ba`
  / `627e7b61a542a5397b040557ca5c8bddbbf0892e`.
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
object ledgers, and retained worktrees found no prior production implementation
or bounded experiment for this symbol. The HCEA body proves the vertex-block
lookup, optional transform, scale, color, and debug-point call topology.

January disassembly independently proves the typed vertex block at `+0x54`
with stride `0x10`, a 12-byte local point, and three `REL32` calls. The point
passed to the renderer is the tag lookup's return when `matrix` is null and
`matrix4x3_transform_point`'s return otherwise.

## Validated baseline

Before this lane the object had 13/30 strict functions, 2,487/10,292 meaningful
code bytes, and 2,608/10,544 padded code bytes. The target vertex renderer has
71 meaningful bytes, 80 padded bytes, three relocations, and normalized SHA-256
`51356c752f66132ff719ac76249335c42f339c8a7e6645f2d5890baa97f85b20`.

The target owns 176 non-code bytes; 16 bytes were already exact before this
lane. No data definition or ownership changed, and no additional data credit is
claimed.

## Accepted control and experiment

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Typed vertex lookup; use `&vertex->point` directly when untransformed and the named matrix helper's returned pointer when transformed; render once with the forwarded scale/color | `80/80` | `3/3` | `51356c752f66132ff719ac76249335c42f339c8a7e6645f2d5890baa97f85b20` | none | `13/13` prior exact functions remain exact | retained |

The retained source declares all locals before statements, accesses
`bsp->vertices` through `TAG_BLOCK_GET_ELEMENT`, uses named types and fields,
and explicitly returns. It contains no layout cast, raw byte offset, instruction
steering, or compiler control.

## Do not repeat and residuals

- Do not replace the typed vertex lookup with `bsp+0x54` arithmetic or cast the
  vertex record to a point.
- Do not discard the transform helper's returned pointer and materialize the
  local address separately; January keeps the helper return in `EAX`.
- Do not retry alternate branch, declaration, or pointer-lifetime spellings for
  this already exact body.
- Do not use assembly, `volatile`, register keywords, pragmas, intrinsics,
  barriers, undefined aliasing, object patches, or compiler-flag changes.

The vertex renderer has no residual. Sixteen other functions remain absent and
were not investigated by this lane; their classification is unchanged and this
note does not park them.

## Disposition and validation

`_render_debug_collision_vertex` is strict exact and eligible for ordinary
function credit. `collision_bsp.obj` advances to 14/30 strict functions,
2,558/10,292 meaningful code bytes, and 2,688/10,544 padded code bytes. It
remains `NonMatching`.

- Hardened comparator: exact normalized bytes and exact relocation type,
  address, target, and addend for all three calls.
- Disassembly gate: all instructions and the nine-byte padded tail agree.
- Whole-TU gate: 14 exact, zero residual, 16 unwritten; all 13 prior exact
  siblings remain exact.
- Aggregate build/report gates: pass (`halobetacache_build`, `libcmt_build`,
  `semantic_progress`, and `progress`). The post-edit incremental run completed
  four actions after the lane's clean 479-action setup build.
- Semantic report: 470 units, 4,855 functions evaluated, 4,728 semantic exact,
  129 hidden exact / 76,388 bytes, one ordinary-only function, one ordinary
  structural function, zero rejected functions, and zero unit errors.
- Progress report: 382/833 complete objects; global code 596,087/2,198,102
  bytes and 4,699/11,060 functions; Halo code 583,173/1,770,166 bytes and
  4,532/7,574 functions; data 1,856,018/4,176,062 bytes.
- Python tool tests: 205/205 pass.
- Admission audit: zero candidates, contradictions, or revocations.
- Parked audit: 12 active ledgers, zero stale, zero invalid.
- `git diff --check`: pass.
