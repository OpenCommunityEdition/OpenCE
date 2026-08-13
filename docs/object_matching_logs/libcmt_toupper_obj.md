# `libs/libcmt/toupper.obj` exact-match ledger

This atomic lane starts from campaign commit `4a925e0a`. At lane start the
object was `MISSING` at config index 822, absent from `config/parked.json`,
absent from every Git ref and worktree, and explicitly cleared by the other
active agents. The January split target exists and contains two functions.

## Plain-English behavior

The small `_toupper` helper converts one known lowercase character to its
uppercase ASCII counterpart by subtracting 32. The public `toupper` routine
first asks whether the input is lowercase. For a multibyte locale it calls
the CRT classification routine; otherwise it reads the lowercase flag from
the current character-type table. It changes the character only when that
flag is present. The object owns no runtime data.

## Provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\toupper.obj` member has SHA-256
`967f0ba128fa979b158b8e070cb99458694e568015dc7d55e6f9f9364773ba4b`,
an August 11 2001 timestamp, compiler identifier `0x001c2426`, ordinary C
CodeView/debug sections, and the two external C functions `__toupper` and
`_toupper`. It is direct XDK CRT C provenance rather than a vendor-assembly
unit.

The archive member is already strict-exact to the January runtime sections:

- `__toupper`: `0x8/0x8`, zero relocations, normalized SHA-256
  `25c795932cd06738a4dd900ee5b977650a62f2ff8b951419a292e2c6dfa1f4fb`;
- `_toupper`: `0x31/0x31`, three relocations, normalized SHA-256
  `29caaad8208ac2eebd3e186e988cd7247c0ac1091bb0cade8153b8de09c0f394`;
- `_toupper` relocates at `+0x02` to `___mb_cur_max`, at `+0x12` to
  `__isctype`, and at `+0x1b` to `__pctype`, all with addend zero;
- both are external, one-byte-aligned, pick-no-duplicates `.text` COMDATs;
- there are no target-owned `.rdata`, `.data`, or `.bss` sections and no
  aliases.

The adjacent admitted `tolower.obj` is the trustworthy source-topology donor:
its two routines have the same branch and classification structure with the
opposite case flag and opposite 32-character adjustment. January and the XDK
member remain authoritative for the uppercase constants and exact bytes.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Object result | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Readable mirror of admitted `tolower.c`, using lowercase flag 2 and `'A' - 'a'` | helper `0x8/0x8`; public `0x31/0x31` | `0/0`; `3/3` | helper `25c795932cd0...`; public `29caaad8208a...` | none | `2/2` functions and all ownership exact | accepted on first build |

No alternate spellings are justified unless E01 fails. The primary archive
member and January target prove the code graph, while the exact sibling proves
the readable project topology. Any further experiment will change one source
factor, be measured here, and be reverted unless it improves the strict whole-
object result.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the production source with the configured
libcmt flags `/O1 /Gy /I"xbox/include"`. The hardened comparator reports
`section_infos_equal` for both routines. Their sizes, normalized hashes, and
relocation details are exactly the values recorded above. Independent COFF
inspection also confirms external function storage class, one-byte alignment,
pick-no-duplicates COMDAT selection, and the same three undefined external
references. The extra candidate `.drectve` and CodeView sections are
discardable compiler metadata, not linked runtime ownership. A clean forced
rebuild produced the same strict result.

The full `halobetacache_build` and `libcmt_build` pass. Regenerated progress
reports 314/833 objects overall, 267/468 Halo objects, and 47/212 libcmt
objects. Libcmt now has 50/476 exact functions and 2,734/55,015 exact code
bytes. Relative to this lane's fixed baseline, `toupper.obj` contributes one
object, two functions, and 57 code bytes; data is unchanged. The semantic
audit scans 417 units, evaluates 3,487 functions, accepts 3,403 exact
functions, and reports zero unit errors. The object-admission audit finds no
uncredited candidate and no revocation; its one contradiction is the unrelated
pre-existing `source/shell/shell_xbox` entry. All 179 tooling tests pass.

The post-commit fail-closed per-unit regression snapshot/check passes:
`__toupper` and `_toupper` are both `still_exact`, with no failures, warnings,
changed non-exact siblings, unexpected new matches, or ownership changes.

## House/Berth and bug audit

The candidate is direct readable C with explicit types and named parameters.
It contains no assembly, `volatile`, forced inlining, undefined behavior,
byte forcing, opaque code-generation cast, raw field offset, alignment trick,
compiler-flag exception, or comparator waiver. The routine implements normal
CRT case conversion; no January behavioral defect is present, so no original-
bug annotation is needed.
