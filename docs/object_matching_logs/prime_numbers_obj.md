# `prime_numbers.obj` - January 2002 / XDK 3911

Target: `build/split/source/bungie_net/common/prime_numbers.obj`.
Candidate: `build/base/source/bungie_net/common/prime_numbers.obj`.

## Inventory and current strict state

| Symbol/data | Target | Candidate | Strict state |
| --- | --- | --- | --- |
| `_compare_prime_numbers_descending` | 32 bytes, 0 relocations, `2e54965488b9bb491fcdf3697701f87ce86c0ea930186ed7a9c765b3fcf5bdc9` | identical | exact |
| `_generate_prime_numbers` | 352 bytes, 12 relocations, `2428a7a00be6ab6f96efe9dda273d824a0cc9829aeeb2dd5f4c9b35b2b7992b1` | 352 bytes, 12 relocations, `66e1c7859e354a6bf7caccb70b260e99d516be6a94837571ecc60dd5b43edda6` | nonmatching |
| `_randomprime` | 80 bytes, 4 relocations, `4741be658ca28e451309f235d46c6f2f8760301ccb232c856d2ee1a46132c2bd` | identical | exact |
| `_probable_prime64` | 192 bytes, 10 relocations, `ece78fdfc3ae49468f8a5634141148861548ab6b7b87faf38ad5d5a26a32b144` | identical | exact |
| target `.rdata` payloads | 8, 11, and 49 bytes; no relocations | matching 8, 11, and 49 byte payloads | exact payloads |
| candidate-only `.drectve` / `.debug$S` and 7-byte `result` assertion literal | not emitted by csplit target object | emitted by compiler | non-code TU metadata/data difference; not strict-exact as a full object |

The source and target relocation destinations agree for all 12 `generate_prime_numbers` relocations. Their addresses do not: target has `38,43,48,55,87,106,142,149,277,293,312,322`; candidate has `38,43,48,55,91,111,140,147,275,291,310,320`.

## Baseline and source-shape experiments

| Shape | Result | Decision |
| --- | --- | --- |
| Reconstructed baseline from `b6883168a` / `a82502346` | `generate_prime_numbers`: 336 bytes, 12 relocs, hash `ba031c73d69fc3cc6bbfbe9046a46c18c7b1d3662f15971d6df4ff746ef3860b`; first raw divergence offset 5 (`sub esp, 8` vs target `sub esp, 12`). | superseded |
| Move `k = 3` before the `sqrt` conversion (uncommitted inherited experiment) | 336 bytes, 12 relocs, hash `bea188af3051b10618c03d9fe606334d415cc03625617811931637be33b3e987`; first divergence offset 10 and register allocation changes (`EBX`/`ESI` no longer match target). | reverted |
| Earlier `prime-numbers-final` worktree shape (`primes = num_primes`, do/while rewrites, `sieve_count`, allocation rewrite) | 368 bytes, hash `82e7e071c6b74de906e2e60ec782db70447530a884adc62e3b63879f7ee606a5`; first divergence offset 10, with different maximum/`num_primes` register ownership. | rejected |
| Current focused shape: allocation as `odd_count * sizeof(*primes) + sizeof(*primes)`, failure folded into `if (primes)`, and an explicit `sieve_count` lifetime | 352 bytes and 12 relocs (both target values); hash and relocation addresses still differ. Objdiff: 81.61%. | accepted, not exact |
| Write `*num_primes = i` rather than literal zero in the `< 2` branch | compiler folded it to the same current object hash; no strict effect. | reverted; do not repeat |

## First divergence and blocker

The current candidate's first raw difference is offset 5: target `83 EC 0C` (`sub esp, 0xC`) versus candidate `83 EC 08` (`sub esp, 0x8`). The target keeps three stack slots (`total_count` at `-0xC`, then live ranges at `-0x8` and `-0x4`); the candidate coalesces the corresponding lifetimes into two. This then changes source-independent scheduling around malloc, x87 conversion, and the sieve, shifting relocations although their ordered destinations and types remain identical.

No inline assembly, `volatile`, undefined behavior, byte editing, or compiler-flag changes were used.

## Do not repeat

- Do not move `k = 3` before the `sqrt` conversion: it breaks the otherwise matching initial register plan.
- Do not use the `primes = num_primes`/do-while reconstruction from `prime-numbers-final`; it creates a distinct allocation plan and a 368-byte function.
- Do not substitute `i` for the zero literal in the early result path; XDK3911 optimizes it identically.

## Validation and reopen criteria

Validated with the project XDK3911 build rule, `tools/coff_compare.py`, and `objdiff-cli` (81.61% for `_generate_prime_numbers`). The regression-manifest gate could not run because this isolated worktree has no `build/regression_manifest.json`; it must be rerun from a worktree containing the campaign baseline before promotion.

Reopen only with source evidence that preserves the target’s `EBX=odd_count`, `ESI=num_primes`, and `EDI=i` entry plan while forcing the target’s three distinct stack-lifetime slots. Required acceptance is strict equality of size, normalized hash, and relocation address/type/destination for all four functions, plus the TU’s non-code data/metadata comparison and the campaign regression manifest.
