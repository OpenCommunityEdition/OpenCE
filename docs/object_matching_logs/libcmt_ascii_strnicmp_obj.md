# `libs/libcmt/_strnicm.obj` vendor-assembly park

This isolated lane starts from authoritative campaign commit `9b666b7d`.
Configuration index 690 remains `MISSING`. The object was unparked, unowned,
and absent from production history and prior object logs. The similarly named
`strnicmp.obj` at index 666 is a separate readable-C locale wrapper and does
not change this member's provenance. No production or configuration file was
edited.

## Plain-English behavior

`__ascii_strnicmp` compares at most a caller-specified number of bytes from two
strings using ASCII-only case folding. It converts `A` through `Z` to lower
case, stops on a terminator, mismatch, or exhausted count, and returns exactly
`-1`, `0`, or `1` according to unsigned byte ordering. The admitted readable-C
`strnicmp.obj` calls this specialized helper when the active locale is the C
locale.

## Provenance and strict evidence

The exact `obj\i386\_strnicm.obj` member was extracted from local XDK 3911
`libcmt.lib` before any source work. Its COFF `.file` record names
`..\i386\_strnicm.asm`; the pinned Microsoft source tree at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f` contains the corresponding file at
`NT/private/sdktools/crt/crtw32/string/i386/_strnicm.asm`. The assembler
compiler identifier and absence of C `.bf/.lf/.ef` records independently
confirm hand-written vendor assembly.

The authentic archive section is already strict-equal to January:

- symbol `___ascii_strnicmp`;
- 91/91 bytes;
- zero relocations on both sides;
- normalized SHA-256
  `f4bcce7fd15fda229960963a353403dc2593211edcf5b727cc537737f00b7a2f`;
- one `.text` section and no owned data, BSS, or read-only data.

## Decision and reopen criterion

Campaign policy forbids checking in assembly and forbids contorting C merely
to imitate an authenticated vendor-assembly instruction stream. No C
experiment was run. Production and configuration remain pristine; this is a
documentation-only vendor park, not an incomplete source-shape search.

Reopen only if the project adopts an explicit reviewed policy for admitting
authenticated XDK vendor-assembly members, or if a genuinely readable C
implementation is desired under a separate semantic/nonmatching track. The
strict archive proof should not be mistaken for permission to substitute the
prebuilt object into the reconstructed build.

## House, Berth, bug, and integrity audit

No source change, assembly insertion, `volatile`, forced inline, undefined
behavior, flag change, byte patch, comparator exception, or prebuilt-object
substitution was introduced. No original behavioral bug was identified; the
ASCII-only folding and three-way return are intentional fast-path semantics.
The only repository change is this plain-English evidence log. Tooling tests
passed 179/179 and `git diff --check` is clean.
