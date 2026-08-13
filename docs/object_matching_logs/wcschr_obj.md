# `libs/libcmt/wcschr.obj` exact-match ledger

This atomic lane independently revalidates an older completed result against
the current January target and campaign gates. Historical commits were treated
only as leads; their labels and old build products were not trusted.

## Plain-English behavior

This CRT routine walks a UTF-16 string until it either finds the requested
character or reaches the string terminator. Searching for zero therefore
returns the terminator itself. Searching for an absent nonzero character
returns a null pointer.

## Provenance and inventory

- The January object contains one public cdecl function, `_wcschr`.
- `_wcschr` owns one 34-byte `.text` COMDAT and has zero relocations.
- The object owns no runtime `.data`, `.bss`, or `.rdata` section.
- `_wcschr` has external storage class in both target and candidate.
- The source is compiled by XDK 3911 `CL.Exe` 13.00.9254.1 with the libcmt
  project's unchanged `/O1 /Gy /I"xbox/include"` flags.

Two old local commits, `ad1874d7` and `0708c799`, contained the same proposed
source. Both descended from the old `f8e8e6b` campaign state. This lane did not
cherry-pick either commit. It copied only the proposed source and documentation
into a fresh worktree, rebuilt both target split and candidate objects, and
re-established every result below on the current campaign history.

## Measured source shapes

| Source family | Measured result | Disposition |
|---|---|---|
| One ternary result after the scan | 41 target-meaningful bytes rather than 34 | Rejected: changes January's exit CFG. |
| Short-circuit scan followed by explicit found/null returns | 34/34 bytes, 0/0 relocations, identical normalized hash | Accepted. |

The second source shape is the ordinary readable CRT algorithm. VC7 emits the
same word loads, short-circuit branches, successful pointer return, and null
failure return as January. The cast on the successful return only removes the
input pointer's `const` qualification to implement the standard C library API;
it is not an artificial code-generation lever.

## Current-campaign strict evidence

The final candidate was transplanted onto campaign tip `ce1993b7` and rebuilt
from source. The hardened comparison result is:

| symbol | target size | candidate size | relocations | normalized SHA-256 | result |
|---|---:|---:|---:|---|---|
| `_wcschr` | 34 B | 34 B | 0 / 0 | `2cef2a46aed18bbe9b60645eeb805adc3609c594390b2be35c630df1de97ae10` | exact |

`section_infos_equal` returned true. Because this is the object's only runtime
code section and it owns no runtime data section, the whole object is closed;
there is no unexamined sibling, anonymous table, string, or BSS symbol hidden
behind the function result. The candidate's additional compiler `.drectve`
and debug metadata are ordinary non-runtime build artifacts.

## House/Berth and readability audit

The parameter list is vertical, the implementation is readable C, and every
path has an explicit return. The preferred single-return rule cannot be used
here without changing the proven 34-byte January control flow, so the two
returns are the documented permitted exception. `(void)` formatting is not
applicable because the function has parameters. There are no tag or object
accesses requiring typed macros.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw structure offset, comparator
exception, or lane-local flag change is used. No original January bug was
identified, so no bug-preservation comment is needed.

## Final admission gates

The final gate results are recorded here after the clean current-tip rebuild:

- Full configured Halo build: passed.
- Full configured libcmt build: passed.
- `ninja semantic_progress`: passed with 3,360 accepted exact functions and
  zero unit errors.
- `ninja progress`: passed and credited `wcschr.obj` as a complete libcmt
  object. At this campaign state libcmt reports 7/476 functions and 249/55,015
  code bytes exact across 6/212 objects.
- Whole-unit regression snapshot/check: passed from clean commit `43c2f676`;
  `_wcschr` is `still_exact`, with no failures, warnings, newly-exact siblings,
  changed nonexact code, or ownership change.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 177/177 passed.
