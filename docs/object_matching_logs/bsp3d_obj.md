# `bsp3d.obj` exact-match strategy ledger

## Scope and baseline

- Translation unit: `source/physics/bsp3d.c`
- Target: January 2002 `build/split/source/physics/bsp3d.obj`
- Candidate compiler: XDK 3911 CL 13.00.9254.1 with
  `/O2 /Oy- /DDEBUG /Dxbox`
- Foundation branch: `codex/bsp3d-full-admission-20260729`
- Closeout branch: `jonas/bsp3d-upstream-20260820`
- Integrated campaign base: `28156055`
- Foundation commit: `4eb70706`
- Decisive upstream provenance: `5093ac1a` (`match bsp3d (#49)`)
- Admission rule: the object is complete only when every function and every
  January-owned data section is strict exact under the hardened comparator,
  followed by whole-TU regression, semantic, build, and house-style gates.

Initial repository inventory:

| Function | Target padded size | Initial state |
| --- | ---: | --- |
| `_bsp3d_test_point` | `0x80` | Implemented; strict baseline pending |
| `_bsp3d_clip_line_to_leaves` | `0x280` | Missing |
| `_code_00136900` | `0x2d0` | Missing |
| `_bsp3d_clip_polygon_to_leaves` | `0x30` | Missing |

The January object also owns seven known `.rdata` items: three assertion
strings, the source-path literal, and the `+/-0.000244140625f` constants.
Their bytes, relocations, linkage, and ownership must be audited independently
of function equality.

## Evidence and recovered interfaces

This section will be updated from January disassembly, relocation tables,
call-site evidence, and corroborating debug/type sources before implementation.
Cross-build evidence may suggest names or structure fields, but January machine
code remains authoritative.

Historical provenance recovered from Git commit `48dff1c68` is independently
useful: its complete `bsp3d.c` line implementation compiles strict exact in a
disposable worktree when paired with that commit's `real_math.h`. Transplanting
the same source alone into the current header environment does not match.
This proves the line residual is controlled by inline-header/compiler context,
not by an unknown BSP algorithm. No historical result is admitted without a
fresh January comparison in the current campaign.

The terminal polygon-helper source topology was recovered from upstream commit
`5093ac1a`. That commit supplied a semantics-preserving typed alias for the
second edge and, critically, the complete surrounding declaration, store, and
child-traversal topology in which the alias controls VC7's x87 schedule. The
upstream source was treated as donor evidence only: every retained line was
adapted to the current typed structures and independently rebuilt against the
January object. No upstream status label was accepted as proof.

## Experiment matrix

Every experiment records the source shape, target/candidate size, relocation
identity and order, normalized hashes or first divergence, sibling effects,
and whether it was retained or reverted.

| ID | Function | Source shape | Strict result | Disposition |
| --- | --- | --- | --- | --- |
| B00 | whole object | Untouched repository baseline, rebuilt with the configured XDK 3911 compiler and freshly extracted January split object. | `_bsp3d_test_point` is strict exact at 128/128 bytes, 3/3 destination-identical relocations, normalized SHA `c3ef00a48652ee90123f344343cace721c289b029aa70e7461b984104eaa693e`. The other three target symbols are absent from the candidate. | Baseline established; retain the exact sibling unchanged. |
| B01 | three missing functions | Reconstruct the January recursive line and polygon clipping algorithms from full disassembly and relocation evidence; add the public callback contracts and express the 48-byte polygon entry point as a wrapper around `code_00136900`. | `_bsp3d_clip_polygon_to_leaves` is **strict exact** immediately at 48/48 bytes, 1/1 relocation, SHA `436d62bff33e3db00323812fe6f36a880ea7fb7a584567dcad1362f90392267f`. `_code_00136900` has the exact 720-byte padded size and all 18 relocation destinations in target order; after the asserts, relocation addresses are two bytes early until realigning at the recursive call. `_bsp3d_clip_line_to_leaves` has all 16 target relocations in order but is 624/640 bytes. The exact `_bsp3d_test_point` sibling remains unchanged. | Retain as the evidence-backed algorithmic foundation. Rename the 64-vertex constant to the target assertion spelling, then isolate the remaining source-shape differences rather than rewriting the algorithms. |
| B02 | both recursive helpers | Split the line classifier from one `boolean[2][2]` into separate `point0_sides[2]` and `point1_sides[2]` arrays; express the leaf test as the January sign-bit predicate `child_index & LONG_MIN`; recover the assertion identifier as `MAXIMUM_VERTICES_PER_CLIPPED_POLYGON`. | The line helper now has the target `0x28` frame and target stack placement: `point0_sides` occupies the unused high bytes at `[ebp+0xe..f]`, `point1_sides` occupies `[ebp-4..-3]`, and the accumulator is `[ebp-0xc]`. It remains 624/640, 16/16, SHA `72a9c81f22beb6e8f5ca8730ea3990ae431bcd3cf72bb5ba71cfc8734b3d0cad`, but its recursive relocation advances from `0x235` to `0x239` toward target `0x243`; the sign branch is now the target `jns`. The polygon helper remains 720/720 and 18/18 with the target assertion symbol spelling; candidate SHA `a5232f364a6611a3b18b1a716bc8b7090c373588de3c266bb5712f6595334cb6`. Both exact siblings remain exact. | **Accepted.** The stack layout and sign-bit predicates are independently proven January source shapes. |
| B03 | `_code_00136900` | Change the coplanarity-loop exit from `fabs(distance) > epsilon` to `fabs(distance) >= epsilon`. | Size and relocation counts remain exact at 720/720 and 18/18. Candidate SHA changes to `9314d67e47c2a89d8af8790ab8fe9a641f4166d794e254fc67b45b440d767576`; the condition becomes `test ah,0x41; jnp`, while January is `test ah,0x41; jne`. The forms agree for finite ordered values but differ for unordered/NaN: January exits on unordered, plain `>=` does not. Exact siblings remain exact and the line helper is unchanged. | **Rejected.** This proves equality belongs to the exit condition but plain `>=` has the wrong unordered behavior. Do not repeat; test the explicit negated strict predicate next. |
| B04 | `_code_00136900` | Express the coplanarity-loop exit as `!(fabs(distance) < epsilon)`, preserving January's unordered/NaN exit behavior. | The complete condition window is now instruction-exact: `fabs; fld epsilon; fcompp; fnstsw ax; test ah,0x41; jne`. Size and relocation counts remain 720/720 and 18/18; candidate SHA improves to `cb044fe925a0716015c00407c7c5724bff8cf1a748684ed367105cc8d9e6a9ab`. Relocations after the normal calculation remain two bytes early until the recursive call, and both exact siblings remain exact. | **Accepted.** This is the January-authoritative predicate, including its NaN behavior. |
| B05 | `_code_00136900` | Reverse the `cross_product3d` operand order and invert the following zero comparison to preserve the selected side. | Size and relocation counts remain 720/720 and 18/18, but candidate SHA regresses to `89f2cf08b2c400693249ecfe5a5626ad88a732810f99047f611f2c062dec9ad0`; the normal calculation diverges more broadly and relocation addresses remain two bytes early. Exact siblings remain exact. | **Rejected and reverted.** The January topology uses `cross_product3d(&edge1, &edge0, ...)`; do not repeat operand reversal. |
| B06 | `_code_00136900` | Express the normal/plane side selection as `!(dot_product3d(...) < 0.f)`. | Size and relocation counts remain 720/720 and 18/18; candidate SHA is `bd5f3f4f7dd8df3cd8ac00ab161875509374192abe8321813f89e1ce1e3fd8b3`. The comparison becomes `test ah,0x05; jnp`, not January's `test ah,0x41; jne`; the explicit negation selects equality, while January sends equality to the false side. | **Rejected.** January's side predicate is strict-positive, not nonnegative. Do not repeat the negated-less-than form. |
| B07 | `_code_00136900` | Use the strict-positive side predicate `dot_product3d(...) > 0.f`. | The comparison window becomes instruction-exact: `fcomps 0; fnstsw ax; test ah,0x41; jne`. Size and relocation counts remain 720/720 and 18/18; candidate SHA improves to `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. The remaining two-byte address drift starts in the cross-product x87 schedule; exact siblings remain exact. | **Accepted.** Equality and unordered values select the false side in January. |
| B08 | `_code_00136900` | Replace the inline `cross_product3d` call with a textually equivalent local scalar expansion using the same `k`, `j`, `i` temporary order as the header. | The compiler collapses the helper body to 704 bytes versus target 720 while preserving 18 relocations; candidate SHA `fd5c9cb35d6f2ec34ddd5edab4d46a66e1b68c38f4acc9146221756fd78412b8`. | **Rejected and reverted.** The function-call inline provenance matters; do not repeat direct textual expansion. |
| B09 | `_code_00136900` | Call a file-local inline cross-product helper whose scalar temporaries are declared `i`, `j`, `k` instead of the shared header's `k`, `j`, `i`. | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. | **Rejected and reverted.** Temporary declaration order inside the inline helper is a non-control for this residual. |
| B10 | `_code_00136900` | Swap the declarations of `edge0` and `edge1` while retaining their initialization and use order. | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. | **Rejected and reverted.** Relative declaration order of the two edge vectors is a non-control. |
| B11 | `_code_00136900` | Replace the shared inline helper with a file-local inline variant that assigns `result->i/j/k` directly without scalar temporaries. | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. | **Rejected and reverted.** Direct result assignment versus staged scalar temporaries is a non-control after inlining. |
| B12 | `_bsp3d_clip_line_to_leaves` | Remove the inferred reusable `opposite_side_index` local and spell `side_index == 0` independently at the short-circuit test and both clipped-point selections. | **Major improvement:** candidate reaches the exact 640-byte padded size and all 16 relocations match in count, destination, order, and address, including the recursive call at target `+0x243`. Candidate SHA improves to `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`. January recomputes the opposite index only after the current-side tests fail and again for pointer selection; the entire loop/callback/recursion suffix now matches. The sole remaining region begins at `+0x56`, where the second plane-distance dot product schedules the same commutative multiplies in a different order. | **Accepted.** The reusable local was a decompiler artifact; January's source repeated the expression at each use. |
| B13 | `_bsp3d_clip_line_to_leaves` | Split `distance1` declaration from its subsequent `plane3d_distance_to_point` assignment. | Output is byte-identical to B12: 640/640, 16/16, SHA `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`. | **Rejected and reverted.** Declaration-with-initializer versus later assignment is a non-control for the remaining x87 order. |
| B14 | `_bsp3d_clip_line_to_leaves` | Expand the second plane distance into one expression whose terms and operand directions follow the January instruction order (`z`, then `x`, then `y`). | Output is byte-identical to B12: 640/640, 16/16, SHA `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`; VC7 reassociates the commutative terms back to the prior schedule. | **Rejected.** A single expanded expression is a non-control; do not repeat term/operand permutations without an independent data-dependency anchor. |
| B15 | `_bsp3d_clip_line_to_leaves` | Stage the second distance as two additions and one subtraction, creating a source-level dependency between the `z+x` partial sum and the `y` term. | Output is byte-identical to B12: 640/640, 16/16, SHA `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`; VC7 folds the staged scalar back into the same x87 schedule. | **Rejected and reverted.** Scalar staging is also a non-control under `/O2`; retain the canonical helper call. |
| B16 | `_code_00136900` | Use a file-local inline cross product that reverses only the commutative operands of the `j` component's right-hand multiply (`b->k*a->i`). | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`; VC7 canonicalizes the multiply direction. | **Rejected and reverted.** Commutative operand spelling is a non-control for the two-byte x87 sequence. |
| B17 | `_code_00136900` | Nest `cross_product3d(...)` directly as the first operand of `dot_product3d(...)` instead of using two source statements. | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. | **Rejected and reverted.** Nested versus separate inline-call topology is a non-control. |
| B18 | `_bsp3d_clip_line_to_leaves` | Reverse the two operands of the second distance's inlined `dot_product3d` call while preserving the same mathematical expression. | Output is byte-identical to B12: 640/640, 16/16, SHA `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`. | **Rejected and reverted.** Reversing the dot-product pointer operands is another non-control for the remaining x87 multiply schedule. |
| B19 | `_bsp3d_clip_line_to_leaves` | Reproduce the local-declaration order from historical commit `48dff1c68` (`clipped_point`, side arrays, loop index, then result/node/plane/distances) while retaining the January-proven B12 logic. | Output is byte-identical to B12: 640/640, 16/16, SHA `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`. | **Rejected and reverted.** Even the recovered historical declaration topology is frame-neutral and cannot control the remaining x87 order. |
| B20 | `_code_00136900` | Reproduce the historical `48dff1c68` top-level local ordering (clipped arrays/counts/indices before result/node/plane) while retaining all January-proven predicates and branches. | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. | **Rejected and reverted.** Whole-function declaration topology is also a non-control for the polygon's x87 window. |
| B21 | `_code_00136900` | Reproduce historical commit `48dff1c68`'s coplanar-case store order by assigning `clipped_vertex_lists[side]` before `clipped_counts[side]`. | Output is byte-identical to B07: 720/720, 18/18, SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. | **Rejected and reverted.** The post-comparison store order is a non-control for the preceding x87 scheduler window. |
| B22 | `_bsp3d_clip_line_to_leaves` | Transplant the complete historical `48dff1c68` line-function source topology into the current campaign TU while retaining the current `real_math.h`. | Output remains byte-identical to B12: 640/640, 16/16, SHA `743ed5d2856f5c0df645cc0775e2861082933a39595bb904a5b64e40ea0802d0`. The historical source text alone does not reproduce its own exact historical object when compiled against the current inline-header environment. | **Rejected as an isolated control.** Retain the historical topology only while testing its proven companion header provenance; source transplantation by itself is insufficient. |
| B23 | `_bsp3d_clip_line_to_leaves` and `_code_00136900` | Compile the historical line topology with the four accompanying `real_math.h` changes from the same historical commit: `midpoint3d` returns its result, `cross_product3d` stores its `i` expression directly, `plane3d_distance_to_point` drops harmless parentheses, and `vector_intersect_plane3d` uses the historical helper-based expression. | **Line becomes strict exact:** 640/640 bytes, 16/16 relocations with identical addresses/destinations/order, SHA `5e810515880b3b18278af47490fef149afe2940b85484b118ac41cc16e1b43d1`. The polygon helper stays at the B07 result: 720/720, 18/18 destinations/order with the same two-byte mid-window address drift and SHA `c64a024d88dfd597d533e5caa016e40f5b83be93c1e848220a02ad0bdd551613`. Exact siblings and all data remain exact. | **Promising provenance control, not yet accepted globally.** Isolate the minimum responsible header change one factor at a time, force-rebuilding because Ninja does not track this header dependency, then run a whole-campaign blast-radius gate before retaining any shared-header edit. |
| B24 | `_bsp3d_clip_line_to_leaves` | Isolate the four historical-header differences one factor at a time. The only controlling factor is removal of the harmless outer parentheses in `plane3d_distance_to_point`; midpoint return, cross-product assignment spelling, and intersection-helper spelling are non-controls. Localize the proven expression in `bsp3d.c` rather than changing the widely included shared header. | The file-local inline distance helper reproduces the historical expression and makes the line function strict exact at 640/640, 16/16, SHA `5e810515880b3b18278af47490fef149afe2940b85484b118ac41cc16e1b43d1`. `source/math/real_math.h` has the same Git blob hash as HEAD, so there is no shared-header blast radius. | **Accepted.** This is the minimum historical source-control and contains its codegen effect to this TU. |
| B25 | `_code_00136900` | Use a second file-local plane-distance helper whose `dot_product3d` arguments are reversed only in the coplanarity loop. | The loop becomes instruction-exact through `+0xac`; the function remains 720/720 and 18/18, candidate SHA improves to `c8749213af592ef64b50c4a905a1d746d2b6618c2cf6f6ea8f32050c523fdd67`. The only structural residual starts in the inlined cross product at `+0x106`: January reloads `edge0.i` then drops an x87 temporary, while the candidate reuses it via `fxch` and `fmuls`. | **Accepted.** The reversed dot operands are a measured loop-only control and preserve semantics. |
| B26 | `_code_00136900` | Store the cross product into `edge1` instead of `normal`, then consume `edge1`. | Function remains 720/18 but regresses broadly, SHA `a728...`; stack/value lifetimes no longer match. | **Rejected and reverted.** Output aliasing an input is not the January topology. |
| B27 | `_code_00136900` | Nest `cross_product3d` as the reversed first argument of `dot_product3d`. | Byte-identical to B25, SHA `c8749213...`. | **Rejected and reverted.** Nesting and dot-argument direction remain non-controls at this site. |
| B28 | `_code_00136900` | Reintroduce the historical direct-`i` cross helper together with B25. | Byte-identical to B25, SHA `c8749213...`. | **Rejected and reverted.** The historical cross spelling is a non-control after inlining. |
| B29 | `_code_00136900` | Replace cross-then-dot with the shared `triple_product3d` helper. | Candidate shrinks to 704/18, SHA `89a...`; the operation grouping no longer matches January. | **Rejected and reverted.** |
| B30 | `_code_00136900` | Nest both `vector_from_points3d` calls directly into `cross_product3d`. | Byte-identical to B25, SHA `c8749213...`. | **Rejected and reverted.** Call nesting is a non-control. |
| B31 | `_code_00136900` | Express the coplanar normal calculation as a fully nested C89 initializer. | Byte-identical to B25, SHA `c8749213...`. | **Rejected and reverted.** Initializer topology is a non-control. |
| B32 | `_code_00136900` | Retain the pointer returned by `cross_product3d` and pass that pointer to `dot_product3d`. | Regresses to the earlier 720/18 SHA `c64a024d...`; the B25 loop-order improvement is lost downstream. | **Rejected and reverted.** Return-pointer liveness is not the January source shape. |
| B33 | `_code_00136900` | Replace `dot_product3d(&normal, &plane->n)` with the equivalent explicit `normal.n[0..2] * plane->n.n[0..2]` expression. | Candidate shrinks to 704/18, SHA `4bcdb599...`, with the first later relocation six bytes early and the recursive relocation eleven bytes early. | **Rejected and reverted.** Manual union-array expansion eliminates required inline-helper scheduling; retain the helper call. |
| B34 | `_code_00136900` | Scan all 833 January split objects for the target's fixed x87 reload/drop opcode motif while wildcarding frame-slot displacements. | The motif occurs exactly once: `_code_00136900` in `bsp3d.obj`. No in-tree machine-code donor exists for source-topology transfer. | **Negative provenance result.** Do not claim a donor for this schedule without new external evidence. |
| B35 | `_code_00136900` | Retain `vector_from_points3d`'s returned pointer for `edge1` and pass that pointer to `cross_product3d`. | Function remains 720/18 but regresses from B25 to SHA `c64a024d...`; the otherwise exact coplanarity loop loses its B25 scheduling improvement. | **Rejected and reverted.** A retained edge pointer alters whole-block liveness and is not the January source shape. |
| B36 | `_code_00136900` | Narrow the `side` boolean's source lifetime by declaring and initializing it in a nested scope immediately after the cross product. | Byte-identical to B25: 720/18, SHA `c8749213...`. | **Rejected and reverted.** Boolean scope/initializer topology is frame-neutral and cannot control the x87 reload/drop decision. |
| B37 | `_code_00136900` | Use a file-local cross helper with the same `k,j,i` scalar calculations but reverse the result-store order to `k,j,i`. | Candidate remains 720/18 but grows the mid-window by six bytes, SHA `d4dc8b6d...`; the first post-window relocation is six bytes late and the recursive relocation is one byte late. | **Rejected and reverted.** Result-store order is a real but wrong codegen control; January does not store the components in `k,j,i` source order. |
| B38 | `_code_00136900` | Exhaust the remaining cross-result store permutations in the same file-local helper: `i,k,j`; `j,i,k`; `j,k,i`; and `k,i,j`. | All remain nonexact at 720/18. Candidate SHAs respectively: `f7e8da8c...`, `83013d3d...`, `711d9090...`, and `36046d8a...`. None reproduces January SHA `d7e2964b...`; all differ more broadly than B25. | **Rejected and reverted.** The complete six-permutation store-order matrix is exhausted; retain the shared `i,j,k` helper form. |
| B39 | `_code_00136900` | Exhaust the four previously unmeasured scalar-calculation declaration orders while retaining `i,j,k` result stores (`k,i,j`; `j,k,i`; `j,i,k`; `i,k,j`). | Every variant is byte-identical to B25 at 720/18, SHA `c8749213...`. Together with B09, all six scalar declaration orders are now measured. | **Rejected and reverted.** Scalar declaration order is conclusively a non-control. |
| B40 | `_code_00136900` | Test zero-byte `_ReadWriteBarrier()` positions between the two edge computations, immediately before the cross product, and immediately after the cross product. | All three variants are byte-identical to B25 at 720/18, SHA `c8749213...`. | **Rejected and reverted.** The intrinsic is eliminated without changing this x87 schedule; all meaningful boundaries around the cross expansion are exhausted. |
| B41 | `_code_00136900` | Apply only `5093ac1a`'s typed second-edge alias to the B25 foundation while retaining the foundation's surrounding polygon-helper topology. | The function remains 720/720 with 18/18 relocations, but is nonexact at SHA `1c09b23514c637e3954dd1711a001aa7305421809e7489eeea34c28c4e09fd3f`. | **Rejected as an isolated control.** The alias is not independently decisive outside its recovered source context. |
| B42 | `_code_00136900` | Pair B41 with a translation-unit-local copy of upstream's direct-`i` cross-product spelling. | Output remains byte-identical to B41 at 720/18, SHA `1c09b235...`; the helper spelling is still a non-control. | **Rejected and removed.** Keep the shared readable `cross_product3d` helper and avoid duplicated math. |
| B43 | `_code_00136900` | Adapt the complete polygon-helper topology from `5093ac1a`: recovered local ordering, clipped-polygon store order, direct child traversal, branch spelling, and the typed `edge2` alias, while retaining current campaign types and the shared cross helper. | **Strict exact:** 720/720 bytes, all 18 relocation addresses/destinations/order exact, SHA `d7e2964b9724f725ebd997f8789f3f01bbf22621041336e92e4083fbd115e41c`. All three exact siblings remain exact. | **Accepted.** This is the smallest measured donor-derived combination that resolves the residual without shared-header changes. |
| B44 | `_code_00136900` | Remove only the typed alias from B43 and pass `&p0p2` directly, preserving every other recovered source choice. | Nonexact at 720/18, SHA `e109516d214a80fdaed105279d9c444e8837367300789119a5092177f2428908`; eight later relocations move two bytes early. | **Rejected and reverted.** This negative control proves the readable typed alias is necessary in the recovered topology. |
| B45 | whole object | Remove the unused public callback typedefs and clip prototypes from `bsp3d.h`; keep the callback contracts local to `bsp3d.c`, where they are used. | A forced rebuild remains 4/4 strict exact and 7/7 target-owned data exact. Repository search finds no external consumer of those declarations, and `bsp3d.h` returns to the integrated-base blob. | **Accepted.** No shared-header churn or blast radius is required. |
| B46 | both distance helpers | Replace the incompatible effective-type casts from `real_point3d *` to `real_vector3d *` with fully typed component expressions. Preserve point-first multiplication in the line helper and plane-first multiplication in the polygon helper. | A forced rebuild is byte-identical to B43: all 4 functions and all 7 target-owned data sections remain strict exact, including `_bsp3d_clip_line_to_leaves` SHA `5e810515...` and `_code_00136900` SHA `d7e2964b...`. | **Accepted.** The final source performs no point/vector pointer reinterpretation and remains strict-complete. |

## Data and ownership audit

Fresh section-level comparison proves all seven January-owned `.rdata` items
strict exact in size, bytes, relocation count, and semantic ownership:

| Item | Size | Relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `t>0.f && t<1.f` assertion | 15 | 0 | `8cb52e767c010bf399f1c1cfc9263ae774660a5f99851965fca044f60900634c` | Exact |
| `c:\halo\SOURCE\physics\bsp3d.c` | 31 | 0 | `ee85dfb302a840043c298961c8c24405abe99b4b9431f7c4bc1909cc4ace8b57` | Exact |
| `__real@39800000` | 4 | 0 | `646f853a3d35415e16510c87bce15830321c3aa5e881eb746eab7b6c13d6bbf5` | Exact |
| `__real@b9800000` | 4 | 0 | `596efe745486347825731e447c921d9ac232490395ed5ae9442ea0eaa5f543ae` | Exact |
| `clipped_counts[0]...` assertion | 51 | 0 | `3e9bc54b9e105a289fcd29c17e1d0b59515689b63f323046acb763201027610e` | Exact |
| `vertex_count<MAXIMUM...` assertion | 50 | 0 | `3b3efa7aea9bd15b07a996ebd2d732a6b75a353139542069800cf8bdadada6cf` | Exact |
| `vertex_count>=NUMBER...` assertion | 46 | 0 | `f960479e105d5dd4d9a2d6b165c775d9ecbf9d2e09be6b792956490ed69adc2c` | Exact |

There is no remaining data or ownership blocker in this object.

## Do not repeat

- Do not retry declaration splitting, scalar staging, term permutation, or
  operand reversal for the second line-distance expression; B13--B15 and B18
  compile identically to B12.
- Do not retry direct scalar expansion, cross-product temporary/declaration
  permutations, commutative multiply spelling, or nested inline calls for the
  polygon normal; B08--B11 and B16--B17 either regress or compile identically
  to B07.
- Do not replace the January predicates with ordinary `>=` or negated `<`
  spellings; B03 and B06 prove those forms have different unordered/NaN or
  equality behavior.
- Do not transplant the typed edge alias alone. B41 proves that the recovered
  surrounding polygon-helper topology is part of the legal-C control.
- Do not duplicate or modify `cross_product3d`, `real_math.h`, or `bsp3d.h`;
  B42 and B45 prove those changes are unnecessary.
- Do not reintroduce point/vector effective-type casts in the local distance
  helpers. B46 proves natural typed component math is byte-exact.

## Resolution provenance

Upstream commit `5093ac1a` satisfies the ledger's former donor reopen criterion.
Its source is not copied blindly: B41, B42, and B44 isolate the controlling
combination and prove that neither the alias alone, a duplicated inline helper,
nor the topology without the alias is sufficient. B43 is the strict result.

## Terminal closeout state

`bsp3d.obj` is strict-complete and admitted as `Matching`:

- 4/4 functions strict exact, totaling 1,536/1,536 padded code bytes and all
  38 relocation identities, addresses, destinations, and order exact;
- 7/7 January-owned `.rdata` sections strict exact, totaling 201 logical
  bytes, with exact ownership identities, flags, symbols, and zero relocations;
- forced object rebuild and the full 568-edge Halo plus libcmt build pass;
- strict semantic report: 470 units, 3,614 functions evaluated, 3,451
  semantic-exact, 3,535 accepted-exact, and zero unit errors;
- progress: 373/833 objects overall and 271/468 Halo objects, with
  423,033/2,198,102 overall code bytes and 410,119/1,770,166 Halo code bytes;
- admission: zero candidates and zero revocations; only the unrelated,
  pre-existing `shell_xbox` contradiction remains;
- parked validation: 4 active, 0 stale, 0 invalid; the BSP3D entry is removed;
- all 179 tooling tests pass;
- post-commit forced whole-TU regression snapshot/check passes without a
  function, non-code, symbol-ownership, or comparison-state change;
- final scope is only `source/physics/bsp3d.c`, its `Matching` status, and this
  Codex-owned ledger. `bsp3d.h`, shared math headers, frozen objects, and
  Claude-authored Markdown are unchanged.
- both local distance helpers use fully typed point and plane component access;
  no incompatible point/vector pointer reinterpretation remains.

Disposition: strict-complete, full-build clean, regression-stable, and
admitted without shared-header churn.
