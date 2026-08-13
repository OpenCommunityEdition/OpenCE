# `libs/libcmt/towupper.obj` exact-match ledger

This atomic lane starts from campaign commit `6f29c473`. The object was
`MISSING` at config index 820, absent from `config/parked.json`, every Git ref,
worktree, and existing object log. The January target contains one function
and no owned runtime data.

## Plain-English behavior

`towupper` preserves the wide end-of-file marker. In the ordinary C locale it
converts ASCII `a` through `z` to uppercase by subtracting 32 and returns every
other character unchanged. In the Xbox CRT's nominal non-C-locale path it
performs a lowercase-character classification call for values below 256 but
still returns the original character.

## Provenance and target inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\towupper.obj` member has SHA-256
`36b1d99ed21417a7bc8f417a2cb96abce22024d16220a96b5526f755d8f0815b`,
compiler identifier `0x001c2426`, ordinary C CodeView/line records, and source
path `d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\towupper.c`.
This is direct Microsoft XDK CRT C provenance, not vendor assembly.

The archive member is itself strict-exact to the January runtime function:

- `_towupper`: `0x47/0x47`, two relocations, normalized SHA-256
  `00ac224ed8e7c868ef9200556ebed5929194870b6050fa7ba34a75a3cebc0216`;
- `DIR32` at `+0x11` to `___lc_handle + 8`, proving the `LC_CTYPE` slot;
- `REL32` at `+0x3c` to `_iswctype + 0`;
- external, one-byte-aligned, pick-no-duplicates `.text` COMDAT;
- no aliases and no target-owned `.rdata`, `.data`, or `.bss`.

The admitted `towlower.obj` is the exact inverse-shape donor. Its proven
16-bit `wint_t`/`wctype_t`, locale-index, WEOF, and ternary-return topology
were mirrored while changing only the case range, arithmetic direction, and
classification mask that the January `towupper` bytes prove.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Object result | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | January/XDK donor topology: early WEOF return, C-locale ternary ASCII fold, then sub-256 `iswctype(character, 2)` | `0x47/0x47` | `2/2` | `00ac224ed8e7...` | none | sole function and all ownership exact | accepted on first compile |

Any later experiment must vary one evidence-backed source factor, record all
strict measurements here, and be reverted unless it improves the complete
object result.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the accepted source with libcmt's exact
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal`: `0x47/0x47`, both relocation
addresses/types/destinations/addends exact, and normalized SHA-256
`00ac224ed8e7c868ef9200556ebed5929194870b6050fa7ba34a75a3cebc0216`.
Independent COFF inspection confirms external linkage, one-byte alignment,
and pick-no-duplicates COMDAT selection. Candidate `.drectve` and CodeView
sections are discardable compiler metadata rather than runtime ownership.

The full `halobetacache_build` and `libcmt_build` pass. Regenerated progress
reports 328/833 objects overall, 267/468 Halo objects, and 61/212 libcmt
objects. Libcmt has 82/476 exact functions and 4,170/55,015 exact code bytes.
This object contributes one object, one function, and 71 bytes from its fixed
baseline. Data is unchanged. The semantic audit scans 431 units, evaluates
3,519 functions, accepts 3,435 exact functions, and reports zero unit errors.

The clean-commit fail-closed per-unit regression snapshot/check was followed
by deletion of the generated object and a forced rebuild. `_towupper` remained
`still_exact`, with no failures, warnings, changed non-exact siblings, newly
exact surprises, or ownership changes. The complete tooling suite passes
179/179 tests.

## House/Berth and original-bug audit

The candidate is readable typed C with explicit narrow types, named
parameters, ordinary conditions, and no hidden byte control. It uses no
assembly, `volatile`, forced inlining, undefined behavior, type-punning,
opaque code-generation cast, packing/alignment trick, flag change, byte patch,
or comparator waiver. The multiple source returns are the same measured
natural CRT topology already proven necessary for the inverse `towlower`
object; the compiler still emits one coherent function and the source remains
clear.

The ignored classification result is preserved exactly because both January
and the authentic XDK member prove it. This lane does not label it a definite
original bug: the call may intentionally preserve CRT side effects or
validation even though it does not map the character. A corrected locale-aware
implementation would require a proven mapping API and is outside this exact
reconstruction.
