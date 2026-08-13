# `libs/libcmt/days.obj` exact-match ledger

This atomic lane independently revalidates an older completed result against
the current January target and campaign gates. Historical branches were used
only as evidence; the object was recreated on the current canonical tip.

## Plain-English behavior

This object contains two calendar lookup tables and no executable code. The
leap-year table says how many days have elapsed before each month in a leap
year; the ordinary-year table supplies the same information for a non-leap
year. Index zero is a sentinel before January.

## Target inventory and recovered representation

- The January object owns one 104-byte writable `.data` section.
- It owns no `.text`, `.rdata`, or `.bss`, and has zero relocations.
- `_lpdays` is an external 13-element signed-`int` array at offset `0x00`.
- `_days` is an external 13-element signed-`int` array at offset `0x34`.
- The two arrays are kept as ordinary typed C data in target order. No packed
  surrogate, union, alignment directive, raw byte array, or byte patch is
  used.

The values were decoded directly from the January object. The leap table ends
at 365 and differs after February; the ordinary table ends at 364.

## Source-shape experiments

No speculative source-shape search was necessary. The direct, readable
declarations are the source-authentic representation and are the only shape
tested in this fresh lane. Admission still depends on strict COFF evidence:
matching values alone are not sufficient if the compiler changes the symbol
order, offsets, linkage, section class, or padding.

## Strict current-campaign evidence

The object was rebuilt with XDK 3911 CL `13.00.9254.1` under the configured
libcmt flags `/O1 /Gy /I"xbox/include"`; no lane-local option was added.
`tools.coff_compare.section_infos_equal` returned true for both public names,
which independently address the same owned section:

| owned section | target | candidate | relocations | normalized SHA-256 |
|---|---:|---:|---:|---|
| `.data` | 104 B | 104 B | 0 / 0 | `c7bae0c8ca6ab58a041bf992c18cc8853c4732f07caa47d49f7b5bb446356be7` |

`llvm-readobj` confirmed that both sides use four-byte section alignment and
read/write initialized-data characteristics. Both symbols have external
storage class: `__lpdays` is at offset `0x00`, and `__days` is at offset
`0x34`. The compiler-only `.drectve` and debug metadata are not target runtime
ownership.

The complete `halobetacache_build` and `libcmt_build` passed. The regenerated
semantic report had zero unit errors, and progress credited all 104 data bytes
to libcmt. At this lane's snapshot, libcmt had 18/212 admitted objects, 18/476
functions, 1,222/55,015 code bytes, and 104/8,637 data bytes.

After the exact candidate was committed, the fail-closed per-unit regression
snapshot/check passed with no failures, warnings, ownership changes, changed
non-exact siblings, or unexpected new matches. Its `still_exact` function list
is correctly empty because this translation unit contains data only; the gate
freezes its complete non-code and symbol-ownership manifest instead.

`python -m tools.audit_semantic_matches` again reported zero unit errors and
3,371 accepted exact functions campaign-wide. The complete tooling suite
passed **179/179** tests. `git diff --check` and the final scope/policy audit
were clean.

## House/Berth audit

This unit is readable typed C and contains no functions, so parameter, return,
and typed-access rules are not applicable. It introduces no assembly,
`volatile`, forced inlining, undefined behavior, compiler-flag override,
alignment workaround, comparator exception, or ownership alias. No original
behavioral bug was found, so no bug-preservation comment is required.
