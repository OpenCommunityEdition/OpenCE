# `action_charge.obj` Jonas two-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`3f2ba63dfb1258f7e36baaec69fe495bcefdc20f`. The three permitted public
action-charge leaves were emitted together in exactly one natural candidate
compile. `_action_charge_begin` and `_action_charge_update` were independently
strict and are retained. `_action_charge_is_leaping` missed and was removed
immediately, without a spelling, declaration, control-flow, or compiler-control
retry.

`action_charge.obj` advances from 0/22 to 2/22 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_action_charge_begin` | 80 | 80 | 3 | `c78c6feeb10fda8857834f5264614aa976498059590ee08304aef25f21af8bf3` |
| `_action_charge_update` | 71 | 80 | 2 | `f2591b2de350c20aefef42c38623875dca494bb46a51d06c6dcd5947b035976e` |
| **Wave gain** | **151** | **160** | **5** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends. The target owns 6,032
meaningful code bytes in 22 functions and 6,176 padded code bytes. Its fifteen
non-code COMDATs contain 243 payload bytes (256 bytes in the progress metric);
all remain outside this wave's credit. The final candidate defines no non-code
symbol.

## Provenance and ABI

- Baseline `source/ai/action_charge.c` blob:
  `4a2578245ccc649a0a6c20823cfb8f204cc201e6`.
- Retained source blob/payload SHA-256:
  `d34d932679150399937475d915ade404f7f94ac9` /
  `f7daeef229232685d688379613981d6e7f25c3ebeee7ccf156876377d82e2041`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its
  `src/action_charge_begin.c`, `src/action_charge_update.c`, and
  `src/action_charge_is_leaping.c` blobs are respectively
  `6692a82a1ba231cc7d91c38860096cf6ec80a8e4`,
  `3e7d6333d32d89108bfa8b6db76390512094044b`, and
  `38fc8c40afa3cc720f5d8f784c22da25150febcc`. Their payload SHA-256 values
  are respectively
  `9cdc2a06b44a3a69e3418bf79e2aced52111cbd5d9ab9404044c9b472de185c7`,
  `a949c7b6fb3ebbd1db455d09f6b0856788d05aa52ce52e0eba3a7b801d1474bd`,
  and
  `c6857cdfcc0225c97dd8bdfe56a8f5a8ea721881a48ca58e71b8af0911b7a7d5`.
  These files supply independently readable behavior, not target-derived
  emission anchors.
- Cross-build correspondence commit
  `8ffdbab2301d17215b6f4b4a109f0e0a6f09b53d`, blob
  `ec76e6464bf79d444779720d5efb9432bf5378d7`, classifies all three names as
  exact-unique, ordinary `cdecl`, one-parameter functions. January COFF and
  callers remain authoritative for the 32-bit parameter type.
- The existing `struct action_specification` types `begin` and `update` as
  `void (*)(long)`. January's action table relocates to the two functions at
  `+580` and `+588`. Existing `actions.h` types `action_charge_is_leaping` as
  `boolean(long)`, and January `_actor_is_leaping` calls it at relocation
  `+33`.
- The earlier evidence-only frontier commit
  `677defb9d9b4dd0ac4f20bc2c5dbd53bf3e8d7f5`, ledger blob
  `bbb630a9fc673dab443948add44b8457e3b465e4`, independently records all three
  public leaves as exact on their first complete readable shapes. It is not a
  production-source donor and none of its tuned control/private-helper forms
  were reused.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The retained signatures are:

```c
void action_charge_begin(
	long actor_index);
void action_charge_update(
	long actor_index);
```

Existing `actions.h`, `actors.h`, and `actor_definitions.h` own the action,
actor, charge-state, and firing-variant layouts. The callee's return/argument
types are corroborated by existing source TUs, so the missing declaration is
kept translation-unit-local. The firing-situation enumerator is likewise
source-local because no shared header presently owns it. No shared-header or
storage repair is needed.

## Authenticated behavior and one-shot rejection record

`action_charge_begin` resolves the full 32-bit actor handle through
`actor_get`. For vehicle-strafing charge with the matching firing-variant
special-fire situation, it decrements a positive signed-short deny-attempt
counter. Its target relocations are `DIR32 _actor_data` at `+4`,
`REL32 _datum_get` at `+16`, and
`REL32 _actor_combat_get_firing_variant_definition` at `+37`.

`action_charge_update` resolves the same typed actor handle. During a launched
melee leap, before a melee attack and after the actor is no longer midair, it
increments the signed-short leap-failure timer. Its target relocations are
`DIR32 _actor_data` at `+8` and `REL32 _datum_get` at `+15`.

All three public candidates were compiled together once. The rejected
`_action_charge_is_leaping` target is 66 meaningful/80 padded bytes with three
relocations and normalized SHA-256
`9ddd3d1296e581d078e6958f4039f9bde736698282368436b60d130978e7b275`.
The first natural candidate was also an 80-byte, three-relocation COMDAT, but
its normalized SHA-256 was
`56aa0e8827a4ebecba9e4310f944bc24c07ffb5c2b2e39ef46ea11ad36899f31`.
The target forms one charge-state pointer and shares its epilogue; the readable
HCEA early-return shape instead accesses full actor offsets and duplicates the
return. Consequently the common `game_time_get` relocation occurs at target
`+48` but candidate `+46`. The candidate COMDAT and its support-only prototype
were removed without retry. The one-shot three-body candidate object SHA-256
was `3ee981a618bc3f891786bc839c2c51a3bd11bfb9a16a1e90f382d0569fe3a8af`.

`action_charge_control`, `action_charge_perform`, `action_charge_setup`, both
anonymous `code_` helpers, generic helper emissions, and every data/storage
section were explicitly outside scope and are absent from the final object.

## Policy, scope, and validation

Only `source/ai/action_charge.c` and this new Jonas-owned ledger are changed.
No shared header, storage definition, data section, configuration, semantic
exception, parked record, completion label, protected-five source,
pre-existing Markdown, or Claude-owned file is edited.

The retained source is readable typed C. The guarded decrement remains in the
positive signed-short range. The timer increment is promoted to 32-bit `int`,
so its arithmetic cannot overflow; a hypothetical value above `SHRT_MAX`
narrows according to the implementation-defined two's-complement behavior of
the authenticated VC7/Xbox target. That ordinary target contract is already
used by the project's signed-short AI timers and is not undefined behavior or
an emission control. The source contains no assembly, raw address or
byte-offset dereference, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing.

- Forced rebuild of the final retained source: pass. The only defined code
  symbols are `_action_charge_begin` and `_action_charge_update`; there is no
  defined non-code symbol. Direct hardened comparison passes both functions.
- Complete `halobetacache_build` and `libcmt_build`: pass (568 remaining edges
  after the final translation unit's forced rebuild).
- Semantic audit: 470 units, 4,074 functions evaluated, 3,934 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,995 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,982/11,060 exact functions,
  476,846/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and 151
  meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state regression snapshot/check followed by deletion and
forced rebuilding of `action_charge.obj` is the final same-path reproducibility
proof. The post-rebuild hardened comparison must remain strict for both
accepted functions, and the rejected symbol must remain absent. No push is
performed.
