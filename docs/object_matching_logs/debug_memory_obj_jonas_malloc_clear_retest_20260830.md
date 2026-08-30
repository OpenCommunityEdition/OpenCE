# `debug_memory.obj` malloc-clear retest admission (2026-08-30)

## Result

`_debug_malloc` is newly strict exact. The unit advances from 15/18 to 16/18
exact functions while retaining all 1,120 padded runtime-data bytes. It remains
honestly `NonMatching`; `_code_0007cf50` and `_debug_realloc` are the two
residuals.

| Measurement | January | Retained candidate |
| --- | ---: | ---: |
| Meaningful bytes | 259 | 259 |
| Padded bytes | 272 | 272 |
| Relocations | 20 | 20 |
| Normalized SHA-256 | `6b3e89dbbabe53f5da1ca66d8f5d43406314b3a2008581326aac5fa7c4273782` | `6b3e89dbbabe53f5da1ca66d8f5d43406314b3a2008581326aac5fa7c4273782` |

Every relocation address, COFF type, symbolic destination, addend, and owner
is pairwise exact. The retained source SHA-256 is
`2488e691c170d8156a5e2afa6fc594db9a6dbe5fb4949b1df2f01c89380b0c0a`,
and the ordinary rebuilt object SHA-256 is
`801fb4f7717d0f4fd2da9f864f85c9eb115c3bd4e56ef1e832a8e188199f9ba8`.

## Why this was a fresh evidence wave

The 2026-08-25 clear-branch experiment predated the strict admission of the
private list-insertion helper `_code_0007cfc0`. That older helper kept the
header in `EDI`, so its caller also compiled with the wrong `EDI`/`ESI`
allocation and the explicit branch could not reach January.

The later head-reload admission made `_code_0007cfc0` strict exact and changed
its private ABI to January's topology: `debug_malloc` now enters the helper
with the header in `ESI` and retains the allocation size in `EDI`. Against that
new caller/helper cluster, the committed ternary expression was only eleven
bijection events from January, with its first divergence at the clear-value
selection.

The one natural C89 candidate replaced the ternary with the source-level
`if (clear)` / `else` branch. It reproduced January's entire clear path and
left only the equivalent maximum-size comparison operand order. Spelling the
same predicate as `maximum_pointer_size < total_pointer_size` aligned that
last direct source-order boundary. The resulting function is byte and
relocation exact without assembly, qualifiers, barriers, pragmas, intrinsics,
raw-address access, pointer reconstruction, or synthetic scheduling devices.

## Current unit status

- 16/18 functions exact;
- 2,344/2,856 meaningful code bytes exact;
- 2,448/2,976 padded code bytes exact;
- 1,120/1,120 padded data bytes exact.

The remaining residuals retain their frozen diagnoses:

- `_code_0007cf50`: 112/96 padded bytes and 8/8 relocations; January's private
  convention passes the pointer in `EBX` and the size on the stack, while VC7
  keeps the reconstructed helper's pointer in `EAX` and size in `EBX`.
  Formal-order, equivalent final-address, and lexical-scope controls are
  closed.
- `_debug_realloc`: 416/400 padded bytes and 25/25 relocations; the correct
  result-or-zero predicate and the public separate-header/reused-parameter
  topology have already been measured and rejected. The residual is the
  broader pre/post-call lifetime topology tied to `_code_0007cf50`'s private
  ABI.

## Validation

- Strict whole-unit gate: 16 exact, 2 residual, 0 unwritten.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,957 functions evaluated, 4,841 semantic exact,
  4,851 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, or revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Tool suite: 212/212 tests pass.
- `git diff --check` and the unchanged `source/units/units.c` sentinel pass.

A clean post-commit regression snapshot/rebuild/check passed before handoff;
all sixteen accepted owners remained exact with no failures or warnings.
