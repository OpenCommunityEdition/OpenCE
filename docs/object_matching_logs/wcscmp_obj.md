# `wcscmp.obj` exact-match log

## Plain-English behavior

This CRT routine compares two UTF-16 strings lexicographically. It returns
exactly negative one, zero, or positive one rather than the raw character
difference.

## Provenance and verification

- January target: one 50-byte cdecl function, zero relocations, no data.
- Compiler: XDK 3911 CL 13.00.9254.1 with the libcmt project's unchanged
  `/O1 /Gy` flags.
- The comparison is written as the canonical CRT assignment in the loop
  condition, followed by a single-return normalization ladder. Admission
  requires the target's word loads, pointer increments, loop predicate, and
  `-1/0/1` normalization—not just equivalent ordering semantics.
- No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
  or compiler-flag change is used.

## Current-campaign strict evidence

The old result was treated as untrusted and transplanted onto campaign tip
`0244dd18`, after the libcmt `/O1 /Gy /I"xbox/include"` configuration had been
independently admitted. A fresh XDK 3911 build reproduced the January function
exactly:

| symbol | target/candidate size | relocations | normalized SHA-256 |
|---|---:|---:|---|
| `_wcscmp` | 50 / 50 bytes | 0 / 0 | `27b69ac2a74e5cc8348050bc9bc35a46505b9497f71d03833f886ded75409c29` |

`section_infos_equal` is true. The target owns exactly one external function
COMDAT and no data, BSS, or read-only data. The candidate's only extra sections
are ordinary compiler directives/debug metadata.

The complete Halo and configured libcmt builds pass. `ninja progress` credits
this as one libcmt object only; it does not alter Halo progress. A clean
per-object regression snapshot/check retains `_wcscmp` as `still_exact` with
no warnings or failures, and the full tool test suite passes.

## House/Berth audit

The function is readable C with vertically formatted parameters and one final
return. Its unsigned-short character type is proven by the target's 16-bit
loads. It adds no assembly, volatility, forced inlining, undefined behavior,
alignment directive, raw layout offset, byte patch, semantic exception, or
compiler workaround. No original behavioral bug was identified in this unit.
