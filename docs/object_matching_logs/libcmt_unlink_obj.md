# `libcmt/unlink.obj` exact-match ledger

## Plain-English behavior

`remove` asks the Xbox operating system to delete a path. On failure it maps
the operating-system error into the CRT error state and returns `-1`; on
success it returns zero. `_unlink` is the historical thin alias for `remove`.

## January and XDK provenance

January owns two external function COMDATs and no runtime data. XDK 3911
`libcmt.lib` contains `obj\\i386\\unlink.obj`; its COFF records identify
`..\\unlink.c`, the original Microsoft path, and VC7. Both XDK functions are
strict-equal to January before reconstruction, establishing the historical C
topology and exact imported calls.

## Strict target evidence

- `_remove`: 40 bytes; three REL32 relocations (`+0x05 -> _DeleteFileA@4`,
  `+0x0E -> _GetLastError@0`, `+0x1C -> __dosmaperr`); normalized SHA-256
  `2682089dedbbd2025d380747e2bb02cf4a98fdabe9cc92d8c6f22252d949ff5f`
- `__unlink`: 5 bytes; one REL32 relocation (`+0x01 -> _remove`); normalized
  SHA-256
  `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449`
- target-owned `.rdata`, `.data`, and `.bss`: none

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| U01 | Named error local, explicit success/failure assignment, failure mapping, and thin `_unlink` wrapper | **STRICT EXACT** on the first fresh rebuild: both functions, 45/45 bytes, 4/4 relocations, hashes and destinations identical |

## House/Berth audit

- The operating-system error has a descriptive name and its unsigned width is
  preserved from the imported API.
- The failure return is a natural readable guard and reflects the historical
  CRT topology; forcing one source return would obscure it.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  magic offsets, alignment directive, flag change, or comparator exception is
  used.
- No original bug was found in this deterministic wrapper.

The rebuilt object owns exactly the target's two external function COMDATs and
introduces no runtime `.rdata`, `.data`, or `.bss`. Admission requires both
functions and complete ownership to be strict-equal,
full Halo and CRT builds, zero semantic errors, a clean forced-rebuild
regression check, all 179 tests, and a clean worktree.
