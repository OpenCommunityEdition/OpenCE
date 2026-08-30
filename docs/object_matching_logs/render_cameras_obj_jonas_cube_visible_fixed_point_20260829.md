# `render_cameras.obj` cube-visibility fixed point — 2026-08-29

## Result

This bounded one-shot lane started from canonical commit
`a7e546fd2e02f08a16913dd8375a4bc40bb8aed1` and screened only
`_render_frustum_cube_visible`. The first code-producing ordinary-C candidate
is not strict COFF exact and has been removed completely. No production source,
header, configuration, comparator, or object-status change is retained.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| meaningful / padded bytes | `715 / 720` | `893 / 896` |
| relocations | `4` | `4` |
| normalized SHA-256 | `84a8a98b967445f28b4cd07f8edf88d2254b9d3417ba63e112bddb95634033be` | `e4ad0af5bc395932262080a140ae0e8849a0baf27417b4a69a2e67954693b15b` |

The January ordered `DIR32` relocation offsets are
`+0x102/+0x13C/+0x178/+0x1B4`, all targeting `__real@00000000`. The candidate
has the same four identities at `+0x1DD/+0x20E/+0x23A/+0x264`; their shifted
locations do not satisfy strict equality. January's function begins with the
three-byte `sub esp,0x60` frame allocation at `+0x03`, while the candidate's
first structural divergence is the six-byte `sub esp,0xAC` at the same offset.
It then preloads the bounds and materializes the donor's plane-component
aliases, whereas January keeps the two input pointers live and addresses the
typed fields directly. The frozen candidate object SHA-256 is
`f99b1f9cb8d179b745815f847b1b92dc1081ee072b724eabf3a0456cee693d51`.

## Census and provenance

All three pre-existing `render_cameras.obj` ledgers were read before emission.
They cover the constructor, projection transformations, adjusted-FOV tangent,
three rejected leaf shapes, and the private warning-condition ABI boundary;
none had emitted or measured this cube function. Reachable repository history
contains only the five known `render_cameras.c` blobs, and targeted searches of
registered worktrees and research caches found no unintegrated production body.

The January split object is authoritative. Its SHA-256 is
`c0194e912ef3b7e9ab6e4ffc9db57c527bd2fc9b011bc00e00c0ddc793cb6974`.
The semantic donor is HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
`src/blam/render/render_frustum_cube_visible.c`, payload SHA-256
`972bea8401884ad14da1539c6ddd5d50d42841a9349251e88c053bedddacfffb`.
Its typed frustum/rectangle classification agrees independently with the
Pastudan Halo history at repository tip
`918af885935ec470a31256ecce9a977b12b01f80`: that history preserves the complete
x86 lift whose comparisons, corner construction, two classification loops,
return classes, and four zero-real references agree with January disassembly.

The frozen production candidate used named `render_frustum`,
`real_rectangle3d`, plane, bound, and vertex fields; a local `real[24]` scratch
array represented the eight corners. Six natural chained assignments filled
all 24 elements without raw offsets. A first compiler invocation produced no
object because the local repository's plane normal is a union; mechanically
selecting its named `i/j/k` members fixed that type spelling without changing
any expression or control flow. The next invocation was the sole
code-producing candidate compile. No alternative declaration placement,
expression ordering, point representation, loop shape, or retry was emitted.

## Reopen boundary and policy

The measured `+176` padded-byte frame/lifetime gap is the fixed point. Do not
retry the HCEA scalar-alias topology through declaration permutations,
expression reordering, helper wrappers, alternate scratch-array spellings, or
compiler-control devices. Reopen only with an independently preserved
January-era source artifact that proves the original direct-field lifetime and
loop topology.

The rejected source passed a diff-scoped policy scan. It contained ordinary
typed C89 only: no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, raw address or object offset, pointer or union
pun, undefined behavior, byte forcing, object patch, comparator exception, or
tool/configuration change. The source blob after inverse reversion is exactly
the base blob `c5ca98582be6f66fca4ad442d35c150a826f3d79`; the normal production edge
rebuilt it successfully. Regression replay reports all four inherited exact
functions still exact, no newly exact or changed-nonexact function, and no
warning.

## Validation

- Focused production rebuild and strict board: pass; `render_cameras.obj`
  remains `4/21` exact and `_render_frustum_cube_visible` is unwritten.
- Regression snapshot/check: pass; all four inherited exact siblings remain
  byte-and-relocation exact.
- Complete `halobetacache_build`/`libcmt_build` graphs and fresh report,
  semantic, and progress generation: pass. The semantic report scans 470
  units, evaluates 4,917 functions, accepts 4,810, and reports zero unit
  errors. Linked campaign progress remains 384/833 objects, 4,771/11,060
  functions, and 606,187/2,198,102 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations. Parked
  audit: 12 active, zero stale, and zero invalid records.
- Protected `source/units/units` sentinel: strict `189/189` functions and
  `54,864/54,864` padded bytes.
- Complete tooling suite: 212/212 tests pass. `git diff --check` is clean.

Only this additive Jonas-owned ledger is committed. No push is performed.
