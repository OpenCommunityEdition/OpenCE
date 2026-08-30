# `stack_memory_pool.obj` Stian guard one-shot (2026-08-30)

## Outcome

**Rejected and restored exactly.**  A newly authenticated Stian PC lift
supplied the missing source-level identity predicted by January's redundant
nonnull test in `_code_0010e490`.  The single evidence-backed candidate
compiled successfully but VC7 optimized that identity away and remained
non-exact.  No alternate spelling or tuning pass was attempted.

The production source is restored to Git blob
`966303bba87c7fe3b27984fed69076a2043a43fc`.  The focused whole-unit gate is
again 30 exact / 3 residual / 0 unwritten, with all target-owned RDATA and
ordered relocation evidence unchanged.  This lane retains no source change.

## Frozen baseline

The lane starts from canonical commit
`251e9f780f610ee2c1962d93fc104c1154d28996`.  The January object raw SHA-256
is `95d5b1f59c90b3a0ffa0e6bfa304a1a3b0c26876aef08d0eb6968a61c77a2833`.
Its three residual owners are:

| owner | January bytes / relocs / normalized SHA-256 | retained bytes / relocs / normalized SHA-256 |
| --- | --- | --- |
| `_code_0010e490` | 128 / 4 / `9ced5444b8465ec80ae62ee31c80caf91a5217ee1f7fc3ee227ef7427a9d1b73` | 128 / 4 / `411fc6dac75b2ef8b36dc6e22e071af7f4bdfcb9d9fd26d78ca4c6382e26f484` |
| `_code_0010ea00` | 640 / 36 / `00ddbe408b35e1532a799be0de34729b5150eb783b73dbdeba5354b9850c7daa` | 640 / 36 / `c49ac9850cc6a1eba2515abca69cc4c31d45ada6db4a788447f22b326babe12d` |
| `_code_0010ef70` | 192 / 13 / `cc68570f2905b4550266130b6f4d8ba0382c136469d97f3d91a9013472dd3de3` | 192 / 13 / `4d1bc2fd72947f08ea536281839e1b7b54c02f33d5d383f679545301877f272b` |

## Prior-work and artifact census

Every canonical Stack Memory Pool ledger was read.  The older isolated
full-admission ledger at
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\stack-memory-pool-full-admission\docs\object_matching_logs\stack_memory_pool_obj.md`
was also recovered and read in full.  It records 82 measured experiments,
including the shared-result, repeated-guard, comparison-orientation,
return-carrier, allocator-CFG, and resize-lifetime families.  Those families
were not repeated.

The all-ref sweep compiled all ten distinct committed source blobs.  None
improved the inherited 30/33 frontier.  A broader read-only census covered
`C:\halo-worktrees`, `C:\tmp`, and the complete campaign workspace.  It found
1,709 source copies with 19 distinct hashes and parsed 9,100 relevant object
artifacts without failure.  It found:

- nine `_code_0010e490` fingerprints;
- fourteen `_code_0010ea00` fingerprints;
- ten `_code_0010ef70` fingerprints;
- zero rebuilt exact instance of any residual owner; and
- zero non-target artifact containing a complete exact object.

The census includes all preserved `exp001.obj` through `exp082.obj` artifacts
from the full-admission lane, the later Pastudan-donor lane, and current
Claude/Fable worktrees.  Literal January split copies were not mistaken for
rebuilt evidence.

## Fresh evidence and candidate

The Pastudan body previously used by the 2026-08-26 recovery checks null
before the first-gap predicate.  Stian commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`, source-file SHA-256
`1adcf3e58f53fd5a54f76d9f7e69e3ef39df952ffa31bb015b8e215ed6ce1173`,
instead reconstructs this materially different topology:

1. initialize one null result;
2. test `block && first_gap >= request` and return the pool base on success;
3. independently test `!block` and return the null result;
4. traverse successors; and
5. compute the successful gap address before storing the predecessor.

That independent second source test directly predicts January's otherwise
unexplained `test esi; je` at offset `+0x19`.  It is distinct from the prior
nested/sibling dominated checks and result-guard experiments.  The donor was
adapted once to the existing typed pool/block fields, unsigned gap comparison,
ordinary C89 declarations, and explicit returns.  It introduced no assembly,
`volatile`, `register`, pragma, intrinsic, barrier, attribute, raw numeric
address/offset, punning, undefined behavior, synthetic retention, compiler
change, or object-byte forcing.

VC7 `/Zs` passed.  The sole code-producing candidate compile preserved all 30
exact siblings, both other residual fingerprints, target-owned data, and
ordered relocations, but `_code_0010e490` remained non-exact:

| artifact | bytes | relocs | normalized SHA-256 | relocation offsets |
| --- | ---: | ---: | --- | --- |
| January | 128 | 4 | `9ced5444b8465ec80ae62ee31c80caf91a5217ee1f7fc3ee227ef7427a9d1b73` | 48, 53, 58, 65 |
| Stian candidate | 128 | 4 | `c6c40e43fb63315f8e138e8b261b12c0ba3512625a3ad3cef2652d9fafdc1581` | 44, 49, 54, 61 |

The compiler removed both the distinct null-result lifetime and the repeated
guard.  The assertion cluster moved four bytes early rather than reaching the
target.  The candidate object has raw SHA-256
`3797b2b7846973c2c8ce976f4b7a496d1137c8451e72d34912856a92363a4b93`
and is retained only as ignored audit evidence under
`build/audit/stack-memory-pool-stian-guard/`.

## Disposition

The complete candidate was inverse-restored immediately.  A post-restore
compile reproduces the inherited 30 exact / 3 residual frontier and the source
content diff is empty.  Reopen only for original January source/local-variable
provenance or a rebuilt strict-exact COFF owner.  The Stian/Pastudan/HaloCEA
donor families, the 82-experiment full-admission matrix, and equivalent
redundant-guard respellings are now closed.
