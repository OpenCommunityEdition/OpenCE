# `stack_memory_pool.obj` HCEA signature and residual audit (2026-09-02)

## Scope and baseline

This audit was performed in an isolated worktree based on canonical commit
`226bdaefade1f5d02ec02672d8a134ed3b4ee255`.  The inherited January gate was:

- 31/33 strict-exact functions;
- 4,112/4,944 exact padded code bytes;
- `_stack_memory_pool_allocate`: residual at 640/640 bytes and 36/36
  relocations;
- `_stack_memory_pool_resize_block`: residual at 192/192 bytes and 13/13
  relocations.

The full build passed before any source experiment.  The parked-function
validator reported 82 active, 0 stale, and 0 invalid entries.  Neither
remaining stack-pool function had parked credit.

## Local frontier and donor census

A read-only recursive census found 1,464 then-readable local worktree copies
of `stack_memory_pool.c` representing 21 distinct source hashes.  Every
distinct source was compiled against the current include tree with
`tools/campaign/gate.py`.  Three equivalent current-frontier spellings reached
31/33; no local source reached 32/33 or 33/33.  All older distinct sources were
strictly behind the inherited frontier or were incompatible non-January
donors.

The complete prior 82-experiment ledger at
`work/stack-memory-pool-full-admission/docs/object_matching_logs/stack_memory_pool_obj.md`
was read before new experiments.  The Stian, Pastudan, HaloCEA, PUNPCKHDQ, and
Marathon source corpora were also searched.  Marathon contains no analogous
stack-pool allocator; the later Halo sources corroborate behavior but do not
resolve the two January register-lifetime differences.

## New HCEA PDB evidence

The June 24, 2011 HCEA release PDB was queried directly with the local
registration-free `DIA2Dump.exe`:

- artifact: `HCEX_Release.pdb`, SHA-256
  `0FD11B084FD2169CAFE071FD8C3E4083D6E9564F01E7964F6C26E5A03EEE4DFF`;
- `_stack_memory_pool_allocate` is recorded as a static, non-inline,
  non-assembly function with the core signature
  `memory_block *stack_memory_pool_allocate(stack_memory_pool *pool, unsigned long size)`;
  its parameter records are named `pool` and `size`;
- `_stack_memory_pool_resize_block` is recorded as a static, non-inline,
  non-assembly function with the core signature
  `memory_block *stack_memory_pool_resize_block(stack_memory_pool *pool, memory_block *reference, unsigned long new_size)`;
  its parameter records are named `pool`, `reference`, and `new_size`.

This is cross-build provenance, not a January byte oracle.  January code and
relocations independently prove the additional `file` and `line` diagnostic
arguments.  The production source now keeps those January-only arguments while
using the PDB-authenticated core order, unsigned sizes, and semantic parameter
names.  A strict candidate-to-candidate COFF comparison proves the change is
code-neutral for both residuals, and the whole-unit gate proves all 31 exact
siblings are unchanged.  The residual candidate hashes remain:

- allocator: `c49ac9850cc6a1eba2515abca69cc4c31d45ada6db4a788447f22b326babe12d`;
- resize: `4d1bc2fd72947f08ea536281839e1b7b54c02f33d5d383f679545301877f272b`.

## Bounded new experiments

### Unsigned allocator formal without in-place mutation

The earlier ledger changed the formal type and mutated the formal in the same
experiment, then showed that mutation alone changes the compiler-derived
private ABI.  The newly authenticated unsigned formal was therefore tested
independently while retaining the separate `aligned_block_size` local.  It is
code-neutral: the target private ABI, all exact callers, and both residual
hashes remain unchanged.  This authentic type correction is retained.

### HCEA core parameter order

Both PDB-authenticated parameter orders were tested with every call site
updated consistently.  XDK C2 derives the same private calling conventions and
emits identical bytes for every function.  The semantic orders and names are
retained because they improve source provenance without source shaping or a
byte regression.

### Multi-use base-address value graph

A typed local held the checked `pool->base_address` and was reused at the
empty-pool placement site.  This recovered the target's explicit
`mov eax,[edi+4]` entry load, but it also created a third stack local, grew the
allocator from 640 to 656 padded bytes, and shifted the otherwise exact core.
It was rejected.  A direct negative spelling of the compound assertion was
also tested and was byte-identical to the inherited residual.  Neither shape
is retained.

## Residual evidence and disposition

`_stack_memory_pool_allocate` retains exact dimensions and relocations.  The
entire list-insertion region from target offset `0x171` through its successful
return is exact.  The remaining difference is limited to the entry pool/base
predicate, reuse of the zero-valued register, and null-epilogue merging.  The
new typed multi-use value graph demonstrates that ordinary C can recover the
explicit base load only by introducing a disproven spill and size growth.

`_stack_memory_pool_resize_block` retains exact dimensions, relocations,
unsigned branch skeleton, calls, and semantics.  The target reloads the
`reference` stack formal on the reuse path and immediately after the grow
allocation; XDK C2 retains the equivalent live value in `%esi`.  The
PDB-authenticated order, types, and names do not alter that allocation.

No park entry is added.  `stack_memory_pool_reset` still contains the inherited
synthetic `if (FALSE)` helper-retention block documented in
`stack_memory_pool_obj_jonas_retention_audit_20260901.md`.  Removing it drops
the unit to 16/33 under stock `/O2`; therefore the two residuals are not
eligible for credible parked credit until authentic January TU context
replaces that scaffold.  Exact closure should reopen only with original/local
variable provenance for the allocator value graph or a legal-C call/lifetime
boundary that explains both resize reloads.
