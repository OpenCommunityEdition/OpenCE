# Exact-match acceleration playbook

This document records techniques with broader application to the January 2002
Halo Xbox decompilation. Object-specific measurements remain in
`docs/object_matching_logs/`; this file contains only controls, triage rules,
and campaign practices that have survived strict comparison.

The objective is faster reconstruction without lowering the bar. A candidate
is not exact because it is correct, similar, the same size, or 99% fuzzy. The
admission evidence remains XDK 3911 output, exact normalized bytes, exact
relocation shape and resolved destinations, exact owned data/layout, and a
whole-translation-unit regression audit.

## Campaign strategy

1. **Finish objects, not isolated functions.** Prefer a unit with a bounded
   residual and known data ownership over a larger number of unrelated leaves.
   Once an object is opened, carry it through code, data, ownership, regression,
   documentation, and `Matching` admission unless a measured compiler tie
   satisfies the parking standard.
2. **Prioritize reusable foundations.** Scenario/BSP accessors, real-math
   leaves, player/unit layouts, tag/object accessors, datum iterators, compiler
   wrapper families, and complete donor translation units reduce later work.
3. **Separate reconstruction from tuning.** First recover the January
   semantics, types, call graph, private ABI, and owned data. Tune source shape
   only after target size/relocation evidence says the structure is close.
4. **Work atomic private-ABI clusters together.** Static helpers and their
   callers can form a register-allocation fixed point. Splitting them across
   lanes produces false blockers.
5. **Use a bounded experiment budget.** Test one source factor at a time,
   record the exact result, and stop repeating compiler-equivalent spellings.
   A parked result needs explicit size, relocation, first-divergence, and
   exhausted-control evidence.
6. **Automate candidate generation, never acceptance.** Shape-transfer and
   donor tools may propose source, but the unchanged strict comparator and
   regression gates decide admission.

### Select closeouts from strict evidence, not the ordinary percentage

Run `python -m tools.rank_object_closeouts` after regenerating the ordinary
and semantic reports. The ranking deliberately combines:

- the complete function inventory from the ordinary report;
- the reviewed strict `accepted_ledger`;
- object `Matching` status; and
- the evidence-backed parked-function catalog.

This prevents three recurring campaign mistakes:

1. treating a differently numbered local jump-table label as unfinished code;
2. repeatedly dispatching a one-function object whose only residual is already
   a proven compiler tie; and
3. overlooking a data-only closeout after every function is exact.

The tool is a triage aid, not an admission oracle. `NEAR_CLOSEOUT` means the
remaining work is unparked and bounded; it does not predict that source tuning
will succeed. `LEDGER_REVIEW` means earlier object work exists and its
do-not-repeat/reopen rules must be read before dispatch. `PROVEN_BLOCKED` means
every remaining function is in the
reviewed parked ledger and must not be reopened without satisfying its stated
reopen criterion. Always read the object's Markdown ledger before editing.

## Evidence hierarchy

From strongest to weakest:

1. January target bytes, relocation metadata, linked addresses, and section
   ownership.
2. January sibling functions and exact donor translation units.
3. January assert strings, source paths, help tables, tag definitions, and call
   sites.
4. XDK 3911 headers and synthetic compiler experiments under the exact flags.
5. October Xbox, Halo PC/CEA/HCEX PDBs, community forks, and later-engine types.

Levels 4 and 5 generate hypotheses. They do not override January evidence.
Every imported field, enum, prototype, or name must be corroborated locally.

## High-yield source controls

### Ownership and alias modeling

VC7 code generation depends on what it can prove about globals:

- Aggregate storage can force address common-subexpression elimination or
  destroy an index because sibling fields may alias.
- Separate globals can preserve a live SIB index and eliminate reloads.
- A one-field structure can retain field-oriented optimizer behavior while a
  separate array supplies non-aliasing proof. This closed
  `game_engine_multiplayer_sounds.obj`: it preserved `dec; mov; je` flags in
  the update while producing SIB stores in the queue helper.
- Static versus external linkage affects alias knowledge, section order,
  symbol storage class, and whether zero-initialized data becomes `.bss` or a
  tentative/common symbol.

Always verify the full object. A linkage change can leave every function exact
while silently deleting or reordering `.bss`.

### Statement and branch order

- VC7 often reflects the original failure-first versus success-first topology.
  Reversing an `if` can fix multiple relocation-address regions at once.
- Tail-merging is sensitive to deliberately asymmetric source duplication.
  If two switch cases must share a forward machine-code tail but an explicit
  shared label sinks a callee-save or changes the prologue, keep the simple
  early path's tiny terminal test locally and route the longer path forward to
  the other case's copy. In `shaders.obj`, VC7 cross-jumped that readable
  duplicate into one shared target tail while preserving the required ESI
  lifetime. Verify relocation identities: this is a measured block-placement
  control, not permission to duplicate arbitrary logic.
- Common stores in two branches may be tail-merged, but a constant-valued
  branch can instead materialize an immediate store and grow the function.
- Comparison polarity matters even when predicates are logically equivalent;
  it controls `cmp` operand order, signed branches, and register lifetimes.
- Use a single return when the target structure permits it. When the exact
  source requires multiple source returns to reproduce a common compiler
  epilogue, record that as an evidence-backed house-style exception.

### Scalar stores versus aggregate operations

- Field-wise stores give the scheduler independent operations and can reproduce
  interleavings that `{0}`, aggregate assignment, `memset`, or `rep stos`
  cannot.
- Conversely, aggregate assignment is useful when the target uses `rep movs`
  or a single calculated element address.
- Preserve target load and store order before trying declaration-order noise.

### Integer width and signedness

- `movsx`/`movzx`, partial-register operations, and signed branch opcodes reveal
  the real promotion boundaries.
- Function parameter widths matter even when the x86 ABI still occupies a
  four-byte stack slot. In `rasterizer_xbox_motion_sensor.obj`, changing a
  render-target parameter from `long` to the proven `word` transformed
  `movzx` into the target's `xor reg, reg; mov reg16, [memory]` sequence and
  closed the entire remaining half of a 1,920-byte function.
- Change signedness only where the target proves it. A `jle` versus `jbe`
  difference is often a type error; an ESI/EDI mirror is not.
- Recover narrow accumulators instruction by instruction rather than relying
  on C's default integer promotions.

### x87 controls

- A value live after a store selects `fst`; a dead value selects `fstp`.
- Source operand order controls non-commutative subtraction/division memory
  forms. VC7 canonicalizes commutative addition/multiplication; spelling order
  is not a lever there.
- Passing, returning, or globally storing a floating result produces distinct
  stack-staging patterns.
- Never substitute mathematically equivalent expressions that change signed
  zero, NaN payload/sign, intermediate precision, or rounding.

### XDK inline wrappers

Stock XDK `D3DINLINE` under this compiler emits the January out-of-line D3D
wrapper families when called normally. Address-taking forces a different
standard-ABI body, and redefining `D3DINLINE` as plain `static` can emit a
thunk. Prefer the stock header form and verify every emitted wrapper in the TU.
One motion-sensor translation unit instantiated ten such wrappers exactly in a
single build, including both 432-byte and 544-byte dispatchers. Treat the
header recipe as a high-yield candidate generator, but still compare every
wrapper and its relocation destinations independently.

### Diagnose the first true divergence

- Ignore downstream branch displacements and relocation-address drift until
  the earliest semantic or instruction-length difference is closed.
- Decode cdecl pushes right-to-left before blaming register allocation. A wrong
  argument order can make a large suffix look like a scheduler problem.
- If a small type correction makes a long suffix exact automatically, the
  suffix was displacement drift, not a collection of independent problems.
- Zero-byte intrinsics such as `_ReadWriteBarrier()` are measured controls, not
  universal fixes. They can prevent desirable argument hoisting and make an
  otherwise close function worse.

### Recover the type before tuning the schedule

A value that stays live in a register longer than expected is often evidence
of a wrong prototype or field width, not a mysterious compiler decision.
Before trying declaration orders, barriers, or control-flow rewrites:

- Check what the target returns in `EAX` or `AL`. If the target preserves a
  meaningful value until `ret`, a function currently declared `void` may
  actually return that value.
- Check whether the target consistently reads, masks, or writes only the low
  byte or low word. A `char`, `boolean`, `short`, or `word` may be the real
  type even when a 32-bit placeholder produces correct gameplay behavior.
- Prefer the smallest type that is supported by all call sites and target
  instructions. Do not narrow a type merely because it happens to improve the
  bytes.

In `texture_page.obj`, recovering `spacing_mask` as a 16-bit `word` made a
736-byte packing function exact. Recovering
`texture_page_textures_end` as a `boolean` function explained why January kept
the packing result in `AL` across the loop and made its remaining 112 bytes
exact. In plain English: the compiler was not being stubborn; we had told it
the wrong facts about the program.

### Private ABI controls

A source-static, non-address-taken, frameless helper can receive arguments in
ECX/EDX or another caller-derived convention. External linkage, address-taking,
or a stack frame can destroy that convention. Work from a forced-register
caller outward; do not tune a private helper in isolation.

### Compiler provenance before compiler hunting

Do not attribute a stubborn register or scheduling residual to an unknown
compiler patch until the target's own debug records have been checked.
PDB 2.00 module streams can contain `S_COMPILE2_ST` records with independent
front-end and back-end version triples. For the January `players.obj`, the
record proves `13.0.9254` for both halves, matching the campaign compiler's
reported build.

Use this order:

1. Decode the target compiland's compiler record.
2. Compile the unchanged TU once with each genuinely distinct local compiler
   binary.
3. Compare normalized hashes for every residual, not just the first
   instruction.
4. If all variants are byte-identical, stop the compiler hunt and return to
   source provenance or a demonstrated legal-C control.

For Players, VC7 `13.00.9210` and `13.00.9254.1` emitted byte-identical
candidate hashes for all four residuals. That falsifies the available
patch-level lever and prevents repeated SDK downloads or speculative flag
changes.

## Transfer and donor rules

- A donor is usable only when its source text is resolved and its target
  machine shape is exact.
- Derive every recipient callee/global substitution from the recipient target
  relocations, never by name similarity.
- Equal relocation-masked bytes can hide a wrong-callee wrapper. Destination
  identity and addends remain strict.
- Preserve target statement order; relocation sequence is often its strongest
  surviving proof.
- Reject stubs, unresolved donors, inconsistent substitution maps, prologue
  mismatches, and data-ownership changes automatically.

### Candidate-only helper COMDATs

An inlined helper can occasionally leave an unreferenced out-of-line COMDAT
copy in the candidate object. Never dismiss it just because the target
function compares equal. It is acceptable only after all of the following are
proved:

- the extra section is a discardable COMDAT with the expected selection rule;
- no relocation in the object refers to it;
- every target-owned function and data section remains strict-exact;
- the ordinary link actually discards it; and
- the consolidated link, semantic audit, and progress report remain clean.

This is a linker-level representation difference, not permission to ignore
extra ordinary code or data.

## Parking standard

Park only when:

- target and candidate padded size are exact;
- relocation count, types, and destinations are exact, or a reviewed
  representation-only difference has independently resolved destinations;
- the remaining divergence is classified precisely;
- legal-C controls appropriate to that class have been exhausted;
- the object ledger records a reproducible first divergence and reopen
  criterion.

A 92% function with different size or relocations is structural work, not a
compiler tie. A one-instruction scheduler swap can be parked only after the
dependency and source-control search are documented.

## Admission checklist

Before marking an object `Matching`:

1. Build with XDK 3911 and the repository flags.
2. Compare every function with the hardened COFF comparator.
3. Verify every owned `.data`, `.rdata`, and `.bss` span, including symbol
   offsets, storage classes, and relocation destinations.
4. Confirm the ordinary report does not hide a strict false positive.
5. Run the whole-TU regression gate against a pre-edit snapshot. Review every
   intentional ownership transition; never suppress unrelated failures.
6. Run the consolidated build, semantic audit, and progress report.
7. Update the object ledger with retained and rejected experiments.
8. Mark `Matching` only after all evidence passes.

### Greenfield regression gates

An empty skeleton has no previously exact code or owned data for a pre-edit
manifest to protect. A correct reconstruction will therefore appear to the
regression gate as a set of new functions, symbols, COMDATs, and data. Do not
force that report green and do not waive its findings:

- confirm that it reports no changed previously exact function or section;
- classify every addition against the January target;
- admit the object only after strict function, relocation, ownership,
  consolidated-build, and semantic-progress checks pass;
- immediately capture a post-admission manifest and verify it is stable, so
  later edits are checked against the newly completed object.

## Original bugs

Exact reconstruction preserves proven original behavior. Mark a defect
`BUG (original)` only when January evidence proves it. In the matching source,
retain the original behavior and add a concise comment describing a safe
corrected-build alternative. Do not silently repair it and do not label our
own reconstruction mistake as an original bug.

## Prohibited shortcuts

No inline assembly, volatile byte forcing, undefined aliasing, object-byte
patches, compiler pragmas used only to force bytes, flag changes, fuzzy-only
credit, or invented symbols/types. Compiler experiments belong in disposable
worktrees; production admission must remain readable evidence-backed C.

