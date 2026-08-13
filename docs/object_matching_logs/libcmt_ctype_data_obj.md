# `libs/libcmt/ctype.obj` exact-match ledger

## Plain-English scope

This object owns the CRT's byte and wide-character classification tables. The
tables say whether each value is a letter, digit, hexadecimal digit, space,
punctuation mark, control character, or blank. Two tiny functions return the
active byte-table and wide-table pointers.

The object is atomic: both functions, both initialized tables, both pointer
variables, every relocation, and every public owner must match before it is
credited.

## Primary provenance

- Campaign base: `714c2ef5`.
- Compiler: XDK 3911 Microsoft CL `13.00.9254.1`.
- Flags: unchanged libcmt `/O1 /Gy /I"xbox/include"`.
- XDK archive: `xbox/lib/libcmt.lib`, SHA-256
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- Member: `obj/i386/ctype.obj`, recorded as ordinary Microsoft C from
  `d:/xbox-finalfre/private/sdktools/crt/crtw32/misc/ctype.c`, compiler id
  `0x001c2426`.
- Historical Microsoft source mirror: `xerohour/xbox_leak_may_2020`, commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, blob
  `fc3790441ca07158bbc0a0d3247592dad894bf36`.

The archive runtime sections are independently strict-equal to the January
object. The historical source is therefore topology and declaration evidence;
January remains the admission authority.

## Strict inventory

| Runtime owner | Size | Relocs | SHA-256 / offset |
|---|---:|---:|---|
| `___pwctype_func` | 6 | 1 | `76811197299fd7215ff45276752d25eaa8889353ee70ada1fd839c8a55d34ffc` |
| `___pctype_func` | 6 | 1 | `76811197299fd7215ff45276752d25eaa8889353ee70ada1fd839c8a55d34ffc` |
| `.rdata` | 1,036 | 0 | `cfac159c56e0da35bd240d9cfe980b766220e6c1f8cd008d62b3e79fec382e58` |
| `.data` | 8 | 2 | `41d805e613efbe1acb36eaa0127a35da4a1faa1ac97985d078e50d1fd96055fd` |

The `.rdata` owners are `__ctype` at `+0x000` and `__wctype` at `+0x208`.
The writable pointer owners are `__pctype` at `+0` and `__pwctype` at `+4`.
The two DIR32 relocations point to `__ctype+2` and `__wctype+2`, respectively.
The two functions each have one DIR32 relocation to their corresponding
pointer variable. Target and candidate section flags are also exact:
`.rdata=0x40400040`, `.data=0xC0300040`.

## Experiment matrix

| ID | One changed factor | Result | Decision |
|---|---|---|---|
| E00 | Compile the authenticated source verbatim | Private `cruntime.h` is not shipped in the public XDK include set | Mechanical include adaptation required |
| E01 | Remove only unused private `cruntime.h` and `windows.h`; retain public `ctype.h`/`wchar.h`; define the Xbox-only `C1_ALPHA` value locally | Both functions and all runtime data, relocations, flags, owners, and offsets strict-exact | Accepted |

No source-shape search, table generation, or copied object bytes were used.
The checked-in initializers are Microsoft's readable named character classes.

## Verification

`tools.coff_compare.section_infos_equal` passes both functions. Independent
COFF inspection confirms exact runtime-section payloads, flags, relocations,
external symbol storage classes, values, and ownership. Candidate-only
`.drectve`, `.debug$S`, and `.debug$F` are compiler metadata and own no runtime
state.

The candidate was force rebuilt with the required XDK command. The full
559-step Halo and libcmt build passed. Progress credited the object and
reported 354/833 objects overall, 3,481/11,060 exact functions, and
415,149/2,198,102 exact code bytes; libcmt reached 87/212 objects,
133/476 functions, 9,143/55,015 code bytes, and 2,772/8,637 data bytes.
The semantic report scanned 456 units, evaluated 3,570 functions, accepted
3,486 exact functions, and reported zero unit errors. The fail-closed
regression gate and complete 179-test tooling suite were also run against the
clean admitting commit.

## House/Berth and bug audit

The production file is readable C using the CRT's public named masks and
explicit `unsigned short` table type. It contains no assembly, `volatile`,
forced inline, undefined behavior, byte forcing, opaque cast, raw offset,
alignment trick, flag change, or comparator exception. The two accessors use
one return each. The historical copyright and table comments are retained as
provenance and make the data auditable. No behavioral defect was identified;
these are static classification constants and direct pointer accessors.
