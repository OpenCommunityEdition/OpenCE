# `libs/libcmt/syserr.obj` exact-match ledger

This atomic lane starts from campaign commit `e5ff86a3`. The object was
`MISSING` at config index 763, absent from `config/parked.json`, and had no
production source. January owns no executable code here: the object is the
CRT's public error-message pointer table, its upper-bound value, and the
string literals referenced by that table.

## Plain-English behavior

`_sys_errlist` maps the standard CRT error numbers to their English messages.
Several unsupported error numbers deliberately share the single
`"Unknown error"` literal. `_sys_nerr` contains 43, the final fallback index;
`strerror.obj` uses it both as the range boundary and as the fallback entry.

## Primary provenance and target inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\syserr.obj` member has SHA-256
`883a9f4aecf55b5419541ba9f23857fa9115e0a752fb398913e9ff0cf382b374`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\misc\syserr.c`. The public
historical Microsoft Xbox CRT source at the same path supplies the message
spellings and table order. January bytes and ownership remain authoritative.

Strict January runtime inventory:

- one writable `.data` section, 180 bytes, with 44 DIR32 relocations and
  normalized SHA-256
  `8a59cc708ce056f9aeb11128812b50f5b38f24bb7bd4fe1e2f5e0c39839e3313`;
- external `__sys_errlist` at data offset `0x00` and external `__sys_nerr` at
  offset `0xb0`, with the latter containing the integer 43;
- 40 unique string COMDATs totaling 824 bytes (44 table entries reuse the
  `"Unknown error"` COMDAT four additional times);
- no `.text`, BSS, or other runtime-owned section.

The target and rebuilt object expose the same 42 external non-code symbols.
All symbol values, types, and storage classes agree. All 41 distinct runtime
sections pass `section_infos_equal`, and the 44 relocations agree in address,
type, destination symbol, and addend. The extracted XDK member independently
reproduces every one of those payloads and owners.

## Experiment matrix

| ID | Source factor | Data T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic Microsoft table, spellings, shared fallback literal, and `sizeof`-derived final index | `1004/1004` | `44/44` | all table bytes, strings, relocations, symbols, offsets, and ownership strict exact on the first build | accepted |

No code-shaping experiment was needed. The compiler naturally coalesces the
five occurrences of `"Unknown error"`, which is why 44 pointers own only 40
unique string sections.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt
`/O1 /Gy /I"xbox/include"` flags. The hardened target-authoritative census
checks every external owner, each owned section, and all relocations; nothing
is accepted through fuzzy percentage, symbol masking, or a semantic-data
exception.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
Progress reports 332/833 objects overall and 65/212 libcmt objects. Libcmt has
88/476 exact functions, 4,512/55,015 exact code bytes, and 1,368/8,637 exact
data bytes. This data-only object contributes one completed object and 1,004
exact data bytes. The semantic scan covers 434 units and 3,525 functions,
accepts 3,441 exact functions, and reports zero unit errors. The admission
audit reports no candidates or revocations; its only contradiction is the
pre-existing unrelated `shell_xbox` accounting item.

The clean-commit regression snapshot/check was followed by deleting the
generated object and forcing a rebuild. It retained every data fingerprint
and owner with no failures or warnings. The complete tool suite passes
179/179 tests.

## House/Berth and original-bug audit

The source is the readable, naturally typed Microsoft table: no assembly,
`volatile`, forced inlining, undefined behavior, type-punning, opaque padding,
alignment trick, magic address, build-flag change, byte patch, or comparator
waiver. `sizeof` derives the final index rather than hard-coding 43. Formatting
and public linkage follow the surrounding CRT source and Berth conventions.

No shipped correctness defect was found in this object. Writable `char *`
entries are the historical CRT ABI even though modern source would usually
spell the string targets `const`; changing that public type is outside this
exact-match reconstruction.
