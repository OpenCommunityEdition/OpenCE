# `stack_memory_pool.obj` exact-boundary closeout (2026-09-13)

## Outcome

The production source remains at the strongest house-rule-compliant frontier:
31 of 33 functions are strict exact, two complete functions are credible fuzzy,
and no function is unwritten. The strict gate accounts for 4,112 of 4,944
padded code bytes. The two residuals account for 832 padded bytes and 821
meaningful target bytes.

This closeout gains zero strict functions and zero strict bytes. It deliberately
retains the natural typed implementations rather than perturbing them for an
unproved compiler-allocation outcome. Both residuals receive zero exact credit.
The lane forbids configuration edits, so their fuzzy disposition is recorded in
this ledger rather than added to `config/parked.json`.

| Function | Target / candidate padded | Target meaningful | Relocations | Normalized target SHA-256 | Normalized candidate SHA-256 | Disposition |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `_stack_memory_pool_allocate` | 640 / 640 | 637 | 36 / 36 | `00ddbe408b35e1532a799be0de34729b5150eb783b73dbdeba5354b9850c7daa` | `c49ac9850cc6a1eba2515abca69cc4c31d45ada6db4a788447f22b326babe12d` | Credible complete fuzzy; zero credit |
| `_stack_memory_pool_resize_block` | 192 / 192 | 184 | 13 / 13 | `cc68570f2905b4550266130b6f4d8ba0382c136469d97f3d91a9013472dd3de3` | `4d1bc2fd72947f08ea536281839e1b7b54c02f33d5d383f679545301877f272b` | Credible complete fuzzy; zero credit |

## Residual evidence

`_stack_memory_pool_allocate` has the exact padded envelope and all 36 ordered
relocation identities. Its allocation, list insertion, diagnostics, block
initialization, and successful return logic agree. The remaining aligned gap is
the early pool/base predicate and zero-value register graph, plus equivalent
NULL-return epilogue routing. The ordinary objdiff view reports 97.37 percent.

`_stack_memory_pool_resize_block` has the exact padded envelope and all 13
ordered relocation identities. Its assertions, reuse/grow choice, allocation,
copy, disposal, and returns agree. January reloads the `reference` parameter
from its stack home in the reuse arm and after allocation in the grow arm;
ordinary VC7 retains the equivalent value in ESI. No semantic or ABI difference
is evidenced by those reload choices.

The preserved full-admission ledger covers 82 bounded natural-C experiments
over allocator result/zero locals, predicate form, scope, branch topology and
epilogue routing, and over resize aliases, lifetimes, types and return forms.
`tools/campaign/branch_sweep.py` found the current 31-exact/two-residual frontier
tied for best among 24 unique historical source blobs. One additional isolated
probe initialized the allocator result to NULL and routed its failure returns
through that result; it was code-neutral and was discarded. No production
source change resulted.

Later HCEA PDB records independently corroborate the private helper identities,
semantic parameter names and pointer/unsigned-long types. They do not expose
locals, and their cross-build signatures omit January's caller-proven file/line
allocation metadata, so they are supporting semantic evidence rather than an
interchangeable ABI recipe. The January PDB exposes public symbols but no useful
private/local records for the remaining allocation choices.

Further declaration, alias, return or branch spelling search would repeat the
exhausted experiment families. Manufacturing a register lifetime, redundant
dependency or inert branch would violate the source-quality rules and would not
establish the original C. Reopen either function only for authentic January
source/local records or a natural compatible-compiler donor that explains the
specific residual.

## Object and owner boundary

All 24 target-owned read-only-data COMDATs remain exact, totaling 966 bytes.
The target has no owned DATA or BSS contribution. The candidate split object
also contains a two-byte `"p"` literal selected by another owner; it receives no
target-owned data credit and is not a reason to alter this translation unit.

`source/memory/stack_memory_pool.h` genuinely owns the opaque public pool type
and the public pointer API. The two residual helpers and both concrete block/pool
layouts are translation-unit-private. Exposing their prototypes or layouts in
the header would be false ownership, so the header is unchanged. HCEA's private
`struct memory_block` spelling is useful naming evidence but does not justify a
code-neutral project-wide type rename.

## Frozen verification

- `python tools/campaign/gate.py source/memory/stack_memory_pool --all`:
  31 exact, two residual, zero unwritten.
- `python tools/campaign/branch_sweep.py source/memory/stack_memory_pool`:
  current frontier tied best across 24 unique historical blobs.
- `python tools/fake_match_scan.py source/memory/stack_memory_pool.c --fail-on-findings`:
  zero findings.
- `git diff --check -- source/memory/stack_memory_pool.c source/memory/stack_memory_pool.h`:
  clean.

No global configure, Ninja build, configuration edit, commit, push, bespoke
runtime harness, or out-of-scope source/header change was performed.
