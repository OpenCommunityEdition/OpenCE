# `libcmt/crt0init.obj` exact-match ledger

## Plain-English behavior

This data-only CRT object owns four null sentinels that delimit pre-termination
and termination callback ranges. The linker section suffixes order the start
and end markers around callback contributions from other objects.

## January and XDK provenance

January owns four separate four-byte writable sections: `.CRT$XPA`,
`.CRT$XPZ`, `.CRT$XTA`, and `.CRT$XTZ`. Each contains one null function
pointer, has zero relocations, and exposes the correspondingly named external
symbol `___xp_a`, `___xp_z`, `___xt_a`, or `___xt_z` at offset zero.

The XDK 3911 `libcmt.lib` member `obj\\i386\\crt0init.obj` identifies its
source as `..\\crt0init.c`, Microsoft VC7, and type `_PVFV`. Its four runtime
sections, names, bytes, flags, and owners are identical to January. The
section pragmas and one-element callback arrays therefore reproduce the actual
CRT source contract; they are not arbitrary alignment or codegen directives.

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| E01 | Four `_PVFV` null arrays, each in its semantic `.CRT$X*` section | All four strict exact on first build |

For each marker, target and candidate contain 4/4 zero bytes, 0/0
relocations, and normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
Every named section has flags `0xC0300040`; every owner is external, type zero,
storage class `2`, at offset zero. `section_infos_equal` is `true` for all
four markers, and the candidate adds no runtime section beyond these four.

## House/Berth audit

- The callback typedef and arrays name the actual startup mechanism.
- Each section pragma has semantic linker meaning proven by the target and XDK
  member; there is no gratuitous packing or alignment.
- No raw offset, assembly, `volatile`, undefined behavior, forced inline, byte
  forcing, compiler flag change, or comparator exception is used.
- Null sentinel arrays are intentional runtime delimiters, not an original
  game bug.

Admission requires exact bytes, four section identities/flags, all symbol
metadata, full builds, zero semantic errors, a clean ownership regression
check, and all 179 tests.
