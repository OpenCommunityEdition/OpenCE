# `render_contrails.obj` exact-match log

## Outcome

`source/render/render_contrails.obj` is a strict whole-object match for the
January 14, 2002 Xbox build. All six functions match in padded size,
relocation count and identity, relocation-normalized bytes, and SHA-256. The
three target-owned string COMDATs also match exactly. The consolidated XDK
3911 build succeeds, and the shared rasterizer ABI/layout corrections preserve
the already-Matching 131-function `rasterizer.obj`.

In plain English: this file builds and traverses each live contrail, computes
the strip's two vertices per point, shades and fades them, builds its triangle
indices, and submits the completed dynamic geometry. The reconstruction uses
ordinary readable C. It contains no inline assembly, volatile codegen trick,
`__forceinline`, undefined behavior, byte patch, compiler-flag override, or
fuzzy acceptance.

## Strict evidence

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_contrail_fade` | 143 | 144 | 6 | `7dc3dde6b3c8f5af61facf91de6d51d91ff268eaf8173d9911a94fbc6a867f83` |
| `_code_00177860` | 1945 | 1952 | 57 | `70d28d083222f55ac171cb1aa9676d018fb2bcae9ee54f09facee54755cef5e8` |
| `_code_00178000` | 176 | 176 | 9 | `16ce1e5bee24c8a0f0e79659476d2d619b783419e9ba49d3cb2a2c71bccd94fa` |
| `_render_contrails_ground_mapped` | 9 | 16 | 1 | `1b1a80b01bf5912843c9d879bc793dd8ac269b037598c7afa05a0752095dbb5f` |
| `_render_contrails_media_mapped` | 9 | 16 | 1 | `f27c91ab09dc5b6f75808b4ae85f5e00e23c249e0ad6466b48ef6c5bc216fe06` |
| `_render_contrails_normal` | 9 | 16 | 1 | `07a8bcb483de4cb8bba2c0b5dac4ad15eb04f47cde8cb833f3b376ba44abf179` |

Total code is 2,291 meaningful bytes and 2,320 padded bytes. The target-owned
`.rdata` COMDATs are also exact:

| Data | Bytes | SHA-256 |
| --- | ---: | --- |
| `"contrail %s uses an unsupported render type."` | 45 | `38b10bfb93e324bcd9ff15b429387e6e917890cf2b008360bddc77ce09375249` |
| `"triangles && vertices"` | 22 | `6faca77985ef8484b9433bf5961df25e5c373bbb4a3515ebc9fc5a29ba2e0aa2` |
| January assert file path | 41 | `9a3ff7ea5d44b5f3e438ca53d746335c870e6add38e784766b7d21f22e4cce17` |

The ordinary visual reporter still assigns `_code_00177860` 98.45% because it
does not understand all local-label/section representations. The hardened
COFF comparator proves the 1,952-byte function and all 57 resolved relocation
destinations are identical; this is a known reporter false negative, not a
tolerance or semantic approximation.

## Recovered types and interfaces

- `contrail_point_state` is `0x68` bytes.
- `contrail_shader` places `framebuffer_fade_mode` at `+0x2c`.
- `contrail_definition` is `0x144` bytes and its `states` block is at `+0x138`.
- `contrail_datum` is `0x44` bytes.
- `contrail_point_datum` is `0x38` bytes.
- `rasterizer_globals.current_lock_operation` is a `short` at `+2`; the full
  rasterizer-global structure remains `0x68` bytes.
- The corrected unlit draw ABI is `(shader, bitmap, animation,
  triangle_buffer_index, vertex_buffer_index, vertex_count, centroid, flags)`.
- Typed access uses `contrail_definition_get`, `object_get`, and
  `TAG_BLOCK_GET_ELEMENT`, in line with the project house rules.

## Experiment history

The object began with only the three tiny render-mode wrappers exact. The
following source shapes were accepted:

1. Expressing `contrail_fade` through the existing vector helpers reproduced
   the 144-byte function; manual scalar arithmetic produced 160 bytes and was
   rejected.
2. The natural `data_next_index` iterator, typed contrail tag getter, and four
   instance loop reproduced `_code_00178000` at 176 bytes and nine relocs.
3. Correcting the recovered vertex, point, state, shader, definition, and
   rasterizer-global layouts moved the large renderer from 1,888 bytes/58
   relocs through 1,904 and 1,920-byte shapes to the target 1,952/57 topology.
4. Direct scalar eye/tangent calculations and the January cross-product
   operand order reproduced the viewer-facing branch and both orientation
   normalizations.
5. Moving `half_width *= 0.5f` immediately before the four texture-coordinate
   stores fixed an otherwise independent three-byte scheduling window.
6. The final mismatch was only two compiler stack slots. Writing the two
   loop-carried updates as `texture_u += texture_u_step;` followed by
   `previous_point = point;` changed their live-range order and closed every
   remaining byte without changing the algorithm.

Measured and rejected shapes included aggregate/union packing of the two loop
locals, declaration-order permutations, declaration initializers, an early
`NULL` initialization, delayed texture initialization, nested scopes, and
several alias forms. These either had no effect, changed broad register
allocation, or regressed already-correct code. They should not be repeated.

The unsupported-render-type arm retains its early `return` because that is the
January control-flow shape. This is the documented house-rule exception to the
preferred single-return style; forcing a common exit changes the exact object.
That arm also preserves an original cleanup bug: it skips the dynamic-buffer
unlock/delete calls and leaves `current_lock_operation` set. The source labels
the bug and describes the non-matching fix (route the arm through common
cleanup) without changing January behavior.

## Shared-header blast radius

The recovered layouts and corrected rasterizer ABI live in shared headers, so
the audit did not rely on Ninja's ordinary header tracking. A detached clean
worktree at campaign tip `e30b1568` and the candidate worktree each rebuilt
the same seven translation units from scratch: `render_contrails`, `contrails`,
`objects`, `rasterizer`, `ai_debug`, `cinematics`, and `main`. Comparing every
defined function with `section_infos_equal` and every non-debug data section
found zero changes outside `render_contrails.obj`.

The raw regression manifest reported debug-record, section-number, and accepted
evidence churn, including an apparent `ai_debug_render_actor` regression. That
was not a code regression: a fresh baseline build already has the same current
nonexact function, and baseline-to-candidate hardened comparison is exact.
This fresh-build cross-check prevents stale `.obj` files from being mistaken
for a patch regression while still failing closed on real code, relocation, or
owned-data changes.

## Verification commands

The final candidate was rebuilt with repository-configured XDK 3911
`CL 13.00.9254.1` and `/O2 /Oy- /DDEBUG /Dxbox`, then checked with:

```text
ninja build/base/source/render/render_contrails.obj
python -m tools.coff_compare build/split/source/render/render_contrails.obj build/base/source/render/render_contrails.obj <all six function symbols>
python -m tools.coff_compare build/split/source/render/render_contrails.obj build/base/source/render/render_contrails.obj <all three target-owned rdata symbols>
ninja halobetacache_build
ninja semantic_progress
ninja progress
python -m unittest discover -s tools -p "test*.py" -v
```

All strict comparisons returned `all_equal: true`; the consolidated build and
semantic report completed with zero errors. The progress gate admitted the
object (262/468 Halo units Matching at this checkout), and all 173 tooling
tests passed.
