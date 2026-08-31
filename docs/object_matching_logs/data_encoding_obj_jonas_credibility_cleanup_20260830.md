# `data_encoding.obj` source-credibility cleanup (2026-08-30)

## Scope

- Translation unit: `source/memory/data_encoding.c`
- Header: `source/memory/data_encoding.h`
- January target: `build/split/source/memory/data_encoding.obj`
- XDK 3911 candidate: `build/base/source/memory/data_encoding.obj`
- Baseline disposition: 16/16 strict-exact functions and 14/14
  target-owned `.rdata` sections.

This pass applies the source-credibility gate after the object had already been
admitted. Exact bytes are necessary but are not sufficient when the retained C
contains explicit compiler steering or logic that is difficult to defend as
original source.

## Findings and evidence

Two comments in the admitted encoder described matching controls rather than
program logic:

- `data_encode_memory` read its ordinary `long` parameter through a
  `volatile` type-pun solely to force a stack reload at switch dispatch;
- `data_encode_array` made the pointed-to input parameter itself `volatile`
  solely to preserve a compiler-selected stack lifetime. The same function
  also carried a code-neutral `register` hint and a redundant `long count`
  alias whose assert expression did not use the `element_count` identifier
  recorded in the January assertion string.

The `data_encode_memory` default arm also assigned
`memory_size = element_size` after the fatal assertion and `system_exit` call.
That assignment looked like a safety repair, but it was the reason ordinary
`switch (element_size)` did not match.

The HCEA cross-build reconstruction at
`surreptitiousresearch/halocea@570c83fd9c365dad6f2a3e7041705d5b84c7847c`
(`src/blam/cache/data_encode_memory.c`) independently records that the binary
uses an indeterminate size on the invalid selector edge and that known callers
produce only the four defined codes. Its `data_encode_array.c` uses an ordinary
input pointer and meaningful typed staging locals, not a volatile lifetime
control. HCEA is corroboration, not the January byte oracle.

The retained direct switch plus omitted invalid-edge assignment is therefore
an evidence-backed original fatal-path bug, not arbitrary undefined behavior
chosen for matching. If `system_exit` unexpectedly returns, `memory_size`
remains indeterminate; the source now says so explicitly. All ordinary paths
remain defined.

## Bounded experiment matrix

Every row rebuilt the real translation unit with XDK 3911. Strict results use
`tools/coff_compare.py` against the freshly split January object.

| ID | Source shape | `_data_encode_memory` | `_data_encode_array` | Decision |
|---|---|---|---|---|
| C00 | admitted volatile switch, volatile array pointer, register hint, alias, and fallback assignment | exact, 304 bytes / 17 relocs, `aeaf7eb5...` | exact, 576 bytes / 43 relocs, `7e3569e1...` | rejected as the credibility baseline |
| C01 | direct `switch (element_size)`, retain fallback assignment | fuzzy, 304 / 17, `c1807760...` | exact | useful control; do not retain the type-pun merely for bytes |
| C02 | C01 plus ordinary array pointer | same fuzzy result | exact | proves the pointer qualifier was code-neutral and unnecessary |
| C03 | C02 plus remove the post-`system_exit` fallback assignment | exact, 304 / 17, `aeaf7eb5...` | exact | natural exact breakthrough, corroborated by HCEA fatal-edge behavior |
| C04 | C03 plus remove `register`, initialize the alias normally | exact | exact | proves the storage-class hint was unnecessary |
| C05 | C04 plus remove the redundant alias and use `element_count` directly | exact | exact | retained final source |

## Final source and strict result

The final source contains:

- a direct, typed `switch (element_size)`;
- an explicit original-bug comment on the unreachable invalid selector edge;
- a plain `void const *source_array` prototype in the associated header;
- direct `element_count` expressions matching the January assertion names;
- no `volatile`, aliasing violation, register hint, synthetic lifetime anchor,
  inert statement, raw byte emission, or inline assembly.

Fresh strict comparison reports all 16 functions exact, totaling 3,296 padded
function bytes. The two touched functions remain:

- `_data_encode_memory`: 304/304 bytes, 17/17 relocations, normalized SHA-256
  `aeaf7eb5fddeb848adc29cfd6c46546c90c32c0981b02f0f430cd511286b7aaf`;
- `_data_encode_array`: 576/576 bytes, 43/43 relocations, normalized SHA-256
  `7e3569e19e98be057aeb330fe1973dfc2afe7e13e8e49f66c61f8ed1d4971b6b`.

No literal or owned-data expression changed, so the established 14/14
target-owned `.rdata` result is preserved. `tools/fake_match_scan.py` reports
zero review leads for the translation unit.

## Validation

- fresh XDK translation-unit build: pass;
- strict 16-function COFF comparison: 16/16 exact, 3,296/3,296 padded bytes;
- consolidated `halobetacache_build`: pass;
- semantic report: 470 units scanned, 4,986 functions evaluated, 4,887
  accepted exact, zero unit errors;
- progress/admission report: `data_encoding.obj` remains admitted with all
  four hardened objdiff exceptions verified;
- fake-match scanner: one file scanned, zero review leads;
- repository test suite: 221 passed.

## Disposition

`data_encoding.obj` remains strict-exact and admitted, now with natural,
semantically coherent source. Reopen only if January source or authenticated
local-variable records contradict the documented fatal-path omission.
