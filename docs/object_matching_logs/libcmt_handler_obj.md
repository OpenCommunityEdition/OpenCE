# `libs/libcmt/handler.obj` exact-match ledger

This atomic lane starts from campaign commit `b831acac`. Before editing it
checked the project configuration, parked ledger, all Git refs, worktrees,
existing logs, and active object ownership. The unit was genuinely `MISSING`,
unparked, absent from all source/history searches, and unowned by another lane.

## Plain-English behavior

This object owns the C++ allocation-failure callback used by the CRT.
`_set_new_handler` takes the heap lock, replaces the callback, releases the
lock, and returns the old callback. `_query_new_handler` returns the currently
registered callback. `_callnewh` invokes it with the failed allocation size and
returns one when allocation should be tried again, or zero when no handler is
installed or the handler declines a retry.

The four-byte `_pnhHeap` variable stores the callback. It starts as null in
`.bss` and is deliberately shared by all three routines.

## Primary provenance and January inventory

The member `obj\i386\handler.obj` was extracted directly from the campaign's
XDK 3911 `libcmt.lib`. Its CodeView records identify ordinary Microsoft C++
source:

```text
..\handler.cpp
d:\xbox-finalfre\private\sdktools\crt\crtw32\heap\handler.cpp
```

Before source reconstruction, every runtime function and the owned `.bss`
symbol in that authentic archive member compared strict-equal to the January
split target. Thus the archive is exact source-provenance evidence, not merely
a related CRT version.

| Symbol | Kind | Bytes | Relocs | Normalized SHA-256 |
|---|---|---:|---:|---|
| `?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z` | code | 36 | 4 | `1bd9620e21577b86098fe0522b9694109f77e8a70b373f5594178aef7a07adb3` |
| `?_query_new_handler@@YAP6AHI@ZXZ` | code | 6 | 1 | `76811197299fd7215ff45276752d25eaa8889353ee70ada1fd839c8a55d34ffc` |
| `__callnewh` | code | 27 | 1 | `27149b6cf20a5ba1c8cf7723061c1780ae4f9a6484dc284f7799740a6ab10e80` |
| `?_pnhHeap@@3P6AHI@ZA` | `.bss` data | 4 | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

The setter's four relocations are `_lock` at `+0x04`, `_pnhHeap` at `+0x0E`
and `+0x15`, and `_unlock` at `+0x1A`. The query and call helper each have one
`_pnhHeap` relocation at `+0x01`. Addresses, types, destinations, and addends
match independently. The unit owns no `.data` or `.rdata`.

## Experiment record

| ID | Readable source shape | Measured result |
|---|---|---|
| E01 | Typed `_PNH` global, lock-protected setter, direct query, and a local callback followed by the ordinary null-or-false predicate | All three functions and the `.bss` owner matched on the first compile |

No alternate spellings were tried because E01 already reproduced every target
property. The callback local in `_callnewh` states the semantic requirement
that one callback value is tested and then invoked; it is not a register hint.

## Strict verification

The `.c` repository filename is compiled as C++ with the same explicit `/TP`
object option already used by the adjacent `new.obj` and `delete.obj`. XDK
3911 CL `13.00.9254.1` uses the unchanged libcmt flags `/O1 /Gy /TP` and the
normal XDK include path.

The hardened comparator reports `section_infos_equal` for all three functions
and the four-byte `.bss` owner. Independent COFF inspection confirms three
external pick-no-duplicates `.text` COMDATs, the external C++-decorated BSS
owner, correct storage classes, and no extra runtime ownership. Compiler
`.drectve` and CodeView sections are discardable metadata, not linked runtime
data.

The final gate record below is filled only from commands run in this isolated
lane; historical objects are not accepted as proof.

- A forced rebuild of `handler.obj` remained exact, and both
  `halobetacache_build` and `libcmt_build` completed successfully as part of a
  clean 560-target build/report run.
- Progress credits 356/833 objects overall and 89/212 libcmt objects. Libcmt
  reaches 136/476 exact functions, 9,212/55,015 exact code bytes, and
  3,476/8,637 exact data bytes.
- The semantic audit scanned 457 units and 3,573 functions, accepted 3,489
  exact functions, and reported zero unit errors.
- The admission audit reports no candidate or revocation. Its one contradicted
  `source/shell/shell_xbox` item is the known pre-existing shell issue and does
  not involve this unit.
- The clean-commit regression snapshot/check and complete tooling-test result
  are recorded after the object commit, as required by the fail-closed gate.

## House/Berth and bug audit

The implementation uses the public `_PNH` type from `<new.h>`, named lock
constant, explicit linkage at the C/C++ boundary, vertically formatted
parameters, and explicit returns. It directly states the allocation-handler
behavior in readable C++.

There is no assembly, `volatile`, forced inlining, undefined behavior, byte
forcing, raw structure offset, compiler pragma, alignment trick, opaque
codegen cast, comparator exception, or compiler-flag change. Multiple returns
in `_callnewh` reflect the target's two distinct success/failure epilogues and
make the behavior clearer than a forced accumulator. No original January bug
was found, so no bug-preservation comment is required.
