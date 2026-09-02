# Small memory-family closeout revalidation (2026-09-01)

## Scope

This audit started from canonical commit `ecb25ee0c` and revalidated the three
smallest memory-family residuals in order:

1. `source/memory/circular_queue.obj`;
2. `source/memory/lra_cache.obj`;
3. `source/memory/lrar_cache.obj`.

The relevant Claude branches, committed object ledgers, campaign tooling
documentation, registered worktrees, and locally available donor trees were
checked before any candidate was compiled.  No `CLAUDE*.md`, `TREE*.md`, or
`AGENTS.md` applies to these files in the canonical repository or in the three
named Claude worktrees.  The work used the XDK 3911 flags from the generated
build graph and `tools/campaign/gate.py`; acceptance still requires strict
normalized bytes and relocation identity rather than an objdiff percentage.

## Claude branch disposition

- `claude/circular-queue-dequeue` at `a40ba7917` is an ancestor of this
  canonical commit.  Its queue reconstruction was subsequently subjected to
  the 37-experiment optimizer audit and the credible fuzzy-park review.
- `claude/lra-cache-17` at `f0e5a48f7` supplies an early LRA foundation, but
  canonical has the later ordinary-C allocator, authenticated private-name
  cleanup, source-organization cleanup, and barrier rejection.  There is no
  branch-only allocator body to integrate.
- `claude/lrar-final-19` at `a9165eccb` is an ancestor of canonical.  Canonical
  also contains the later rejection of the `noinline` plus copied-helper exact
  construction and the descriptive private-name cleanup.

Copying any of the three branch tips would therefore regress provenance,
house-rule compliance, or both.

## Fresh strict measurements

| Unit | Strict exact | Sole residual | January | Canonical | Relocations T/B | Disposition |
|---|---:|---|---:|---:|---:|---|
| `circular_queue.obj` | 7/8 | `_circular_queue_dequeue_data` | 256 | 240 | 12/12 | existing honest park, 93.947365% |
| `lra_cache.obj` | 13/14 | `_lra_allocate` | 400 | 368 | 4/4 | existing honest park, 65.6755% |
| `lrar_cache.obj` | 11/12 | `_lrar_allocate` | 784 | 1056 | 27/47 | existing honest park, 67.61811% |

All 31 exact siblings remained exact in the clean one-TU rebuilds.  The source
hashes revalidated here are:

- `circular_queue.c`: `47ba0a98b03b983720a564bd54abb8aa1abaff1c`;
- `lra_cache.c`: `15cedbfe81f1d95e0f5fa0a4a4eae429fecf03a3`;
- `lrar_cache.c`: `65e9892a425dc2efc854ea6f57875f8c68314467`.

## Newly checked LRAR machine-code analogue

The registered `claude-hsmap-2` tree contains a separately reconstructed
LRAR allocator in `src/halo/memory/lruv_cache.c` (introduced by its commit
`34b96d51e`; current file blob `81af0ec4beb60dec9b6050426ddb9dd873461eb0`).
It is not original Bungie C and is therefore only a weak machine-topology
analogue.  Its recovered code has the same significant mixed boundary seen in
January: two block lookups remain calls while the lookup after ring-index
advance appears expanded.  This corroborates the existing
TU-context/selective-inlining diagnosis; it does not authorize copying a
helper body or forcing `noinline`.

One donor-grounded ordinary-C hypothesis was nevertheless new enough to test:
retain a separate `next_block_index` lifetime throughout `lrar_allocate`, as
shown by that cross-build reconstruction.  With every other canonical source
choice unchanged, VC7 produced 1,088 padded bytes and 47 relocations, versus
the 1,056/47 retained form and January's 784/27.  All 11 siblings stayed exact,
but the allocator moved farther away, so the hypothesis was reverted.

An attempted house-style substitution of `VALID_INDEX` in `lrar_get_block`
was also reverted without admission.  `match_assert` stringifies its predicate;
the substitution changed January's exact assertion COMDAT and consequently
the relocation identities of the exact getter and its callers.  The explicit
predicate is target-proven source text here, not gratuitous manual bit logic.

## Final disposition

No production source, header, symbol ownership, status, or parked measurement
changed.  The canonical ordinary C remains the most complete admissible state:

- the circular queue residual is the exhausted `BL`/`EBX` lifetime versus
  nested-validator-inline fixed point;
- the LRA residual is the exhausted loop-carried-zero scheduling fixed point,
  with the size-matching `_ReadWriteBarrier()` form still rejected as fake
  compiler steering;
- the LRAR residual is the natural selective-inlining mismatch, with the old
  exact `noinline` plus selectively copied helper bodies still rejected.

Reopen only when the existing object-specific ledgers' evidence conditions are
met: authoritative January source/local records, a genuine source donor, a
compatible original compiler/QFE, or a new semantically meaningful C topology.
Do not rerun the already exhausted declaration, branch, barrier, copied-helper,
or forced-inline/noinline families.
