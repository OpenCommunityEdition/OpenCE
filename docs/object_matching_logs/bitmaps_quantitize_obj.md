# bitmaps_quantitize.obj exact-match strategy ledger

## Scope and provenance

- Translation unit: `source/bitmaps/bitmaps_quantitize.c`
- January target object: `source/bitmaps/bitmaps_quantitize.obj`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- January `_code_0006d5f0` is the function known cross-build as `row_dither`.
- The exact 464-byte January target body also occurs in the October 12, 2001
  `2276betaP.xbe`, corroborating the machine-code target and algorithm.
- No authentic C donor or useful January/HCEA local-variable record has been
  found.

## Current validated baseline

Two of three functions are strict exact. All owned `.rdata`, `.data`, and
`.bss` sections are exact.

| Function | Target/base size | Relocs | Result |
| --- | ---: | ---: | --- |
| `_bitmap_quantitize` | 592/592 | 28/28 | strict exact |
| `_code_0006d5d0` | 32/32 | 0/0 | strict exact |
| `_code_0006d5f0` / `row_dither` | 464/464 | 0/0 | nonexact scheduler residual |

`row_dither` target normalized SHA-256 is
`a5feef0c98bc8565991a9cf9722e0258d2bd556e23fc0326891aa0ce0ebca5db`.
The restored baseline candidate hash is
`7afe02e297bf06dd682a9c44bfd00aacf39c2f7ea0c0f45acce838dc9b696da4`.

The sole baseline machine-code residual is the ordering of independent
inner-loop maintenance:

```text
target:    inc eax; add ebx,2; add ecx,2
candidate: add ebx,2; inc eax; add ecx,2
```

Here EAX is the channel index, EBX is the minimum-error pointer, and ECX is a
compiler-synthesized next-row channel pointer.

## Accepted source controls

- The current clamping, quantization, and Floyd-Steinberg error-diffusion
  topology reproduces the target size, frame, branches, and all bytes outside
  the three independent loop-maintenance operations.
- `_bitmap_quantitize` and `_code_0006d5d0` are protected exact siblings; no
  closeout experiment may regress them.
- Existing source comments preserve the original one-allocation-failure leak
  and explain the nonmatching safe fix.

## Bounded experiment matrix

The prior closeout research exhausted source-order, loop-shape, pragma,
barrier, dependency, and pointer-topology families. The original detailed
research packet is `C:\tmp\bitmaps-quantitize-agent\ROW_DITHER_PROVENANCE_REPORT.md`.

| Experiment | Size | Relocs | Candidate hash / first divergence | Decision |
| --- | ---: | ---: | --- | --- |
| Restored source: `minimum_channel++` then `channel_index++` | 464/464 | 0/0 | `7afe02e2...`; `add ebx,2` precedes `inc eax` | best baseline |
| Reverse those two source statements | 464/464 | 0/0 | `a5d1033d681cec568e19033f09f52ef415f989e690717817a578423a4f9b52d5`; emits `inc eax; add ecx,2; add ebx,2` | reverted |
| Declaration/source-order and equivalent loop-tail permutations | 464/464 in best forms | 0/0 | either baseline ordering or moves ECX between EAX and EBX | exhausted/reverted |
| Compiler barriers around the tail | varied/regressed | 0/0 | alter the loop schedule beyond the target | exhausted/reverted |
| Pointer aliases/dependencies intended to order the advances | varied/regressed | 0/0 | change earlier register allocation or remain baseline | exhausted/reverted |

An earlier research lane also measured three narrow inline-assembly
diagnostics. None matched, and all were reverted. They are retained in the
historical report only as negative evidence; inline assembly is prohibited and
must not be retried or admitted.

## Do-not-repeat list

- Do not retry either ordering of only `minimum_channel++` and
  `channel_index++`; both exact compiler outcomes are recorded above.
- Do not repeat declaration-order, loop-tail, barrier, pragma, explicit
  pointer-topology, or artificial dependency variants from the prior report.
- Do not use inline assembly, `_emit`, volatile forcing, undefined behavior,
  compiler-flag changes, comparator weakening, or object-byte patches.
- Do not treat the October body as an original-C donor. It proves target
  identity, not source spelling.

## Residual classification

The residual is a measured scheduler tie among three independent maintenance
operations. Size, frame, relocation count, control flow, and every other
instruction are already exact. Two direct source statement orders were tested:
one leaves EBX before EAX; the other puts ECX between EAX and EBX. Neither
produces January's EAX/EBX/ECX order.

## Reopen criteria

Reopen only for:

1. original Bungie C or local-variable/statement provenance for `row_dither`;
2. a strict-exact C donor under the same compiler and flags with the same
   three-operation dependency context; or
3. a newly demonstrated legal-C scheduling control not represented in the
   exhausted matrix.

## Current disposition

`bitmaps_quantitize.obj` remains 2/3 strict exact with all data exact. It is
rigorously parked and must remain `NonMatching`; no fuzzy or semantic credit is
claimed for `row_dither`.
