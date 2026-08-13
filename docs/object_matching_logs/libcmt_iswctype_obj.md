# `libs/libcmt/iswctype.obj` exact-match ledger

This atomic lane starts from campaign commit `70e301d1`. The authoritative
config places the unit at index 827 (the assignment's index 826 had drifted),
where it was `MISSING`. It was absent from `config/parked.json`, all Git refs,
worktrees, and object logs, and both active teammates explicitly cleared it.
The January split target exists and contains two functions.

## Plain-English behavior

`iswctype` looks up the classification flags for one wide character and
returns only the flags requested by its mask. It deliberately returns zero
for the wide end-of-file marker and for values outside the CRT's 256-entry
classification table. `is_wctype` is the obsolete public spelling and simply
tail-calls `iswctype`. The object owns no runtime data.

## Provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\iswctype.obj` member has SHA-256
`363595aba1db117ad0f0bb79e16f7ae2467b0a2d36a1ff1dbb18e2a23c47c788`,
an August 11 2001 timestamp, compiler identifier `0x001c2426`, ordinary C
CodeView/line records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\iswctype.c`.
This proves ordinary XDK CRT C provenance rather than vendor assembly.

The archive member is already strict-exact to the January runtime sections:

- `_iswctype`: `0x30/0x30`, one `DIR32` relocation at `+0x19` to
  `__pwctype + 0`, normalized SHA-256
  `2455e8eaebdf9ecda5501f17440c732c4eb884abbaa909229ac280c60497d7d8`;
- `_is_wctype`: `0x5/0x5`, one `REL32` relocation at `+0x01` to
  `_iswctype + 0`, normalized SHA-256
  `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449`;
- both are external, one-byte-aligned, pick-no-duplicates `.text` COMDATs;
- there are no target-owned `.rdata`, `.data`, or `.bss` sections and no
  aliases.

XDK headers independently prove `wint_t` and `wctype_t` are 16-bit unsigned
types, `WEOF` is `0xFFFF`, `_pwctype` points to constant `wctype_t` entries,
and `is_wctype` is the obsolete two-parameter synonym. These headers and
nearby admitted classifiers are typed context only; January bytes and the
strict-exact archive member are authoritative.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Object result | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Narrow typed local; reject `WEOF` or `>= 256`, otherwise table lookup, then apply mask; obsolete wrapper returns `iswctype` | public `0x30/0x30`; wrapper `0x5/0x5` | `1/1`; `1/1` | public `2455e8eaebdf...`; wrapper `e8ebd827d1f3...` | none | `2/2` functions and all ownership exact | accepted on first build |

No alternative spelling is justified unless E01 fails. Every later experiment
must vary one evidence-backed source factor, record exact measurements here,
and be reverted unless it improves the strict whole-object result.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the production source with libcmt's
configured `/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal` for both functions, with the sizes, hashes, relocation
addresses/types/destinations/addends recorded above. Independent COFF
inspection confirms external function storage class, one-byte alignment, and
pick-no-duplicates COMDAT selection. Candidate `.drectve` and CodeView
sections are discardable compiler metadata rather than runtime ownership. A
clean forced rebuild produces the same strict result.

The full `halobetacache_build` and `libcmt_build` pass. Regenerated progress
reports 320/833 objects overall, 267/468 Halo objects, and 53/212 libcmt
objects. Libcmt now has 58/476 exact functions and 3,031/55,015 exact code
bytes. Relative to this lane's fixed baseline, this object contributes one
object, two functions, and 53 code bytes; concurrent atomic admissions explain
the remaining aggregate movement. Data is unchanged. The semantic audit scans
423 units, evaluates 3,495 functions, accepts 3,411 exact functions, and
reports zero unit errors. The object-admission audit has no candidate or
revocation; its sole contradiction is the unrelated pre-existing
`source/shell/shell_xbox` item. All 179 tooling tests pass.

The post-commit fail-closed per-unit regression snapshot/check passes:
`_iswctype` and `_is_wctype` are both `still_exact`, with no failures,
warnings, changed non-exact siblings, unexpected new matches, or ownership
changes.

## House/Berth and bug audit

The candidate is direct readable C with explicit 16-bit types, named
parameters, and normal control flow. It contains no assembly, `volatile`,
forced inlining, undefined behavior, byte forcing, opaque cast used to perturb
code generation, raw structure offset, packing/alignment trick, flag change,
or comparator waiver. Rejecting `WEOF` and out-of-table values is deliberate
CRT behavior, not an original bug, so no bug/fix annotation is needed.
