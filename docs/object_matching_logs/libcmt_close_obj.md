# `libs/libcmt/close.obj` provenance park

This isolated screening lane starts from authoritative campaign commit
`5fdba08c`. Configuration index 791 remains `MISSING`. The object is absent
from `config/parked.json`, production history, prior matching logs, active
worktrees, and peer claims. No production or configuration file was edited.

## Plain-English behavior

Microsoft's `_close` validates and locks a CRT file descriptor, closes its
underlying Xbox/Win32 handle, clears the CRT handle slot, maps any operating
system error, and unlocks the descriptor even if the protected operation
fails. `_close_lk` is the inner already-locked operation. The public wrapper
uses Microsoft structured exception handling so the unlock behaves like a C
`finally` block.

## Authentic provenance

The exact `obj\i386\close.obj` member was extracted from the local XDK 3911
`libcmt.lib` before any source work. CodeView names
`d:\xbox-finalfre\private\sdktools\crt\crtw32\lowio\close.c` and carries
ordinary function records. The corresponding Microsoft 1989-2001 source was
recovered from the pinned Easy-Build-2k NT mirror at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. This is authentic C, not vendor
assembly.

## Measured blocker

The archive member proves that `_close_lk` itself is fully reproducible:

- 129/129 bytes;
- 9/9 relocations, all with identical address, type, destination, and addend;
- normalized SHA-256
  `0bc9d757f7c557d2faea35ddc16e5140402a1f2bed0c77277807b14ce58a0d3c`.

The public `_close` section also has the same 155 machine-code bytes in linked
meaning and the same SEH scope-table ownership, but the January split object
contains one synthetic relocation that the authentic compiler object cannot
contain:

| Site | January split | Authentic XDK member |
|---|---|---|
| `_close + 0x6B` | `IMAGE_REL_I386_REL32`, internal destination `+0x77` | ordinary encoded local displacement `+8`, no relocation |

The transfer is the compiler-generated call to the local `finally` funclet.
After applying the January relocation, both encode the same effective branch
from `+0x6B` to `+0x77`. Csplit nevertheless represents the January side as
12 relocations while Microsoft's actual `/O1 /Gy` object has 11. The hardened
comparator correctly refuses to equate those COFF representations.

The other relocations and topology agree. January's scope-table symbol
`_rdata_002b5df0` corresponds to Microsoft's `$T17686`; both are 12 bytes with
one relocation to the same cleanup entry. This naming difference is ordinary
split ownership, not the blocker.

## Decision and reopen criterion

No source experiment was run. Source cannot create a relocation that the
authentic compiler itself omits without inventing an external boundary or
otherwise changing the SEH implementation. Doing that would be byte forcing,
not reconstruction. Therefore production and configuration remain pristine,
and this is a documentation-only park rather than a false `Matching` claim.

Reopen only if the hardened comparison policy gains a reviewed, fail-closed
equivalence rule for csplit's synthetic same-section local relocations, or if
a target extractor is produced that preserves the original compiler object's
local displacement representation. Do not spend source-shape experiments on
this object before one of those conditions is met.

## House, Berth, bug, and integrity audit

The authenticated Microsoft source was read only. No assembly, `volatile`,
forced inline, undefined behavior, byte patch, compiler-flag change, source
cast, comparator exception, or prebuilt-object substitution was introduced.
No implementation bug was confirmed during this bounded provenance screen;
the `finally` cleanup and the stdout/stderr shared-handle exception are
intentional behavior.

Production source and `config/config.json` are unchanged. The only repository
change in this lane is this plain-English evidence log. Tooling tests passed
179/179, and `git diff --check` is clean.
