# Defined countdown increment reconstruction — 2026-09-06

## Result and policy

Baseline: `2b2589b5cb760d36bf51fa1a24185fd2015ea589`.
The missing `countdown_timer_increment` now has a natural, defined-arithmetic
implementation, retained at **84.28571% fuzzy** with **zero exact-byte credit**.
The manager advances from 59 exact / 4 parked / 7 unwritten to **59 exact /
5 parked / 6 unwritten**. The six remaining unwritten owners are the separately
audited idle chain, still held on UI error-enum ownership.

The [August wave-two ledger](network_server_manager_codex_wave2_20260820.md)
rejected an exact increment because its overflow probe used signed addition.
It also removed the defined but nonexact alternative under that packet's
exact-only admission policy. The current user rule explicitly permits credible
fuzzy reconstructions. This packet therefore retains the first natural defined
form; it does not restore the rejected undefined form or conduct a source-shape
search to force its bytes.

## Source and target contracts

The supplied server source, lines 100–120, corroborates the genuine public
function and its operation order: update the timer, assert nonnegative
adjustment, clamp an overflowing addition to the maximum, otherwise apply
`MIN`, then assert nonnegative remaining time. It is reconstruction evidence,
not the January byte authority; the attachment is pinned in the
[source reconstruction map](../user_source_reconstruction_map_20260906.md).

January's function at `0x0011B660` has 144 meaningful/padded bytes, nine
relocations, external storage class 2, type 0x20 and NODUPLICATES code COMDAT.
It takes the real timer pointer, signed adjustment and signed maximum through
the cdecl stack and returns `void`. Volatile `EAX` is not a result contract.
Its signed comparison after the x86 ADD is not an unsigned carry test.

The natural source calls the actual `countdown_timer_update` provider. It forms
the sum in `unsigned long`, then converts the modulo result to signed `long`
before January's signed comparison and `MIN`. Out-of-range unsigned-to-signed
conversion is implementation-defined ISO C behavior, with a pinned
two's-complement result on VC7/Xbox; the arithmetic itself does not invoke
signed-overflow UB.

The existing caller `_network_game_server_update_countdown` already references
this symbol, at target relocation +0xA1 and canonical +0xA4. The new function
resolves that real dependency. No fake caller, public/private linkage change,
manual inline, foreign prototype, new helper COMDAT or data owner is added.

## Necessary provider repair

Checking only the new addition would miss another hazard. The actual update
provider previously used signed `time_remaining -= elapsed_time`. For last
clock `0x80000000`, current clock `0x7FFFFFFF`, elapsed time has signed value
`-1`; subtracting it from `LONG_MAX` overflows in abstract C. January's x86
wraps that result before the increment clamps it.

The provider now performs that subtraction in `unsigned long` and converts
the modulo result back to `long`. Its signed clock gate and signed
elapsed-versus-remaining predicate are preserved. This is a source repair,
not a provider ABI change: **all 48 provider bytes and its relocation remain
strict-exact**, and the new increment's machine code is unchanged by the
repair. The inherited getter/decrement manual copies are not silently edited
or claimed repaired by this packet.

## Honest residual

| Measurement | January | Natural candidate |
| --- | --- | --- |
| Padded bytes | 144 | 144 |
| Relocations | 9 | 9 |
| Normalized SHA-256 | `0291de1fcc1f65baf92a784ad590c78925729c9c4ba0b9a1bbf33a6e8cdd7aa5` | `96937b1e7ff0aa3415fa0b0a397feddc2b30699e3d34b7be1db7f80b3eb96485` |

The nine dependency identities/types remain in the same order. Differences
include the naturally inlined update's register allocation and last-update
store position, maximum-load scheduling, and a register test instead of a
memory compare for the final assertion. They are not fixed by copying the
target instruction schedule into source. The function is explicitly parked
in `config/parked.json` as an unclassified credible residual.

## Verification completed before publication

The full Ninja build and stable whole-tree sweep pass: **6,328 / 8,245 strict
functions**, zero exact gains and zero regressions. The root frozen audit
compares **572 rebuilt objects and 833 target objects**:

- All **204 inherited server runtime sections** are identical, including all
  63 inherited functions and the existing fuzzy bodies.
- Exactly one external code owner is added; no literal, BSS, COMMON or other
  runtime-data owner is added. The former undefined increment is resolved,
  with no new undefined APIs.
- All other rebuilt objects and every target object are unchanged after
  timestamp normalization.
- Ordinary report changes are limited to the new function's fuzzy result
  and its aggregate fuzzy percentage. Exact code/function/data/object totals
  do not change. Semantic evaluated functions increase by one and missing
  source functions decrease by one; accepted-exact records are unchanged.
- Park audit: **297 active, zero stale, zero invalid**. Admission: zero
  candidates, contradictions or revocations; five inherited rejections.

The independent real-body Unicorn harness executes actual relocated target
and candidate instructions, not a Python reimplementation of their algorithm.
Across **337 pairs** (17 curated boundaries and 320 deterministic randomized
cases), complete eight-byte timer state, normal/fatal outcome and assertion
identity agree. Caller arguments and timer guards remain intact; normal return
preserves EBP/EBX/ESI/EDI and cdecl stack balance. The legitimate volatile EAX
differences are ignored for the void API. Two precise real-byte branch mutants
produce detected behavioral differences; setup/emulation failures cannot make
those controls pass. This is bounded single-function evidence with explicit
clock/assert/exit stubs, not exhaustive inputs or a live engine test.

The initial harness used the increment-only object. Root proves the repaired
provider candidate and actual integrated object have identical full runtime
inventories and identical increment instructions/relocations. Source safety
and this machine-level behavioral evidence are separate checks; matching
machine behavior alone is not proof that the C expressions are defined.

The committed `tools/test_countdown_timer_increment_runtime.py` makes this
evidence reproducible without scratch manifests: it freshly compiles the
actual canonical translation unit, pins the target/candidate code and ordered
relocations, verifies literal owners, and repeats all 337 real-body pairs and
both branch-mutant controls. It also checks the unsigned source expressions,
API order and allowed memory accesses. Root independently reran its three
tests, then the full suite: **999 passed, two skipped, 26 subtests passed**.
The test file SHA-256 at review is
`341f3697205bd153286c95a7da37ff799eb463640beb9bea631906cce7190e1`.

## Immutable local evidence

| Artifact | SHA-256 |
| --- | --- |
| `scratch/server-countdown-defined-before-20260906.json` | `8f2c04cac6594705afae435316bb2b583746247c95e5e40438bfb1c5627d5e2a` |
| `scratch/server-countdown-defined-final-20260906.json` | `ef517a01142e239ad170864d9f8e540c871ab34782f3e657ad69439f4b5dcd5a` |
| `scratch/countdown-timer-increment-defined-with-update-20260906.edits.json` | `9e0f9013488e6945b6b3d5d523fbff0ead74fe4b6bead9f411ec12f3bf447865` |
| `scratch/countdown-timer-increment-defined-with-update-20260906.obj` | `d24b4752cc34ed8a34457fae480e1535c2245dcf0a75128b669f9b4b05ce96ae` |
| `scratch/countdown-timer-increment-defined-audit-20260906.json` | `8b5fe2cfb0d70418584adc55a2c8432016a9cf71125202062f61acdcacddda54` |
| `scratch/server-countdown-root-whole-audit-20260906.json` | `32e63e58f97f21dc6c272b802b0424e9d3cd302f87158edabf47625f35d7d953` |
| `scratch/countdown_timer_increment_realbody_audit_20260906.py` | `43d826292534546c3fed428f39dbd859323646f246ded012f36ceeac89a5812f` |
| `scratch/countdown_timer_increment_defined_handoff_20260906.md` | `7959c8e5d7bb2692b94e9629e8e6ca6cd25002a60e618edd773af338ac1afa74` |

Adjusted normal-report totals remain **930,074 meaningful exact code bytes,
6,282 credited functions and 391 / 833 Matching objects**. Scratch residuals
and the held idle chain are not additional canonical exact progress.
Active Fable/Opus lanes remain untouched. IDA is unavailable; no IDA execution
or whole-engine linkage result is claimed.
