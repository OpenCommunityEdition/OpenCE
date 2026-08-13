# `libs/libcmt/_wopen.obj` exact-match ledger

This atomic lane starts from campaign commit `7d664c91`. Before production
edits it checked `config/config.json`, the parked ledger, all refs, worktrees,
existing object logs, and active agent claims. The object was genuinely
`MISSING`, unparked, absent from prior source/history, and unowned.

## Plain-English behavior

`_wopenfile` turns a wide-character `fopen` mode such as `L"rb+"` into the
low-level flags used by `_wsopen`. The first character selects read, write, or
append. Later characters can select binary/text translation, read/write mode,
commit behavior, sequential/random access, short-lived storage, or deletion
on close. Duplicate mutually-exclusive modifiers stop the scan.

If the low-level open succeeds, the routine initializes the caller-provided
`FILE`: it records the descriptor and stream flags, clears the buffer pointers
and counters, and increments the CRT stream-cleanup count. It returns null
when the leading mode is invalid or `_wsopen` fails.

## Primary provenance and strict inventory

The exact member `obj\i386\_wopen.obj` was extracted from XDK 3911
`libcmt.lib` before reconstruction. Its CodeView records name both the
specialization and Microsoft source template:

```text
..\_wopen.c
d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\_open.c
```

The ordinary-C classification is therefore direct provenance, not an
inference from disassembly. The pinned Microsoft XP-era CRT `_open.c` at
Easy-Build-2k commit `36fce23389e43918faf4cd88688c64aa2668ff8f`
independently supplies the readable control-flow topology. January bytes,
types, constants, and ownership remain authoritative where platform variants
could differ.

The object owns one external pick-no-duplicates `.text` COMDAT and no runtime
data:

- symbol: `__wopenfile`;
- size: `367` bytes (`0x16F`);
- relocations: `3`;
- normalized SHA-256:
  `890c594f96b35c8cad75e91f6383d8303d09e82af4d5c5e36a1d0852751b8090`;
- relocation destinations/addends: `__commode + 0` at `+0x14`, `__wsopen + 0`
  at `+0x13F`, and `__cflush + 0` at `+0x156`.

The authentic archive member is independently strict-equal to the January
target for the complete runtime section before its source form is used.

## Experiment record

| ID | One changed source factor | Result | Decision |
|---|---|---|---|
| E01 | Faithful mode parser, but initialize the loop-validity flag at its declaration | `375/367` bytes and `3/3` relocs; the flag spills as a third local, shifting relocations | Rejected |
| E02 | Declare the loop-validity flag without an initializer, then assign `1` immediately before the parse loop, as in Microsoft `_open.c` | Exact `367/367`, `3/3`, normalized hash and relocation identity | Accepted |

E02 explains the eight-byte E01 gap in ordinary source terms. Delaying the
initial assignment shortens the flag's live range, so VC7 holds it in EDI and
uses only two stack locals for the duplicate-option flags. This is a generally
useful lifetime rule, not a register keyword or byte-forcing device.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiles the accepted source under the unchanged
libcmt flags `/O1 /Gy /I"xbox/include"`. The hardened comparator reports
`section_infos_equal`: size, normalized bytes, all three relocation addresses,
types, destinations, and addends match. Independent COFF inspection confirms
the symbol storage class, COMDAT selection, and absence of `.data`, `.rdata`,
or `.bss` ownership.

The fresh isolated lane then passed every required gate:

- the object was deleted and rebuilt, and both `halobetacache_build` and
  `libcmt_build` completed successfully;
- the regenerated report evaluated 3,579 functions in 460 units, accepted
  3,495 exact functions, and reported zero unit errors;
- the admission audit found no new candidate, contradiction, or revocation for
  `libs/libcmt/_wopen`;
- campaign progress became 359/833 complete objects overall and 92/212 libcmt
  objects, with libcmt at 142/476 exact functions and 9,877/55,015 exact code
  bytes;
- all 179 tooling tests passed, configuration JSON parsed, the prohibited-
  construct scan was empty, and `git diff --check` was clean.

These figures come from the forced build in this lane; historical artifacts
grant no credit. After rebasing onto campaign tip `08f86f9b`, a second forced
object build remained exact. The refreshed semantic report evaluated 3,586
functions in 461 units, accepted 3,502 exact functions, and still reported
zero unit errors. The clean-commit regression snapshot/check classified
`__wopenfile` as `still_exact`, with no changed nonexact functions, failures,
or warnings.

## House/Berth and bug audit

The source uses the XDK's `FILE`, `wchar_t`, and named `_O_*`/`_IO*` flags. Its
parameters and calls are vertically formatted, variables have semantic names,
and the control flow mirrors the CRT mode grammar rather than the disassembly.
The two early null returns are justified by the target's distinct failure
epilogue and keep the parser readable.

There is no assembly, `volatile`, forced inlining, undefined behavior, byte
forcing, raw structure offset, compiler pragma, alignment trick, opaque
codegen cast, compiler-flag change, or comparator exception. No independently
actionable January bug was established: behavior for invalid extension text is
part of the historical Microsoft CRT mode parser and is preserved without
speculatively changing it.
