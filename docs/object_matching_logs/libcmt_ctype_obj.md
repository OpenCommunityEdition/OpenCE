# `libs/libcmt/_ctype.obj` exact-match ledger

This atomic lane starts from campaign commit `b6d5887c`. At lane start the
object was `MISSING` at config index 832, absent from `config/parked.json`, and
had no source, object log, topic branch, or active worktree. The January split
target contains 15 public character-classification functions and no owned
runtime data.

## Plain-English behavior

Eleven routines answer ordinary questions about a character: whether it is a
letter, uppercase, lowercase, a digit, a hexadecimal digit, whitespace,
punctuation, alphanumeric, printable, graphical, or a control character. In a
multibyte locale they delegate to `_isctype`; otherwise they read the relevant
bit from the active character-type table. Two helpers test or strip the high
ASCII bit. The final two accept characters that may begin or continue a C
identifier, including underscore.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its actual `obj\i386\_ctype.obj` member records ordinary C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\_ctype.c`, compiler
identifier `0x001c2426`, and all 15 external functions. This is primary CRT C
provenance, not a vendor-assembly object. Every runtime function in that XDK
member is independently strict-equal to the January target before its source
topology is used.

The strict January inventory is:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_isalpha` | 46 | 3 | `b4bf7e5331055495774f5db7dd0cdf7c2ea01c3cadfc8c1abc6c29b012215ab5` |
| `_isupper` | 41 | 3 | `6bde8909bc09e5dfa4cb4a54f301b3efd60d354ff323045f0cc20be7ed3c5ea8` |
| `_islower` | 41 | 3 | `1de128123ae0466daa67e923bbd030825f7ce75a29a408bcb13f9af131860827` |
| `_isdigit` | 41 | 3 | `0a02e4c983aa5be26dbc2c7503914a1354dc297d9e022e73bb3930a840c444c1` |
| `_isxdigit` | 46 | 3 | `c3baf6e232edb7ede07f2d5cad80056942b8dca8d702f64ec48bb98a7e6fc4bf` |
| `_isspace` | 41 | 3 | `a8dd88487ac77c27685648b88e24a824658e0a713d76c39fbf30cd46b139653c` |
| `_ispunct` | 41 | 3 | `a12411c84592e3c6a2b1985dc696fb37e0be4d00d7f7add9980a22000c83f26a` |
| `_isalnum` | 46 | 3 | `9441c8150444635330fd70aee72c4b602b0de2b3911185f7a791cb59e292d62d` |
| `_isprint` | 46 | 3 | `04a98e460d4064108e65654a60f1830a90e57a58b260ebc96d9cac4a88fed2e9` |
| `_isgraph` | 46 | 3 | `40ffa8cb918192231242382d76caa9c4e3d45230ae674db40316baaadfef90fb` |
| `_iscntrl` | 41 | 3 | `06bffe01d388886322552d4ce18207d8edd3845dc52a92773f35aee9f6b77732` |
| `___isascii` | 13 | 0 | `bd1ea3bc9f169d9ebde3847ca42ec2ae6d360a44525e0d5269ec354b5c8380fb` |
| `___toascii` | 8 | 0 | `6848bbc5b147fdd273c915c33d5a0f2a76cbb6e974091aacc90986ab0f0694e2` |
| `___iscsymf` | 61 | 3 | `3d568ad1479b1b2f1a5c4f6d81eaf01fc5807f0215b0e88a6d8815ecc578443a` |
| `___iscsym` | 61 | 3 | `c1b6920af94e7cbecc24b64df20541c11e6cb85310a18e3d31a4e4a62da01589` |

The eleven classification routines each reference `___mb_cur_max`,
`__isctype`, and `__pctype` at the same relocation addresses as January. The
two identifier helpers have the same three references. The object totals 619
meaningful code bytes and 39 exact relocation records. It owns exactly 15
external, pick-no-duplicates `.text` COMDATs and no runtime `.rdata`, `.data`,
or `.bss`.

## Experiment matrix

| ID | One changed factor | Result | Decision |
|---|---|---|---|
| E01 | Direct readable C using named CRT character-mask constants, the locale/table conditional, unsigned ASCII comparison, and explicit identifier predicates | All 15 functions match their size, normalized bytes, relocation identity, destination, and addend on the first compile | Accepted; no unsupported shape search justified |

The accepted source is deliberately transparent: it spells the CRT decisions
and named masks rather than importing bytes, assembly, or opaque generated
tables. There were no rejected production-source variants.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiles this unit with the unchanged libcmt flags
`/O1 /Gy /I"xbox/include"`. The hardened comparator reports
`section_infos_equal` for all 15 functions. Independent COFF inspection
confirms the external symbol storage classes, COMDAT selection, alignment,
undefined references, and absence of owned data. The candidate's extra
`.drectve` and CodeView sections are discardable compiler metadata, not
runtime ownership.

The final forced rebuild remained strict-exact. The full Halo and libcmt builds
passed, including a clean 525-object rebuild in this isolated lane. Progress
credited this object and reported 320/833 objects overall and 53/212 libcmt
objects; libcmt reached 71/476 exact functions and 3,597/55,015 exact code
bytes. The semantic audit scanned 423 units, evaluated 3,508 functions,
accepted 3,424 exact functions, and reported zero unit errors.

The clean-commit fail-closed regression snapshot/check reports all 15
functions as `still_exact`, with no failures or warnings. All 179 tooling tests
pass. `git diff --check` and the final worktree status are clean.

## House/Berth and bug audit

The source uses readable C, named constants, explicit types, named parameters,
multiline declarations, and no duplicated private ABI declarations. It
contains no assembly, `volatile`, forced inlining, undefined behavior, byte
forcing, opaque code-generation cast, raw field offset, alignment trick,
compiler-flag exception, or comparator waiver. These standard CRT predicates
have no identified January behavioral defect, so no original-bug annotation
is required.
