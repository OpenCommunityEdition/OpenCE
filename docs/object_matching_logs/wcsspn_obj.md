# `wcsspn.obj` exact-match log

## Plain-English behavior

This CRT routine counts the initial UTF-16 characters in `string` that all
belong to `control`. It stops at the first character absent from the control
set or at the string terminator and returns that character index.

## Provenance and target inventory

The completed local result at commit `98f28ba7` (duplicated later as
`ec562740`) was treated as untrusted. Its object-scoped source was transplanted
onto the current campaign line, rebuilt from scratch with XDK 3911 CL
13.00.9254.1, and checked independently against the January target.

- Generated object flags: `/O1 /Gy /I"xbox/include"`, unchanged.
- The January target owns exactly one 70-byte `.text` COMDAT.
- `_wcsspn` is the only function and is externally linked (COFF type `0x20`,
  storage class `2`).
- There are zero relocations and no target-owned `.rdata`, `.data`, or `.bss`.

January's bytes prove the canonical Microsoft CRT topology: an outer scan of
the input and an inner scan of the control set. A missing character exits from
the inner loop; the string terminator exits the outer loop; both paths converge
on the same pointer-subtraction and divide-by-two machine epilogue.

## Measured source shapes

| ID | Source shape | Size T/B | Relocs T/B | Candidate hash | Result |
| --- | --- | ---: | ---: | --- | --- |
| E01 | Canonical nested scans with early source return | `70/70` | `0/0` | `daf0926e...035d04a` | Accepted: strict-exact |
| E02 | Single source return using `goto done` | `70/70` | `0/0` | `a3ee2c4f...ec48e4` | Rejected: pushes moved below empty-string branch |
| E03 | E01 with const-correct local scan pointers | `70/70` | `0/0` | `daf0926e...035d04a` | Accepted: same exact bytes, cleaner source |

The accepted full normalized SHA-256 is
`daf0926e297ef92eb1c4cced409b2f9f8afd164fc2a0b5dd3a8f5a9ca035d04a`.
The strict comparator also verifies the empty relocation list, and the symbol
audit verifies exact external ownership.

## House/Berth and safety audit

Each parameter is on its own line and the two return expressions are explicit.
This function retains two source-level returns as a narrowly measured exception
to the preferred single-return rule: E02 shows that merging them changes the
otherwise exact prologue and empty-string path even though total size remains
70 bytes. VC7 already cross-jumps E01's two source exits into the single target
machine epilogue.

Both scan pointers remain `const`, and neither string is modified. The source
uses no assembly, `volatile`, `__forceinline`, alignment pragma, undefined
behavior, byte forcing, magic offset, compiler-flag change, or comparator
exception. It contains no known original bug requiring a preservation comment.

## Final admission gates

The final isolated lane was rebased onto authoritative campaign tip
`fe4065f6` before admission. The object was then cleaned and rebuilt, rather
than reusing an earlier candidate artifact.

- Hardened whole-object audit: exact one-function inventory, `70/70` raw
  bytes, `0/0` relocations, identical normalized SHA-256, and matching
  external function ownership.
- Owned data audit: the target owns no `.rdata`, `.data`, or `.bss`, and the
  candidate introduces none. Compiler-only `.drectve` and discardable debug
  sections are not runtime-owned target sections.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Semantic progress: 3,448 functions evaluated, 3,364 accepted exact, and
  zero unit errors.
- Ordinary progress credits `wcsspn.obj` as one complete libcmt object. The
  current aggregate is 277/833 objects; libcmt is 10/212 objects and 11/476
  functions.
- Admission audit: no `wcsspn` candidate, contradiction, or revocation.
- Fail-closed regression manifest/check: `_wcsspn` remains exact, with no
  failures or warnings.
- Tool tests: all 177 passed.
- `git diff --check`: clean.

No production file outside this object, its configuration entry, and this log
was changed. Nothing was pushed.
