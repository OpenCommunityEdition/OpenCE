# `libs/libcmt/dtoxtime.obj` exact-match park ledger

This isolated atomic lane starts from campaign commit `fa9a93c1`. At lane
start `dtoxtime.obj` was `MISSING` at config index 664, absent from
`config/parked.json`, and had no production source, object log, topic branch,
worktree, or active peer claim. It remains `MISSING`: production source and
configuration were restored when the only surviving difference failed the
project's strict symbolic-relocation rule.

## Plain-English behavior

`__loctotime_t` converts a broken-down local calendar date into the CRT's
32-bit seconds-since-1970 representation. It validates the year, counts days
with Gregorian leap-year rules, adds hours/minutes/seconds and the current
time-zone offset, then applies the daylight-saving bias when explicitly
requested or when the CRT's daylight test says the supplied local date is in
summer time.

## Primary provenance and January inventory

The authentic XDK 3911 `libcmt.lib` member is `obj\i386\dtoxtime.obj`, SHA-256
`4b294148ccb748b368def24ac14df3b837ffd8e430ba7bea54093f198e8681df`.
Its CodeView identifies ordinary Microsoft C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\time\dtoxtime.c`, compiler ID
`0x001c2426`, and the public function `___loctotime_t`. It is not handwritten
assembly and contains no exception machinery.

January and the authentic archive member have the same 270 instruction bytes,
normalized SHA-256
`073f1aea54d755af3f0894565ce5f0133c31bcce6ac73414c006964ed3c34370`,
and six relocations at `+0x35`, `+0x65`, `+0xc4`, `+0xe7`, `+0xf3`, and
`+0xfe`. The object owns one external, one-byte-aligned, no-duplicates `.text`
COMDAT and no runtime `.rdata`, `.data`, `.bss`, or aliases.

## Measured source-shape experiment

| ID | One changed factor | Size T/B | Relocs T/B | Normalized bytes | Result |
|---|---|---:|---:|---|---|
| E01 | Readable typed C following the authenticated Microsoft algorithm: named time constants, `_days[month - 1]`, Gregorian leap test, `_tzset`, `struct tm`, `_isindst`, and the three CRT time-zone globals | `270/270` | `6/6` | SHA identical; every instruction byte and relocation address/type identical | Rejected solely because the first relocation is spelled differently by csplit |

The authentic archive member and E01 both encode the first relocation as
`__days - 4`. Csplit encodes the same linked image address as
`__lpdays + 48`. The evidence is exact:

- `__lpdays` is at executable file offset `3250404`; `+48` resolves to
  `3250452`;
- `__days` is at executable file offset `3250456`; `-4` also resolves to
  `3250452`;
- the January executable relocation manifest independently records destination
  file offset `3250452`;
- the XDK member proves that Microsoft's source/compiler chose `__days - 4`.

Despite identical final addresses, the hardened comparator deliberately
requires symbolic owner/addend identity and therefore rejects
`__lpdays+48` versus `__days-4`. No exception, allowlist, comparator change,
or semantic-credit entry was added.

## Why production was restored

Forcing the target spelling from C would require addressing before the start
of `_days`, which is undefined behavior and violates the house rules. Renaming
or co-owning the tables would regress the already exact `days.obj` and lie
about source ownership. Editing the executable relocation manifest, weakening
the comparator, or adding a one-off semantic exception would also lower the
campaign's standard. Accordingly, this atomic lane retains documentation only;
`libs/libcmt/dtoxtime.c` is absent and config index 664 remains `MISSING`.

## Reopen criterion

Reopen only when the splitting/tooling pipeline can preserve the authentic
archive member's symbolic relocation as `__days - 4` while retaining the
January destination, or when independent primary source evidence reveals a
defined, readable, non-UB declaration topology that makes VC7 emit
`__lpdays + 48`. Any solution must pass direct `section_infos_equal` with all
six symbolic destinations/addends and must not change `days.obj` ownership.

## House, Berth, and bug audit

The rejected E01 source was ordinary readable C, used explicit types and
named constants, and reproduced the authentic Microsoft machine code without
assembly, `volatile`, forced inlining, byte forcing, optimizer pragmas, flag
changes, or opaque casts. No original runtime bug was proved. The tempting
out-of-bounds table expression was specifically refused as undefined behavior.
This ledger preserves the solved algorithm and the precise blocker so a future
lane does not repeat source-shape work that cannot affect symbolic relocation
ownership.

The final docs-only tree passes `git diff --check`, all configuration JSON
remains parseable and unchanged, and the complete tooling suite passes
179/179 tests. Because production source and configuration were restored, no
rebuild, progress credit, semantic admission, or regression-baseline change is
claimed by this documentation commit.
