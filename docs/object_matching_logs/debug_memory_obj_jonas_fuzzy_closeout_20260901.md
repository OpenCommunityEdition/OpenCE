# `debug_memory.obj` exact-helper / fuzzy-realloc closeout (2026-09-01)

## Result

Starting from canonical `ec4ae894a10ac440566b41a017abf4f94ea3501a`,
`source/cseries/debug_memory.obj` advances from 16/18 to 17/18 strict-exact
functions. `_debug_memory_fill_with_random` is now byte-exact at 112 padded
bytes and eight relocations. `_debug_realloc` retains the target-proven
accounting condition, reaches January's 416-byte padded extent and 25
relocations, and is honestly fuzzy-parked at 81.89706% rather than being
forced through a register-allocation residual.

The source and symbol manifest also adopt the authenticated global name
`debug_memory_globals` and field name `current_heap_size`. No address-derived
function or global name remains in the translation unit, and the temporary
macro alias previously used only inside `debug_dump_memory_by_file` is gone.

This work was performed in isolated worktree
`work/next-debug-memory-20260901`, branch
`jonas/debug-memory-fuzzy-closeout-20260901`. It did not edit canonical, run
`configure.py` or a full Ninja build, commit, or push.

## Baseline

The PID-safe one-TU gate established:

| Function | January | Baseline | Baseline normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_debug_memory_fill_with_random` | 112 B / 8 relocs | 96 B / 8 relocs | `e063d7a27e23dbd1c780a499f6bd63aad10eed3345202930089568609831bee5` |
| `_debug_realloc` | 416 B / 25 relocs | 400 B / 25 relocs | `9b7027a370ac4b8122d49c751d252c4cc0af823f403fc717d2188486edfa9d9d` |

The other sixteen functions and all 1,120 padded runtime-data bytes were
strict exact.

## Provenance and donor audit

All repository matching methodology, throughput, fake-match, and existing
`debug_memory.obj` ledgers were read before editing. The durable rejections
cover helper formal-order swaps, end-pointer spellings, lexical aliases, the
predicate by itself, the Stian/Pastudan separate-header PC topology, and the
single-header parameter-reuse topology. Named `debug_memory` worktrees were
source-clean and contained only the recorded historical shapes. The full Git
history contains no later committed source candidate beyond the two unmerged
campaign commits used here.

The source change came from unmerged commit
`c8ff585ee941bd8454e991d7512b5b36ba0f161a`. Its ordinary loop is:

```c
for (current = pointer;
	current < (byte *)pointer + size - 1;
	current += sizeof(unsigned short))
{
	*(unsigned short *)current = local_random();
}
if (size & 1)
{
	((byte *)pointer)[size - 1] = (byte)local_random();
}
```

This is defined, typed C with the same even-word fill and odd trailing-byte
semantics as the prior reconstruction. It introduces no inert operation or
compiler steering. The integrated spelling uses the owning `cseries.h`
`word` type rather than its raw `unsigned short` underlying type; this is
code-byte neutral. It reproduces January's normalized SHA-256
`b0a0af7697011f181c2a3b1aba0114ff1f5e3dccba9e5d3059106e73d8916300`
and all eight ordered relocations exactly.

The independently recovered HCEA cache and Marathon 2/Infinity source cache
were searched for the manager strings, signatures, global/type names,
random-fill helper, and realloc body. Neither cache contains this Halo 1
implementation. Marathon's Macintosh `realloc` wrapper supplies general
allocator semantics but no January register or private-call topology. The
Stian/Pastudan PC reconstruction remains useful semantic corroboration but its
combined topology is already rejected by the 2026-08-26 ledger.

Authenticated naming comes from house-rules commit
`2a9ede1cc37821492fe56ddbecb5898f60073605`, whose HCEA PDB / exact-build
symbol-atlas pass maps image offset `0x002DCD0C` to
`_debug_memory_globals`. The surviving macro
`#define current_heap_size total_pointer_size` was additional local evidence
that the reconstructed field spelling was wrong. The symbol manifest is the
source of truth, so the same rename is applied at file offset 3001612.

## `_debug_realloc` retained fuzzy form

The retained condition is the direct January CFG and correct realloc
bookkeeping:

```c
if (result != NULL || size == 0)
```

A successful realloc updates the tracked heap size, a zero-size free subtracts
the old allocation, and a failed nonzero realloc leaves accounting unchanged.
The old `if (size != 0)` did the opposite in both exceptional cases.

With the private random-fill helper now exact, this predicate produces:

| Measurement | January | Retained candidate |
| --- | ---: | ---: |
| Padded bytes | 416 | 416 |
| Relocations | 25 | 25 |
| Normalized SHA-256 | `d812408198d3b0ffe7e64a41f32db28bceb17093d3b341f8c2c7d75e0f47f1d1` | `697e6e5109aee7ca97b938ee02877b832ecfb48ed0456393977b12be2688bebb` |
| objdiff fuzzy match | 100% | 81.89706% |

January retains the original pointer in `ESI`, requested size in `EDI`, and
allocation file in `EBX` from entry. The reconstructed allocation-file and
allocation-line locals receive a different register/stack schedule. The now
exact helper is genuinely new caller-ABI evidence, so one bounded retest of
the source-natural single-header parameter-reuse form was justified. It still
reduced the frame from 16 to 12 bytes and the padded function from 416 to 400,
so it was reverted without variants.

No `register`, `volatile`, barrier, pragma, forced inlining, assembly,
undefined behavior, type punning, fake dependency, inert expression,
nonsensical branch, or object patch is retained. Reopen only for authoritative
January/cache-beta source, authenticated local/debug information, or a natural
same-compiler donor that explains the `ESI`/`EDI`/`EBX` entry lifetimes while
retaining the 16-byte frame.

## Verification and rename blast radius

There are no header edits, so the header-includer blast radius is zero. Public
allocator prototypes remain in `source/cseries/cseries.h`; private helper
prototypes remain with their owning translation unit.

Focused verification:

- PID-safe gate before the global rename: 17 exact, one residual, zero
  unwritten;
- `_debug_memory_fill_with_random`: exact 112/112 bytes, 8/8 relocations;
- `_debug_realloc`: 416/416 padded bytes, 25/25 relocations, 81.89706%;
- bounded parameter-lifetime retest: rejected at 400/416 and inverse-reverted;
- fake-match scan of `source/cseries/debug_memory.c`: zero findings;
- isolated parked-manifest validation: one active, zero stale, zero invalid;
- focused fake-scan and parked-validator tests: 24/24 pass;
- focused symbol-import and strict-comparator tests: 64/64 pass;
- `git diff --check`: pass.

The copied January split object still spells the target global
`_data_002dcd0c`, so an ordinary gate after the source-of-truth rename reports
ten exact siblings as relocation-identity residuals. This is expected stale
split metadata, not a byte regression. A pre/post candidate-object audit
checked all 18 function owners and found identical sizes, relocation counts,
normalized bytes, relocation addresses/types/addends after the single mapping
`_data_002dcd0c -> _debug_memory_globals`. It also compared all 32 non-debug,
non-text sections byte-for-byte with zero differences. The regenerated split
must therefore restore the effective 17/18 gate.

Exact-loss checklist for integration:

1. regenerate the split object from the updated `config/symbols.json`;
2. require 17 exact / one parked residual / zero unwritten;
3. require `_debug_memory_fill_with_random` to retain its exact hash and all
   eight relocations;
4. require all sixteen inherited exact siblings to remain exact;
5. require all 1,120 padded runtime-data bytes to remain exact; and
6. reject integration if any relocation mismatch remains other than the stale
   pre-regeneration global spelling described above.
