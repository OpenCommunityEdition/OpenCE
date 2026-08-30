# `prime_numbers.obj` Stian/Pastudan sieve-lifetime fixed point (2026-08-30)

## Scope and strict baseline

This bounded packet audited `source/bungie_net/common/prime_numbers.obj` from
the isolated `agent/light-volumes-closure-20260830` worktree. Canonical was
read-only at `2531522066925dc70d0a3532f819ad671c2e393f`; the packet started from
local `f79656f259964c4e60fa48a4c677c351c8717694`.

The hardened gate reports three exact owners and one residual:

| Owner | Target padded bytes | Relocations | Strict state |
| --- | ---: | ---: | --- |
| `_compare_prime_numbers_descending` | 32 | 0 | exact |
| `_generate_prime_numbers` | 352 | 12 | residual |
| `_randomprime` | 80 | 4 | exact |
| `_probable_prime64` | 192 | 10 | exact |

The residual target has 340 meaningful / 352 padded bytes, normalized SHA-256
`2428a7a00be6ab6f96efe9dda273d824a0cc9829aeeb2dd5f4c9b35b2b7992b1`.
The restored production baseline has 338 meaningful / 352 padded bytes,
12 relocations, and SHA-256
`66e1c7859e354a6bf7caccb70b260e99d516be6a94837571ecc60dd5b43edda6`.
All twelve relocation destinations and types agree in order. Their addresses
are target `38, 43, 48, 55, 87, 106, 142, 149, 277, 293, 312, 322` versus
baseline `38, 43, 48, 55, 91, 111, 140, 147, 275, 291, 310, 320`.

The inherited `prime_numbers_obj.md` ledger and the E1-E27 histories were read
in full before compiling. Historical branches/worktrees were also censused.
The two surviving dirty `prime-numbers-final*` spellings are already recorded
rejects; the August `prime-numbers-atomic` and `prime-donor-scope-experiment`
trees contain the earlier unwritten generator, not a hidden exact candidate.

## New authenticated provenance

This packet reopened the residual only because two recently supplied external
trees add a premise that the E1-E27 source-only campaign did not have:

- Stian Eklund commit `3c7c3cbf07d5cd41d60a597918ef97aa135d2272`, present on
  `upstream/main` in
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo`, contains a
  readable C lift of `sieve_of_eratosthenes` in
  `src/halo/networking/message_header.c`. Its local topology reuses the fill
  index as the outer-loop start, reuses the fill prime as the inner index,
  walks outer divisors through a pointer, and keeps a distinct sieve
  countdown.
- The Pastudan reference at
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/pastudan-halo-ai-comm-research-20260828/src/halo/networking/message_header.c`
  preserves the original XBE body at `0x80d50`. Its instructions from the
  prologue through the return at `+0x153` agree instruction-for-instruction
  with the January target captured by `tools/campaign/gate.py`. The naked
  assembly was used only as read-only evidence and was not copied, compiled,
  or admitted.

HCEA, punpckhdq, the remaining Stian/Pastudan mirrors, and the local
Claude/Fable/prime worktree census supplied signatures or duplicates but no
additional legal-C production candidate.

## One fresh donor-backed candidate

The single production candidate translated Stian's lifetime topology into the
house C89 body while preserving the current allocation, assertion, sorting,
and reallocation APIs:

- initialize `prime = 3` before the square-root conversion;
- express the fill and scan as the donor's guarded `do` loops;
- reuse `i` as the outer-loop starting index;
- walk the current divisor with typed `unsigned long *cursor`;
- reuse `prime` as the inner sieve index;
- retain a distinct unsigned `sieve_count`.

No assembly, `volatile`, `register`, pragma, intrinsic, attribute, barrier,
raw address/offset, pun, undefined behavior, byte forcing, or compiler-flag
change was used.

The candidate retained the target 352-byte padded envelope, twelve relocation
destinations/types in the exact order, and all three exact sibling functions.
It did not produce a strict gain:

- 351 meaningful / 352 padded bytes;
- normalized SHA-256
  `506c16295caba60a6eda2d5ae6d84483f0ffbe49cded7b439932ae66f5ec1b38`;
- relocation addresses
  `44, 49, 54, 61, 97, 117, 151, 158, 288, 304, 323, 333`.

The source was restored exactly after rejection. Its restored raw-file SHA-256
is `e2bbb948a83c35f99ef252d857e8d0a8bf4d3a51fdca76c0028957236c415697`.

## Exact fixed point

The new donor topology does force the target's 12-byte frame, but it enters the
same allocation class previously seen when a late prime value crosses the
square-root conversion:

| Lifetime | January target | donor-backed candidate |
| --- | --- | --- |
| odd candidate count | `EBX` | begins in `ESI`, then spills to `[EBP-4]` |
| `num_primes` | `ESI` | begins in `EBX`, then reloads from its argument home |
| zero/fill index | `EDI` | `EDI` |
| total count | `[EBP-0xC]` | `[EBP-0xC]` |
| scan state | `[EBP-4]` | `[EBP-8]` |
| late fill prime | `[EBP-8]` | `EBX` |

January simultaneously keeps `odd_count` in `EBX` and spills the prime to
`[EBP-8]`. XDK 3911 instead assigns that available nonvolatile register to the
late prime, displacing the entry owners. The cursor and exact donor reuse then
operate on the wrong physical allocation. This is not a missing loop or
semantic reconstruction: Pastudan independently proves the target CFG, and
Stian independently supplies the natural C-level lifetime intent.

Do not repeat this Stian cursor/reused-index family, including spelling-only
changes to the guarded `do` loops. Reopen only with original local-variable
provenance, a verified compiler-build/patch distinction, or a legal-C donor
that demonstrates the otherwise missing simultaneous choice:
`EBX=odd_count`, `ESI=num_primes`, late `prime=[EBP-8]`, and
`scan=[EBP-4]` under XDK 3911 CL `13.00.9254.1` and the production flags.

Classification remains `tu-context-optimization`; the object remains 3/4
strict exact with no production-source change from this packet.

## Claude-ready handoff

Audit only; do not compile another spelling without new provenance. Read these
files first:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/light-volumes-closure-20260830/docs/object_matching_logs/prime_numbers_obj.md`
- this ledger;
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/light-volumes-closure-20260830/source/bungie_net/common/prime_numbers.c`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/src/halo/networking/message_header.c`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/pastudan-halo-ai-comm-research-20260828/src/halo/networking/message_header.c`

Required new evidence must explain why the original compiler spills the late
prime while retaining `odd_count` in `EBX`. A candidate is admissible only if
all four owners, target-owned data, and all ordered relocation identities and
addresses remain strict exact under the full campaign gates and Units sentinel.

## Restored-tree validation

- Focused Prime Numbers gate: three exact owners, one 352-byte residual, zero
  unwritten owners.
- Complete `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass.
- Semantic report: 470 units, 4,957 functions evaluated, 4,841 semantic exact,
  4,851 accepted exact, 131 hidden exact / 78,940 hidden bytes, and zero unit
  errors.
- Progress: 384/833 linked objects, 4,812/11,060 functions,
  610,646/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Python tooling suite: 212/212 passed.
- Protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `build/report.json` SHA-256:
  `13c9f7e8ac93b0a6de047e0f40bc27408dd076504228e7e1211f2cc4fe526f4f`.
- `build/semantic_report.json` SHA-256:
  `9f948fc7de4a7d8d11f52ebbfb7f17126cb03ea893662921475c6e9484f66db0`.
- `git diff --check`: clean; no scratch artifact survives.
