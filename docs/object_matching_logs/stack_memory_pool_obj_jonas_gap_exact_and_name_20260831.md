# `stack_memory_pool.obj` gap search exact recovery (2026-08-31)

## Result

The result-variable block-gap walk recovered in Claude/Fable commit
`21f4fc550` closes the former `_code_0010e490` residual.  Strong HCEA
compiland evidence also restores its private owner name,
`_stack_memory_pool_find_space_between_blocks`, in the source prototype,
definition, two call sites, symbol inventory, and the one static symbol-map
entry.  The object advances from **30 exact / 3 residual** to
**31 exact / 2 residual** functions and remains correctly `NonMatching`.

| Function | Padded bytes | Relocations | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_stack_memory_pool_find_space_between_blocks` | 128 | 4 | `9ced5444b8465ec80ae62ee31c80caf91a5217ee1f7fc3ee227ef7427a9d1b73` | new strict exact |

The four relocation records agree in address, type, destination, addend, and
order: offsets 48 and 53 are the two assert strings, offset 58 calls
`display_assert`, and offset 65 calls `system_exit`.

## Name provenance and source model

The HCEA `stack_memory_pool.obj` compiland records the exact static signature
`void *stack_memory_pool_find_space_between_blocks(struct stack_memory_pool *,
unsigned long, struct memory_block **)`.  Its parameter roles and block-list
gap walk uniquely match the January function at file offset 1,107,088.  The
name is recovered, not inferred; the emitted candidate symbol has COFF storage
class 3 (`static`), matching the renamed January split owner.

The retained C initializes a null result, returns the pool base when the first
block leaves a large enough leading gap, otherwise walks adjacent blocks and
returns the first sufficiently large interior gap while recording its
predecessor.  Failure leaves the result null.  The predecessor is written only
on success.  This is ordinary allocator logic with defined control flow; it
adds no raw structure offsets, register hints, assembly, barrier, fake branch,
or dead dependency.

## Anti-fake retention audit

The translation unit had a pre-existing `if (FALSE)` helper-retention block in
`stack_memory_pool_reset`.  The first exact candidate still listed this helper
inside that block, even though the real allocator also calls it.  A scratch
candidate removed only that dead call.  VC7 continued to emit the private
helper as its own 128-byte strict-exact section, and the alias-aware whole-unit
result remained `31 exact / 2 residual / 0 unwritten`.  The dead reference is
therefore removed from production.  This helper's emission and exactness now
depend only on real allocator control flow; no artificial call-count context is
part of its admission.  The other pre-existing retention calls were outside
this bounded audit and were not changed.

## Validation boundary

- The current placeholder-named January section and the independently
  regenerated HCEA-named split section both compare strict-equal to the renamed
  candidate section.
- Alias-aware whole-unit gate: `31 exact / 2 residual / 0 unwritten`; the only
  remaining residuals are `_code_0010ea00` and `_code_0010ef70`.
- Repeating both gates after removing the helper's sole dead retention call
  produced the same focused and whole-unit results.
- `tools/fake_match_scan.py` over the edited source: zero review leads.
- `git diff --check` over the edited source: pass.
- Only `source/memory/stack_memory_pool.c` and the single non-overlapping
  `config/symbols.json` entry changed; no header, semantic exception, parked
  record, generated object, build edge, or target object changed.

The orchestrator owns regeneration of the canonical split object, the
consolidated build, and the whole-board sweep.
