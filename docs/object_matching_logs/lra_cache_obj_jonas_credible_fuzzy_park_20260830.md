# `lra_cache.obj` credible fuzzy park and private-name closeout (2026-08-30)

## Scope and provenance

This lane started from canonical commit `c4fe084a9` in the isolated worktree
`work/lra-cache-closeout-audit-20260830`.  It covers
`source/memory/lra_cache.c` and the January target
`source/memory/lra_cache.obj`, built with XDK 3911 VC7 13.00.9254.1 and the
repository flags `/O2 /Oy- /DDEBUG /Dxbox`.

The complete prior corpus was read before editing: the current policy and
global-census ledgers; the 202-line E1-E50 historical ledger at
`b298f939:docs/object_matching_logs/lra_cache_obj.md`; Claude's later naming
commit `cfc4e895a`; the matching methodology and exact-match playbook; the COFF,
campaign, audit, semantic, parking, and regression-gate documentation; the C2
backend/IR records; and both spill/residual research notes.  January PDB/map
evidence still supplies compiler and object records but no locals, scopes, or
line table.  The newly open Marathon 2/Infinity source tree at commit
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c` was searched in full; it contains
no LRA implementation or analogous movable-block cache allocator.

## Validated final state

- Strict code: `13/14` functions exact.
- Accepted code: `1,004/1,392` meaningful target bytes.
- Target-owned non-code: `5/5` sections, `160/160` padded bytes exact.
- Sole residual and new active fuzzy park: `_lra_allocate`.
- Retained source is coherent ordinary C.  It contains no inline assembly,
  volatility, barriers, pragmas, raw-byte emission, undefined aliasing, inert
  branches, or contradictory logic.  `tools/fake_match_scan.py` reports no
  findings, followed by a manual all-input semantic review.

| `_lra_allocate` measurement | January | retained rebuild |
| --- | ---: | ---: |
| meaningful bytes | `388` | `365` |
| padded bytes | `400` | `368` |
| relocations | `4` | `4` |
| relocation addresses | `13, 6B, 89, 8E` | `13, 5F, 83, 88` |
| normalized differences, including length | - | `320` |
| normalized SHA-256 | `93beee39f5b398662960640adbdef20b5569e5407bff1526a6021970f3001e2f` | `bd671ca359d22c92b9d24e5428f9c62f6fc4397f1643ed41d5a7df44ccdaa9ef` |
| ordinary objdiff | - | `65.6755%` |

The relocation types, addends, and destination sequence are exact.  The first
controlling divergence remains January's loop-carried zero: January keeps the
zero live while splitting the cache-empty initialization arms and reuses it for
`write_offset = 0`; VC7 merges/rematerializes those stores in the retained
build.  This is a spill/allocation and tail-duplication fixed point, not missing
allocator semantics.

Final reproducibility hashes:

- source: `ba173f689cb61cc2dc9488af0fa2209fb80f04583101d93bac59d1be48681be7`;
- rebuilt object: `45cd6d128e75f9ec5ebd5bf0c811a92041ab6344e543de231f73fec062dc6f3c`;
- regenerated named split target:
  `4f75e3d0c8140ae7c23788e52dbafcd1f62119430080909b0a224e46ac34f354`.

## Private names and source organization retained

Claude's six descriptive names were independently checked from bodies,
callback types, and call sites, then applied to both source and the csplit
source of truth.  Every target entry is marked `"static": true`; no
`code_<address>` placeholder remains in this translation unit.

| old split label | retained source name | padded bytes / relocs / normalized SHA-256 |
| --- | --- | --- |
| `code_0010b9f0` | `lra_default_update_proc` | `16 / 0 / 21c4d57ab9cba336579640a56fa4aeaf0e225122d873fb5cea4f59d9982a2b13` |
| `code_0010ba00` | `lra_default_delete_proc` | `16 / 0 / 8c6f9b23a045e09d762bfe2dd6abd3487d313d1e3bc01e643fde3f3cc0c39a72` |
| `code_0010ba10` | `lra_block_delete` | `32 / 0 / 70e14824c183c54f34216b7d7a95c2b9d9882ff1e75b9c6d338c8cac0a9ec2c0` |
| `code_0010ba30` | `verify_lra_cache_block` | `128 / 6 / 6aab6630fbe789fb85fbbd0ee4c035918e521ebe2d1f713156b27cc2a6240aa6` |
| `code_0010bab0` | `verify_lra_cache` | `128 / 11 / 20efb86936868f30cf8ccc34d63d1a924c9f0c5b761276072325b430ad267562` |
| `code_0010bdf0` | `lra_block_offset` | `32 / 1 / b3e71fb5aa448be835c2a6905d8bc0bca66881aa5338cb6c33d7db2bb90329ce` |

Their prototypes remain in the translation unit's private-prototype section.
Their definitions now live under the existing `private code` section at the
end of the file, consistent with the surviving line anchors and Bungie source
organization.  That move is byte-neutral: the post-naming regression manifest
still reports all thirteen functions exact with no ownership or data change.

## Bounded experiment and rejection record

| ID | Evidence-backed source shape | Result | Decision |
| --- | --- | --- | --- |
| N1 | Rename the six identifiable private helpers in source and `symbols.json`; mark each split symbol static. | `13/14` exact before and after; no byte/data regression. | retained |
| N2 | Move the six private definitions beneath the established `private code` marker while retaining their correct prototypes above all callers. | Regression gate clean; `_lra_allocate` unchanged at `368/4/bd671c...`. | retained |
| N3 | Also move `lra_allocate` between `lra_flush` and `lra_free`, as the assert-line anchors superficially suggest. | No allocator gain; regressed accepted evidence for `_lra_free`, `_lra_lock`, `_lra_unlock`, the `"pointer"` COMDAT, and symbol ownership. | rejected and reverted |
| A1 | Re-measure the prior 400-byte source after removing its sole `_ReadWriteBarrier()` anchor: single valid-size wrapper, explicit empty-cache arms, failure-first fit test, ordinary `while (!result)`. | Coherent but still `368` bytes and four relocations; normalized SHA `f719c088b68258c0df6975d46964b6982228cec681f90814404bccccbd9b37ef`, `331` differences versus the retained form's `320`; siblings stayed exact. | rejected and reverted; not the best honest fuzzy form |

The historical 400-byte candidate is explicitly inadmissible.  It used
`while ((_ReadWriteBarrier(), !result))` solely to preserve the compiler's
shared bottom test.  Its size was `400`, candidate relocation addresses were
`13, 6B, 90, 95`, normalized SHA was
`9ea9b8efcaea76ab4d35f520558c7e2315033e96fa194550de868b95b7f40e0c`,
and it still differed at 258 normalized positions.  The barrier is fake
compiler steering with no program meaning; exact padded size cannot rescue it.
No volatile, asm, fixed condition, empty branch, synthetic alias, or undefined
read was tried or retained.

## Regression and build evidence

- Clean XDK object rebuild and strict function gate: `13/14`, sole residual
  `_lra_allocate`.
- Fail-closed manifest captured at commit `ed0bd9d56` after naming, then checked
  after private-definition relocation: thirteen `still_exact`, no failures,
  warnings, changed nonexact siblings, or newly exact claims.
- Full `halobetacache_build`: pass (`466` translation-unit compiles in the
  clean worktree; emitted warnings were pre-existing and unrelated).
- Full report generation: pass; board `277/619` strict-exact objects and
  `4,860/8,246` strict-exact functions before parked credit.
- Parked manifest validation after this entry: `active=14`, `stale=0`,
  `invalid=0`.
- Fake-match scan of `source/memory/lra_cache.c`: zero findings.
- JSON validation and `git diff --check`: pass.

## Disposition and reopen rule

`lra_cache.obj` remains correctly `NonMatching`; no exact credit is claimed.
Under the current house rule, `_lra_allocate` is now an active fuzzy park so
the campaign can move on without accepting fake source.  Reopen only with one
of the following genuinely new facts:

1. authoritative January source, locals, scopes, or line topology;
2. a real contemporary donor carrying this exact loop-carried-zero dependency;
3. a campaign-compatible original C2/QFE compiler;
4. a meaningful, defined-C topology outside E1-E50 that directly explains the
   zero lifetime without regressing any of the thirteen exact siblings or
   owned data.

Do not repeat declaration permutations, predicate spellings, loop/switch/goto
boundaries, zero aliases/defaults/resets, `register`, aggregate versus separate
locals, barrier placements, compiler-flag searches, or the old private-helper
ABI.  Do not restore `_ReadWriteBarrier()` or any other compiler-steering
construct.
