# `_structure_test_line2d` exact-match log

Target: January 14, 2002 Xbox build. Compiler: XDK 3911 CL 13.00.9254.1,
`/O2 /Oy- /DDEBUG /Dxbox`.

This isolated lane owns only `_structure_test_line2d`. The six-function object
remains an atomic parent-lane integration.

## Authoritative baseline

- Target padded size: 1,104 bytes.
- Target relocations: 29.
- Target normalized SHA-256:
  `664912aede956fbb3bc49b4517a90d315d445777054a1823c0eafadd2a1f2bc0`.
- HCEA provides an exact-unique name, prototype, three named locals, and a
  control-flow hypothesis. January disassembly remains authoritative.

## Experiments

1. **January-adapted HCEA topology.** Typed tag-block access, January assertion
   sites/line numbers, direct inlined passability logic, short edge count,
   recursive centroid correction, and three January return epilogues.
   Result: target/base padded size `1104/1088`, frame `0x70/0x78`, `29/29`
   relocations. First address drift was the first assert (`+0x4e/+0x44`) because
   `surface_index` and `recursed` were initialized after it. Rejected as the
   baseline only.

2. **January initialization order and surface-pointer reuse.** Moved
   `surface_index`/`recursed` before the result assert, removed a redundant
   `first_edge_index` scalar in favor of the live `surface` pointer, preserved
   the crossing neighbor only where it is live, and used the centroid's x/y
   storage as the recursive point. This made the target/base padded sizes
   `1104/1104`; all 29 relocation addresses and identities became exact through
   the `1.0f` relocation at `+0x2e0`. Frame improved to `0x70/0x74`. This was the
   first size-exact structural foundation.

3. **Eliminate the named crossing-neighbor local completely.** Repeated the
   edge subscript at each use. Padded size and relocation count stayed
   `1104/29`, but VC7 retained an address into `collision_edge` instead of the
   neighbor value, drifting at `+0x207` and moving the breakable-surface call to
   `+0x237` instead of target `+0x233`. Rejected; the scoped scalar is proven.

4. **Recursive-point allocation families.** Tested (a) direct cast of
   `point_in_surface`, (b) a named pointer to that same storage, and (c) a
   separately declared `real_point2d`, including declaration order on both
   sides of `p0_result`. The pointer form was byte-identical to the direct cast.
   A separate point regressed the x87/branch region (first zero-constant
   relocation moved `+0x195 -> +0x18c`) and did not fix the frame. Rejected.

5. **Direct `result->t` computation.** Removed the local `t` and computed the
   collision fraction directly into the result. VC7 shortened the meaningful
   body and padded size to `1088`, with the first collision projection moving
   `+0x3a7 -> +0x3a3`. Rejected; January has a local `t` staged in the first
   argument home at `[ebp+8]`.

6. **January numerator expression.** Replaced HCEA's
   `-(epsilon - crossing) / denominator` spelling with the January opcode order
   `(crossing - epsilon) / denominator`. This removed the non-January `fchs`
   while preserving all exceptional-value behavior implied by the target x87
   sequence. Accepted.

7. **Hit-point statement order.** Tested `y` then `x` and `x` then `y` after
   computing `t`. `x` then `y` follows January's store order, but the current
   local colouring still keeps `t` live with `fsts` rather than January's
   `fstps` plus reload. Kept as the evidence-backed source order; it does not
   alone change the result.

8. **Float declaration order.** Tested the initial semantic grouping, HCEA's
   scalar-local order, and all six permutations of the collision-tail scalar
   declarations. These preserve size and relocation count, but do not alter
   the `fsts`/`fstps` decision. Rejected as neutral.

9. **Scalar direction values.** Tested HCEA's individual `dx`/`dy` and eight
   individual edge-relative scalar values. This was `1104/29`, frame `0x74`.
   It demonstrated correct semantics but also proved that scalar spelling was
   responsible for the four-byte surplus local slot. Rejected after experiment
   12 recovered January's aggregate layout.

10. **Scope, pointer, and tail-control families.** Tested a nested calculation
    scope, `long const *` versus byte-pointer views of the bit vector, duplicated
    blocked-result tails, and an explicit `goto blocked_at_start`. None reduced
    the frame or changed the collision x87 lifetime. The duplicated natural
    tail remains because it is readable and VC7 cross-jumps it; the pointer and
    extra-scope forms were rejected as neutral.

11. **Recursive and collision-local allocation families.** Retested a separate
    recursive `real_point2d`, all meaningful collision-local declaration
    permutations, and a separately named hit-point alias. A separate recursive
    point produced frame `0x78`; declaration permutations were byte-neutral.
    Rejected.

12. **Native vector aggregates.** Replaced the eight individual direction
    scalars with five native aggregates: `delta`, `a_to_p0`, `edge_vector`,
    `b_to_p1`, and `b_to_p0`. This was the structural breakthrough: VC7 coloured
    the frame exactly at `0x70`, assigned every vector and point to January's
    stack slot, retained padded size `1104`, and retained all 29 relocations.
    Accepted. These readable engine types are now the current foundation.

13. **Native inline math helpers.** Expressed the determinant tests and
    collision fraction with `cross_product2d`, the edge length with
    `magnitude2d`, and the hit point with `point_from_line2d`. The determinant
    and magnitude helpers reproduce the natural January dependency graph and
    are retained. `point_from_line2d` remains an inline spelling option but does
    not force the required `fstps` of `t`; direct field assignments make the
    remaining mismatch easier to inspect and are retained.

14. **`t`-lifetime diagnostics.** Tested an early `result->t` write, loading
    coordinates through `result->t`, `(void)&t`, `*(&t)`, a scoped pointer alias,
    and `_ReadWriteBarrier()` immediately after computing `t`. The early result
    forms changed instruction selection and/or shortened the function. Address
    aliases were neutral. The barrier proved the key mechanism by forcing the
    exact `fstps [ebp+8]`, exact hit-point stack slots, and exact first projection
    relocation at `+0x3a7`, but it severely rescheduled the preceding collision
    setup and moved the epsilon relocation. It was diagnostic only and was
    reverted; no barrier is present in the foundation.

15. **HCEA hit-point and declaration topology.** Tested `y` then `x`, exact
    declaration order `edge_length`, `t`, `hit_point`, a legal automatic
    aggregate initializer, and a natural `scale_vector2d` temporary. The first
    two were byte-neutral. The initializer selected the direct x-before-y
    ordering but still kept `t` in x87. `scale_vector2d` added an `fxch` and
    moved the first call to `+0x3a6`, one byte short, without forcing `fstps`.
    Rejected; restored the smallest direct x/y source.

16. **Boolean store order and determinant operand spelling.** Reordered the
    per-iteration `crossed_any`/`reached_target` zero stores to match January's
    shallow stack-slot write order. Accepted. Directly spelling the two inner
    cross products, including January's exact left-memory operand order for all
    four multiplications, was codegen-equivalent to the inline helper and did
    not fix the residual; the helper form is preferred for readable final
    source.

17. **Hit-coordinate commutative operand order.** Tested January-compatible
    `p0 + t * delta` against `delta * t + p0`. VC7 canonicalized both to the
    same x87 sequence and retained `t` in the stack. Rejected as neutral.

18. **Named numerator and denominator.** Split the collision fraction into
    natural `numerator`, `denominator`, and `t` statements. VC7 fully folded
    the first two locals and emitted the same frame and x87 sequence, including
    `fsts [ebp+8]`. Rejected as neutral.

19. **HCEA `.n[]` member spelling.** Expanded both inner determinants using
    the HCEA array-member spelling and original operand order. VC7 emitted the
    same two commutative operand selections as the inline helper. Rejected as
    neutral; no cast or codegen-forcing construct was used.

20. **Blocked-before-check CFG topology.** Placed the blocked result block
    textually before a separately labelled final check, then tested both the
    positive success predicate and January's negative predicate branching
    backward to the blocked label. VC7 canonicalized both to the existing
    condition-before-blocked layout. Rejected; restored the clearer HCEA
    `goto blocked_at_start` topology.

21. **HCEA scalar aliases over vector storage.** Introduced four scoped,
    semantically named scalar aliases for the aggregate components before the
    determinant tests. VC7 eliminated every alias and emitted the same frame,
    hash, and operand choices. Rejected as neutral.

22. **Recursive state statement order.** January stores `recursed = TRUE`
    before copying `p0_result.surface_index`; the candidate initially used the
    opposite source order. Swapping these two independent statements made the
    four-byte window at `+0x32f` instruction-identical without changing frame,
    size, or relocations. Accepted.

23. **Magnitude component topology.** Tested an explicit reversed component
    sum and named `edge_i_squared`/`edge_j_squared` locals in target-evidenced
    statement order. VC7 eliminated the named locals and retained its existing
    y-before-x x87 schedule. Rejected; restored readable `magnitude2d`.

24. **Chained inline point helper.** Passed the return from
    `point_from_line2d` directly as the projection call's point argument. VC7
    emitted byte-identical code to the two direct field statements and kept
    `t` in x87. Rejected in favor of the explicit HCEA-recovered assignments.

25. **HCEA block-scoped initialized `t`.** Declared and initialized `t` only
    after computing `edge_length`, in the same nested lifetime as `hit_point`.
    VC7 emitted the same code and frame. Combining that scope with HCEA's
    y-before-x source statement order still emitted y before x, unlike January.
    Restored January's proven x-before-y statement order and the simpler single
    collision-local block.

26. **Vector declaration scope.** Moved the four vector declarations to the
    top of their natural edge-iteration block so the reconstructed C follows
    the project's house formatting without a surplus nested scope. The rebuilt
    normalized hash was unchanged. Accepted as a codegen-neutral readability
    cleanup.

## Current best

Status: evidence-backed readable foundation, **not strict exact** and not safe
to mark the object Matching.

- Padded size: target/base `1104/1104`.
- Base normalized SHA-256:
  `55ccd1c4c2cab82eb867a354392b848b9a1851bc29a4c0dab3f4e64aadaece83`.
- Relocation-masked differing bytes: 193; first at `+0x1ae`, last in trailing
  padding at `+0x449` because the final block order differs.
- Relocations: `29/29`; all identities match and every relocation address is
  exact through `+0x376`. The remaining projection-call addresses are
  `+0x3a7/+0x3a4`, `+0x3de/+0x3ee`, and `+0x426/+0x423`.
- Frame: target/base `0x70/0x70`, exact.
- The remaining collision-tail mechanism is specific: January stores `t` with
  `fstps [ebp+8]` and reloads it for both point coordinates; the candidate uses
  `fsts [ebp+8]`, retains `t` in x87 for one coordinate, and reaches the first
  projection call three bytes early. After that, January orders the blocked
  return before the final success-condition block while the candidate orders
  the condition before the blocked return. These are codegen/source-topology
  residuals, not missing behavior, arguments, calls, or data.
- No experiment weakened relocation identity or changed a compiler flag.

## Preserved shipped bug

January indexes `pathfinding_surfaces[neighbor_surface_index]` before checking
the boundary sentinel. On a boundary edge this reads one byte before the array.
The source preserves that ordering for binary fidelity. A safe build would test
`neighbor_surface_index != NONE` before reading the byte.

## Validation

- Isolated XDK 3911 rebuild succeeded.
- Consolidated `halobetacache_build` succeeded with exit code 0.
- Hardened COFF comparison intentionally reports non-exact; no Matching label,
  semantic credit, comparator exception, compiler flag, or configuration was
  changed.
