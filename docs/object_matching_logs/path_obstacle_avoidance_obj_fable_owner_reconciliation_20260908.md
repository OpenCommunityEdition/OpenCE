# `path_obstacle_avoidance.obj` owner reconciliation

This packet reconciles the Fable obstacle-search graph from commit
`e836c4113` on `fable/50k-resume-20260906` against canonical
`7f7b72cc3819f05e3e85956101f314b544825e08`. Credit is measured against the
current canonical owner set, not the donor lane headline.

## Exact accounting

The focused unit advances from 9 exact / 0 residual / 15 unwritten functions
to **19 exact / 4 residual / 1 unwritten**. Ten new functions contribute
**3,330 meaningful / 3,392 padded exact code bytes**:

| private function | meaningful | padded |
|---|---:|---:|
| `error_heap` | 201 | 208 |
| `heap_verify` | 419 | 432 |
| `heap_up` | 396 | 400 |
| `heap_down` | 670 | 672 |
| `heap_insert` | 92 | 96 |
| `heap_remove` | 98 | 112 |
| `path_add_step` | 538 | 544 |
| `path_test_pill2d` | 505 | 512 |
| `path_iterate` | 304 | 304 |
| `path_find` | 107 | 112 |

Each owner passes `coff_compare.section_infos_equal`, including ordered
relocation identities. The existing nine exact owners remain exact. No
function-only result is promoted to whole-object or data completion.

## Names, types and declarations

Twelve address placeholders are replaced by the semantic private names
`error_heap`, `heap_verify`, `heap_up`, `heap_down`, `heap_insert`,
`heap_remove`, `path_add_step`, `path_new`, `path_test_pill2d`,
`path_add_steps`, `path_iterate`, and `path_find`. The Fable/HCEA
correspondence evidence, strings and call graph identify the names and the
January object proves private ownership. All twelve target symbol rows carry
`"static": true`; no `code_...` definition remains in source.

`ai/path.h` now owns the obstacle-path structures embedded by
`path_debug_storage`, the named maximum counts, public obstacle APIs and debug
globals. `ai/path_structure_bsp.h` owns `structure_test_ray2d` and its result
type. This removes opaque debug blobs, raw casts to hidden structures and
consumer-local public prototypes. Four direct Path-family gates preserve their
prior exact sets: `path` 15/32, `path_smoothing` 4/6,
`path_structure_bsp` 3/6 and `path_obstacles` 8/16.

The translation unit macro-renames the `real_math.h` inline `normalize2d`
while importing January's measured inline set, then declares the same
out-of-line `normalize2d` ABI for the one genuine call. That local declaration
is an explicit narrow exception to ordinary prototype ownership: placing a
second declaration in the broadly shared header changes the inline owner
schedule this packet is required to preserve. The same measured exception is
already used by the Actor combat family. It must be revisited only as a
whole-header inline-schedule change, not moved casually into another C file.

The `error_heap` diagnostic uses a named local union to view the bit pattern of
its `real` cost. This replaces the donor's undefined pointer-pun while
retaining exact code. Project `real`/point/vector types, bit-vector macros and
named enum/constants are used throughout. Parameters are one-per-line,
no-argument lists use `void`, and void functions end with explicit returns.
No forced inline, volatile/register pressure, pragma, raw byte emission or
invented logic is admitted.

## Honest fuzzy work

Four complete natural bodies remain non-exact and receive zero exact credit:

- `path_new`: 476 meaningful / 480 padded target bytes; field-store
  scheduling remains different.
- `path_add_steps`: 816 meaningful / 816 padded target bytes; the remaining
  difference is a private register/word-store allocation tie.
- `path_avoid_obstacles`: 1,345 meaningful / 1,360 padded target bytes versus
  a 1,312-byte candidate; the complete typed algorithm is retained, but
  January pins a boolean/register schedule the natural source does not.
- `render_debug_path`: 604 meaningful / 608 padded target bytes; the complete
  instruction graph is otherwise aligned, with two independent point locals
  assigned to opposite stack slots.

Fresh post-build measurements are recorded in `config/parked.json`: 95.76%
for `path_new`, 99.4% for `path_add_steps`, 90.164444% for
`path_avoid_obstacles`, and 99.96172% for `render_debug_path`. Each entry
remains zero-credit. The 224-byte
`render_debug_obstacle_path` root is still unwritten in canonical because the
donor version depends on unresolved data-symbol identities. It is not counted
or disguised as a park.

## Data boundary

The source names the search snapshots `debug_path` and `debug_obstacles`,
followed by the two public debug booleans. Their candidate layout is the same
8,514-byte extent as January: a 0x1534 obstacle path, alignment to 0x1538, a
0xC08 obstacle collection and two bytes. The target split nevertheless labels
offset zero `_current_traverse_index`, which is likely a static/global boundary
collision rather than evidence that the semantic names are interchangeable.
No BSS alias, data credit or whole-object claim is made until that ownership
boundary is resolved from stronger symbol evidence.

## Verification boundary

Focused code equality, the fake-match scan and the forbidden
`_point_from_line3d` emitted-owner guard pass. Shared-header admission still
requires the orchestrator's full build, rename-stable whole-tree sweep,
park/admission audits and complete test suite before publication.
