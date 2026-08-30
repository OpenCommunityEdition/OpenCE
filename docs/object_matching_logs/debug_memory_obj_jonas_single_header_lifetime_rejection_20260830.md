# `debug_memory.obj` single-header lifetime rejection (2026-08-30)

## Result

`source/cseries/debug_memory.obj` remains honestly `NonMatching` at 16/18
strict-exact functions. One genuinely distinct, disassembly-backed
`_debug_realloc` lifetime packet compiled once. It preserved all sixteen
inherited exact owners, all 1,120 padded runtime-data bytes, and every ordered
relocation destination, but shortened `_debug_realloc` from the 400-byte
baseline to 384 padded bytes instead of January's 416. The private
`_code_0007cf50` convention did not move and the packet was inverse-reverted
without a retry.

No production source, header, configuration, compiler option, completion
label, semantic exception, parked record, target artifact, Units path, or
tooling change survives. The first-shot object is retained only under ignored
`build/audit/debug_memory_single_header_lifetime_20260830/`.

## Evidence and freshness census

This wave began at local integration commit
`6a4747bf63904d9a335b9d7a538ea7378ef08be0`, which includes the newly exact
`_debug_malloc` admission from `251e9f780f610ee2c1962d93fc104c1154d28996`.
The starting `debug_memory.c` Git blob is
`6802d0f0787153d67daf1e03dbee84c285b33303`. The current unit baseline is:

- `_debug_realloc`: 400/416 padded bytes, 25/25 relocations;
- `_code_0007cf50`: 96/112 padded bytes, 8/8 relocations;
- sixteen exact code owners; and
- 1,120/1,120 padded runtime-data bytes.

Every existing object ledger was read before editing, including the allocator
checkpoint, predicate-only rejection, external donor topology rejection,
helper formal-order and lexical controls, unlink/head-helper admissions, and
the exact malloc retest. The histories and live worktrees for
`codex/debug-memory-allocation-cluster`, `codex/debug-memory-small-final`,
`jonas/debug-memory-large-20260820`,
`jonas/debug-memory-malloc-clear-20260825`,
`jonas/debug-realloc-accounting-20260825`, and
`agent/malloc-final-20260813` were also screened. They were source-clean and
contained no caller-side grow-region alias or single-header parameter-lifetime
experiment. `git log --all -S "new_memory"` returned no hit for this source.

The authenticated Stian/Pastudan PC donor remains useful evidence, but its
combined separate-old/new-header, mutable-allocation-size, parameter-reuse,
and goto family is closed by the 2026-08-26 384-byte rejection. This wave did
not repeat its separate header, mutable size, or goto. The earlier correct
`result != NULL || size == 0` predicate-only candidate is also closed in
isolation.

The new premise came directly from January's current disassembly and the now
exact in-TU insertion helper. January carries the original pointer in `ESI`,
the requested size in `EDI`, and the allocation file in `EBX`. On an existing
allocation it writes the retained line back to `[ebp+0x14]`, reloads the file
into `EBX`, and later reuses `ESI` for the returned header/user pointer. Its
grow arm therefore emits:

```text
mov edx, edi
sub edx, eax
push edx
lea ebx, [eax + esi]
call _code_0007cf50
```

That sequence proves both the caller lifetime and the private helper
convention: grow pointer in `EBX`, grow length on the stack. The reconstructed
source instead kept separate allocation-file/line locals, causing `EDI` and
`EBX` to hold the wrong long-lived values. The exact `_code_0007cfc0` now
accepts the header in `ESI`, so testing the remaining single-header caller
lifetime against the current cluster was distinct from the older broad donor
shot and from every helper-only spelling control.

## Frozen one-shot packet

The natural House-C packet retained the single `header` variable and the
existing allocator-size ternary. It removed only the reconstructed
`allocation_file` and `allocation_line` locals, assigned the retained
provenance back to the typed `file` and `line` parameters, used those parameters
to initialize the returned header, expressed the January accounting join as
`result != NULL || size == 0`, and used the already admitted in-TU comparison
order `maximum_pointer_size < total_pointer_size`.

The direct grow expression was deliberately left intact. If the proven
`ESI`/`EDI` caller lifetimes emerged, normal VC7 private-call coalescing would
naturally form January's `LEA EBX` plus pushed delta. A speculative named
pointer alias was not added because it would provide no additional semantic or
provenance evidence and prior lexical-scope controls already show that aliases
alone do not change this helper convention.

The packet was typed, readable C89 with one parameter per line and the existing
explicit terminal return. It used no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, compiler barrier, raw address/offset, pointer-integer
reconstruction, pointer or union pun, undefined behavior, byte forcing,
synthetic anchor, object patch, or tool/flag substitution.

The frozen candidate source raw SHA-256 was
`2db48760022150778d9bb3b08930ab8410a647cc00afb92c781c0b6ff37c1395`.
A Ninja dry run exposed exactly one ordinary edge:

```text
[1/1] CL build\base\source\cseries\debug_memory.obj
```

Exactly that edge compiled once. The preserved first-shot object raw SHA-256
is `219aff27ace439c2b9019f956fcfeee7db84ba3406e3bf5d93425b8f99176bea`.

## First-shot comparison

| Owner | January | Starting baseline | First shot |
| --- | ---: | ---: | ---: |
| `_debug_realloc` padded bytes | 416 | 400 | 384 |
| `_debug_realloc` relocations | 25 | 25 | 25 |
| `_debug_realloc` normalized SHA-256 | `d812408198d3...` | `9b7027a370ac...` | `3768b90fc83e...` |
| `_code_0007cf50` padded bytes | 112 | 96 | 96 |
| `_code_0007cf50` relocations | 8 | 8 | 8 |
| `_code_0007cf50` normalized SHA-256 | `b0a0af769701...` | `e063d7a27e23...` | `e063d7a27e23...` |

The new candidate is not the older donor candidate despite sharing a 384-byte
padded extent: the old donor shot normalized to `b8c79b0d99f9...`, while this
single-header packet normalized to
`3768b90fc83e86303024ea254548e684a56af495245856180045361fbc03f6e3`.

VC7 reduced the frame from the target's 16 bytes to 12 and selected another
wrong lifetime allocation. It loaded size into `EBX`, kept line in `EDI`, and
did not load the original pointer until after the manager check. After the
exact `_code_0007cfc0` call, its grow arm was:

```text
mov eax, [ebp-4]
add esi, 0x20
cmp ebx, eax
sub ebx, eax
add eax, esi
call _code_0007cf50
```

Thus the helper still received pointer in `EAX` and size in `EBX`; its body was
byte-for-byte unchanged. The helper relocation was at candidate `+323` rather
than January `+341`. The candidate did implement the correct success-or-zero
join and the target comparison instruction order, but it reached accounting
too early and loaded the total/maximum globals in the opposite schedule. Its
last four data relocations were `+342`, `+348`, `+359`, and `+366`, versus
January `+366`, `+373`, `+382`, and `+389`.

The strict whole-unit gate reported 16 exact, 2 residual, and 0 unwritten.
All sixteen inherited functions remained exact; `_debug_realloc` was the only
changed nonexact owner, and `_code_0007cf50` was unchanged. No relocation
destination, type, addend, symbol owner, or runtime-data owner regressed.

## Durable boundary

The packet was inverse-reverted wholesale to Git blob
`6802d0f0787153d67daf1e03dbee84c285b33303`. Do not retry:

- helper formal-order swaps, equivalent final-address spellings, or lexical
  aliases;
- the accounting predicate alone;
- the public separate-header/mutable-size/parameter-reuse/goto packet; or
- this single-header file/line reuse plus correct accounting packet, including
  declaration-order or named-grow-pointer cosmetic variants.

Reopen the pair only with original/cache-beta source provenance, authenticated
debug information, or a caller topology that concretely explains how VC7
retains pointer in `ESI`, size in `EDI`, and file in `EBX` from entry while
keeping the 16-byte frame. Without that evidence, both residuals are a fixed
caller/private-ABI cluster rather than independent C spelling problems.

## Restored-state validation

The restored source rebuilt to the 400-byte baseline with normalized
`_debug_realloc` SHA-256
`9b7027a370ac4b8122d49c751d252c4cc0af823f403fc717d2188486edfa9d9d`.
The strict whole-unit gate again reports 16 exact, 2 residual, and 0 unwritten,
with 2,448/2,976 padded code bytes and 1,120/1,120 padded data bytes exact.

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,957 functions evaluated, 4,841 semantic exact,
  4,851 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, or revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Tool suite: 212/212 tests pass.
- Strict campaign board: 277/619 exact objects, 667,863/1,922,669 padded
  bytes, and 4,823/8,246 strict functions.
- `git diff --check` and the unchanged `source/units/units.c` sentinel pass.
