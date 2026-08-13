# `libs/libcmt/heapinit.obj` exact-match ledger

This atomic lane treats prior commit `19506a93` only as an untrusted readable-C
proposal. It screened the parked ledger, active ownership, historical refs, and
canonical configuration before copying the proposal into a fresh worktree from
the authoritative campaign tip. All proof is regenerated from the January
object and a new XDK build.

## Plain-English behavior

This tiny CRT object does two things. `__heap_select(void)` returns one, which
selects the normal heap implementation. The initialized global `_amblksiz`
stores the CRT allocation-block size, 16 bytes.

## January target inventory

- One four-byte cdecl function
- One four-byte initialized writable data section
- No relocations

The function should compile as `xor eax,eax; inc eax; ret`. The data payload
should be the little-endian bytes `10 00 00 00`. Admission requires more than
those bytes: the symbol storage, section ownership, flags, and offset must also
match the January object.

## Experiment record

| ID | Readable C shape | Result |
|---|---|---|
| E01 | `return 1;` plus externally linked `_amblksiz = 16` | Candidate under fresh strict validation |

No alternative spelling is justified unless E01 fails a measured target
property.

## Strict verification

- XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the source with the generated
  `/O1 /Gy /I"xbox/include"` flags.
- Function size: `4/4`; relocations: `0/0`; normalized SHA-256:
  `194f81a127723ec366ff0b8410df190c0649a05808a94d5554d82de5af7f425b`.
- Data size: `4/4`; relocations: `0/0`; normalized SHA-256:
  `097328e8c957de2428283954f6a1ee8ff7ad7def12e100a600178407f5decf24`.
- The target and candidate external-runtime symbol inventories match exactly:
  `___heap_select` is an external function at offset zero in `.text`, and
  `__amblksiz` is external data at offset zero in writable `.data`. Symbol
  type, storage class, section flags, and section ownership all match.
- Neither object owns another runtime section, and neither has a relocation.

## Current-campaign admission gates

- Full `halobetacache_build` and `libcmt_build`: passed.
- `semantic_progress`: passed with zero unit errors.
- `progress`: credits the complete object, its four code bytes, and its four
  initialized data bytes. Global totals are omitted because concurrent atomic
  admissions make them stale without changing this proof.
- The semantic audit records `___heap_select` with both `objdiff` and
  `semantic-coff` proof. The admission audit reports no `heapinit`
  contradiction, revocation, or unreviewed candidate.
- The clean-commit whole-object regression snapshot/check is run before
  handoff and must preserve both the exact function and complete data/symbol
  ownership without warning.
- `python -m unittest discover -s tools -p 'test_*.py'`: 179/179 passed.

No old object file is accepted as proof.

## House/Berth, readability, and bug audit

The no-argument prototype uses explicit `(void)`, and the function has one
explicit return. The global is named and typed; there are no magic offsets,
tag accesses, or datum accesses. The value 16 is the recovered CRT block-size
configuration and is documented here rather than hidden behind a speculative
enum.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw structure offset, comparator
exception, or codegen-only cast is used. No original January bug was found, so
no bug-preservation comment is required.
