# `stack_memory_pool.obj` honest retention reconciliation (2026-09-02)

## Scope

This audit was performed in an isolated worktree at canonical base
`78c08edfcaf574be9165288ac9a17b36bdce2ff8`.  Its purpose was to remove the
fixed-false private-helper retention scaffold from
`source/memory/stack_memory_pool.c` without exchanging it for another source
shaper.  False linkage, anchors, forced inline/noinline attributes, pragmas,
volatile steering, fake calls, inline assembly, and object-byte patching were
out of scope.

The inherited and counterfactual baselines were reproduced before source
work:

| Candidate | Exact | Residual | Unwritten |
|---|---:|---:|---:|
| inherited source with `if (FALSE)` scaffold | 31 | 2 | 0 |
| scaffold removed, no other changes | 16 | 14 | 3 |

The three omitted sections in the stripped baseline were
`_memory_block_unlock`, `_stack_memory_pool_new_block`, and
`_stack_memory_pool_new_block_clear`.

## Evidence

All existing stack-pool ledgers and the campaign, admission, C2-debugging,
matching-methodology, and fake-match documentation were read before changing
the source.  The June 24, 2011 HCEA PDB was then queried directly with the
local `DIA2Dump.exe`.

The PDB records all of `memory_block_valid`, `memory_block_lock`,
`memory_block_unlock`, `memory_block_is_locked`,
`memory_block_set_size_and_index`, `memory_block_get_real_size`,
`memory_block_get_user_size`, `memory_block_get_user_address`, and
`memory_block_get_pool_index` as static functions that were specified inline
and actually inlined.  It also records `stack_memory_pool_new_block`,
`stack_memory_pool_new_block_clear`, and `stack_memory_pool_unlock_block` as
static functions that were inlined.  This is cross-build provenance for the
helper architecture, not a January byte oracle.

The PDB does not expose usable `S_INLINESITE` records, so it cannot directly
enumerate every edge.  Line and image evidence narrows the important ones:

- HCEA `stack_memory_pool_unlock_block` is instruction-identical to the
  standalone `memory_block_unlock` body, with its pool parameter optimized
  away and its block parameter promoted to the helper's input register;
- one source line in each HCEA clear wrapper covers the complete
  allocate/null-check/payload-clear sequence, exactly the standalone
  `stack_memory_pool_new_block_clear` body that DIA says was inlined;
- HCEA `stack_memory_pool_new_block` is a one-instruction tail branch to the
  allocator, and the non-clear public wrappers' call-site lines cover direct
  allocator calls, which is compatible with the recorded inlining;
- HCEA dispose-block reads the block index in the shape of the explicitly
  inline `memory_block_get_pool_index`; this edge is likely but not uniquely
  distinguishable from a direct field access in release code.

January relocations independently constrain the result: the public new-block
wrappers ultimately relocate to `_stack_memory_pool_allocate`, valid-block
relocates to `_memory_block_get_pool_index`, and unlock-block contains the two
inlined `memory_block_valid` assertion paths.  Those constraints are exactly
what the recovered helper composition emits under stock XDK 3911 C2.

Marathon has no analogous stack allocator.  Its absence was already recorded
in the signature audit and supplies no competing caller topology.

## Recovered source topology

The scaffold was deleted and duplicated implementations were replaced with
ordinary semantic helper calls:

- `pool_new_handle` calls `stack_memory_pool_new_block`;
- `pool_new_handle_clear` calls `stack_memory_pool_new_block_clear`;
- `pool_new_pointer` calls `stack_memory_pool_new_block`;
- `pool_new_pointer_clear` calls `stack_memory_pool_new_block_clear`, then
  obtains its result through `memory_block_get_user_address`;
- `stack_memory_pool_dispose_block` obtains the slot through
  `memory_block_get_pool_index`;
- `stack_memory_pool_unlock_block` composes
  `memory_block_is_locked` and `memory_block_unlock` instead of duplicating
  their bodies.

These calls are behavior-preserving abstractions with real runtime callers.
They explain why the three private sections exist and reproduce C2's January
inline/call-count schedule naturally.  No dead or redundant source remains.

An evidence-led experiment added ordinary `__inline` to the nine helpers for
which the HCEA PDB reports `WasSpecifiedAsInline`.  It did not change the
stripped baseline at all.  The annotations were removed: the authentic caller
topology alone is sufficient, avoids unnecessary cross-build assumptions, and
respects the project's preference against explicit inlining.

## Result

The final source reaches the same strict exact set as the inherited scaffold:

- 31/33 strict-exact functions;
- 4,112/4,944 exact padded code bytes;
- 1,000/1,000 report data bytes matched;
- zero unwritten target functions;
- no exact function lost relative to the inherited scaffold candidate.

The only residuals remain the previously audited, genuine source-level fixed
points:

| Function | Target/candidate padded bytes | Disposition |
|---|---:|---|
| `_stack_memory_pool_allocate` | 640/640 | residual; no parked credit |
| `_stack_memory_pool_resize_block` | 192/192 | residual; no parked credit |

Both retain their existing exact dimensions, relocations, behavior, and prior
reopen criteria.  This reconciliation does not make the object `Matching` and
does not add fuzzy-park credit.

## Verification

- `python tools/campaign/gate.py source/memory/stack_memory_pool --all`:
  31 exact, 2 residual, 0 unwritten;
- strict exact-set comparison against the inherited scaffold candidate:
  zero lost exact functions;
- full `ninja`: pass, including report, semantic report, and progress stages;
- full report for this unit: 33 functions, 31 matched, 100% data;
- `python tools/fake_match_scan.py source/memory/stack_memory_pool.c
  --fail-on-findings`: zero findings;
- `python -m pytest -q`: 258 passed;
- `git diff --check`: pass.

The fixed-false scaffold is therefore superseded by an authenticated,
runtime-reachable caller/helper topology with no strict progress loss.
