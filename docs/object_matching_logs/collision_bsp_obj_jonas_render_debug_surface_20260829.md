# `collision_bsp.obj` Jonas render-debug surface (2026-08-29)

This additive ledger records the first bounded production candidate for
`_render_debug_collision_surface` in `source/physics/collision_bsp.c`. January
COFF is authoritative; the HCEA body is supporting semantic provenance only.

## Scope and provenance

- Integration base: `ea0f48ef8ffbad27a61c2158b194832da41839aa`.
- Baseline/final `collision_bsp.c` blobs: `627e7b61a542a5397b040557ca5c8bddbbf0892e`
  / `7c37697266fd67b180faa787fc0c396a28d0223d`.
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
or bounded experiment for this symbol. The only exact-tree occurrence before
this lane was the unwritten symbol in the translation-unit inventory.

HCEA identifies the circular edge walk and forwarded debug-render arguments.
January independently proves the typed surface block at `+0x3C` with stride
`0x0C`, `first_edge_index` at surface offset `+0x04`, the edge block at
`+0x48` with stride `0x18`, the side-1 surface comparison at edge offset
`+0x14`, and selection of `edge_indices[reverse]` from edge offset `+0x08`.
The already exact `_collision_surface_edge_count` supplies an in-TU typed-field
code-generation witness for the same walk topology.

## Validated baseline

Before this lane the object had 14/30 strict functions, 2,558/10,292 meaningful
code bytes, and 2,688/10,544 padded code bytes. The target surface renderer has
106 meaningful bytes, 112 padded bytes, three relocations, and normalized
SHA-256
`8e4cbab6741ce96d73383147cad8fa7b6d12ae97fb604ff378900ec868e4b775`.

Its relocations are `REL32` calls at `+0x15` and `+0x2F` to
`_tag_block_get_element_with_size`, and at `+0x4E` to
`_render_debug_collision_edge`. The target owns 176 non-code bytes; 16 bytes
were already exact before this lane. No data definition or ownership changed,
and no additional data credit is claimed.

## Accepted control and experiment

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Typed surface lookup; circular `do` walk; compare `surface_indices[1]`; render the current edge; advance through `edge_indices[reverse]` | `112/112` | `3/3` | `8e4cbab6741ce96d73383147cad8fa7b6d12ae97fb604ff378900ec868e4b775` | none | all 14 prior exact functions remain exact | retained |

This was the only production candidate. It follows the January control flow
and the established collision structure definitions directly. All declarations
occur at the start of their C89 blocks, the parameters are typed, and the
function explicitly returns. It contains no layout cast, raw byte offset,
instruction steering, compiler control, or object manipulation.

## Do not repeat and residuals

- Do not replace `bsp->surfaces`, `bsp->edges`, or their named element types
  with base-plus-offset arithmetic.
- Do not compare `surface_indices[0]`; January reads `surface_indices[1]` and
  uses that boolean as the `edge_indices` subscript.
- Do not rotate the circular `do` loop or move the edge advance before the
  render call; the retained natural form is already exact.
- Do not use assembly, `volatile`, register keywords, pragmas, intrinsics,
  barriers, undefined aliasing, object patches, or compiler-flag changes.

The surface renderer has no residual. Fifteen other functions remain absent
and were not investigated by this lane; their classification is unchanged and
this note does not park them.

## Disposition and validation

`_render_debug_collision_surface` is strict exact and eligible for ordinary
function credit. `collision_bsp.obj` advances to 15/30 strict functions,
2,664/10,292 meaningful code bytes, and 2,800/10,544 padded code bytes. It
remains `NonMatching`.

- Hardened comparator: exact normalized bytes and exact relocation type,
  address, target, and addend for all three calls.
- Disassembly gate: all 54 instructions and the six-byte padded tail agree.
- Whole-TU gate: 15 exact, zero residual, 15 unwritten; all 14 prior exact
  siblings remain exact.
- Aggregate build/report gates: pass (`halobetacache_build`, `libcmt_build`,
  `semantic_progress`, and `progress`) in four incremental actions.
- Semantic report: 470 units, 4,856 functions evaluated, 4,729 semantic exact,
  129 hidden exact / 76,388 bytes, one ordinary-only function, one ordinary
  structural function, zero rejected functions, and zero unit errors.
- Progress report: 382/833 complete objects; global code 596,193/2,198,102
  bytes and 4,700/11,060 functions; Halo code 583,279/1,770,166 bytes and
  4,533/7,574 functions; data 1,856,018/4,176,062 bytes.
- Python tool tests: 205/205 pass.
- Admission audit: zero candidates, contradictions, or revocations.
- Parked audit: 12 active ledgers, zero stale, zero invalid.
- `git diff --check`: pass.
