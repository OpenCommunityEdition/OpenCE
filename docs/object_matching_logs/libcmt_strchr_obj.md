# `libs/libcmt/strchr.obj` provenance and park ledger

This lane began from campaign commit `037de80f`. Before touching production
source or configuration it checked `config/config.json`, `config/parked.json`,
all Git refs, worktrees, existing object logs, and active agent ownership.
`libs/libcmt/strchr.c` was `MISSING`, was not already parked, and had no prior
implementation or active owner. The authentic XDK 3911 archive member was then
examined before any C reconstruction was attempted.

## Plain-English behavior

The public `strchr` entry searches a byte string for the first occurrence of a
requested byte and returns a pointer to it, or null if the byte does not occur
before the string terminator. The implementation aligns the input, then checks
four bytes at a time with bit tricks that detect either the requested byte or a
zero byte.

The same machine-code body also exposes the internal
`___from_strstr_to_strchr` entry at offset `+0x16`. That entry is reached from
the XDK's hand-written `strstr` implementation with part of its state already
in registers. Several return paths jump backward into a shared epilogue before
the public entry, so this is intentionally one assembly layout rather than two
ordinary C functions.

## Authoritative provenance

The member `obj\i386\strchr.obj` was extracted directly from the campaign's
XDK 3911 `libcmt.lib`. Its CodeView `.file` record is:

```text
..\i386\strchr.asm
```

This is direct Microsoft object provenance. It is not an inference based on
the disassembly and it rules out an authentic ordinary-C reconstruction.

## January target and archive inventory

Both the January split target and the XDK archive member own one runtime
`.text` section:

- size: `0xCC` bytes;
- relocations: `0`;
- public `_strchr` symbol at `+0x10`;
- internal `___from_strstr_to_strchr` symbol at `+0x16`;
- no runtime `.data`, `.rdata`, or `.bss`;
- no owned runtime data and therefore no data-relocation destinations or
  addends to reconcile.

The archive additionally contains `.debug$S` and `.debug$F`, which are debug
metadata and are not present in the linked-image split. The runtime section is
strict-equal despite that expected physical-object packaging difference.

`tools/coff_compare.py` reports `all_equal: true` for both exposed symbols.
The target and archive each normalize to:

```text
a4d43aa01f5b3db8839024433e2579f873e3cd2a234dbd89ac151292935a770c
```

The comparator measures the complete `0xCC` runtime section for both aliases,
including the shared pre-entry epilogue and all internal control-flow edges.

## Experiment record

| ID | Evidence or source family | Measured result |
|---|---|---|
| P01 | Configuration, parked ledger, refs, logs, worktrees, and peer claims | Genuinely missing, unparked, and unowned at lane start |
| P02 | Authentic `libcmt.lib` member and CodeView file record | Proven Microsoft `strchr.asm` vendor assembly |
| P03 | Strict January-target versus archive-member comparison | Exact `0xCC/0xCC`, `0/0` relocations, normalized hashes equal |

No C spelling experiments were run. Once P02 and P03 established that the
January bytes are the authentic hand-written assembly member, source-shape
experiments would not be decompilation; they would be attempts to imitate
vendor assembly through compiler accidents.

The complete repository tooling suite passed `179/179` tests, and
`git diff --check` is clean. Full game/libcmt builds, progress/admission credit,
and the per-unit regression gate are intentionally not represented as
acceptance evidence: production source and configuration were never changed,
and this docs-only result grants no exact-match credit.

## House/Berth and bug audit

The lane preserves the project's readable-C rule by declining to add inline
assembly, `volatile`, forced inlining, undefined behavior, byte arrays, naked
functions, optimization or alignment directives, comparator exceptions, or
codegen-only casts. It also does not relabel the object as Matching: exact
archive provenance demonstrates what the object is, but no acceptable source
implementation owns those bytes.

No independent January bug was identified. The unusual shared epilogue and
private register-entry label are deliberate properties of the Microsoft
assembly implementation, not bugs to “fix” in a hypothetical C translation.

## Decision and reopen criterion

Park this object as **proven vendor assembly**. Production source and all
matching configuration remain untouched.

Reopen only if project policy explicitly admits authenticated vendor assembly
or an exact original-source artifact under a separately reviewed provenance
path. Ordinary readable C is not a credible route to this object while the
current house rules remain in force.
