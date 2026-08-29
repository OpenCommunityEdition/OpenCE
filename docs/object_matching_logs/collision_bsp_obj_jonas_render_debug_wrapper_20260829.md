# `collision_bsp.obj` Jonas render-debug wrapper (2026-08-29)

This additive ledger records the one-shot recovery of
`_render_debug_collision_bsp` in `source/physics/collision_bsp.c`. January
COFF remains authoritative; the HCEA lift was used only to confirm the loop's
semantic intent.

## Scope and provenance

- Integration base: `1c9fb1573e548a7f60d5800e6d0bae2ed75823a9`.
- Baseline/final `collision_bsp.c` blobs: `e484b9c76718514d996943b6f0085b3c668d643a`
  / `9b1c969689d8da289762cd1a9a95f7f943f2f9ba`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `collision_bsp.obj` SHA-256:
  `ebed4b7ca91a0f2e3cd73a5f286c6d3cdcc33cf2127996b1d3f3afe56b2cf879`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 and source include paths.
- HCEA source oracle: `afc14e5b1d016560ac5808a7da5aa9f2adb2b885`,
  `src/halo/physics/collision_bsp.c` blob
  `4b5b30271497014aedac9bcfd293a5f981f04487`.

The January target proves a signed `long` induction variable, the edge count at
the typed `bsp->edges.count` field, and two relocations: an absolute load of
`_global_real_argb_green` at `+0x16`, followed by a `REL32` call to
`_render_debug_collision_edge` at `+0x1F`. The HCEA lift independently describes
the same guarded for-loop and forwarding of the matrix argument.

## Validated baseline

Before this lane the object had 12/30 strict functions, 2,436/10,292 meaningful
code bytes, and 2,544/10,544 padded code bytes. The wrapper was absent. The
target wrapper contains 51 meaningful bytes, 64 padded bytes, two relocations,
and normalized SHA-256
`d301f94e3e3d4b94bc79f55b076c02434b6cb7efae87d58dd6a98e8ad836d302`.

The target owns 176 non-code bytes; 16 bytes were already exact before this
lane. No data definition or ownership was changed, and no additional data
credit is claimed.

## Accepted control and experiment

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Typed `for` loop from zero through `bsp->edges.count`, forwarding `bsp`, the signed edge index, `matrix`, and `global_real_argb_green` to the declared edge renderer | `64/64` | `2/2` | `d301f94e3e3d4b94bc79f55b076c02434b6cb7efae87d58dd6a98e8ad836d302` | none | `12/12` prior exact functions remain exact | retained |

The retained body uses the repository's named `collision_bsp` and
`real_matrix4x3` types, declares the loop variable before statements, and has
an explicit `return`. It contains no byte-offset view, cast-based layout access,
compiler control, or instruction-level steering.

## Do not repeat and residuals

- Do not replace `bsp->edges.count` with `bsp+0x48` arithmetic or a cast.
- Do not rewrite the exact loop, cache the count, or alter the signed induction
  type merely to explore equivalent code generation.
- Do not use assembly, `volatile`, register keywords, pragmas, intrinsics,
  barriers, undefined aliasing, object patches, or compiler-flag changes.

The wrapper has no residual. Seventeen other functions remain absent and were
not investigated by this lane; their classification is unchanged and this note
does not park them.

## Disposition and validation

`_render_debug_collision_bsp` is strict exact and eligible for ordinary
function credit. `collision_bsp.obj` advances to 13/30 strict functions,
2,487/10,292 meaningful code bytes, and 2,608/10,544 padded code bytes. It
remains `NonMatching`.

- Hardened comparator: exact normalized bytes and exact relocation type,
  address, target, and addend for both relocations.
- Disassembly gate: all instructions and the 13-byte padded tail agree.
- Whole-TU gate: 13 exact, zero residual, 17 unwritten; all 12 prior exact
  siblings remain exact.
- Complete `halobetacache_build`, `libcmt_build`, semantic report, and progress
  generation: pass, 479 actions in the final configured run.
- Semantic audit: 470 units, 4,854 functions evaluated, 4,727 semantic exact,
  129 hidden exact / 76,388 hidden code bytes, 4,737 accepted exact, and zero
  unit errors.
- Campaign progress: 382/833 complete objects, 4,698/11,060 functions, and
  596,016/2,198,102 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, and zero invalid entries.
- Complete tooling suite: 205/205 tests pass.
- `git diff --check`: pass.
