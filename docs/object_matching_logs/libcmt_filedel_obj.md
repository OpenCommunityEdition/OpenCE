# `libcmt/filedel.obj` exact-match ledger

## Plain-English behavior

This Xbox CRT compatibility wrapper converts a wide-character file name into
a 260-byte narrow path buffer, forces the final byte to a terminator, and asks
the narrow operating-system API to delete that path.

## January and XDK provenance

January owns one 57-byte external function COMDAT and no runtime data. XDK
3911 `libcmt.lib` contains `obj\\i386\\filedel.obj`; its COFF records identify
`..\\filedel.c`, the original Microsoft source path, and VC7. The XDK member is
strict-equal to January. Its CodeView names the wide input `lpFileName`, the
local array `szPathName`, and `wchar_t`, while the exact 0x104-byte frame proves
the 260-byte path capacity.

## Strict target evidence

- function: `___crtDeleteFileW`
- size: 57 bytes
- relocations: `+0x20 -> _wcstombs + 0`; `+0x33 -> _DeleteFileA@4 + 0`
- normalized SHA-256:
  `815148338ca4d151152660fe6262cd5d7c468a93c04d449741934770492a27db`
- target-owned `.rdata`, `.data`, and `.bss`: none

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| F01 | Named 260-byte path, conversion, explicit last-byte terminator, return `DeleteFileA` | 50/57 bytes and 2/2 relocs; missing January's defensive first-byte clear; rejected |
| F02 | Clear both buffer endpoints around the same conversion and delete call | **STRICT EXACT**: 57/57 bytes, 2/2 relocations, identical hash and destinations |

## House/Berth and bug audit

- The path capacity is named and derived from the January frame/CodeView rather
  than appearing as unexplained stack arithmetic.
- The source is readable C with a typed wide string and one final return.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  alignment directive, flag change, or comparator exception is used.
- Original behavior ignores conversion failure/truncation and still calls
  `DeleteFileA`. That historical weakness is preserved for byte accuracy. A
  safer implementation would check whether `wcstombs` returned `(size_t)-1`
  or filled the buffer before attempting deletion.

The rebuilt object owns exactly the target's one external function COMDAT and
introduces no runtime `.rdata`, `.data`, or `.bss`. Admission requires strict
whole-object equality, full Halo and CRT builds,
zero semantic errors, a clean forced-rebuild regression check, all 179 tests,
and a clean worktree.
