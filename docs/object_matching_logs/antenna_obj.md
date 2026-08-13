# `antenna.obj` exact-match log

## Plain-English behavior

This object owns the springy antenna widgets attached to objects. It allocates
an antenna datum, builds the chain of simulated vertices from the `ant!` tag,
keeps the root attached to an object marker, advances the spring simulation,
and renders each segment as an alpha-blended sprite.

The later HCEA reconstruction is useful as a control-flow and type hint only.
Every accepted statement, type, offset, call, constant, and source shape is
adjudicated against the January 14, 2002 XDK 3911 object.

Compiler provenance: Microsoft 32-bit C/C++ Optimizing Compiler
13.00.9254.1 from XDK 3911, with the repository's unchanged
`/O2 /Oy- /DDEBUG /Dxbox` flags.

## Baseline

- 11 functions total.
- 5 lifecycle/deletion functions were already strict-exact.
- `antenna_new`, three private helpers, `antenna_render`, and
  `antennas_update` were initially absent.
- Target-owned data consists of two strings; `antenna_data` is external.

## Accepted reconstruction findings

- `antenna_datum` is `0x2BC`; its 21 runtime vertices begin at `+0x1C` and
  have a `0x20` stride.
- `antenna_definition` is `0xD0`; its vertices tag block is at `+0xC4`.
- `antenna_vertex_definition` is `0x80`; its cached vector is at `+0x74`.
- `antenna_render` and `antennas_update` matched exactly from the natural HCEA
  topology after correcting the January-only final sprite flag to zero.
- `antenna_new` matches with one inner-scoped `real_point3d position`, a true
  `short` index, the January header-store order, `k/j/i` velocity clears,
  position accumulation before the index increment, and one scoped sentinel
  pointer. This is exact at `0x200`, 11 relocations.
- `code_001209a0` (attachment refresh) requires standard `abs((long)delta)`
  expressions, a `short` translation-loop index, and a named
  `real_point3d *vertex_position`. The pointer shape changes indexed stores
  from `+0x1C/+0x20/+0x24` to the target's `+0/+4/+8`; this function is now
  strict-exact at `0x130`, 8 relocations.
- `code_00120c40` uses a natural pretested `for` loop with a `short` index, no
  count local, and no long-lived vertex-array alias. `previous_position` and
  `carried_tip` live across iterations; the other vectors are iteration-local.
  The sprite vector has its own nested aggregate-initializer scope, with the
  angle scoped inside it. This delays the `EBX` save to the January loop
  preheader and is exact at `0x310`, 15 relocations.
- The simulation also uses a local `{0,0,1}` up vector, inline
  `cross_product3d`, and normalize/fallback to `global_left3d`.
- `code_00120ad0` needs a named long-lived `vertices` tag-block pointer,
  an independently indexed `next_vertex`, and the color copy after all three
  direction assignments. This is exact at `0x170`, 10 relocations.
- Recovered supporting layouts are `bitmap_group_sprite` `0x20`,
  `bitmap_group_sequence` `0x40`, and `build_sprite_data` `0xA4`;
  `sprite_spacing` is signed.

## Rejected or superseded shapes

- Direct indexed vertex-position writes in `code_001209a0`: correct semantics
  but wrong addressing form; superseded by the named point pointer.
- `ABS` macro in the jump test: emitted three branch-based absolute-value
  sequences and oversized the function. The CRT `abs` intrinsic emits the
  target `cdq; xor; sub` idiom.
- Long simulation loop index: cached the definition pointer in `EBX`, emitted
  a dword root test, and changed the private helper convention.
- A three-term simulation guard plus `do/while` compiled to `0x300` and cached
  the definition pointer in `EBX` from entry. Adding a named count did not
  change that allocation.
- A long-lived runtime-vertex pointer restored `0x310` but grew the frame to
  `0x9C` and broke the private helper ABI.
- Manual cross-product expressions: missed two zero-constant relocations and
  undersized the simulation. The normal inline helper is structurally right.
- Declaration-order-only experiments on `antenna_new`: moving the three
  position scalars changed frame slots but did not by itself recover the
  target pointer/induction topology.

## Representative measured topology matrix

| Function/shape | Padded size | Relocations | Result |
| --- | ---: | ---: | --- |
| `code_00120c40`, three-term guard plus `do/while` | `0x300` | 15 | Rejected: definition cached in `EBX` from entry |
| `code_00120c40`, long-lived runtime-vertex pointer | `0x310` | 15 | Rejected: frame grew to `0x9C`; private ABI drifted |
| `code_00120c40`, natural `for` loop and nested sprite scope | `0x310` | 15 | Accepted: strict hash `eb6cd6ae...ddd82a` |
| `antenna_new`, recovered index/pointer lifetime | `0x200` | 11 | Accepted: strict hash `27e92e23...0738d` |
| `code_001209a0`, named vertex-position pointer | `0x130` | 8 | Accepted: strict hash `74070256...1d26c` |
| `code_00120ad0`, tag-block pointer and independent next vertex | `0x170` | 10 | Accepted: strict hash `35ec5666...dfd90` |

## Final strict verification

- 11/11 functions strict-exact, totaling 2,432 padded code bytes.
- The 34-byte allocation-error string and 8-byte `antenna` name string are
  exact; `antenna_data` remains external as in January.
- The reconstruction uses XDK 3911 and the symbol-aware hardened comparator.
  It uses no assembly, `volatile`, `__forceinline`, undefined behavior, byte
  patch, or compiler-flag change.

The object is safe to mark `Matching` after the consolidated build and
whole-object regression gates pass.

## 2026-08-13 independent salvage revalidation

The completed object was treated as untrusted old work and transplanted from
commit `031cf2d67743bee9131347ca51c2be6c3ecdd17b`. It was first audited on
campaign tip `1bf72e846d3f0b877d19ebfe72243df74bc10dd0`, then rebased and fully
revalidated on authoritative tip
`915c7879c71a70a3381f5179398a2ee848639784`. Every target function and every
target-owned data section was independently rebuilt and checked with the
current hardened comparator after the final rebase.

- All 11 functions are strict-exact: 2,432/2,432 padded code bytes and all
  relocation addresses, types, destinations, and addends match January.
- Both target-owned strings are exact, including COMDAT ownership/storage:
  34 bytes for the allocation error and 8 bytes for the data-array name.
- `antenna_data` remains an external undefined symbol, matching the target's
  ownership model; no candidate-owned `.data` or `.bss` was introduced.
- `bitmap_group.sprite_spacing` is proven signed by `_antenna_new`'s January
  sign-extending load. The first transplant briefly exposed bitmap sequence,
  sprite, and bitmap-access declarations in the shared bitmap header. Those
  definitions were localized to `antenna.c`, because antenna is their only new
  consumer and keeping the shared header smaller reduces unnecessary compiler
  and integration blast radius.
- A ten-unit forced-rebuild gate covered every direct consumer of
  `bitmap_group.h`, `antenna.h`, and the new `render_sprite.h`: `antenna`,
  `widgets`, `bitmaps_quantitize`, `targa_file`, `tiff_file`,
  `predicted_resources`, `overhead_map`, `rasterizer_text`,
  `rasterizer_xbox_hardware_bitmaps`, and
  `rasterizer_xbox_motion_sensor`. After regenerating the current objdiff
  report, every previously accepted function remained exact and there were
  zero `changed_nonexact` functions. The only gate failures were the expected
  Matching-label transition and newly exact antenna evidence.
- The active `render_sprite.obj` lane changes only `render_sprite.c` and its
  log. Its independently recovered `build_sprite_data` size is also `0xA4`, so
  this new public header neither collides textually nor contradicts the known
  January layout; the render-sprite lane will consume it only after both atomic
  objects land.

The house/Berth audit found typed `antenna_get`, `antenna_definition_get`,
`bitmap_group_get`, `object_get`, and `TAG_BLOCK_GET_ELEMENT` access; one
parameter per line; explicit returns in all void functions; and a single
return in every function. There is no assembly, volatile qualifier,
force-inlining, alignment directive, magic offset, compiler flag change, or
comparison exception. The source also labels the original integer-truncation
threshold bug and explains the non-matching `fabs` repair without altering the
shipped behavior.

### Final authoritative-tip gates

- A clean detached `915c7879` worktree and the antenna lane both force-built
  all ten direct consumers with XDK 3911. The hardened comparator found every
  runtime section identical in the other nine objects: code, `.rdata`,
  `.data`, `.bss`, relocation identity/addends, and ownership all remained
  unchanged. Debug records were intentionally excluded because their embedded
  source paths differ between worktrees and are not linked runtime content.
- The pre-change fail-closed regression manifest reported zero
  `changed_nonexact` entries for all nine shared-header consumers. Its antenna
  and configuration findings were the expected reviewed admission transition:
  six newly implemented functions, newly owned string COMDATs, and the
  `NonMatching` to `Matching` label. A fresh post-admission ten-unit snapshot
  and rebuild then passed with `ok: true`, no failures, and no warnings; all 11
  antenna functions remained in `still_exact`.
- `ninja halobetacache_build`, `ninja semantic_progress`, and `ninja progress`
  completed successfully. The resulting Halo cache progress was 267/468
  complete objects, 3,348/7,574 accepted functions, and 406,006/1,770,166
  accepted code bytes. Semantic evaluation reported 3,353 accepted-exact
  functions and zero unit errors.
- The complete tooling suite passed 177/177 `unittest` tests. The admission
  audit found no antenna contradiction or revocation, and `git diff --check`
  was clean.
