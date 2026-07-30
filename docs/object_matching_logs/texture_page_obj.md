# `texture_page.obj` exact-match ledger

## Scope and invariant

- Translation unit: `source/memory/texture_page.c`
- Target: January 2002 `build/split/source/memory/texture_page.obj`
- Closeout base: `c53047fb`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Acceptance: every function and every target-owned data section must pass the
  hardened COFF comparator. No inline assembly, volatile codegen coercion,
  undefined behavior, byte patches, or compiler-flag changes.

This object is already behaviorally reconstructed. Nine of thirteen functions
are strict-exact, and all target-owned data is strict-exact. The four remaining
functions have equal target/candidate padded sizes and relocation counts.

## Baseline residuals

| Function | Size T/B | Relocations T/B | Target SHA-256 | Candidate SHA-256 |
| --- | ---: | ---: | --- | --- |
| `code_0010f730` | 96/96 | 6/6 | `4ea8aca9a5fe88693bb7cb72005f6a504b410ad15fd073c0725b08e4ec189953` | `1549ee0ee677c135307b88208026d6b19fe86540328a518949a03c778b8b5520` |
| `code_0010f790` | 736/736 | 21/21 | `451c78bcbdd4d6db18dbfe91ce5e1c6da0bffc6946409f4e62f7657763ec3dca` | `e318ef56a0771d9c007ca8e236d03c945b93c4175e5c5c670dda2dc548e640a0` |
| `texture_page_texture_new` | 240/240 | 9/9 | `d75574be99cf9a26ae8fc060f5d7d51bdf5005cb6699660f647adc2db545e513` | `1d5abf71760148ec35dca4d32316dc7b661e09a915023f3d1d3e4fe590802e28` |
| `texture_page_textures_end` | 112/112 | 6/6 | `6c74519b0d25aba59784237633ae13cb02dccffd26fe8cebc8f018a87b0a5702` | `3c9dc6840e0ada4946f9f409b77e9b2a6c11341672cb72082844d5bfe94911e5` |

The production blocker classifier returns `UNKNOWN` for all four. It confirms
that `code_0010f790` and `texture_page_textures_end` already have exact
relocation addresses as well as identities. The other two have identical
relocation identities but shifted addresses, so their source topology must be
corrected before register/scheduler tuning.

## Recovered foundation

- `af7e2340` / `66bf7aa7`: exact lifecycle, accessors, and allocation
  foundation.
- `fab81db6` / `1ee7b15d` / `afffe8a7`: reconstructed packing algorithm and
  data structures.
- The committed implementation recovers the 512-entry channel stack, 32768
  texture-index stack, height comparator, power-of-two spacing, sorted packing,
  cancel/end transactions, and immediate-allocation rollback.

## Experiment log

Record every one-factor source experiment here before rebuilding. Each result
must include all four residual sizes, relocation counts/addresses, normalized
hashes or first divergence, and the nine exact sibling functions' regression
status.

### E01 — defer pixel-count multiplication until dimensions fit

Changed `texture_page_texture_new` so `width*height` is evaluated only after
the width and height bounds pass, matching January's instruction order.

- Result: **STRICT EXACT**
- Size: 240/240
- Relocations: 9/9, addresses and identities exact
- Normalized SHA-256:
  `d75574be99cf9a26ae8fc060f5d7d51bdf5005cb6699660f647adc2db545e513`
- Object state: 10/13 strict-exact
- Regression sweep: all nine previously exact siblings retained

### E02 — express comparator through two exact accessor calls

Replaced the manually expanded verify/get pairs with two
`texture_page_texture_get` calls.

- Result: rejected, still 96/96 and 6/6
- Improvement: reproduced January's three-callee-saved-register prologue and
  epilogue exactly
- Regression: interleaved each validation with its lookup, while January
  validates both pages before either lookup
- Siblings: unchanged; object remained 10/13
- Do not repeat this direct two-accessor form

### E03 — `register` second page in original comparator topology

Restored the original validate/validate/get/get order and marked the second page
pointer `register`.

- Result: no code-generation change from baseline
- The compiler ignored the hint under `/O2`
- Siblings: unchanged; object remained 10/13
- Do not repeat `register` hints for this helper

### E04 — function-scope `textures` alias in `texture_page_textures_end`

Cached `texture_page->textures` in an explicit `register` pointer while leaving
the loop variables at function scope.

- Result: no code-generation change from baseline
- The compiler continued to use EDX/EAX/ECX and emitted no inner-block EDI
  lifetime
- Siblings: unchanged; object remained 10/13
- Do not repeat `register` hints for this loop

### E05 — confine the end-loop locals to the successful-resort block

Moved `absolute_index`, `textures`, and `texture` into the
`if (code_0010f790(texture_page))` block. January first saves EDI only after
that call succeeds, so the shorter source lifetime is directly suggested by
the machine code.

- Result: no code-generation change from baseline
- Size/relocations remained 112/112 and 6/6 with exact relocation addresses
- Normalized SHA remained
  `3c9dc6840e0ada4946f9f409b77e9b2a6c11341672cb72082844d5bfe94911e5`
- All ten exact siblings retained
- Declaration scope alone does not control this register allocation

### E06 — split `masked_dimension` assignment from its condition

Separated each `masked_dimension &= spacing_mask` statement from the following
zero test. This tested whether the combined assignment-in-condition caused
VC7 to commute the `and` operands.

- Result: no code-generation change
- `code_0010f790` stayed 736/736, 21/21 with exact relocation addresses and
  candidate SHA
  `e318ef56a0771d9c007ca8e236d03c945b93c4175e5c5c670dda2dc548e640a0`
- All ten exact siblings retained
- Do not repeat simple expression splitting for these masks

### E07 — use the page global directly at all four helper sites

Removed the two page-pointer locals and referenced `bss_00456628` directly for
both validation calls and both datum lookups.

- Result: rejected
- Size stayed 96/96 but relocations regressed from 6 to 8 because the compiler
  reloaded the global for both datum lookups
- All ten exact siblings retained
- The target proves that two loaded page values are preserved; direct repeated
  global expressions are not the original topology

### E08 — statement-initialize both page-pointer locals

Kept the validate/validate/get/get order but changed the first page pointer from
a declaration initializer to a normal assignment, giving both page locals the
same source form.

- Result: no code-generation change
- `code_0010f730` remained 96/96 and 6/6 with candidate SHA
  `1549ee0ee677c135307b88208026d6b19fe86540328a518949a03c778b8b5520`
- All ten exact siblings retained

### E09 — loop-local register hints in `texture_page_textures_end`

Applied `register` to the loop counter and texture pointer.

- Result: no code-generation change
- The candidate retained EDX/EAX/ECX rather than January's EDI/ECX/EDX
- Do not repeat register hints in this loop

### E10 — no-op barrier positions in `texture_page_textures_end`

Tested `_ReadWriteBarrier()` after the successful packing call, after loading
the texture pointer, and at the loop back edge.

- After the call: no code-generation change
- At the back edge: no code-generation change
- After the pointer load: rejected; candidate SHA changed to
  `d620617f...` by adding an unwanted reload
- All ten exact siblings retained
- The barrier family does not reproduce the target register assignment

### E11 — success-block declaration initializer for the texture pointer

Declared and initialized the texture pointer inside the successful packing
block rather than assigning it after declaration.

- Result: no code-generation change
- Size/relocations stayed 112/112 and 6/6 with exact relocation addresses
- All ten exact siblings retained

### E12 — indexed texture access instead of pointer induction

Replaced the pointer-increment loop with
`textures->data + absolute_index`.

- Result: rejected structural regression
- Candidate grew from 112 to 128 bytes; relocations remained 6/6
- Candidate SHA became
  `e92a1c645b9e824dbb94342d209040295f5acbc05c1d31b1e6bfc290c0504fa8`
- All ten exact siblings retained
- January proves pointer induction; do not repeat indexed access

### E13 — declaration-initialize the data-array and texture pointers

Inside the successful packing block, declare
`textures = texture_page->textures` and `texture = textures->data`, while
retaining January's pointer-increment loop. This directly tests whether the
initializer topology assigns the data-array lifetime to EDI and the element
pointer to ECX as in the target.

- Result: no code-generation change
- Size/relocations remained 112/112 and 6/6
- Candidate SHA remained
  `3c9dc6840e0ada4946f9f409b77e9b2a6c11341672cb72082844d5bfe94911e5`

Result: no code-generation change. The named boolean was optimized away before
register allocation; the function remained at the baseline SHA.

### E15 — reverse the loop-local declaration order

Declare the texture pointer before the short absolute index while restoring the
direct branch on `code_0010f790`. This is the only untested declaration-order
permutation of the two real loop locals.

- Result: no code-generation change
- VC7 assigned the same stackless SSA values regardless of source declaration
  order

Result: no code-generation change. The compiler folded both aliases back into
the same EDX/EAX/ECX allocation, leaving 112/112, 6/6 and SHA
`3c9dc6840e0ada4946f9f409b77e9b2a6c11341672cb72082844d5bfe94911e5`.

### E14 — explicit packing-result local

Declare `boolean resort_succeeded`, assign the `code_0010f790` return to it,
and branch on the named value. The exact sibling
`texture_page_textures_cancel` uses this source topology, making it a
target-authored allocator lead rather than a generic perturbation.

- Result: no code-generation change
- The named boolean was optimized away before register allocation

### E16 — nested lifetime for the comparator's second page

Declare `texture_page_b` in a block beginning after the first validation and
ending after the first `datum_get`. This mirrors the measured target lifetime:
the first page survives both validation calls while the second page is loaded
only after the first call and dies after the first lookup.

- Result: no code-generation change
- The compiler coalesced the nested lifetime to the same SSA value

### E17 — make the packing mask temporary unsigned

Change only `masked_dimension` from signed `short` to unsigned `word`. The value
is used exclusively as a bitmask and zero predicate, so this is
behavior-preserving. It tests whether January's `mov width, EDX; and mask, EDX`
operand choice came from an unsigned mask temporary.

- Result: no code-generation change
- Signedness does not control the commutative `and` operand choice

### E18 — widen the mask temporary's source lifetime

Move `masked_dimension` from the sorted-texture loop block to function scope
while restoring its signed-short type. This tests whether January's schedule
came from the mask temporary being live in the function's main local set.

- Result: no code-generation change
- Source scope does not affect the optimized mask SSA value

### E19 — named true value for the end-loop store

Declare `boolean sorted = TRUE` and store that named value instead of the
literal. An exact January donor (`objects_information_get`) gains a late EDI
lifetime from an additional live byte value in an otherwise similar no-call
data-array loop, so this tests the corresponding topology without adding
observable operations.

- Result: no code-generation change
- Constant propagation removed the named boolean before it could influence the
  loop allocation

### E20 — initialize both loop variables in the `for` initializer

Move the texture-pointer initialization into the comma-separated `for`
initializer beside `absolute_index = 0`. This preserves January's preheader
instructions but changes the source/IR origin of both induction variables as a
single loop initialization expression.

- Result: no code-generation change
- VC7 canonicalized the comma initializer to the baseline preheader

### E21 — explicit precheck plus `do/while`

Express the already-measured CFG directly: initialize the pointer and counter,
precheck the count, then execute a bottom-tested `do/while`. The January
machine code has exactly this control-flow topology even though a `for` can
also lower to it.

- Result: no code-generation change
- VC7 reconstructed the same preheader and bottom-tested loop emitted by the
  concise `for` form

### E22 - alternate locally available optimizer backends

Compile the unchanged best source using the two additional backend DLLs found
in the workspace's prior compiler-variant research (`SP4` and `SP5`). This
tests the patch-level optimizer hypothesis before spending more source shapes.

- Result: rejected before code generation
- Both backends report C1007 for the VC7 front end's `-Ob2` intermediate flag
- These are incompatible older optimizer DLLs, not usable XDK 3911 QFEs
- The valid 13.00.9210 and 13.00.9254.1 toolchains remain the only compatible
  locally available variants; both emit the same three residuals

### E23-E24 - remaining declaration-order permutations in the packer

Tested `texture_width` before `texture_height`, then `masked_dimension` before
`channel_index`.

- Result: no code-generation change in either case
- VC7 assigned the same stack slots and SSA values

### E25-E26 - direct mask-value expressions

Tested a separate `dimension & mask` assignment followed by a zero test, then
removed `masked_dimension` and tested the expression directly.

- Separate assignment: no code-generation change
- Direct predicate: rejected; relocation addresses changed and the candidate
  SHA became `83d76628a4a3f6987ba11fa00c24eaf8d633ce2f1b3a8c3500d0d3494a84f3d6`

### E27 - comparator page declaration order

Declared the second page before the first while retaining the
validate/validate/get/get sequence.

- Result: no code-generation change

### E28 - inline verified-page identity helper

Wrapped validation in an inline helper returning the same page. With the first
page initialized before calling the helper, this restored January's
three-callee-saved-register body and made the function byte-identical from
offset `0x0e` onward.

- Result: improvement, but not exact
- Candidate SHA:
  `9600d22f437e4458aebcd785ea57fd1ced3fbb9b5c0b9d23242a03a20cf2c792`
- Sole remaining region was the first page's prologue materialization

### E29 - assignment inside each validation call

Assigned each global page pointer in the argument expression to
`code_0010f570`.

- Result: canonicalized back to the original two-register baseline

### E30 - direct global input to the inline verified-page helper

Leave both page locals uninitialized, then assign each from
`texture_page_verify_and_return(bss_00456628)`.

- Result: **STRICT EXACT**
- Size: 96/96
- Relocations: 6/6, addresses and identities exact
- Normalized SHA-256:
  `4ea8aca9a5fe88693bb7cb72005f6a504b410ad15fd073c0725b08e4ec189953`
- Object state: 11/13 strict-exact

### E31-E34 - inline provenance follow-ups

Applied the successful inline-provenance idea to the two residual functions:

- Cached data-array accessor in `textures_end`: rejected; it removed January's
  required per-iteration alias reload and produced SHA
  `4fff2764d551c362047ff666f28e68815b4cb82f60a3e18df0d8ba9e596a1725`
- Inline mask helper in the packer: no code-generation change
- Inline first-texture accessor in `textures_end`: no code-generation change
- Repeated uncached inline data-array accessor at both load sites: no
  code-generation change

### E35 - bounded packer-barrier supersearch

Compiled 127 combinations of zero-byte `_ReadWriteBarrier()` placements around
the remaining packing-mask scheduling window.

- Result: no exact candidate
- Best result still had 26 differing normalized bytes
- The barrier family can move the mask operation, but cannot reproduce all
  five January operand/order pairs together

### E36 - cross the barrier lead with the prior source-shape atlas

Crossed the best packer-barrier placement with 101 previously measured,
semantically valid source shapes.

- Result: no exact candidate
- The cross-product did not improve upon the 26-byte barrier residual
- This closes the barrier-plus-expression family rather than repeating it
  manually

### E37 - recover the packing-mask width

Changed `spacing_mask` from `long` to `word`. January consistently performs
low-word masks and zero-extends this value; `spacing` is also a `word`, so this
is type recovery rather than a code-generation hint.

- Result: **STRICT EXACT**
- Size: 736/736
- Relocations: 21/21, addresses and identities exact
- Normalized SHA-256:
  `451c78bcbdd4d6db18dbfe91ce5e1c6da0bffc6946409f4e62f7657763ec3dca`
- Object state: 12/13 strict-exact

### E38 - preserve the packing result through `textures_end`

Stored the packing result in a `boolean` and used `result ^ TRUE` for the final
page flag. This reproduced January's EDI/ECX/EDX loop allocation and reduced
the residual from 28 to 9 bytes.

- Result: rejected
- The expression is not equivalent for noncanonical nonzero boolean values
- This experiment proved that January keeps the packing result live across the
  loop, but it was not eligible production C

### E39 - bounded end-loop barrier supersearch

Crossed the nine-byte live-result candidate with 127 zero-byte barrier
placements around the call, loop, and final store.

- Result: no exact candidate
- Every best candidate retained the same nine-byte tail
- The barrier family cannot convert the dynamic result store into January's
  literal zero store

### E40 - translation-unit order and linkage audit

Tested January's function order, static/external helper linkage, and
`__forceinline` variants.

- January function order did not change `textures_end`
- Externalizing the private helpers regressed their private register
  conventions and multiple callers
- `__forceinline` retained the exact comparator body but still emitted the
  unreferenced 16-byte helper COMDAT

### E41 - recover the `textures_end` return type

January leaves the packing result in `AL` across the entire successful loop and
returns without replacing it. Changing the function from `void` to `boolean`,
returning the named packing result, explains that lifetime naturally.

- Result: **STRICT EXACT**
- Size: 112/112
- Relocations: 6/6, addresses and identities exact
- Normalized SHA-256:
  `6c74519b0d25aba59784237633ae13cb02dccffd26fe8cebc8f018a87b0a5702`
- Object state: 13/13 strict-exact

### E42 - inline-helper ownership audit

The exact comparator uses a small inline identity helper to retain January's
private-ABI provenance. The compiler emits an unreferenced 16-byte COMDAT copy
in addition to inlining both calls.

- Six macro/explicit-parameter replacements removed the extra COMDAT but
  regressed the exact comparator register allocation
- The emitted helper is COMDAT `Selection: Any`, has no incoming relocation,
  and is discarded by the normal reference-eliminating link
- It does not change any target-owned code or data section; ordinary progress
  reports 100% code, 100% data, and all 13 functions exact

## Final strict result

- Target functions: 13/13 strict-exact
- Target code: 1922/1922 meaningful bytes
- Target data: 460/460 bytes
- Hardened relocation identities: exact for every function
- Ordinary object report: 100% code, 100% data, 100% functions
- Full project rebuild: successful
- House-style audit: passed; the recovered boolean function has one explicit
  return, and all parameters remain one-per-line
