# `debug_memory.obj` unlink-reload admission (2026-08-26)

## Result

`_code_0007d060` is newly strict exact. The ordinary typed-C change removes a
named successor local and follows the linked list through `previous->next`,
which reproduces January's repeated reloads and register lifetimes.

| Measurement | January | Retained candidate |
| --- | ---: | ---: |
| Meaningful bytes | 214 | 214 |
| Padded bytes | 224 | 224 |
| Relocations | 14 | 14 |
| Normalized SHA-256 | `609ad851cc6695b293affcd19e7450fa949e6e8ef3a6536e017311007de69b16` | `609ad851cc6695b293affcd19e7450fa949e6e8ef3a6536e017311007de69b16` |

Every relocation address, COFF type, symbolic destination, addend, and owner
is pairwise exact. The generated candidate object has raw SHA-256
`8d7fe78b7e974a2b856c6829f0d94bac107ccb60cfcbf8640b31baedb67af20e`.

The unit advances from 14/18 to 15/18 exact functions, from 1,871 to 2,085
accepted meaningful code bytes, and from 1,952 to 2,176 accepted padded code
bytes. All 1,120 padded runtime-data bytes remain exact. The three honest
residuals are `_code_0007cf50`, `_debug_malloc`, and `_debug_realloc`, so the
object remains `NonMatching`.

## Evidence and source topology

January keeps the header in `EDI` and the previous node in `ESI`. After
writing `previous->next`, it reloads that field for the null test, previous-
link store, checksum input, and checksum destination. The prior source instead
kept `header->next` in a named `next` local, allowing VC7 to reuse `ESI` for
the successor and emit a 208-byte function.

The neighboring `_code_0007cfc0` admission had already established the same
source-level rule: January's list code deliberately reloads through the
written link instead of retaining a named alias. Applying that topology to
the unlink helper produced the target's 224-byte body and all 14 relocations
on the first code-producing compile of this evidence wave. The result is
defined C89, preserves list semantics, and retains the existing typed record
accesses and explicit terminal return.

## Separate rejected random-fill wave

Before the unlink evidence was frozen, the complete current and historical
Claude ledgers, source history, January caller/callee disassembly, and the
user-supplied public repositories were audited for `_code_0007cf50`. Public
search found only the empty `punpckhdq/halo` shell; no source donor exists in
the searched material.

January passes the random-fill pointer in `EBX` and its size on the stack,
whereas the current compiler-private convention uses `EAX` and `EBX`.
Swapping the helper's formal order and the sole call was tested once because
an exact campaign donor proves that formal order can control some private
ABIs. Here VC7 preserved the semantic register convention byte-for-byte: the
helper stayed 96/112 padded bytes and no function became exact. All 14
accepted siblings remained exact. The rejected object is preserved at
`build/audit/debug_memory_random_fill_parameter_swap_20260826.obj`, raw
SHA-256
`6b537a25cf13a419b83fdba4e519fe7ca6314179f05fdae4eeb7407fcb9ade97`.
The parameter-order family is therefore closed and was fully reverted before
the unlink wave.

## Validation

- Strict comparator: `_code_0007d060` passes padded bytes, normalized bytes,
  and all 14 relocation records.
- Fail-closed unit regression: 14 inherited functions remain `still_exact`,
  `_code_0007d060` is the sole reviewed `newly_exact`, and there are no
  failures or changed nonexact functions.
- Full Halo and libcmt builds: pass.
- Semantic audit: 470 units, 4,332 functions evaluated, 4,190 semantic exact,
  4,251 accepted exact, and zero unit errors.
- Campaign progress: 510,138/2,198,102 code bytes and 4,225/11,060 functions;
  Halo CE contributes 497,224/1,770,166 code bytes and 4,058/7,574 functions.
- Complete objects remain 377/833 overall and 275/468 for Halo CE.
- Object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` contradiction remains.
- Parked validation: 3 active, 0 stale, 0 invalid.
- Tool suite: 179/179 tests pass.
- `git diff --check`, parse-only XDK compilation, and the forbidden-construct
  audit pass. No configuration, semantic exception, completion label, header,
  protected source, or Units source changed.

## Clean committed-state replay

Implementation commit `0671bb1003c44a99f833f7f948b3459a44ae3c35`
was clean before replay. A fresh fail-closed snapshot at that commit was
written to
`build/audit/debug_memory_unlink_reload_regression_20260826.json`. The sole
generated object was resolved inside the isolated worktree, removed with a
literal verified path, rebuilt through its ordinary Ninja edge, and checked
immediately. All 15 accepted functions report `still_exact`, with zero
failures, warnings, newly exact functions, or changed nonexact functions.
Independent strict comparison again proves `_code_0007d060` at 224/224 padded
bytes, 14/14 relocations, and normalized SHA-256 `609ad851cc6695b2...`.
