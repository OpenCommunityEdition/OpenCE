# `libs/libcmt/ncommode.obj` exact-match ledger

This atomic lane independently reconstructed and verified the object on a fresh
worktree based on campaign commit `134d4424`. An older completed branch was
screened only to identify the candidate; none of its measurements were trusted
until repeated here.

## Plain-English behavior

This object contains the Microsoft CRT's global file-commit mode. Its initial
value is zero, meaning the optional commit-on-flush behavior starts disabled.
There is no executable code in this translation unit.

## Provenance and target inventory

The January split target owns exactly one four-byte `.bss` section. It defines
external symbol `__commode` at offset zero, with four-byte alignment and no
relocations. It owns no `.text`, `.data`, or `.rdata` runtime section.

The XDK 3911 `libcmt.lib` used by the project contains member
`obj\i386\ncommode.obj`. I extracted that member from the locally archived
library whose SHA-256 is
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its COFF file record names `..\ncommode.c`, its compiler id is `0x001c2426`,
and it independently owns the same external `__commode` symbol in a four-byte,
four-byte-aligned `.bss` section with zero relocations. The archive member is
therefore primary XDK provenance that this is readable C data, not vendored
assembly or an anonymous-byte placeholder.

## Accepted source and experiment record

The single retained declaration is:

```c
int _commode = 0;
```

The explicit zero expresses the CRT default while VC7 places the definition in
owned zero-initialized storage. A synthetic compilation with the campaign's
exact CL and `/O1 /Gy` produced compiler id `0x001c2426`, external symbol
`__commode`, a four-byte `.bss` section, four-byte alignment, and no
relocations. Because that direct source shape reproduced every runtime
property on the first build, no speculative variants were justified.

| ID | Source shape | Runtime result | Decision |
|---|---|---|---|
| E01 | `int _commode = 0;` | `.bss` 4/4 bytes, 0/0 relocs, external symbol at offset 0 | retained |

Do not retry alignment pragmas, raw arrays, aliases, assembly, `volatile`, or
compiler-flag changes: none is supported by the target or XDK provenance, and
none is needed.

## Strict verification

The production object is rebuilt with XDK 3911 CL `13.00.9254.1` using the
configured libcmt flags `/O1 /Gy /I"xbox/include"`. Admission requires all of
the following, not merely a visual percentage:

- target and candidate own the same four-byte uninitialized writable section;
- normalized section bytes and padding are identical;
- relocation count and identity are both empty;
- `__commode` has external storage, offset zero, and belongs to that section;
- the object owns no other runtime data or code;
- full Halo and libcmt builds, semantic/progress reports, the per-unit
  regression snapshot/check, and all 179 tooling tests pass.

The forced clean production rebuild emitted the configured command line
`CL.Exe /nologo /c /O1 /Gy /I"xbox/include"`; this rules out stale-object
credit. The strict owned-section measurement after that rebuild was:

| owned section | target | candidate | relocs | target/candidate SHA-256 |
|---|---:|---:|---:|---|
| `.bss` via `__commode` | 4 B | 4 B | 0 / 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

`tools.coff_compare.section_infos_equal` returned `true`. Both sections have
flags `0xc0300080` (uninitialized writable data, four-byte alignment). On both
sides, `__commode` has value zero, type zero, external storage class 2, and is
owned by that section. The only extra candidate sections are ordinary compiler
directives/debug/build metadata; they own no runtime bytes.

The full `halobetacache_build` and `libcmt_build` completed successfully. The
semantic audit scanned 400 implemented units and reported zero unit errors,
3,292 semantic-exact functions, and 3,376 accepted exact functions. Progress
reported 290/833 objects overall and 23/212 libcmt objects; libcmt now has
160/8,637 exact data bytes. This object deliberately adds no function count
because it owns no code. The complete tooling suite passed **179/179** tests.

After the atomic commit, the fail-closed per-unit regression manifest was
snapshotted and checked successfully; it preserved this object's data and
symbol ownership with no regressions or unexpected changes. `git diff --check`
and the final scope audit were also clean.

## House/Berth audit

This unit is ordinary typed C data. Function formatting and return rules do
not apply because it contains no functions. It uses no assembly, `volatile`,
forced inlining, undefined behavior, packed surrogate, alignment trick,
compiler override, byte patch, or comparator exception. No original bug is
present, so no bug-preservation comment is required.
