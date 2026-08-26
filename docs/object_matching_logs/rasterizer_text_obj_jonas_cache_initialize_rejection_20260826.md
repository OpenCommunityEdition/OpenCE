# `rasterizer_text.obj` cache-initializer rejection (2026-08-26)

## Scope and result

This wave opened the previously undefined public
`_rasterizer_text_cache_initialize` leaf from cumulative commit
`30658aed84c48b4c2bbd2e80837991339f522e85`. The recovered behavior and all
three January literals are source-backed, but none of the bounded candidates
matched the target compiler shape. No production source or semantic-match
metadata is retained, and `source/rasterizer/rasterizer_text.c` is restored to
its original Git blob `a9e938606c9f809c65ea7a3ea17313bd6523c16a`.

The object therefore remains 6/14 strict-exact functions. The seven existing
exact functions were treated as inherited evidence; the whole-TU gate granted
no newly exact function in this wave.

## Frozen baseline and provenance

- January target object SHA-256:
  `3AE1578C8061A767D7BAF68C5D87203E8FB855B7CE3C5EF7326D255103D81878`
- cumulative baseline object SHA-256:
  `E65E830DAECDFDB97674788B60705B73BAD773D3B64B3C3F28E5466BCF324DF6`
- frozen baseline source SHA-256:
  `A1522477AA5DC18A5888221791AD364D9263ABA40DEBE5DFA101602DAFE44971`
- clean pre-wave regression manifest:
  `outputs/rasterizer_text_evidence_20260826/regression-baseline.json`

Relevant local reconstruction evidence was read before compiling:

- `research-cache/halocea-cseries-20260820/src/blam/rasterizer/rasterizer_text_cache_initialize.c`
- `research-cache/stian-halo-cseries-20260820/src/halo/rasterizer/rasterizer_text.c`
- `research-cache/stian-halo-cseries-20260820/CLAUDE.md`
- `research-cache/stian-halo-cseries-20260820/AGENTS.md`
- `research-cache/stian-halo-cseries-20260820/docs/vc71-byte-accuracy-playbook.md`
- `docs/exact_match_acceleration_playbook.md`

The later HaloCEA donor is from a different 512-by-512 cache build and was
used only for semantic roles. The Stian reconstruction and January COFF agree
on the relevant 128-by-128 build: assert that the cache is not initialized,
allocate format 9, clear `0x810` bytes, register the bitmap, store it at
offset `0xC`, mark the cache initialized, and report the exact error literal
on failure.

## January target shape

The target function is 144 padded bytes with 13 relocations and normalized
SHA-256
`2fa3158fdf45b82f2ecc28f3009273ff781b9d95850603b18b22bbf7b702ca6c`.
Its distinctive compiler shape is:

- save `EBX` and `ESI`, then set `EBX = 1` before the assertion;
- reuse `EBX` for the `display_assert` truth argument, the initialized byte,
  and the successful byte return;
- retain the bitmap pointer in `ESI`;
- use the exact assertion expression
  `!hardware_character_cache.initialized`;
- issue the expected 13 relocations in the target order.

## Bounded candidates

All successful candidates were preserved before any later edit. Each remained
144 padded bytes with 13 relocations, but emitted the same nonexact normalized
code hash
`e4ad716723d1d26d9b33fb2087331aafbf0628e25b306fb30e86ed6afaa62835`.

| Wave | Source hypothesis | Source SHA-256 | Object SHA-256 |
|---|---|---|---|
| 1 | natural typed reconstruction | `45E984159DD4FBD2524792502E55EF3AC8D3079ADC0693ED55F3198AF659C873` | `0470ECD1C601FEA3461E6A35BC94E1F4A5F2F795C99605C4B618EA92D3BEB056` |
| 2 | live `TRUE` byte plus exact assertion identifier | `D4397F5B15DCFA9E53440F00165C5213F29F4733C5C2B09560D0B077F0C6D874` | `F62FA3CB484057AABFF5188DE53A3D5200C441A78EA0BF9A3C5EAB4304991ADD` |
| 3 | explicit assertion using the live result | `92E8C294E0C0A30B736C32D0A344A9C4E86F84DEC99964244129B68BCFD724C4` | `E5C6459EEB92D8DD5D307E5393A988AFEACD38B0B99124571E23D797E659631C` |
| 4 | Stian local registration-success form | `4822E524912B0F5FCC692AF6AD3C6B88BD6618DD2F431201E16FF885A5839377` | `CEA241E05692902D2D4063C384944B166D394AE67964C9B5DAF6A3FAC3A40B4C` |
| 5 | full-width result narrowed at the field and return | `56F76167845033D1F9C5AE85827D4D8F651F9E0FFEB5A76FB15527368B64D9CB` | `39111AE4E940999C8FCE045302B085719F5A6B5ACE6DB88EE24CCF042FBDA42F` |

The initial Wave 1 compile attempt failed before producing an object because
of a deliberately over-strong structure-size assertion and an unavailable
local enum name. Those compile errors were corrected from the frozen January
offset/format evidence before the first object was emitted.

Wave 1 used immediates for `TRUE` and carried only `ESI`; its relocations from
the assertion onward were six bytes earlier than January. Waves 2 through 5
fixed the assertion literal ownership but did not change that code shape.
MSVC continued to fold the successful value into immediate byte stores and
returns instead of retaining January's callee-saved `EBX` value.

## Regression and ownership result

The hardened per-function comparator kept the tiny no-op pair strict, and the
first candidate also kept the other four inherited leaf comparisons strict.
The whole-TU regression gate was intentionally run against the frozen
manifest before admission. It reported:

- no `NEWLY_EXACT` function;
- `_rasterizer_text_cache_initialize` only as changed nonexact;
- contextual ownership changes for four inherited exact functions;
- changed assertion/error `.rdata`, debug sections, and symbol ownership.

Because the candidate supplied no strict code credit and did not preserve the
accepted translation-unit evidence, the source was rejected rather than
admitted for semantic completeness alone. The cumulative baseline object was
restored after the temporary gate check and its SHA-256 was reverified as
`E65E830DAECDFDB97674788B60705B73BAD773D3B64B3C3F28E5466BCF324DF6`.

The classifier could not run because no pinned `llvm-objdump` executable is
available in this isolated tree. No classifier credit is claimed.

## Do not repeat / reopen

Do not retry another spelling of a constant `TRUE` local, an explicit assert,
the documented local registration result, or a wider integer result. All five
forms collapse to the same nonexact function bytes under the pinned compiler.

Reopen this initializer only with new source or compiler-provenance evidence
that explains why January retains `EBX = 1` across both calls. Recover the
remaining private helper/caller cluster together: `_code_00172fc0` uses the
private `AX`/`EBX`/stack surface, `_code_00173070` consumes its slot in `ESI`,
and the cache/draw callers determine those ABIs. Do not tune either private
leaf as an external cdecl function in isolation.

