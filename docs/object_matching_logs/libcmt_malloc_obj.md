# `libs/libcmt/malloc.obj` exact-match ledger

This fresh atomic lane starts from authoritative campaign commit `fc8e1b07`.
Before editing, the object was confirmed `MISSING` at index 802, unparked,
absent from all prior object logs, refs, and worktrees, and free of active peer
ownership.

## Plain-English behavior

`malloc` passes the requested byte count and the process-wide new-handler mode
to `_nh_malloc`. That retry routine rejects requests too large for the CRT,
tries the heap, and, after failure, invokes the installed C++ new handler when
enabled. A successful handler causes another allocation attempt.

`_heap_alloc` performs the actual system call. A zero-byte request becomes one
byte. Legacy CRT heap modes round requests to a 16-byte paragraph; the system
heap does not. It then allocates from the process heap without zero-filling.

## Primary provenance and strict inventory

The exact XDK 3911 member `obj\i386\malloc.obj` was extracted before source
work. CodeView identifies ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\heap\malloc.c`. The matching
1991-2001 Microsoft source was also recovered from pinned Easy-Build-2k commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. January bytes remain authoritative.

Archive and January are independently strict-equal for all runtime sections,
and the object owns no data:

| Function | Size | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_malloc` | 18 | 2 | `7f388340df849960003a441cc927c3ded189f49d9ea02af2e49fdd7ade73df0a` |
| `__nh_malloc` | 44 | 2 | `dbb3e472b55752a4522ce5ad849d314a7da2b8c9f63a5cacc72522d5c98b0d0e` |
| `__heap_alloc` | 39 | 3 | `89e1a6a35c8604b033bdca1b6363d749a67000283dd305ad7b32aa9c3889e828` |

All seven relocation addresses, types, destinations, and addends match. They
reference `_newmode`, `_nh_malloc`, `_heap_alloc`, `_callnewh`, `__active_heap`,
`GetProcessHeap`, and `RtlAllocateHeap` exactly once each.

## Measured source experiments

| ID | One changed source factor | Strict result | Decision |
|---|---|---|---|
| E01 | Microsoft allocator behavior with the redundant maximum-size check simplified out of the retry loop | `_malloc` and `_heap_alloc` exact; `_nh_malloc` 45/44 bytes, 2/2 relocs in reversed call order | Rejected |
| E02 | Restore Microsoft's inner `size <= HEAP_MAXIMUM_REQUEST` guard inside the retry loop | All three functions exact, including relocation order and identity | Accepted |

The repeated check is readable defensive CRT logic, not a codegen-only dummy.
It keeps the retry path locally valid even though the entry guard has already
excluded oversized requests. Preserving it also reproduces VC7's original loop
layout.

## Strict gates and house/Berth audit

The accepted source uses `size_t`, named heap modes, a named maximum-request
limit, and typed system API declarations. It mirrors the Microsoft algorithm
rather than the disassembly. It contains no assembly, `volatile`, forced
inlining, undefined behavior, byte forcing, compiler pragma, alignment trick,
flag change, comparator exception, or relocation waiver.

No independently actionable January bug was established. In particular,
returning a distinct non-null allocation for a zero-byte request is deliberate
historical CRT behavior and is preserved, not marked as a defect.

The forced object rebuild and both full builds passed. The semantic report
evaluated 3,594 functions in 465 units, accepted 3,510 exact functions, and
reported zero unit errors. The admission audit found no new candidate,
contradiction, or revocation for `libs/libcmt/malloc`. Progress reached 364/833
complete objects overall and 97/212 libcmt objects; libcmt has 157/476 exact
functions and 11,422/55,015 exact code bytes. All 179 tests pass. JSON,
prohibited-construct, and diff checks are clean. The clean-commit regression
snapshot/check classifies all three functions as `still_exact`, with no changed
nonexact functions, failures, or warnings.
