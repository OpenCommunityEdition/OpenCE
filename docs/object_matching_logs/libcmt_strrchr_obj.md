# `libs/libcmt/strrchr.obj` provenance and park ledger

## Plain-English behavior

`strrchr` finds the final occurrence of a byte in a null-terminated string.
Searching for zero returns the address of the terminator itself. When the
byte is absent, it returns null.

## January target inventory

- Function: `_strrchr`
- Meaningful code: 39 bytes (`0x27`)
- Relocations: zero
- Owned runtime data: none
- Normalized SHA-256:
  `61e62304347e294cb52e76f3b5cc064b748e44dd24d48e3550c00e35746640a3`

The machine code scans forward with `repne scasb`, changes the x86 direction
flag with `std`, scans backward with a second `repne scasb`, and restores the
direction flag with `cld` before returning. In plain English, this is an
explicitly hand-selected string-instruction implementation rather than the
kind of scalar loop VC7 normally emits from C.

## XDK 3911 provenance proof

The user-provided `XBOXSDK_3911.ISO` contains `XDKSetupEng.exe`, whose cabinet
contains `XDK\\xbox\\lib\\libcmt.lib` (SHA-256
`60AA4C87C97EEB6662B31D9D37CF3CABA1AFABAEC72C47B090E7A0CE8B632F55`).
Extracting the archive member `obj\\i386\\strrchr.obj` produced a COFF object
whose embedded records name both:

```text
..\\i386\\strrchr.asm
Microsoft (R) Macro Assembler Version 6.15.8803
```

The hardened comparator reports that actual XDK member and the January target
are strictly identical:

| Evidence | January target | XDK member |
| --- | ---: | ---: |
| Meaningful code | 39 bytes | 39 bytes |
| Relocations | 0 | 0 |
| Normalized SHA-256 | `61e62304...6640a3` | `61e62304...6640a3` |
| `section_infos_equal` | | `true` |

This proves that January linked Microsoft's unchanged XDK 3911 assembly
implementation. The original implementation is not missing C source.

## Readable-C experiment

One evidence-backed readable-C family was measured with the configured XDK
3911 compiler and unchanged libcmt `/O1 /Gy` flags. It mirrored the already
exact `wcsrchr` topology: retain the original pointer, scan through the NUL,
walk backward to the requested byte, and return one conditional result.

Coincidentally, the compiler also emitted 39 bytes and zero relocations, but
the strict normalized hash was
`39b85402f6ea093a3c312e6388b25a6739c38d1da134088d695f75d093c53066`.
It emitted an ordinary scalar forward loop and scalar backward comparisons;
it did not emit either `repne scasb`, `std`, or `cld`. Equal length is not
equal code, so the candidate was rejected and removed.

No additional C spelling can honestly recreate the proven assembly source
without transcribing or forcing its instruction choices. Those techniques are
forbidden by the project's readable-C policy.

## Decision and reopen rule

`strrchr.obj` remains `MISSING`. Production source and the temporary matching
label were removed, so no progress credit is granted. This is a policy park,
not an unresolved semantic reconstruction.

Reopen only if the project explicitly permits verified vendor assembly or
prebuilt XDK library members. If that policy changes, the exact XDK member is
already identified and strictly verified; no further reverse engineering is
needed.

## House/Berth audit

- No assembly was copied into project source.
- No `volatile`, undefined behavior, inline forcing, byte patching, flag
  change, alignment trick, comparator exception, or semantic credit was used.
- The one C experiment was ordinary readable code and was fully removed after
  it failed strict comparison.
- The object remains uncredited because 99% or equal size is not 100% exact.
