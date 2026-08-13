# `libs/libcmt/fileatt.obj` exact-match ledger

This is an isolated atomic reconstruction from authoritative campaign commit
`1b270ed1`. Before editing, configuration index 835 was `MISSING`; the unit
was absent from `config/parked.json`, source/history/logs, all active
worktrees, and the other agents' active claims.

## Plain-English behavior

These two Microsoft CRT helpers adapt wide-character paths to the Xbox's
narrow Win32-style file-attribute APIs. Each helper converts at most 260
bytes into a stack buffer, forces both the first and final bytes to be safe
terminators, then asks the narrow API either to read or set the attributes.

The original code deliberately does not branch on `wcstombs`'s result. That
legacy behavior is preserved: adding a conversion-error check would change
observable behavior and the January machine code. A modern caller that wants
stricter behavior should validate conversion success before invoking the
file API, rather than silently changing this reconstruction.

## Primary provenance and inventory

The exact `obj\i386\fileatt.obj` member was extracted from the local XDK 3911
`libcmt.lib`. Its CodeView records name the original Microsoft source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\awxapi\fileatt.c`, contain
ordinary function and line records, and prove this is compiled C rather than
handwritten assembly.

The historical Microsoft CRT source at that exact `crtw32\awxapi\fileatt.c`
path was independently recovered from the Easy-Build-2k NT source mirror,
commit `36fce23389e43918faf4cd88688c64aa2668ff8f`. It expresses the same two
260-byte conversion-buffer wrappers through the original `OCHAR`,
`tcstoocs`, and `Get/SetFileAttributes` portability macros. January bytes and
relocations remain the acceptance authority.

Before production source work, the extracted member compared strict-equal to
January for both runtime functions:

- `___crtGetFileAttributesW`: 57 bytes, two `REL32` relocations at `+0x20`
  to `_wcstombs` and `+0x33` to `_GetFileAttributesA@4`, normalized SHA-256
  `815148338ca4d151152660fe6262cd5d7c468a93c04d449741934770492a27db`;
- `___crtSetFileAttributesW`: 60 bytes, two `REL32` relocations at `+0x20`
  to `_wcstombs` and `+0x36` to `_SetFileAttributesA@8`, normalized SHA-256
  `5bcb591913cde9f9fa4f9e16f3fe7029ea17f5b4d0547cba7937065f23f4fa67`.

The archive also carries select-any XDK header tables and build metadata.
January's linked object owns neither; only the two external function COMDATs
belong to this runtime unit. Those discarded archive definitions must not be
recreated as source-owned data.

## Source-shape experiments

| ID | Readable C shape | Measured result |
|---|---|---|
| E01 | Expand the authentic portability macros into typed `wchar_t` input, a 260-byte `char` buffer, `wcstombs`, an explicit final terminator, and the matching narrow API call | Both functions strict exact on the first configured `/O1 /Gy` rebuild, including all four relocation identities |

No assembly, byte patch, `volatile`, forced inline, undefined behavior,
opaque offset, optimizer pragma, flag change, or comparator exception is
permitted.

## Acceptance evidence

The candidate was built with XDK 3911 CL under this unit's configured
`/O1 /Gy` flags. Both functions compare strict-equal to both the January
split target and the extracted archive member, including sizes, normalized
bytes, all relocation addresses/types/destinations, COMDAT flags, linkage,
and symbol ownership. January and the candidate each own exactly two external
`.text` COMDATs and no runtime `.data`, `.bss`, or `.rdata`; the archive-only
select-any definitions are correctly absent.

A forced clean followed by the full Halo and libcmt builds passed, along with
semantic and progress generation. The semantic ledger accepted both functions
with zero unit errors. The admission audit reported no candidate,
contradiction, or revocation for `libs/libcmt/fileatt`. All 179 tooling tests
and `git diff --check` passed. The final clean-commit whole-unit regression
snapshot/check then rebuilt the object and reported both functions as
`still_exact`, with zero failures, warnings, changed-nonexact functions, or
unreviewed newly-exact functions.

## House, Berth, readability, and original-bug audit

This is ordinary typed C recovered from primary archive and Microsoft source
provenance: parameters occupy their own lines, returns are explicit, the
260-byte limit is named, and all external calling conventions and types are
stated. There is no assembly, `volatile`, forced inline, undefined behavior,
byte forcing, raw structure offset, optimization pragma, flag change,
comparator exception, or ownership fiction. Neither function is zero-argument,
so the project's explicit `(void)` rule does not apply.

The preserved conversion-error/truncation behavior is called out directly in
the production source as an original bug, together with the defensive fix a
future non-matching port could choose. The exact reconstruction intentionally
does not apply that fix because doing so would change the original runtime's
observable behavior and bytes.
