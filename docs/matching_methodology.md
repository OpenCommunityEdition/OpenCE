# Exact-matching methodology

This document records recurring January 2002/XDK 3911 matching failures and
the source-level changes that have actually moved them. It is an operational
checklist, not permission to accept approximate code. Exact status still
requires padded bytes, normalized bytes, relocation count/type/address/target/
addend, ABI, build, and regression evidence.

## Triage decision tree

1. Establish the untouched baseline: padded size, meaningful size, normalized
   hash, relocation table, first differing instruction, and sibling status.
2. If relocation targets or addresses differ, recover source structure before
   tuning registers: statement order, branch fall-through, linkage, field
   ownership, and eager versus lazy evaluation.
3. If the target uses 16-bit arithmetic, map every promotion and truncation
   boundary. Treat this separately from register allocation.
4. If instruction structure is identical modulo registers, locate a
   hardware-forced anchor before changing declarations:
   - `rep movs`/`rep stos` force ESI, EDI, and ECX;
   - variable shifts force CL/ECX;
   - multiply and divide force EAX/EDX;
   - ABI/API boundaries may force ECX/EDX or preserve callee-saved registers.
5. For private same-translation-unit helpers, make callers exact first. Start
   from the hardware-forced caller and propagate the convention toward the
   helper; do not tune the helper in isolation.
6. Stop after five evidence-based source shapes if only an interchangeable
   register or dependency-free instruction ordering remains. Record and park
   the case instead of perturbing unrelated exact code.
7. Run the strict semantic ledger before assigning work. A function already
   listed in `build/semantic_report.json` must not be reimplemented merely
   because objdiff displays a fuzzy percentage.

## Blocker classes

### A. Recoverable source-shape mismatch

The emitted differences indicate that the reconstructed C is not yet shaped
like the original source. Productive levers include statement order, branch
polarity, local lifetime, explicit versus inlined expressions, linkage, and
structure ownership.

### B. Integer promotion/truncation mismatch

The target contains operand-size prefixes, partial-register operations, or
`movsx`/`movzx` boundaries that ordinary C promotions erase. Recover the width
of the accumulator and place signed/unsigned casts only at disassembly-proven
boundaries. Do not file this as a register mirror.

### C. Private-ABI/register fixed point

The instruction graph, sizes, and relocations agree, but a translation-unit
cluster assigns interchangeable registers differently. Find a forced-register
caller and solve outward. If the complete cluster remains a pure permutation
after the checklist, park it as a compiler allocation tie.

### D. Scheduler/lifetime tie

Two dependency-free instructions are transposed, or local lifetimes alter
spills without changing semantics. Try only source changes justified by the
surrounding target ordering. Do not add undefined behavior, inline assembly,
compiler pragmas, or flag changes to force the result.

### E. Comparator/reporting mismatch

The machine code is exact but COFF producers encode internal labels or
relocation placeholders differently. Credit only through the hardened
semantic comparator and the reviewed `config/semantic_matches.json` allowlist;
each entry is reverified against current objects during `ninja progress`.

## Case catalog

| Case | Class | Evidence and source lever | Outcome / next action |
|---|---|---|---|
| `lra_new` (`f0e5a48`) | A | Reordering field initialization to the target's emitted store order aligned the failure block and lifecycle setup. | Exact; make store-order recovery a standard first pass. |
| `object_new` (`claude/object-new-16`) | A then D | Writing the cleanup as failure-first eliminated 10 relocation differences and two divergence regions. Only `fmuls` versus deferred `add esp, 0x18` ordering remained. | Preserve failure-first shape; park the dependency-free scheduler tie. |
| `hashtable_remove` (`f2d30b9`) | A | Source-oriented comparison order, reload points, and the shared deletion path aligned 496 padded bytes and 20 relocations. | Exact; use comparison polarity and reload timing before register tuning. |
| `csstrcasecmp` (`e5852f8`, `claude/csstrcasecmp-15`) | A then C | `if (c1 != c2)` reproduces the target operand order; `if (c2 != c1)` does not. The remainder is an ESI/EDI parameter/local permutation. | Land only evidence-backed logic changes; handle the remaining mirror through a forced-register caller or park. |
| terminal tab stops (`a917110`) | A | Identical six-byte data still failed because `static` produced storage class 3 while the target symbol was external, class 2. | Exact after external linkage/name recovery; audit symbol class, not just bytes. |
| `tif_packbits.obj` (`7c9c092`) | A | Target linkage, RCSID ownership, `_csmemcpy`, and disassembly-proven TIFF field offsets were required. Repository header layout differed from the target object. | Five functions and data exact; raw offsets are acceptable only with explicit target-layout evidence. |
| random-math globals (`aef5a7c`) | A | Replacing an anonymous address placeholder with the real externally linked 20-byte globals structure fixed BSS ownership and relocation addends. | Exact functions retained; model globals explicitly before function tuning. |
| `default_hash_function` (`f2d30b9`) | B | Target uses 16-bit product/partial-register code while natural C promotes the polynomial and key byte to 32 bits (80 target versus 64 candidate bytes). | Reopen only with a promotion/truncation lattice derived instruction by instruction. |
| LRA verify/free/lock cluster (`f0e5a48`) | C | Seven functions share cache-in-EBX versus cache-in-EDI and private helper conventions. Isolated helper shaping did not move the joint allocation. | Anchor on callers with forced registers, then propagate; otherwise park the whole permutation together. |
| hashtable probe helper `code_0010b270` (`073e794`) | C | Size, relocations, control flow, and operations agree; two return blocks mirror ECX and EDX. | Try return-materialization/single-exit shape once under the checklist, then park if unchanged. |
| `reference_list_copy` lane | C | Target and candidate reduce to the same operations with a callee-saved register mirror after source-shape attempts. | Revisit only from its caller cluster and a hardware-forced anchor. |
| `object_get_marker_by_name` lane | C | Repeated source shapes left an interchangeable register allocation difference while size/relocation structure stayed aligned. | Park until the object marker caller cluster supplies a fixed point. |
| `object_compute_node_matrices` (`claude/object-compute-node-matrices`) | D | 6,560 padded bytes, 318 relocations, a 24-byte frame delta, and 142 lifetime/scheduling regions. The first 47 relocations and call order already agree. | Complexity-gated; do not queue until the methodology succeeds on smaller cases. |
| `data_decode_array` (`d0a735d`) | E | Padded bytes and 31 semantic relocations are exact; objdiff reports 85.19% because one target symbol plus addends corresponds to several rebuilt local labels. | Credited through the dynamic semantic ledger; no source work required. |
| 37-function switch/local-label set (`24c998a`) | E | Full dual sweep found 37 real functions and 21,051 meaningful bytes exact under conservative COFF normalization but fuzzy under objdiff. | The allowlist is reverified on every progress run; uncertain or changed entries fail closed. |

## Complexity gate

For large aggregate objects such as `objects.obj`, queue a remaining function
only when all of the following hold:

- padded size is below approximately `0x60`;
- local state is small and unambiguous;
- relocation count is low;
- no large inlined helper or private-ABI cluster dominates the function.

Larger functions require a specific leverage reason (new structure proof,
forced-register anchor, or reusable subsystem insight) before consuming an
implementation lane.

## External type-information gate

HCEX, Halo PC, October 2001, and community sources are hypothesis generators.
Every imported field or enum value must carry a January-Xbox disassembly proof.
Unproven spans remain named `reserved`/`unknown` and documented as such. No CEA
offset enters a trusted header merely because it compiles or looks plausible.

## Original-bug policy

Exact builds preserve confirmed original behavior. Mark only defects supported
by binary and layout evidence:

```c
/* BUG (preserved for exact matching): <target behavior and evidence>.
 * A corrected build should <safe replacement>.
 */
```

Keep the safe fix disabled in the exact build. Do not label suspicious code as
a bug without target-machine and structure-layout proof.
