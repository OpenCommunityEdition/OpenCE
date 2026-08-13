# `libs/libcmt/wcslwr.obj` exact-match ledger

Historical source is a lead only. This lane independently validates the whole
object, including its unusual same-address linked symbol aliases.

## Plain-English behavior

The routine walks a UTF-16 string, converts ASCII `A` through `Z` to lowercase,
and returns the original pointer. Characters outside that C-locale range are
unchanged.

## Target inventory and alias provenance

- One 44-byte `/Gy` `.text` COMDAT, zero relocations, no runtime data.
- Linked-image names `_wcslwr` and `__wcslwr` share the same target address and
  section. They are aliases for one body, not two duplicate functions.
- The C identifier `_wcslwr` emits COFF symbol `__wcslwr`. The legacy
  `_wcslwr` spelling is supplied at link time through the CRT `OLDNAMES`
  library named by normal compiler directives.

Defining a second source body would claim an extra target-absent code section,
so the correct candidate intentionally owns one runtime COMDAT.

## Measured source shapes

| Shape | Measurement | Disposition |
|---|---|---|
| Historical `_wcslwr` body only | Prior lead: 44/44 bytes; both target aliases resolve to the one candidate body | accepted for fresh validation |
| Add `wcslwr` forwarding wrapper | Proven analogue creates a jump thunk and relocation | rejected |
| Duplicate full body under `wcslwr` | Creates a second runtime COMDAT absent from January | rejected |

## Fresh current-campaign evidence

The source was independently transplanted onto campaign tip `7d2d26ea` and
built with XDK 3911 using the configured `/O1 /Gy` libcmt flags. No lane-local
compiler or build-rule change was made.

| mapping | target | candidate | relocs | normalized SHA-256 | result |
|---|---:|---:|---:|---|---|
| target `__wcslwr` -> candidate `__wcslwr` | 44 B | 44 B | 0 / 0 | `6ca01f8d94879f155a6e9f7e8438aaefea80786f0d5b080c2cbd4cbf4e18a25c` | exact |
| target `_wcslwr` -> candidate `__wcslwr` | same target section/address | same candidate section | 0 / 0 | same exact hash | proven same-address alias |

All 44 code bytes are literally identical. `llvm-readobj` proves the two target
names have value zero in one 44-byte section, while the candidate correctly
owns one exact external COMDAT. There is no target runtime data.

The regression gate needed one narrow representation fix: when a target has an
unreported alias name, meaningful size may be inherited only if exactly one
report-listed function occupies the identical COFF section and offset. Zero or
multiple candidates still fail. The change is covered by positive and
ambiguity-negative tests and does not compare or grant code credit.

## Gates and blast radius

- Complete libcmt and 467-object Halo builds: passed.
- Progress and semantic progress: passed with zero unit errors.
- Expanded tool suite after the alias fix: 179 / 179 passed.
- Direct hardened code comparison and complete section/symbol inventory: passed.

No shared production header changed. The tooling change is separately committed
and strictly fail-closed; this object's production source affects one TU.

## House/Berth and readability audit

The parameter is on its own line and the non-void function has one final return.
`(void)` formatting is not applicable. The implementation uses ordinary C and
no assembly, `volatile`, forced inlining, undefined behavior, byte forcing,
alignment directive, raw structure offset, comparator exception, or local flag
workaround. It has no tag/object access and no identified January bug.
