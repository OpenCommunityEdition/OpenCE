# `libs/libcmt/towlower.obj` exact-match ledger

This atomic lane starts from campaign commit `1b270ed1`. The object was
`MISSING` at config index 821, absent from `config/parked.json`, every Git ref,
worktree, and existing object log, and explicitly cleared by both active
teammates. The January target contains one function and no owned runtime data.

## Plain-English behavior

`towlower` preserves the wide end-of-file marker. In the ordinary C locale it
converts ASCII `A` through `Z` to lowercase by adding 32 and returns every
other character unchanged. In the Xbox CRT's nominal non-C-locale path it
performs an uppercase classification call for values below 256 but still
returns the original character. That last behavior is unusual, but it is
proven by the January and XDK instructions and is preserved rather than
silently repaired.

## Provenance and target inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\towlower.obj` member has SHA-256
`20d6a4ecf4d7f720a7a6cc0ee568a97a51651ba90aa368370d3d74152ab673fb`,
an August 11 2001 timestamp, compiler identifier `0x001c2426`, ordinary C
CodeView/line records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\towlower.c`.
This is direct Microsoft XDK CRT C provenance, not vendor assembly.

The archive member is already strict-exact to the January runtime function:

- `_towlower`: `0x47/0x47`, two relocations, normalized SHA-256
  `5c19dedec64a0cfd0761a1423bf92ae9776ec6f7dbc9a2bfa00eb3a3731e8e94`;
- `DIR32` at `+0x11` to `___lc_handle + 8`, proving the `LC_CTYPE` slot;
- `REL32` at `+0x3c` to `_iswctype + 0`;
- external, one-byte-aligned, pick-no-duplicates `.text` COMDAT;
- no aliases and no target-owned `.rdata`, `.data`, or `.bss`.

The CodeView line table maps the ASCII fast path and non-C-locale path to
distinct original source regions. XDK headers prove `wint_t` and `wctype_t`
are unsigned 16-bit types, `WEOF` is `0xFFFF`, and the uppercase mask is 1.
The admitted `wcsicmp` and locale-data objects corroborate the C-locale handle
and `LC_CTYPE` array index, but January bytes and the exact XDK member remain
authoritative.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Object result | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Early WEOF return; C-locale nested `if` plus two returns; non-C-locale sub-256 `iswctype` call | `0x47/0x3f` | `2/2` | `516ad5f21d88...` | target keeps separate false-range return at `+0x2c`; candidate merges it with final return | one function non-exact | rejected: source return topology too mergeable |
| E02 | Same, but C-locale ASCII fold is one ternary return | `0x47/0x47` | `2/2` | `5c19dedec64a...` | none | sole function and all ownership exact | accepted: restores January's separate return topology |

Any later experiment must vary one evidence-backed source factor, record all
strict measurements here, and be reverted unless it improves the complete
object result.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the accepted source with libcmt's exact
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal`: `0x47/0x47`, both relocation
addresses/types/destinations/addends exact, and normalized SHA-256
`5c19dedec64a0cfd0761a1423bf92ae9776ec6f7dbc9a2bfa00eb3a3731e8e94`.
Independent COFF inspection confirms external linkage, one-byte alignment,
and pick-no-duplicates COMDAT selection. Candidate `.drectve` and CodeView
sections are discardable compiler metadata rather than runtime ownership. A
clean forced rebuild produces the identical strict result.

The full `halobetacache_build` and `libcmt_build` pass. Regenerated progress
reports 324/833 objects overall, 267/468 Halo objects, and 57/212 libcmt
objects. Libcmt now has 77/476 exact functions and 3,842/55,015 exact code
bytes. This object contributes one object, one function, and 71 bytes relative
to its fixed baseline; concurrent atomic admissions account for the other
aggregate movement. Data is unchanged. The semantic audit scans 427 units,
evaluates 3,514 functions, accepts 3,430 exact functions, and reports zero
unit errors. The object-admission audit reports no candidate or revocation;
its sole contradiction is the unrelated pre-existing
`source/shell/shell_xbox` item. All 179 tooling tests pass.

The post-commit fail-closed per-unit regression snapshot/check passes:
`_towlower` is `still_exact`, with no failures, warnings, changed non-exact
siblings, unexpected new matches, or ownership changes.

## House/Berth and original-bug audit

The candidate is readable typed C with explicit narrow types, named
parameters, ordinary conditions, and no hidden byte control. It uses no
assembly, `volatile`, forced inlining, undefined behavior, type-punning,
opaque code-generation cast, packing/alignment trick, flag change, byte patch,
or comparator waiver.

The ignored classification result is preserved exactly because both January
and the provenance member prove it. This lane does not label it a definite
original bug: the call may intentionally preserve CRT side effects or
validation even though it does not map the character. A corrected locale-aware
implementation would require a proven mapping API and is outside this exact
reconstruction.
