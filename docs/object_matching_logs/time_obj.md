# `libs/libcmt/time.obj` exact-match ledger

This atomic lane independently revalidates an older completed result on a
fresh worktree based on campaign commit `63501296`. The historical branch was
used only to identify the candidate; target provenance and every admission
measurement below were repeated in this lane.

## Plain-English behavior

`time` asks Windows for the current system clock as a 64-bit count of
100-nanosecond ticks since 1601. It subtracts the difference between the
Windows and Unix epochs, divides by the number of ticks in one second,
optionally stores that Unix timestamp through the caller's pointer, and returns
the same value.

## Primary provenance and target inventory

The locally archived XDK 3911 `libcmt.lib` has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its member `obj\i386\time.obj` records both `..\time.c` and the full original
path `d:\xbox-finalfre\private\sdktools\crt\crtw32\time\time.c`, plus compiler
id `0x001c2426`. The member's `_time` COMDAT is already strict-equal to the
January split target:

- code size `56/56` bytes;
- two `IMAGE_REL_I386_REL32` relocations, at `+0x0a` to
  `_GetSystemTimeAsFileTime@4` and `+0x29` to `__aulldiv`, both addend zero;
- identical normalized SHA-256
  `f481c71336d955df61673ad2844701121e93e39d9392cce86c4c261bfd870d7f`;
- no target-owned `.data`, `.bss`, or `.rdata`.

This proves the target is Microsoft's readable CRT C translation unit rather
than a vendored-assembly object, and gives a primary reference object for the
function, relocation destinations, ownership, compiler identity, and size.

## Recovered representation and experiments

The retained source overlays the two 32-bit `FILETIME` halves with an unsigned
64-bit tick value in a named union. That is the ordinary VC7/XDK representation
for this Windows structure and lets the compiler perform the epoch subtraction
directly on the two halves before calling its unsigned 64-bit divide helper.
The constants are typed 64-bit values: `116444736000000000ui64` ticks between
the epochs and `10000000ui64` ticks per second.

| ID | Source shape | Size T/B | Relocs T/B | Result |
|---|---|---:|---:|---|
| E01 | Named union with `parts` and `ticks` views | `56/56` | `2/2` | retained; strict exact |
| E02 | Build the 64-bit value explicitly with shift and OR | `65/56` | `2/2` | rejected; redundant zero/OR work and an extra saved register |

No raw offsets, pointer-cast punning, packing, assembly, `volatile`, forced
inlining, undefined behavior, object-byte patch, comparator exception, or
compiler-flag override is used.

## Strict production verification

The production object was rebuilt with XDK 3911 CL `13.00.9254.1` under the
configured libcmt flags `/O1 /Gy /I"xbox/include"`. The fresh result is strict
exact:

| symbol | target | candidate | relocs | normalized SHA-256 |
|---|---:|---:|---:|---|
| `_time` | 56 B | 56 B | 2 / 2 | `f481c71336d955df61673ad2844701121e93e39d9392cce86c4c261bfd870d7f` |

Both relocations match in address, type, destination, and addend: `+0x0a`
targets `_GetSystemTimeAsFileTime@4 + 0`, and `+0x29` targets
`__aulldiv + 0`. `tools.coff_compare.section_infos_equal` returned true. Both
objects contain one 56-byte `.text` COMDAT with identical flags, externally
owned by function symbol `_time` at offset zero; neither owns runtime data.
The candidate's other sections are ordinary compiler directives and debug
metadata.

The complete `halobetacache_build` and `libcmt_build` succeeded. A forced clean
rebuild again emitted the configured `/O1 /Gy` command and reproduced strict
equality, ruling out stale-object credit. The regenerated semantic audit
scanned 405 implemented units with zero unit errors and 3,381 accepted exact
functions. Progress reported 295/833 exact objects overall and 28/212 libcmt
objects; libcmt had 1,981/55,015 exact code bytes and 196/8,637 exact data
bytes at this lane's snapshot. The full tooling suite passed **179/179** tests.

After the atomic commit, the fail-closed per-unit regression manifest was
snapshotted and checked successfully with no failures, warnings, regressions,
ownership changes, or unexpected new matches. `git diff --check` and the final
scope/policy audit were also clean.

## House/Berth audit

The implementation is ordinary readable C with its sole parameter on its own
line and one explicit final return. It uses named typed members rather than raw
structure offsets. No tag, datum, or object access is involved. No original
January bug was found, so no bug-preservation comment is needed.
