# `wcsxfrm.obj` exact-match log

## Plain-English behavior

The Xbox CRT has no locale transformation here: if a destination is supplied,
the source is copied with `wcsncpy`; the return value is always the source
length from `wcslen`.

## Reconstruction discipline

- January target: one 37-byte cdecl function and two exact REL32 relocations.
- The natural wrapper has one final return and one parameter per line.
- The already exact `wcsncpy.obj` and `wcslen.obj` establish both call targets;
  strict admission still requires relocation addresses and identities to match.
- No assembly, `volatile`, forced inlining, undefined behavior, or flag change.
- Hardened result: 37/37 bytes, two exact REL32 relocations at offsets 19 and
  30, normalized SHA-256
  `002ff749799b2e79740fcfd2a6624dce6678f714ee64df4ae324be5903829f6a`.

## Independent current-campaign verification

The older completed candidate was treated as a lead, not as proof.  It was
transplanted onto campaign commit `4b1fc5f1`, compiled afresh with XDK 3911
under libcmt's unchanged `/O1 /Gy /I"xbox/include"` command, and compared to
the January split object with the hardened COFF comparator.

- `_wcsxfrm`: `37/37` bytes and `2/2` relocations.
- `+0x13`: `IMAGE_REL_I386_REL32` to `_wcsncpy + 0`.
- `+0x1E`: `IMAGE_REL_I386_REL32` to `_wcslen + 0`.
- The target owns no `.rdata`, `.data`, or `.bss`; the candidate adds none.
- Full Halo and libcmt builds, semantic progress, and the per-unit regression
  snapshot/check completed with zero failures or warnings.
- All 179 tooling tests passed.

## House and Berth audit

The implementation is ordinary readable C: parameters are written one per
line, the function has one final return, and its two calls use their recovered
typed prototypes.  It contains no assembly, `volatile`, `__forceinline`,
alignment directive, magic structure offset, undefined behavior, byte patch,
compiler substitution, flag change, or comparator exception.  There is no
original-game defect at this call-through wrapper that needs a bug/fix note.
