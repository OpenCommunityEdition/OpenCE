# `libs/libcmt/nlsdata2.obj` exact-match ledger

This atomic lane reconstructs a previously unstarted CRT locale-data object on
a fresh worktree based on campaign commit `8df3ea9e`. The source types and
layout are recovered from January plus the matching XDK 3911 archive member,
not inferred from symbol names alone.

## Plain-English behavior

This object owns the CRT's process-wide locale state. It starts in the simple
"C" locale, represented by `__lc_clike == 1`. Six Windows locale handles, the
active code page, and the collation code page all begin at zero until locale
initialization fills them. The unit contains no executable code.

## Primary provenance and target inventory

The January target owns:

- a four-byte `.data` section containing integer value 1, externally owned by
  `___lc_clike` at offset zero;
- a 32-byte `.bss` section, four-byte aligned and relocation-free, with
  `___lc_handle` at `+0x00`, `___lc_codepage` at `+0x18`, and
  `___lc_collate_cp` at `+0x1c`;
- no `.text` or target-owned `.rdata`.

The locally archived XDK 3911 `libcmt.lib` has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\nlsdata2.obj` member records source filename `..\nlsdata2.c`,
compiler id `0x001c2426`, and the same runtime sections, values, symbol offsets,
linkage, alignment, and relocation counts as January.

The member's CodeView records prove the source types: `__lc_clike` is signed
32-bit `int` (primitive type `0x74`), `__lc_codepage` and `__lc_collate_cp` are
unsigned 32-bit `int` (`0x75`), and `__lc_handle` is an array record whose
element type is unsigned `long` (`0x22`) and whose total byte size is `0x18`.
Thus the typed six-element declaration is evidence-backed rather than a raw
32-byte placeholder.

The XDK member also emits unrelated select-any XDK header tables in `.rdata`.
Those COMDAT definitions are header side effects, not symbols owned by the
January `nlsdata2` target after link splitting. They must not be copied into
this source or credited as part of this object.

## Source-shape experiment

The direct typed definitions are the only evidence-backed family:

```c
int __lc_clike = 1;
unsigned long __lc_handle[6] = { 0 };
unsigned int __lc_codepage = 0;
unsigned int __lc_collate_cp = 0;
```

| ID | Source shape | Runtime measurement | Decision |
|---|---|---|---|
| E01 | Typed definitions in target symbol order | `.data` `4/4`, `.bss` `32/32`; zero relocations; all four aliases at the target offsets | accepted: both owned sections strict-exact |

No packing, anonymous byte array, alignment directive, assembly, `volatile`,
undefined behavior, byte patch, compiler override, or comparator exception is
permitted or needed.

## Strict production verification

The object was rebuilt, then force-cleaned and rebuilt again, with XDK 3911 CL
`13.00.9254.1` and the configured libcmt flags
`/O1 /Gy /I"xbox/include"`. Both builds produced the same strict result:

- `.data`: `4/4` bytes, `0/0` relocations, SHA-256
  `67abdd721024f0ff4e0b3f4c2fc13bc5bad42d0b7851d456d88d203d15aaa450`;
- `.bss`: `32/32` bytes, `0/0` relocations, SHA-256
  `66687aadf862bd776c8fc18b8e9f8e20089714856ee233b3902a591d0d5f2925`;
- `___lc_clike`: external, `.data+0x00`;
- `___lc_handle`: external, `.bss+0x00`;
- `___lc_codepage`: external, `.bss+0x18`;
- `___lc_collate_cp`: external, `.bss+0x1c`.

`section_infos_equal` passes when the two sections are addressed through all
four symbols, so the result proves bytes, section sizes, relocation absence,
aliases, addends, and ownership. The candidate's `.drectve` and `.debug$S`
sections are normal compiler metadata and contain no January-owned runtime
state.

The complete `halobetacache_build` and `libcmt_build` both pass. `progress`
reports 304/833 objects overall, 267/468 Halo objects, and 37/212 libcmt
objects; this data-only admission raises matched libcmt data to `260/8637`
bytes and correctly adds no function credit. The semantic audit scans 410
units, accepts 3,386 exact functions, and reports zero unit errors. All 179
tooling tests pass. The post-commit regression snapshot/check for
`libs/libcmt/nlsdata2` passes with no failures or warnings; its function lists
are correctly empty because this object contains data only.

## House/Berth audit

All runtime storage has recovered scalar or array types; no raw offsets or
anonymous padding substitutes are used. Function-formatting rules do not apply
because the unit has no functions. There is no original behavioral bug and no
bug-preservation comment is needed.
