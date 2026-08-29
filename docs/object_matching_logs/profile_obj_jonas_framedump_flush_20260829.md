# `profile.obj`: framedump flush leaf (2026-08-29)

## Outcome

This bounded wave adds the ordinary-C body for `_code_0007edc0`. The function
is strict exact at 51 meaningful / 64 padded bytes, seven relocations, and
normalized SHA-256
`eea7e3377f89acaafb7cb480ccbde58b8468cd212325b5d4da0859ced1297b57`.
`source/cseries/profile.obj` advances from 17/44 to 18/44 strict-exact
functions and from 2,064 to 2,128 accepted padded bytes. The object remains
incomplete; no completion label, exception, park, or configuration change was
made.

The retained function flushes the pending framedump line only when the
private pending flag is set. If the framedump stream exists, it writes the
target's `"\r\n"` literal and flushes the stream, then clears the flag. This
directly explains every target load, branch, call, and store.

## Evidence and bounded probes

The clean starting commit was
`27cf96515307b485ff05c6c337b124029534491a`. The January split object SHA-256
is `f40acee3f68569049519a0f2c71775232cba27c6578e2bfc2aebfc52eca40587`.
The retained source blob before commit is
`5c1403975fd07692ca0e473bcebf929b7f3e3d9f`; its SHA-256 is
`4db552b7ec0955a94679bf758df31ca01d8854fba0267f028e5ed336aa67b3ff`.

Direct hardened comparison proves equality for all 64 padded bytes and all
seven relocation address/type/destination/addend tuples. The relocations are
the pending flag twice, the framedump stream twice, the CRLF literal,
`fprintf`, and `fflush`. The complete per-unit gate reports 18 exact, zero
residual, and 26 unwritten functions, so all 17 inherited exact owners remain
strict exact.

Before selecting this leaf, two bounded alternatives were rejected without
retained source:

- `cinematic_set_title_delayed` reproduced its known `__ftol2` compiler
  blocker instead of January's inline x87 conversion; and
- the remaining tiny Profile leaves were screened out because their target
  bodies are compiler-private register-convention or `rdtsc` helpers rather
  than independent ordinary-C functions.

No rejected probe, Cinematics declaration, Scenario layout, generated object,
or scratch artifact is included in this change.

## Validation and policy

- Full `halobetacache_build` and `libcmt_build`: pass.
- Strict board: 274/619 complete objects, 628,983/1,922,669 padded code bytes,
  and 4,592/8,246 strict functions after excluding parks and assembly credit.
- Progress report: 574,943/2,198,102 meaningful code bytes and 4,584/11,060
  functions overall; Halo contributes 562,029/1,770,166 bytes and
  4,417/7,574 functions.
- Semantic audit: 470 units, 4,733 functions evaluated, 4,611 semantic exact,
  4,621 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 13 active, zero stale, zero invalid.
- Tool suite: 205 tests pass; only the inherited unwritable pytest-cache
  warning remains.
- `git diff --check` and the complete Profile function gate pass.

The retained body is readable typed C with the no-argument `void` and terminal
return formatting required by the campaign. It uses no assembly, intrinsic,
`volatile`, `register`, pragma, attribute, raw address or offset, type pun,
undefined behavior, object patch, compiler-option forcing, or comparator
exception. Protected Units, Vehicles, AI, and game-engine sources were not
edited.
