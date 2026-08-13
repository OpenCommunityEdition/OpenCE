# `libcmt/isatty.obj` exact-match ledger

## Plain-English behavior

This function checks whether a valid CRT file descriptor refers to a character
device. Invalid descriptors return zero; valid descriptors return the device
bit from the matching CRT `io_info` entry.

## January and XDK provenance

The January target owns one 42-byte external `__isatty` function and no runtime
data. XDK 3911 `libcmt.lib` contains `obj\\i386\\isatty.obj`; its COFF records
identify `..\\isatty.c`, the original Microsoft source path, and VC7. The XDK
member is itself strict-equal to January, so it is the authoritative topology
and type-provenance source rather than a guessed modern CRT substitute.

The XDK CodeView records name the 40-byte `ioinfo` fields (`osfhnd`, `osfile`,
`pipech`, `lockinitflag`, `lock`, and `RawEvent`). Production uses readable
equivalent names and proves both the 24-byte critical-section prefix and the
40-byte record size at compile time.

## Strict target evidence

- function: `__isatty`
- size: 42 bytes
- relocations: DIR32 `+0x06 -> __nhandle + 0`; DIR32
  `+0x1A -> ___pioinfo + 0`
- normalized SHA-256:
  `988ac534c377e917441fe59973e39d604afe38f707432f92aebf99c8319dc412`
- target-owned `.rdata`, `.data`, and `.bss`: none

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| I01 | Unsigned descriptor bound, two-level 32-entry `io_info` table, unsigned flag field | 42/42 and 2/2, but `movzx` differed from January's `movsx`; rejected |
| I02 | Same topology with the XDK CodeView-proven plain `char osfile` field | **STRICT EXACT**: 42/42 bytes, 2/2 relocations, identical normalized hash and destinations |

## House/Berth audit

- The structs and constants are named; no opaque byte array is used to force an
  offset.
- The short invalid-handle return is retained because it is the natural CRT
  guard and is required by the January control-flow shape.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  alignment directive, compiler-flag change, or comparator exception is used.
- This small validation routine contains no discovered original bug.

The rebuilt object owns exactly the target's one external function COMDAT and
introduces no runtime `.rdata`, `.data`, or `.bss`. Admission requires strict
function and ownership equality, full Halo and CRT
builds, zero semantic errors, a clean forced-rebuild regression check, all 179
tool tests, and a clean worktree.
