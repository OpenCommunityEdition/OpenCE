# `libs/libcmt/txtmode.obj` exact-match ledger

This fresh atomic lane reconstructs a genuinely unstarted, data-only CRT
object on campaign commit `73ce2922`. No historical completed source exists
for this path, so the declaration is recovered from January and primary XDK
3911 object evidence.

## Plain-English behavior

This object owns the CRT's default file translation mode. The value begins at
zero, which is the Xbox CRT's default mode until the program changes it. There
is no executable code in this translation unit.

## Primary provenance and target inventory

The January target owns exactly one four-byte `.bss` section. It defines
external symbol `__fmode` at offset zero, uses four-byte alignment, and has no
relocations or other runtime sections.

The locally archived XDK 3911 `libcmt.lib` has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its member `obj\i386\txtmode.obj` records source filename `..\txtmode.c` and
compiler id `0x001c2426`. That member independently owns external `__fmode` at
offset zero in a four-byte, four-byte-aligned `.bss` section with no
relocations. `tools.coff_compare.section_infos_equal` proves that section is
strict-equal to January, with shared normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

This is primary evidence that the target is ordinary C data rather than
vendored assembly or an anonymous byte surrogate.

## Accepted source and experiment matrix

The direct typed declaration is:

```c
int _fmode = 0;
```

The explicit initializer documents the default value while VC7 emits owned
zero-initialized storage. Since the source-authentic declaration reproduced
every runtime property on the first production build, speculative variants
were neither necessary nor justified.

| ID | Source shape | Runtime measurement | Decision |
|---|---|---|---|
| E01 | `int _fmode = 0;` | `.bss` 4/4 B, 0/0 relocs, identical symbol/flags/hash | retained |

Do not retry aliases, alignment directives, raw byte arrays, assembly,
`volatile`, packing, byte patches, or compiler-option changes. None is
supported by the primary evidence and none is needed.

## Strict production verification

The production object was built with XDK 3911 CL `13.00.9254.1` and the
configured libcmt flags `/O1 /Gy /I"xbox/include"`. A forced clean rebuild
repeated that command and strict result, excluding stale-object credit.

| owned section | target | candidate | relocs | target/candidate SHA-256 |
|---|---:|---:|---:|---|
| `.bss` via `__fmode` | 4 B | 4 B | 0 / 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

`tools.coff_compare.section_infos_equal` returned true. Both objects use
section flags `0xc0300080`, and both define external symbol `__fmode` at value
zero with type zero and storage class 2. The target owns no other runtime
section; the candidate's remaining sections are compiler directives and debug
metadata, not runtime ownership.

The complete `halobetacache_build` and `libcmt_build` succeeded. The regenerated
semantic audit scanned 409 implemented units, reported zero unit errors, and
accepted 3,385 exact functions. Progress reported 300/833 exact objects overall
and 33/212 libcmt objects; libcmt had 2,158/55,015 exact code bytes and
208/8,637 exact data bytes. This data-only unit correctly adds no function
credit. The complete tooling suite passed **179/179** tests.

After the local atomic commit, the fail-closed per-unit regression manifest was
snapshotted and checked successfully with no failures, warnings, ownership
changes, or unexpected matches. `git diff --check` and the final scope audit
were clean.

## House/Berth audit

The unit is ordinary typed C data. Function parameter, return, and typed datum
access rules do not apply because it contains no functions. It uses no
assembly, `volatile`, forced inlining, undefined behavior, raw offset, packed
surrogate, alignment trick, compiler override, or comparator exception. No
original bug is present, so no bug-preservation comment is required.
