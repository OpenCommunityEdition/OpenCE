# `wcspbrk.obj` exact-match log

## Plain-English behavior

This CRT routine finds the first UTF-16 string character that occurs in a
second UTF-16 set. It returns null if no character is shared.

## Reconstruction discipline

- January target: one 64-byte cdecl function, zero relocations, no data.
- Compiler: XDK 3911 CL 13.00.9254.1 with unchanged `/O1 /Gy` flags.
- A persistent single-result local compiled to 73 bytes and was rejected.
  A compound inner-loop condition followed by `if (*candidate)` compiled to
  70 bytes because it reloaded the terminating character and was rejected.
  January's inner loop tests equality and returns immediately before advancing;
  falling out of that loop therefore already proves the control character is
  null. The canonical routine uses that direct hit exit and one final null
  return.
  This is a documented evidence-required exception to the preferred
  single-return house rule.
- No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
  or compiler-flag change is allowed.

## Current-campaign strict evidence

The historical reconstruction was treated as untrusted and transplanted onto
campaign tip `b935e2dc`. A fresh XDK 3911 build under the canonical libcmt
`/O1 /Gy /I"xbox/include"` configuration produced 64/64 literal bytes, 0/0
relocations, and normalized SHA-256
`8ad650b26e0d1d817ad4833c9c1fd39cedd06acf0db0dff9df92f80e72b6e3fa`.
`section_infos_equal` is true. The target owns only the external `_wcspbrk`
function COMDAT and no runtime data.

The complete Halo and libcmt builds, progress/semantic reports, clean per-unit
regression snapshot/check, and full tool suite were rerun. The early hit return
is retained as the measured house-style exception described above; parameters
remain vertical and the source is ordinary readable C. It adds no alignment
directive, raw offset, semantic exception, code-generation-only cast, or flag
workaround. No original target bug was identified.
