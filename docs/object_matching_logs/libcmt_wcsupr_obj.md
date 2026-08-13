# `libs/libcmt/wcsupr.obj` exact-match ledger

This atomic lane treats the older result at commit `74c09059` as an untrusted
source proposal. The implementation was transplanted into a fresh worktree
from the current campaign tip and rebuilt from scratch before admission.

## Target inventory

- Runtime section: one 44-byte `/Gy` `.text` COMDAT
- Relocations: none
- Owned data: none
- Linked-image names at the same address: `_wcsupr` and `__wcsupr`
- Reported function: `__wcsupr` (one function, not two duplicate bodies)

Both target symbol names have value zero in the same target section. They are a
legacy-name alias pair, not two independent functions with coincident bytes.

## Plain-English behavior and source provenance

The function walks a wide-character string in place. For each ASCII lowercase
letter from `a` through `z`, it subtracts the distance between lowercase and
uppercase ASCII. All other wide characters are left unchanged, and the
function returns the original pointer.

The source shape is the readable C-locale branch of Microsoft's historical CRT
`wcsupr.c`. The C identifier `_wcsupr` emits the COFF symbol `__wcsupr`. The
ordinary default-library directives include `OLDNAMES`, which supplies the
legacy `wcsupr` spelling at link time. A second source body would incorrectly
create code ownership absent from the January target.

## Experiment record

| ID | Source shape | Result |
|---|---|---|
| E01 | Historical `_wcsupr` body only | Accepted candidate: one runtime section |
| E02 | Add `wcsupr` as a forwarding wrapper | Rejected: extra jump thunk and relocation |
| E03 | Duplicate the full body under `wcsupr` | Rejected: second 44-byte COMDAT absent from target |

## Strict verification

- XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the object with the generated
  `/O1 /Gy /I"xbox/include"` flags.
- Target `_wcsupr` -> candidate `__wcsupr`: `section_infos_equal = true`.
- Target `__wcsupr` -> candidate `__wcsupr`: `section_infos_equal = true`.
- Size: `44/44`; relocations: `0/0`.
- Normalized SHA-256, target and candidate:
  `b8bf49345888d96c9d87fdc8b50a9c21435b9a44da8d98d2266b92e63ad30685`.
- Both target aliases are external, have value zero, and name the same target
  section. The candidate owns one external runtime symbol and one runtime
  section; neither side owns runtime data.

## Current-campaign admission gates

- Full `halobetacache_build` and `libcmt_build`: passed.
- `semantic_progress`: passed with zero unit errors.
- `progress`: `wcsupr.obj` is credited as one 44-byte function and one complete
  libcmt object. Global campaign totals are intentionally omitted because
  concurrent atomic admissions make them stale without changing this proof.
- The strict semantic audit records `__wcsupr` with both `objdiff` and
  `semantic-coff` proof. The admission audit reports no `wcsupr` contradiction,
  revocation, or unreviewed candidate.
- The clean-commit whole-unit regression snapshot/check retains `__wcsupr` as
  `still_exact`, with no failures, warnings, newly exact sibling, changed
  nonexact code, or ownership warning.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 179/179 passed.

## House/Berth and readability audit

The parameter is on its own line and the function has one explicit return.
`(void)` formatting is inapplicable because the function takes a parameter.
The loop is ordinary readable C with no raw structure offsets, tag access, or
object access requiring project-specific typed helpers.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw offset, comparator exception, or
codegen-only cast is used. No original January bug was found, so no
bug-preservation comment is needed.
