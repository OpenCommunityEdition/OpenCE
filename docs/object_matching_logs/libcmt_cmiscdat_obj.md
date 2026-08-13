# `libs/libcmt/cmiscdat.obj` exact-match ledger

This is an atomic, data-only reconstruction from campaign commit `eb1e45b0`.
Before selection, the lane screened the canonical configuration, parked ledger,
all local refs and worktrees, object logs, and active agent ownership. The unit
was genuinely unstarted, absent from the parked ledger, and owned by this lane.

## Plain-English behavior

The object owns the CRT floating-conversion dispatch table. It has six slots,
and this build initializes every slot to `_fptrap`. In plain English, any use
of one of these unconfigured conversion hooks deliberately reaches the CRT's
floating-point trap routine.

## Provenance and January inventory

This layout was not synthesized from likely names. The exact XDK 3911 archive
member `obj\\i386\\cmiscdat.obj` was extracted from `xbox/lib/libcmt.lib` with
the XDK 7.00.9290 librarian. Its runtime `.data` section is strict-equal to the
January split object:

- one externally linked table symbol, `__cfltcvt_tab`, at offset zero;
- 24 bytes of initialized writable `.data`;
- six four-byte `IMAGE_REL_I386_DIR32` entries at offsets `0, 4, 8, 12, 16,
  20`;
- every relocation resolves to external function `__fptrap + 0`;
- no executable code or other runtime-owned section.

The extracted archive member's compiler/debug/build-number sections are
metadata and are not source-owned runtime state. The reconstructed object must
reproduce the table's bytes, relocations, external symbol class/type, and data
section flags.

## Experiment record

| ID | Readable C shape | Result |
|---|---|---|
| E01 | Typed six-element function-pointer table, each entry `_fptrap` | Candidate under fresh validation |

No alternate shape is justified unless E01 fails a measured target property.

## Strict verification

XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the candidate with the generated
`/O1 /Gy /I"xbox/include"` flags. The January target, candidate, and extracted
XDK archive member all have:

- data size `24/24/24`;
- six relocations at the same offsets, all `IMAGE_REL_I386_DIR32` to
  `__fptrap + 0`;
- normalized SHA-256
  `9d908ecfb6b256def8b49a7c504e6c889c4b0e41fe6ce3e01863dd7b61a20aa0`;
- external `__cfltcvt_tab` at offset zero with data type and storage class 2;
- initialized writable `.data` flags `0xC0300040`;
- no other runtime-owned section.

`section_infos_equal` is true for target versus candidate, target versus the
XDK member, and candidate versus the XDK member. The candidate's section number
differs only because ordinary compiler metadata precedes `.data`; ownership is
compared by section content, flags, symbol offset/class, and relocation
identity rather than an incidental COFF ordinal.

Current-campaign gates passed:

- full `halobetacache_build` and `libcmt_build`;
- `semantic_progress` with zero unit errors;
- `progress`, reporting `libs/libcmt/cmiscdat` complete with all 24 data bytes;
- semantic audit with zero unit errors;
- admission audit with no `cmiscdat` contradiction, revocation, or unreviewed
  candidate (the sole `shell_xbox` contradiction is pre-existing and unrelated);
- 179/179 tool unit tests.

The required clean-commit regression snapshot/check is run after this ledger
is committed. No historical candidate object is used as proof.

## House/Berth, readability, and bug audit

The table has a named function-pointer type and a normal C initializer. The
no-argument callback prototype uses explicit `(void)`. There are no magic
structure offsets, tag/datum/object accesses, or speculative types. The value
six is the January- and XDK-proven table width documented above.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, comparator exception, or codegen-only
cast is used. No original January bug was found, so no bug-preservation comment
is needed.
