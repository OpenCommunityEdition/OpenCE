# `path_structure_bsp.obj` atomic closeout log

Target: January 14, 2002 Xbox build. Compiler: XDK 3911 CL
13.00.9254.1 with `/O2 /Oy- /DDEBUG /Dxbox`.

This log records an atomic six-function closeout attempt. January COFF bytes,
relocations, section ownership, and the hardened comparator are authoritative.
HCEX/HCEA names and source topology were used only as hypotheses. No matching
credit, compiler flag, comparator rule, configuration, or object label changed.

## Inventory and provenance

| Symbol | Target bytes | Relocations | Best result |
| --- | ---: | ---: | --- |
| `_structure_test_ray2d` | 480 | 14 | strict exact |
| `_structure_surfaces_are_equivalent` | 128 | 4 | strict exact |
| `_clip_empty_interval_by_solid_interval` | 112 | 0 | strict exact |
| `_structure_test_line2d` | 1,104 | 29 | same size/frame/count; nonexact |
| `_structure_surface_index_from_point` | 96 | 1 | strict exact |
| `_structure_test_pill2d` | 624 | 11 | same size/count; nonexact |

The object owns no `.data` or `.bss`. Its named table
`_rdata_0024ec0c` is 64 bytes, has zero relocations, and compares strict exact
with normalized SHA-256
`74970b924ac1a42d16bd2b93cf0db13c54a0df7e5c6da7f1980eb8c22dc7fbe1`.
The remaining `.rdata` contributions are compiler constants and assertion
strings whose identities are checked through the code relocations.

All six function names have exact-unique HCEX mappings. HCEA source for
`structure_test_line2d` and `structure_test_pill2d` confirmed the broad control
flow, local roles, and two-probe pill algorithm. It did not determine January
declaration lifetimes or VC7 scheduling; every accepted statement order below
was adjudicated against January bytes.

## Strict exact results recovered in this lane

| Symbol | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_structure_test_ray2d` | 480/480 | 14/14 | `6d8124c8c4241b78053d9354c96d8d7527b7c055c2c68ebc2d6e7578916e93eb` |
| `_structure_surfaces_are_equivalent` | 128/128 | 4/4 | `821f618f6425baa324bfbe68c75e8c144d5d0d4bd190dff1606fcd276f781b83` |
| `_clip_empty_interval_by_solid_interval` | 112/112 | 0/0 | `dcef7f55dd07ed3e27a77950d3436383dc2151902aaf5a752fb8b704e4aa1330` |
| `_structure_surface_index_from_point` | 96/96 | 1/1 | `29cfb8955fc45cfc7972505dddb757d4374ca33735024c18aa7f5baac71b8f49` |

The ray result layout is 12 bytes: `real distance`, `long surface_index`, and
`long edge_index`. The pill/line result layout is 28 bytes with `t` at `+0x18`.
Typed tag-block accessors, named engine math types, and house formatting were
kept throughout.

## `clip_empty_interval_by_solid_interval`

Seven measured source families are retained in this file's Git history. The
winning legal-C control assigns each selection arm to a distinct local and
copies that local back to the parameter. It preserves January's redundant
false-arm load/store and reaches strict exactness. Earlier `MAX`/`MIN`, direct
selection, pointer selection, alias, and expression-order variants were 96
bytes or reordered the x87 comparison. See the detailed history in this log's
predecessor commit.

## `_structure_test_line2d`

The full 26-experiment matrix lives in
`docs/object_matching_logs/path_structure_bsp_line2d.md`. It established the
complete algorithm, exact `0x70` frame, exact 1,104-byte envelope, exact 29
relocation identities, native aggregate lifetimes, statement order, assertion
spelling, and recursive state order.

Current best:

- Target/base size: `1104/1104`.
- Target/base frame: `0x70/0x70`.
- Relocations: `29/29`; all identities match, with addresses exact through
  `+0x376`.
- Target SHA-256:
  `664912aede956fbb3bc49b4517a90d315d445777054a1823c0eafadd2a1f2bc0`.
- Best base SHA-256:
  `55ccd1c4c2cab82eb867a354392b848b9a1851bc29a4c0dab3f4e64aadaece83`.
- Final projection call addresses target/base:
  `+0x3a7/+0x3a4`, `+0x3de/+0x3ee`, `+0x426/+0x423`.

The remaining mechanism is bounded: January spills `t` with
`fstps [ebp+8]` and reloads it for both hit coordinates; this compiler/source
combination emits `fsts [ebp+8]`, retains `t` in x87 for one coordinate, and
then chooses a different final basic-block order. Twenty-six readable source
families, including all meaningful declaration, scope, alias, expression,
helper, and CFG spellings, did not produce the January pairing. A barrier
proved the spill mechanism but rescheduled earlier bytes and was rejected.

## `_structure_test_pill2d` experiment matrix

The baseline January-adapted HCEA topology was 624 bytes and 11 relocations,
but normalized SHA-256
`bbbc822...` and relocation addresses drifted after the initial normalization.
The following bounded families were built and strictly measured:

1. **Perpendicular declaration/assignment order.** Naming `perpendicular_j`
   before assigning `i` computed the y component first and did not match.
2. **Explicit HCEA scalars.** `dx`, `dy`, `length`, and `inverse_length` kept
   the `0x84` frame but regressed relocation addresses and scheduling.
3. **`set_real_vector2d`.** Inlining the helper returned to the baseline
   y-first ordering; neutral and rejected.
4. **`vector_from_points2d` plus component swap.** Added an `fxch` and a stack
   temporary; rejected.
5. **`vector_from_points2d` followed by component overwrite.** Added redundant
   stores and loads; rejected.
6. **Aggregate initializer.** `{ -dy, dx }` emitted y-first stores and did not
   alter the normalization lifetime.
7. **Separate direction vector, scoped and unscoped.** Both grew the frame to
   `0x88`; rejected.
8. **Small semantic static helper.** VC7 emitted an unwanted out-of-line
   helper section while inlining the same baseline body; rejected and removed.
9. **HCEA point declaration order.** `p0`, `p1`, `q0`, `q1` permutations were
   byte-neutral.
10. **Readable expanded normalization.** `magnitude2d`, inverse, and two `*=`
    stores reproduced the baseline bytes exactly; useful proof, not a fix.
11. **`point_from_line2d` for four offset points.** The native inline helper
    was byte-neutral.
12. **Reverse multiplication operand spelling for `q1`.** Byte-neutral; VC7
    canonicalized the commutative products.
13. **Reverse magnitude component sum.** Byte-neutral; restored the helper.
14. **HCEA local declaration sequence.** All meaningful point/result/vector
    orderings were byte-neutral under the exact `0x84` frame.
15. **Success-fallthrough tail CFG.** Replacing the false-first tail with
    `if (!(distance_squared < radius_squared)) { success } else { false }`
    was the only material improvement. It kept `624/624` and `11/11`, changed
    the hash to
    `e4f034ffebad19d3bd8fd3e57828bc62b622810acd55bcf77b71375ef2ad6ba1`,
    and reproduced January's forward false branch and success block order.
16. **Direct early returns.** Eliminated the shared result local and shifted
    every later relocation; rejected.
17. **Set the boolean before the success copy.** Byte-neutral relative to the
    improved CFG.
18. **Distinct `memcpy` false arm.** Added an out-of-line `_csmemcpy`
    relocation and grew the candidate to 640 bytes with 12 relocations;
    rejected immediately.

Current best pill evidence:

- Size: target/base `624/624`.
- Frame: target/base `0x84/0x84`.
- Relocation count and semantic targets: `11/11`, identical sequence.
- Target SHA-256:
  `c3dac4abc09c7afb09ef84a44f434814cb3e22ea87d563991b222183e9fdb2f4`.
- Best base SHA-256:
  `e4f034ffebad19d3bd8fd3e57828bc62b622810acd55bcf77b71375ef2ad6ba1`.
- Target relocation addresses:
  `56, 73, 99, 162, 201, 254, 299, 337, 382, 432, 477`.
- Best base relocation addresses:
  `56, 73, 95, 158, 197, 249, 285, 323, 368, 418, 463`.

January keeps `perpendicular.j` live through the fourth offset-point
construction, spills/scales both perpendicular components, and recomputes the
negative-radius products for `q1`. The candidate coalesces those products and
reuses the second component's stack slot for a surface index. At the tail,
January loads the output pointer and `rep movsl` count after its x87 predicate;
the candidate hoists those branch-common values before the predicate. The
experiment matrix shows these are coupled local-colouring/scheduling effects,
not missing calls, fields, data, or control-flow behavior.

## Preserved shipped behavior

Two January defects were identified and must remain documented if this object
is reopened:

- `_structure_test_line2d` reads
  `pathfinding_surfaces[neighbor_surface_index]` before checking the `NONE`
  sentinel. On a boundary edge that reads one byte before the array. A safe
  derivative should check the sentinel first.
- `_structure_test_pill2d` copies `result0` to the output on the false
  within-radius path. A safe derivative could leave the output untouched or
  explicitly clear it, but the exact decomp must preserve January behavior.

## Atomic verdict and reopen criteria

**PARKED, not Matching.** Four of six functions and the named data table are
strict exact, but an atomic object cannot retain partial production changes.
The production/header/config changes from this lane are restored before the
evidence commit; only these logs remain.

Reopen only with one of the following:

1. January local-variable or statement records showing the original lifetime
   topology for either residual.
2. A readable exact donor compiled by the same VC7 flags whose source explains
   the `fstps`/reload or pill component-colouring pattern.
3. A new measured compiler-control rule that changes one of those exact
   mechanisms without changing section size, relocation identity, or sibling
   functions.

Do not repeat the declaration, scope, alias, helper, operand-order, early
return, barrier, or branch-copy families enumerated above and in the line2d
log.
