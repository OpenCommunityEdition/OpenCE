# `libs/libcmt/dosmap.obj` exact-match ledger

This isolated atomic lane starts from authoritative campaign commit
`b9bad27a`. Configuration index 811 was `MISSING`; the object was absent from
`config/parked.json`, production history, prior matching logs, active
worktrees, and all peer claims. Root and both peer agents explicitly excluded
it before production editing began.

## Plain-English behavior

This object is the CRT bridge between Win32/Xbox operating-system errors and
C's `errno`. `_errno` and `__doserrno` return pointers to the current thread's
two error slots. `_dosmaperr` always records the original OS error, searches a
45-entry translation table for a C error number, then applies Microsoft's
fallback rules for access errors, executable-format errors, and unknown
values.

## Primary provenance and January inventory

The exact `obj\i386\dosmap.obj` member was extracted from the local XDK 3911
`libcmt.lib` before production source was written. Its three code sections and
360-byte error table are byte-identical to January. CodeView names Microsoft's
`d:\xbox-finalfre\private\sdktools\crt\crtw32\dos\dosmap.c` and carries
ordinary `.bf`, `.lf`, and `.ef` records for all three routines, proving C
rather than assembly.

The corresponding Microsoft 1989-2001 source was recovered from the pinned
Easy-Build-2k NT mirror at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. The reconstruction preserves its
typed error-entry table and control flow while using a minimal typed view of
the thread-data prefix. This avoids the unrelated D3D constant tables emitted
by broad Xbox umbrella headers in the archive object.

| January symbol | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `__errno` | 9 | 1 | `6226b875dee6bb94294f3b1fe32bf09b8aaa4a6d0671775b484d1d50ba6138ca` |
| `___doserrno` | 9 | 1 | `be744c9e149c0b070e4b11827b8617a4daf28e48a685d6505a229d91ac30e61c` |
| `__dosmaperr` | 115 | 7 | `c2028f17995133e58ffebc9e967e9d4140e4c45b9830b4402c1206370b3d3463` |
| `_data_003190c0` | 360 data bytes | 0 | `dc5aaf854d47b0fa4deddacb6ff02e9a7d99cb1ac06c16e03e587d4599067fbe` |

January owns exactly these three `.text` COMDATs and one `.data` section.
The rebuilt table has the same public split label, size, bytes, alignment,
section flags, and zero relocations. No BSS or read-only data is owned.

## Measured source-shape experiments

All production measurements used XDK 3911 CL 13.00.9254.1 under the inherited,
unchanged `/O1 /Gy` command.

| ID | Readable-C shape | Result |
|---|---|---|
| E01 | Faithfully reproduce Microsoft's authenticated `dosmap.c`: a typed two-field error table, typed thread-data prefix, five source-level `_getptd` accesses, unsigned range checks, and explicit final assignments; give the table January's recovered ownership name | Accepted on the first compile. All three functions and all seven function relocations match exactly; the full 360-byte table also matches exactly. |

The two accessor functions each call `_getptd` once. `_dosmaperr` has five
calls to `_getptd` plus two table relocations at `+0x12` and `+0x3E`, resolving
to `_data_003190c0 + 0` and `+4` exactly as January does.

## Original-behavior and bug audit

One genuine original table defect is preserved and marked beside the code.
`ERROR_INVALID_HANDLE` is listed twice: the first row maps it to `EBADF`, so
the later row mapping it to `EINVAL` is unreachable. Microsoft's source labels
the later row as OS error 124, strongly indicating it intended
`ERROR_INVALID_LEVEL`. A corrected derivative should replace only that later
key. Exact reconstruction must retain the duplicate because it is present in
January's owned bytes.

The routines otherwise deliberately overwrite the thread's prior error state
and provide a generic `EINVAL` fallback. No modern defensive behavior was
added because that would change the original CRT contract.

## House, Berth, and readability audit

The final source is ordinary typed C derived from authenticated Microsoft
source. Parameters are vertically formatted, no-argument prototypes use
`void`, returns are explicit, and all table entries use named Win32 and errno
constants. The minimal `thread_data` prefix names real proven fields instead
of using opaque byte offsets.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
codegen cast, alignment coercion, compiler-flag change, optimizer pragma,
comparator exception, or prebuilt-object substitution is used. The original
bug is preserved, clearly identified, and accompanied by a readable fix note.

## Final validation

- The object was deleted and force-rebuilt under unchanged `/O1 /Gy`; the
  post-rebuild hardened comparison kept all three functions strict exact.
- Whole-object runtime ownership is exact: three code
  COMDATs, one 360-byte data section, no BSS/rdata, and no unexpected runtime
  definition.
- Full `halobetacache_build` and `libcmt_build`: successful.
- `progress` and `semantic_progress`: successful; `dosmap.obj` contributes
  exactly 133 code bytes, three functions, and 360 data bytes.
- Explicit semantic audit: zero unit errors and all three functions accepted
  by strict COFF evidence.
- Admission audit: no unmarked completion candidate. Its sole contradicted
  `shell_xbox` result is the pre-existing project-wide finding and is unrelated
  to this object.
- Tooling tests: 179/179 passed.
- `git diff --check`: clean.
- Clean committed per-unit regression snapshot/check: force-rebuilt the unit,
  retained `__errno`, `___doserrno`, and `__dosmaperr` as `still_exact`, and
  reported no changed non-exact function, failure, or warning.
