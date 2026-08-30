# `files_windows.obj` `file_open` first-shot boundary (2026-08-30)

## Result

One frozen, normal typed C89 candidate was compiled for the previously
unwritten `source/tag_files/files_windows.obj::_file_open`.  It was a strict
miss and has been inverse-reverted completely.  This lane retains no
production source or header change and claims no function, byte, data, or
object credit.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `391 / 400` | `23` | `b91ff127948eb458e70ae51cf1efc5158a353749d27b2e29a03ad3abcb7b930b` |
| Frozen natural candidate | `380 / 384` | `23` | `1d4f76284ad165c392a99c337bbc3c623b952d69b9acf672b362781e9d12b3b9` |

The January split object has raw SHA-256
`d4e063cdfb45edad262d33aab2be2a646319118cf68601838af968d53dd62551`.
The frozen candidate object is preserved outside the production graph as
`build/audit/files_windows_file_open_first_shot_20260830.obj`, raw SHA-256
`f876bc707cdf8b7ea3433259232f094e1b02331543b1cf1bcff9e19c0d60b7d8`.
Whole-object hashes include compiler metadata; the table records complete
padded function packets and ordered semantic relocations.

## Documentation, history, and provenance

The matching methodology, exact-match acceleration playbook, object-ledger
README, authenticated Stian `CLAUDE.md` and `AGENTS.md`, and the complete
reachable `files_windows.obj` checkpoint were read before source work.  A
census of all 1,524 registered worktrees found zero definitions.  Canonical
history contains only the initial symbol inventory and owner-header
declaration.  The prior object checkpoint lists `_file_open` only as an
omitted residual, with no candidate bytes or fixed boundary.  This was
therefore the first emitted campaign candidate for the function.

Authenticated Stian commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`, source blob
`6050a9a2c9267702a93416158c46f6a9a8626876`, supplies the natural operation
in `src/halo/tag_files/files.c`.  It validates the file reference; checks the
permission mask, read/write requirement, and append/write relationship;
builds the full path; constructs the Win32 access mask; opens the existing
file; seeks to the end for append; closes and clears a failed append handle;
reports the Win32 error; and returns success or failure.  January disassembly
independently fixes the typed fields, assertion text and lines `0x134` through
`0x136`, named APIs and constants, complete branch envelope, and all 23
relocations.

The frozen transcription used the owner-header signature, typed
`file_reference_info`, named permission bits and Win32 constants, public
file/Win32 APIs, a 256-byte path, normal control flow, one parameter per line,
and explicit returns.  It used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or byte offset, pointer/integer
reconstruction, representation pun, undefined behavior, byte forcing,
compiler change, tool exception, or admission exception.

## Fixed boundary

The candidate reproduces January byte for byte from the prologue through all
three assertions, path construction, access-mask construction, the complete
`CreateFileA` call, invalid-handle comparison, append-bit test, and handle
store.  The first difference is at `+0x10E`, in the successful-open return
topology.  January materializes `TRUE` in `CL` and gives non-append success a
dedicated epilogue; the natural early-return transcription branches directly
to one shared `mov al, 1` epilogue.

All 23 relocation identities and the first 15 relocation addresses agree.
The shared candidate epilogue moves `SetFilePointer` two bytes earlier
(target `+0x11A`, candidate `+0x118`).  Its opposite fall-through and shared
success return then move `CloseHandle` seven bytes later (target `+0x12B`,
candidate `+0x132`), after which the error-reporting suffix remains
semantically identical at different addresses.  January has three distinct
epilogues; the donor-shaped source allows VC7 to merge the two success cases.

Closing this residue requires another boolean-result or control-flow source
shape.  The one-shot rule forbids deriving and compiling that spelling from
the miss, so no result local, combined condition, branch inversion, goto,
shared-tail, or return variant was attempted.  Do not retry this early-return
transcription without new independent January PC source or same-compiler
provenance that predicts the target's three-epilogue topology.

## Restoration and validation

The candidate body was inverse-removed.  Final production source is restored
to Git blob `5af5b9e7424e468db77d95d1fb7ae2c68917d2d2`.  A normal restoration
rebuild and the frozen whole-translation-unit regression gate pass.  The
complete inherited exact set remains unchanged at 20 functions, including
all file path leaves, create/delete/rename/existence, close, position/EOF,
last-modification-date, and positioned read/write wrappers.

The complete Halo/libcmt/report/semantic/progress graph passes.  Semantic
audit scans 470 units and 4,919 functions, reports 4,802 semantic-exact and
4,812 accepted-exact functions, and has zero unit errors.  Campaign progress
remains 384/833 linked objects, 4,773/11,060 functions, and
606,876/2,198,102 code bytes; Halo remains 282/468 linked objects,
4,606/7,574 functions, and 593,962/1,770,166 code bytes.  The strict board
remains 277/619 objects, 4,784/8,246 functions, and 663,783/1,922,669 padded
bytes.  Admission is 0 candidates / 0 contradictions / 0 revocations; parks
are 12 active / 0 stale / 0 invalid; and all 212 tooling tests pass.  The
report and semantic-report SHA-256 values remain
`b13ef78fdc5dce935a82cdf93d76a474c3273dc54148975139f7c3114b2d43d0`
and
`51d605379dee1e1d44685c048d51150e13d504ae617a5ee1d1cb450d93fb431b`.

The protected `_unit_preprocess_node_orientations` sentinel remains strict
exact at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
