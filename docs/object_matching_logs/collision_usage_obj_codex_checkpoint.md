# `collision_usage.obj` Codex checkpoint

This Codex-owned ledger records a bounded partial recovery in
`source/physics/collision_usage.obj`. The unit remains `NonMatching`: eight of
its fourteen functions and most aggregate runtime-data ownership remain open.
No pre-existing or Claude-authored Markdown was edited or deleted.

## Scope and provenance

- Authoritative integration base:
  `8b7a83037da304625ff54a90ae9e2f3b2dacd3fc`.
- Historical source donor:
  `7bb31754111b5662a4c9e28216dced82f4d324d6`.
- Historical private-symbol evidence:
  `7135dcdd0c41ee66978224a8dd34ec9afe4c652b`.
- The donor was audited rather than transplanted wholesale. Only six strict
  functions, their typed structures/globals, and the target's grounded private
  `collision_log_store_period` symbol name remain.
- The user-frozen `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units`
  objects are outside this lane and untouched. No shared header changed.

## Strict code result

The forced untouched baseline was `0/14`. The retained candidate is `6/14`,
with 359 meaningful code bytes, 416 padded bytes, and 35 relocations all
strict-exact. No semantic exception or relaxed relocation comparison is used.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_collision_log_initialize` | 90 | 96 | 10 | `1e9d4139556a4810ee6abe5924c9eb001f7289d95185fe423c51d2e12f40e9c8` |
| `_collision_log_enable` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_collision_log_store_period` | 113 | 128 | 12 | `eb7d6da86636d8ddfd9767d07305674944fc36adb438367ac7183cd41acf1cf5` |
| `_collision_log_begin_period` | 20 | 32 | 1 | `2d15e53b5c7bd2b869251fa7022f91d2cab40a2e913acaa871f144dd237da6ca` |
| `_collision_log_continue_period` | 20 | 32 | 1 | `ffce0c2104a96d1eb9dab0804bc193c86a5cab7e93ffa98fbb3245f35f86d5f5` |
| `_collision_log_end_period` | 103 | 112 | 10 | `baf2104f877ca7e5935072af510bf299c9673b50d3fd689e4688cc4311fd66f8` |

The private symbol rename is limited to the target address at `0x13C8D0` and
is required for the static helper and both callers to compare by their actual
relocation identity. The second private residual remains address-named.

All eight residual function bodies are absent. A single bounded timing probe
used real `LARGE_INTEGER` locals and `.QuadPart`, with no pointer cast:

- `collision_log_start_time` grew to 48 padded bytes versus the target's 16
  and was removed.
- `collision_log_end_time` itself became strict-exact at 128 padded bytes and
  five relocations, but its required `collision_log_get_current_user` remained
  nonexact (`99.14286%`, equal 256-byte size and 23 relocations, different
  normalized bytes). Both were removed so no exact caller depends on a
  nonexact private body.

No incompatible `PLARGE_INTEGER` cast from the donor is retained.

## Runtime-data and ownership audit

The January unit contains 1,016 aggregate data bytes: 10 BSS, 138 DATA, and
868 RDATA bytes. The retained declarations restore the target's named DATA
and BSS ordering and offsets. The result is intentionally not described as
whole-data exact:

- Twenty-six target-owned RDATA COMDAT sections are independently strict-exact
  by owner, logical size, flags/alignment, COMDAT selection, normalized payload,
  and relocations. They total 492 logical bytes.
- Nine candidate-only RDATA owners total 119 logical bytes. Eight are shared
  user-name literals folded to other target translation units, and one is the
  donor's assertion-expression literal. Seven target-only RDATA owners total
  321 logical bytes and belong to omitted residual functions.
- The 138-byte `.data` section has the exact target owner order and offsets,
  exact normalized payload hash
  `804797955ced17a1f6b1e4b86c0a83a5d25fcb03fa846e690dae51ef13bc5c2f`,
  and the same 30 relocation addresses/types. It is not hardened-exact because
  eight shared string relocations resolve to candidate-owned COMDATs while the
  split target records them as undefined cross-unit symbols.
- The 10-byte `.bss` section has the target's flags, alignment, six external
  owners, and offsets. Ordinary objdiff reports it 100% and campaign progress
  credits those ten bytes. The hardened whole-TU fingerprint does not call it
  strict because csplit stores ten explicit zero bytes while VC7 emits genuine
  payload-less BSS.
- The three common symbols have grounded candidate allocation sizes:
  `_global_current_collision_users` is 64 bytes,
  `_collision_usage_buffer` is 8,856 bytes, and
  `_collision_usage_current` is 2,952 bytes. The split target records zero
  common-symbol sizes, so no hardened ownership credit is claimed for them.

No semantic-data ledger entry or object-completion label was added. The config
entry remains `NonMatching`.

## Source quality and validation

The retained implementation is readable typed C. It uses no assembly,
volatile scheduling device, forced inline, optimizer pragma/barrier, undefined
type punning, byte forcing, raw layout access, or incompatible pointer cast.
No-argument functions use explicit `void`, parameters are one per line, and
every retained void function ends with `return;`.

- Full `halobetacache_build` and `libcmt_build` graphs pass.
- Semantic audit: 470 units, 3,871 functions evaluated, 3,728 semantic exact,
  3,792 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,781/11,060 exact functions,
  459,964/2,198,102 code bytes, and 1,803,170/4,176,062 data bytes.
- Halo progress: 273/468 complete objects, 3,614/7,574 exact functions,
  447,050/1,770,166 code bytes, and 1,797,978/3,923,451 data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Park validation: three active, zero stale, and zero invalid.
- Complete tooling suite: 179/179 tests pass.
- A clean committed snapshot, forced translation-unit rebuild, and regression
  check retain all six exact functions without failures or warnings.

Reproducibility identities before the checkpoint commit:

- Source SHA-256:
  `793a3569cb6d55481be880aa02879ae16f489942076752d1c9b4081717cca271`.
- January target object SHA-256:
  `e829727abb548e1a649425732e4c12b60a4ee50d60060fb60bb4f68ef95cb6a4`.
- Forced rebuilt object SHA-256:
  `7feabda598bf912ea18b6d1889d5519b23b9b5dc9bf50244c6d5c61d030ee914`.

No GitHub push is performed.
